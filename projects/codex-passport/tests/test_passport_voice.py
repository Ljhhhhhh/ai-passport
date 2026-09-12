"""Offline voice protocol/ASR checks: python3 projects/codex-passport/tests/test_passport_voice.py."""

import asyncio
from pathlib import Path
import stat
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import AsyncMock, Mock, patch
from uuid import UUID
import wave

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import passport_voice as voice

TARGET = "019c6e27-e55b-73d1-87d8-4e01f1f75043"
OTHER = "019c7714-3b77-74d1-9866-e1f484aae2ab"
AUDIO = b"\x77\xff" * 1200


def begin(rid=1, target=TARGET, samples=4800):
    return struct.pack("<BI16sI", 1, rid, UUID(target).bytes, samples)


def block(index, data, rid=1):
    return struct.pack("<BIH", 2, rid, index) + data


def command(op, rid=1):
    return struct.pack("<BI", op, rid)


def result(packet):
    return (*struct.unpack_from("<IBH", packet), packet[7:].decode("utf-8"))


class VoiceTests(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.sent = []
        self.delivery = AsyncMock(return_value="Message accepted by Codex.")
        self.asr = AsyncMock(return_value=" 请继续\n检查。 \n")
        self.patch_asr = patch("passport_voice.transcribe", self.asr)
        self.patch_asr.start()
        self.addCleanup(self.patch_asr.stop)
        logs = patch("builtins.print")
        self.logs = logs.start()
        self.addCleanup(logs.stop)

        async def send(packet):
            self.assertLessEqual(len(packet), 7 + 768)
            result(packet)  # All outbound text must be valid UTF-8.
            self.sent.append(packet)

        self.host = voice.VoiceHost(send, self.delivery, model=None, whisper=None)
        self.host.allow_targets([{"id": TARGET}, {"id": OTHER}])
        self.addAsyncCleanup(self.host.close)

    async def upload(self, rid=1, audio=AUDIO, target=TARGET):
        await self.host.handle(begin(rid, target, len(audio) * 2))
        for index, offset in enumerate(range(0, len(audio), 512)):
            await self.host.handle(block(index, audio[offset:offset + 512], rid))

    async def review(self, rid=1, audio=AUDIO):
        await self.upload(rid, audio)
        await self.host.handle(command(3, rid))
        await self.host._tx.task

    def last(self):
        return result(self.sent[-1])

    async def test_confirm_once_exact_preview_pinned_target_and_replay(self):
        await self.upload()
        self.assertEqual(self.last(), (1, voice.RECEIVING, 4, ""))
        self.assertTrue(self.host.active)
        self.host.allow_targets([{"id": OTHER}])  # Removal/reordering cannot retarget.
        await self.host.handle(command(3))
        self.assertEqual(self.last()[1], voice.TRANSCRIBING)
        await self.host._tx.task
        self.assertEqual(self.last(), (1, voice.REVIEW, 65535, "请继续 检查。"))
        preview = self.last()[3]
        self.delivery.assert_not_awaited()
        await self.host.handle(command(3))
        self.asr.assert_awaited_once()
        entered = asyncio.Event()
        release = asyncio.Event()

        async def deliver(target, text):
            entered.set()
            await release.wait()
            return "Queued in Codex."

        self.delivery.side_effect = deliver
        await self.host.handle(command(4))
        await entered.wait()
        await asyncio.gather(*(self.host.handle(command(4)) for _ in range(5)))
        self.assertEqual(self.last()[1], voice.SENDING)
        self.delivery.assert_awaited_once_with(TARGET, preview)
        release.set()
        await self.host._tx.task
        terminal = self.sent[-1]
        self.assertEqual(self.last()[1], voice.SENT)
        self.assertEqual(self.last()[3], "Queued in Codex.")
        self.assertFalse(self.host.active)
        self.assertEqual(self.host._tx.audio, b"")
        self.assertEqual(self.host._tx.text, "")
        for packet in (begin(), command(3), command(4), command(5), block(0, AUDIO[:512])):
            await self.host.handle(packet)
            self.assertEqual(self.sent[-1], terminal)
        self.delivery.assert_awaited_once()

    async def test_begin_and_block_duplicate_ack_only_when_identical(self):
        await self.host.handle(begin())
        await self.host.handle(block(0, AUDIO[:512]))
        await self.host.handle(begin())
        self.assertEqual(self.last(), (1, voice.RECEIVING, 65535, ""))
        await self.host.handle(block(0, AUDIO[:512]))
        self.assertEqual(self.last(), (1, voice.RECEIVING, 0, ""))
        self.assertEqual(len(self.host._tx.audio), 512)
        await self.host.handle(block(0, b"x" * 512))
        self.assertEqual(self.last()[1], voice.ERROR)
        terminal = self.sent[-1]
        await self.host.handle(command(3))
        self.assertEqual(self.sent[-1], terminal)
        self.asr.assert_not_awaited()
        self.delivery.assert_not_awaited()

    async def test_out_of_order_truncated_oversized_short_and_overflow_audio(self):
        cases = [
            [block(1, AUDIO[:512])],
            [block(0, AUDIO[:511])],
            [block(0, AUDIO[:513])],
            [block(0, b"")],
            [block(0, AUDIO[:512]), command(3)],
            [block(i, AUDIO[i * 512:(i + 1) * 512]) for i in range(5)] + [block(5, b"x")],
            [block(i, AUDIO[i * 512:(i + 1) * 512]) for i in range(4)] + [block(4, b"x" * 512)],
        ]
        for rid, packets in enumerate(cases, 10):
            with self.subTest(rid=rid):
                await self.host.handle(begin(rid))
                for packet in packets:
                    await self.host.handle(packet[:1] + struct.pack("<I", rid) + packet[5:])
                self.assertEqual(self.last()[1], voice.ERROR)
                self.assertFalse(self.host.active)
        self.asr.assert_not_awaited()

    async def test_wrong_rid_conflicting_begin_and_stale_requests(self):
        await self.host.handle(begin(50))
        await self.host.handle(begin(51))
        self.assertEqual(self.last()[1], voice.ERROR)
        self.assertEqual(self.host._tx.rid, 50)
        await self.host.handle(block(0, AUDIO[:512], 51))
        self.assertEqual(len(self.host._tx.audio), 0)
        await self.host.handle(begin(50, OTHER))
        self.assertEqual(self.last()[1], voice.ERROR)
        self.assertEqual(self.host._tx.target, TARGET)
        await self.host.handle(command(5, 50))
        await self.host.handle(begin(3))  # IDs need not be monotonically increasing.
        for packet in (begin(50), command(3, 50), command(4, 50), command(5, 50)):
            await self.host.handle(packet)
            self.assertEqual(self.last()[1], voice.ERROR)
            self.assertEqual(self.host._tx.rid, 3)
            self.assertEqual(self.host._tx.state, voice.RECEIVING)
        self.delivery.assert_not_awaited()

    async def test_snapshot_is_bounded_valid_and_expires(self):
        ids = [str(UUID(int=UUID(TARGET).int + i)) for i in range(257)]
        self.host.allow_targets([{}, None, {"id": "fake-id"}, {"id": 3},
                                 {"id": str(UUID(int=0))}, {"id": str(UUID(int=1))}] +
                                [{"id": uid} for uid in ids])
        self.assertEqual(len(self.host._targets), 256)
        await self.host.handle(begin(1, ids[-1]))
        self.assertEqual(self.last()[1], voice.ERROR)
        with patch("passport_voice.monotonic", return_value=voice.monotonic() + 31):
            self.host.allow_targets([{"id": OTHER.upper()}])
            await self.host.handle(begin(2, ids[0]))
            self.assertEqual(self.last()[1], voice.ERROR)
            await self.host.handle(begin(3, OTHER))
            self.assertEqual(self.last()[1], voice.RECEIVING)

    async def test_recording_grace_ttl_refresh_and_active_pin(self):
        self.host._targets.clear()
        with patch("passport_voice.monotonic", return_value=100):
            self.host.allow_targets([{"id": TARGET}])
        with patch("passport_voice.monotonic", return_value=106):
            self.host.allow_targets([{"id": OTHER}])
            await self.host.handle(begin())  # Pinned on the device six seconds ago.
            self.assertEqual(self.last()[1], voice.RECEIVING)
        with patch("passport_voice.monotonic", return_value=131):
            self.host.allow_targets([])
            self.assertNotIn(TARGET, self.host._targets)
            self.assertEqual(self.host._tx.target, TARGET)
            await self.host.handle(command(5))
            await self.host.handle(begin(2))
            self.assertEqual(self.last()[1], voice.ERROR)
            self.host.allow_targets([{"id": OTHER}])
        with patch("passport_voice.monotonic", return_value=160):
            await self.host.handle(begin(3, OTHER))
            self.assertEqual(self.last()[1], voice.RECEIVING)
            await self.host.handle(command(5, 3))
        with patch("passport_voice.monotonic", return_value=161):
            await self.host.handle(begin(4, OTHER))  # Expires even without another snapshot.
            self.assertEqual(self.last()[1], voice.ERROR)

    async def test_id_history_cap_requires_reconnect(self):
        self.host._used = set(range(1, 4097))
        await self.host.handle(begin(4097))
        self.assertEqual(self.last()[1:], (voice.ERROR, 65535, "Voice session limit reached. Reconnect device."))
        self.assertEqual(len(self.host._used), 4096)

    async def test_begin_validation_and_early_confirm(self):
        for rid, samples in enumerate((0, 4798, 4801, 96001, 96002, 0xffffffff), 1):
            await self.host.handle(begin(rid, samples=samples))
            self.assertEqual(self.last()[1], voice.ERROR)
        for packet in (b"", b"\x01", command(3, 0), begin(0), command(6, 1), begin(7) + b"x"):
            await self.host.handle(packet)
            self.assertEqual(self.last()[1], voice.ERROR)
        await self.host.handle(begin(8))
        await self.host.handle(command(4, 8))
        self.assertEqual(self.last()[1], voice.ERROR)
        self.delivery.assert_not_awaited()

    async def test_minimum_maximum_and_exact_multiple_of_block_size(self):
        for rid, count in enumerate((4800, 5120, 96000), 1):
            await self.review(rid, audio=b"\x77\xff" * (count // 4))
            self.assertEqual(self.last()[1], voice.REVIEW)
            await self.host.handle(command(5, rid))

    async def test_no_speech_and_invalid_transcripts_never_reach_delivery(self):
        await self.review(1, bytes(2400))
        self.assertEqual(self.last()[1:], (voice.ERROR, 65535, "No speech detected. Re-record."))
        self.asr.assert_not_awaited()
        for rid, text in enumerate(("", " \n\t", "[BLANK_AUDIO]", "[NO_SPEECH]", "用😀继续", "文" * 257), 2):
            self.asr.return_value = text
            await self.review(rid)
            self.assertEqual(self.last()[1], voice.ERROR)
            await self.host.handle(command(4, rid))
            self.assertEqual(self.last()[1], voice.ERROR)
        self.delivery.assert_not_awaited()
        self.asr.return_value = "文" * 256
        await self.review(100)
        self.assertEqual(self.last()[1], voice.REVIEW)
        self.assertEqual(len(self.sent[-1]), 775)

    async def test_wav_layout_private_permissions_and_cleanup(self):
        paths = []

        async def recognize(path, **kwargs):
            paths.append(path)
            self.assertEqual(stat.S_IMODE(path.parent.stat().st_mode), 0o700)
            self.assertEqual(stat.S_IMODE(path.stat().st_mode), 0o600)
            with wave.open(str(path), "rb") as wav:
                self.assertEqual((wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getnframes()),
                                 (1, 2, 16000, 4800))
                self.assertEqual(wav.readframes(4800), voice._decode_ima(AUDIO))
            return "检查"

        self.asr.side_effect = recognize
        await self.review()
        self.assertEqual(self.last()[1], voice.REVIEW)
        self.assertTrue(paths)
        self.assertFalse(paths[0].parent.exists())

    async def test_cancel_and_close_during_asr_cleanup_without_delivery(self):
        for rid, closing in ((1, False), (2, True)):
            entered = asyncio.Event()
            cancelled = asyncio.Event()
            paths = []

            async def recognize(path, **kwargs):
                paths.append(path)
                entered.set()
                try:
                    await asyncio.Event().wait()
                finally:
                    cancelled.set()

            self.asr.side_effect = recognize
            await self.upload(rid)
            await self.host.handle(command(3, rid))
            await entered.wait()
            if closing:
                await self.host.close()
            else:
                await self.host.handle(command(5, rid))
            self.assertTrue(cancelled.is_set())
            self.assertFalse(paths[0].parent.exists())
            self.assertFalse(self.host.active)
            self.assertEqual(self.last()[1:], (voice.ERROR, 65535, "Cancelled."))
        old_count = len(self.sent)
        self.host.allow_targets([{"id": TARGET}])
        await self.host.handle(begin(3))
        self.assertEqual(len(self.sent), old_count)
        self.assertFalse(self.host._targets)
        self.delivery.assert_not_awaited()

    async def test_cancel_receiving_or_review_is_terminal(self):
        await self.host.handle(begin())
        await self.host.handle(command(5))
        self.assertFalse(self.host.active)
        await self.review(2)
        await self.host.handle(command(5, 2))
        terminal = self.sent[-1]
        await self.host.handle(command(4, 2))
        self.assertEqual(self.sent[-1], terminal)
        self.delivery.assert_not_awaited()

    async def test_delivery_failure_empty_receipt_and_cancellation_are_uncertain(self):
        for rid, failure in enumerate((RuntimeError("secret /private/path"), "", None,
                                       "private CLI dump", asyncio.CancelledError()), 1):
            await self.review(rid)
            self.delivery.side_effect = failure if isinstance(failure, BaseException) else None
            self.delivery.return_value = failure
            await self.host.handle(command(4, rid))
            await self.host._tx.task
            self.assertEqual(self.last()[1:], (voice.ERROR, 65535, voice.UNCERTAIN))
            await self.host.handle(command(4, rid))
            self.assertEqual(self.delivery.await_count, rid)

    async def test_cancel_or_disconnect_during_delivery_does_not_claim_unsent(self):
        for rid, closing in ((1, False), (2, True)):
            await self.review(rid)
            entered = asyncio.Event()

            async def deliver(*args):
                entered.set()
                try:
                    await asyncio.Event().wait()
                except asyncio.CancelledError:
                    # Cancellation may happen after actual submission.
                    return "already-submitted"

            self.delivery.side_effect = deliver
            await self.host.handle(command(4, rid))
            await entered.wait()
            if closing:
                await self.host.close()
            else:
                await self.host.handle(command(5, rid))
            self.assertEqual(self.last()[1:], (voice.ERROR, 65535, voice.UNCERTAIN))
            await self.host.handle(command(4, rid))
            self.assertEqual(self.delivery.await_count, rid)
            self.assertFalse(self.host.active)

    async def test_result_write_failure_cannot_trigger_delivery_retry(self):
        await self.review()
        self.host._send_result = AsyncMock(side_effect=RuntimeError("private transport detail"))
        await self.host.handle(command(4))
        await self.host._tx.task
        self.assertEqual(self.host._tx.state, voice.SENT)
        await self.host.handle(command(4))
        self.delivery.assert_awaited_once()

    async def test_review_write_must_succeed_before_confirmation(self):
        original = self.host._send_result

        async def fail_review(packet):
            if result(packet)[1] == voice.REVIEW:
                raise RuntimeError("private transport detail")
            await original(packet)

        self.host._send_result = fail_review
        await self.review()
        self.assertFalse(self.host._tx.review_sent)
        await self.host.handle(command(4))
        self.delivery.assert_not_awaited()

    async def test_asr_errors_are_sanitized_and_reconnect_has_no_replay(self):
        self.asr.side_effect = RuntimeError("private audio text /private/path")
        await self.review()
        self.assertEqual(self.last()[1:], (voice.ERROR, 65535, "Transcription failed. Re-record."))
        await self.host.close()
        host = voice.VoiceHost(self.host._send_result, self.delivery)
        self.addAsyncCleanup(host.close)
        await host.handle(command(4))
        self.assertEqual(self.last()[1], voice.ERROR)
        await host.handle(begin())  # Previous connection's targets are also forgotten.
        self.assertEqual(self.last()[1], voice.ERROR)
        self.delivery.assert_not_awaited()

    async def test_safe_receipts_and_metadata_only_logs(self):
        for rid, receipt in enumerate(sorted(voice.DELIVERY_RECEIPTS), 1):
            self.delivery.return_value = receipt
            await self.review(rid)
            await self.host.handle(command(4, rid))
            await self.host._tx.task
            self.assertEqual(self.last()[1:], (voice.SENT, 65535, receipt))
        self.assertEqual(self.logs.call_count, 6)
        for call in self.logs.call_args_list:
            self.assertRegex(call.args[0], r"^voice (asr|delivery) rid=\d+ duration_ms=\d+ bytes=\d+$")


class DecoderTests(unittest.TestCase):
    def test_known_low_nibble_vector_and_firmware_parity(self):
        self.assertEqual(struct.unpack("<4h", voice._decode_ima(b"\x17\x8f")), (11, 17, -8, -11))
        # Compile the real decoder as an independent oracle, including both clamps.
        data = bytes(range(256)) * 20 + b"\x77" * 200 + b"\xff" * 200
        with tempfile.TemporaryDirectory() as folder:
            binary = Path(folder) / "decoder"
            source = '''#include "passport_adpcm.h"
#include <stdio.h>
int main(void) {
    passport_adpcm_state_t state;
    passport_adpcm_init(&state, 0, 0);
    int b;
    while ((b = getchar()) != EOF) {
        for (int shift = 0; shift <= 4; shift += 4) {
            uint16_t v = (uint16_t)passport_adpcm_decode_nibble(&state, (b >> shift) & 15);
            putchar(v & 255); putchar(v >> 8);
        }
    }
    return 0;
}
'''
            subprocess.run(["cc", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "main"),
                            "-x", "c", "-", str(ROOT / "main/passport_adpcm.c"), "-o", str(binary)],
                           input=source.encode(), capture_output=True, check=True)
            expected = subprocess.run([str(binary)], input=data, capture_output=True, check=True).stdout
            self.assertEqual(voice._decode_ima(data), expected)


class TranscribeTests(unittest.IsolatedAsyncioTestCase):
    async def test_defaults_command_output_and_private_temp_cleanup(self):
        paths = []

        async def spawn(*args, **kwargs):
            self.assertEqual(args[0], "/opt/homebrew/bin/whisper-cli")
            self.assertEqual(args[1:3], ("-m", str(Path("~/.cache/whisper/ggml-base.bin").expanduser())))
            self.assertEqual(args[3:10], ("-f", "/fake/input.wav", "-l", "zh", "-nt", "-otxt", "-of"))
            self.assertEqual(kwargs, {"stdout": asyncio.subprocess.PIPE, "stderr": asyncio.subprocess.PIPE})
            prefix = Path(args[-1])
            paths.append(prefix.parent)
            self.assertEqual(stat.S_IMODE(prefix.parent.stat().st_mode), 0o700)
            prefix.with_suffix(".txt").write_text(" 检查\n继续 \n", encoding="utf-8")
            child = AsyncMock()
            child.returncode = 0
            child.communicate.return_value = (b"private stdout", b"private paths")
            return child

        with patch.object(Path, "is_file", return_value=True), patch("passport_voice.asyncio.create_subprocess_exec", side_effect=spawn):
            text = await voice.transcribe("/fake/input.wav", model=None, whisper=None)
        self.assertEqual(text, "检查 继续")
        self.assertFalse(paths[0].exists())

    async def test_child_is_killed_and_reaped_on_timeout_or_cancel(self):
        for cancelling in (False, True):
            with self.subTest(cancelling=cancelling):
                started = asyncio.Event()
                paths = []

                class Child:
                    returncode = None
                    killed = False
                    calls = 0

                    async def communicate(self):
                        self.calls += 1
                        started.set()
                        if self.killed:
                            self.returncode = -9
                            return b"", b""
                        await asyncio.Event().wait()

                    def kill(self):
                        self.killed = True

                child = Child()

                async def spawn(*args, **kwargs):
                    paths.append(Path(args[-1]).parent)
                    return child

                with patch.object(Path, "is_file", return_value=True), \
                        patch("passport_voice.asyncio.create_subprocess_exec", side_effect=spawn), \
                        patch("passport_voice.ASR_TIMEOUT", 60 if cancelling else 0.01):
                    task = asyncio.create_task(voice.transcribe("/fake/input.wav"))
                    await started.wait()
                    if cancelling:
                        task.cancel()
                        with self.assertRaises(asyncio.CancelledError):
                            await task
                    else:
                        with self.assertRaisesRegex(Exception, "Transcription timed out. Re-record."):
                            await task
                self.assertTrue(child.killed)
                self.assertEqual(child.returncode, -9)
                self.assertEqual(child.calls, 2)
                self.assertFalse(paths[0].exists())

    async def test_cancel_during_process_creation_still_reaps_child(self):
        entered = asyncio.Event()
        release = asyncio.Event()
        child = Mock()
        child.communicate = AsyncMock()
        child.returncode = None
        paths = []

        async def spawn(*args, **kwargs):
            paths.append(Path(args[-1]).parent)
            entered.set()
            await release.wait()
            return child

        with patch.object(Path, "is_file", return_value=True), patch("passport_voice.asyncio.create_subprocess_exec", side_effect=spawn):
            task = asyncio.create_task(voice.transcribe("/fake/input.wav"))
            await entered.wait()
            task.cancel()
            await asyncio.sleep(0)
            release.set()
            with self.assertRaises(asyncio.CancelledError):
                await task
        child.kill.assert_called_once()
        child.communicate.assert_awaited_once()
        self.assertFalse(paths[0].exists())

    async def test_setup_and_process_errors_do_not_expose_diagnostics(self):
        with patch.object(Path, "is_file", return_value=False):
            with self.assertRaisesRegex(Exception, "Speech recognition unavailable. Check the Mac setup."):
                await voice.transcribe("/private/input.wav")
        with patch.object(Path, "is_file", return_value=True), \
                patch("passport_voice.asyncio.create_subprocess_exec", side_effect=OSError("private /path")):
            with self.assertRaisesRegex(Exception, "^Transcription failed. Re-record.$"):
                await voice.transcribe("/private/input.wav")


if __name__ == "__main__":
    unittest.main()

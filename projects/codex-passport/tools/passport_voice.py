"""Connection-local voice transactions; transport framing and Codex delivery are external.

Call allow_targets(snapshot) before publishing a snapshot, handle(reassembled_0x0e)
for each request, and await close() on disconnect. Request IDs may be unordered,
but must never be reused during a connection. Only the latest result is replayed.
Results are <IBH> (request ID, state, block index) followed by up to 768 UTF-8
bytes; non-block results use 65535. Only the fixed Codex receipts confirm success.
"""

import asyncio
from collections.abc import Awaitable, Callable, Iterable, Mapping
from dataclasses import dataclass, field
import hashlib
from pathlib import Path
import struct
import tempfile
from time import monotonic
from uuid import RFC_4122, UUID
import wave

from passport_protocol import encode_text, supported_chars

RECEIVING, TRANSCRIBING, REVIEW, SENDING, SENT, ERROR = range(1, 7)
NO_BLOCK = 65535
MAX_TEXT_BYTES = 768
ASR_TIMEOUT = 60
TARGET_TTL = 30
MAX_REQUEST_IDS = 4096
DELIVERY_RECEIPTS = frozenset(("Queued in Codex.", "Message accepted by Codex.", "Started in Codex."))
# PCM16 amplitude; tune against measured microphone noise, not ASR confidence.
SILENCE_RMS = 100
UNCERTAIN = "Delivery unconfirmed. Check Codex before retrying."
SIMPLER_WORDS = "Re-record using simpler words."

_STEPS = (
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
    253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
    1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
    3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493,
    10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623,
    27086, 29794, 32767,
)
_INDEX = (-1, -1, -1, -1, 2, 4, 6, 8)


def _decode_ima(data: bytes) -> bytes:
    """Match passport_adpcm_decode_nibble: predictor/index zero, low nibble first."""
    pcm = bytearray(len(data) * 4)
    predictor = index = offset = 0
    for byte in data:
        for nibble in (byte & 15, byte >> 4):
            step = _STEPS[index]
            diff = step >> 3
            if nibble & 4:
                diff += step
            if nibble & 2:
                diff += step >> 1
            if nibble & 1:
                diff += step >> 2
            predictor += -diff if nibble & 8 else diff
            predictor = max(-32768, min(32767, predictor))
            index = max(0, min(88, index + _INDEX[nibble & 7]))
            struct.pack_into("<h", pcm, offset, predictor)
            offset += 2
    return bytes(pcm)


class _VoiceError(Exception):
    """Only fixed, safe messages belong in this exception."""


def _preview(raw: str) -> str:
    # Deliberately normalize whitespace; never replace a word/glyph or truncate.
    text = "".join(c if c.isprintable() else " " for c in raw).strip()
    if not text or text in ("[BLANK_AUDIO]", "[NO_SPEECH]"):
        raise _VoiceError("No speech detected. Re-record.")
    if (any(c not in supported_chars() for c in text) or
            len(text.encode("utf-8")) > MAX_TEXT_BYTES or
            encode_text(text, MAX_TEXT_BYTES).decode("utf-8") != text):
        raise _VoiceError(SIMPLER_WORDS)
    return text


async def transcribe(wav_path: str | Path, model=None, whisper=None) -> str:
    """Transcribe a local WAV without delivery; return the exact display-safe text.

    Uses private temporary output and sanitized errors. The caller owns the input
    WAV; this helper never removes it. Cancelling reaps the child before returning.
    """
    model_path = Path(model or "~/.cache/whisper/ggml-base.bin").expanduser()
    whisper_path = Path(whisper or "/opt/homebrew/bin/whisper-cli").expanduser()
    process = None
    try:
        if not model_path.is_file() or not whisper_path.is_file():
            raise _VoiceError("Speech recognition unavailable. Check the Mac setup.")
        with tempfile.TemporaryDirectory(prefix="passport-asr-") as folder:
            prefix = Path(folder) / "transcript"
            spawn = asyncio.create_task(asyncio.create_subprocess_exec(
                str(whisper_path), "-m", str(model_path), "-f", str(wav_path),
                "-l", "zh", "-nt", "-otxt", "-of", str(prefix),
                stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE,
            ))
            try:
                try:
                    process = await asyncio.shield(spawn)
                except asyncio.CancelledError:
                    # Acquire and reap the child even when cancellation races creation.
                    process = await spawn
                    raise
                await asyncio.wait_for(process.communicate(), timeout=ASR_TIMEOUT)
                if process.returncode != 0:
                    raise _VoiceError("Transcription failed. Re-record.")
                return _preview(prefix.with_suffix(".txt").read_text(encoding="utf-8"))
            finally:
                if process is not None and process.returncode is None:
                    try:
                        process.kill()
                    except ProcessLookupError:
                        pass
                    await process.communicate()
    except asyncio.TimeoutError:
        raise _VoiceError("Transcription timed out. Re-record.") from None
    except _VoiceError:
        raise
    except Exception:
        raise _VoiceError("Transcription failed. Re-record.") from None


@dataclass
class _Transaction:
    rid: int
    target: str
    samples: int
    begin: bytes
    state: int = RECEIVING
    result: bytes = b""
    audio: bytearray = field(default_factory=bytearray)
    blocks: list[bytes] = field(default_factory=list)
    text: str = ""
    review_sent: bool = False
    task: asyncio.Task | None = None


class VoiceHost:
    """Use on one asyncio loop, with one instance per BLE connection.

    send_result(payload) sends an unframed result. deliver(uuid, text) must
    return one of DELIVERY_RECEIPTS only after acknowledged submission; exceptions,
    cancellation, and other receipts are uncertain and are never retried.
    Failed result writes retain state for an identical device retry.
    """

    def __init__(
        self,
        send_result: Callable[[bytes], Awaitable[None]],
        deliver: Callable[[str, str], Awaitable[str]],
        model: str | Path | None = None,
        whisper: str | Path | None = None,
    ):
        self._send_result = send_result
        self._deliver = deliver
        self.model_path = Path(model or "~/.cache/whisper/ggml-base.bin").expanduser()
        self.whisper_path = Path(whisper or "/opt/homebrew/bin/whisper-cli").expanduser()
        self._targets: dict[str, float] = {}
        self._used: set[int] = set()
        self._tx: _Transaction | None = None
        self._closed = False
        self._lock = asyncio.Lock()

    def allow_targets(self, items: Iterable[Mapping]) -> None:
        """Remember at most 256 real UUIDs advertised in the last 30 seconds.

        Invalid/non-RFC IDs are skipped. This grace period covers recording while
        snapshots change; an already begun transaction keeps its original target.
        """
        now = monotonic()
        self._prune_targets(now)
        if not self._closed:
            targets = set()
            for item in items:
                value = item.get("id") if isinstance(item, Mapping) else None
                if not isinstance(value, str):
                    continue
                try:
                    uid = UUID(value)
                except ValueError:
                    continue
                if uid.variant != RFC_4122 or uid.version not in range(1, 9):
                    continue
                targets.add(str(uid))
                if len(targets) == 256:
                    break
            for target in targets:
                self._targets.pop(target, None)
                self._targets[target] = now
            # Insertion order preserves recency, including equal clock readings.
            self._targets = dict(list(self._targets.items())[-256:])

    def _prune_targets(self, now: float):
        self._targets = {target: seen for target, seen in self._targets.items()
                         if now - seen < TARGET_TTL}

    @property
    def active(self) -> bool:
        tx = self._tx
        return bool(tx and (tx.state < SENT or (tx.task and not tx.task.done())))

    @staticmethod
    def _packet(rid: int, state: int, text: str = "", block: int = NO_BLOCK) -> bytes:
        return struct.pack("<IBH", rid, state, block) + text.encode("utf-8")

    def _set(self, tx: _Transaction, state: int, text: str = "", block: int = NO_BLOCK):
        tx.state = state
        tx.result = self._packet(tx.rid, state, text, block)
        if state in (SENT, ERROR):
            tx.audio.clear()
            tx.text = ""

    async def _send(self, packet: bytes) -> bool:
        try:
            await self._send_result(packet)
            return True
        except Exception:
            # A transport retry may replay the result, but never the delivery.
            return False

    async def _reply(self, tx: _Transaction):
        if await self._send(tx.result) and tx.state == REVIEW:
            tx.review_sent = True

    def _fail(self, tx: _Transaction, message: str):
        if tx.state == SENDING:
            message = UNCERTAIN
        self._set(tx, ERROR, message)
        if tx.task and tx.task is not asyncio.current_task() and not tx.task.done():
            tx.task.cancel()

    async def _reject(self, rid: int, message: str):
        tx = self._tx
        if tx and tx.rid == rid and tx.state < SENT:
            self._fail(tx, message)
            await self._reply(tx)
        else:
            await self._send(self._packet(rid, ERROR, message))

    async def handle(self, payload: bytes) -> None:
        """Accept one complete opcode payload, never an individual BLE frame."""
        async with self._lock:
            if self._closed:
                return
            await self._handle(bytes(payload))
            tx = self._tx
            pending = tx.task if tx and tx.state == ERROR else None
        if pending and not pending.done():
            await asyncio.gather(pending, return_exceptions=True)

    async def _handle(self, payload: bytes):
        if len(payload) < 5:
            await self._reject(0, "Invalid voice request.")
            return
        op, rid = struct.unpack_from("<BI", payload)
        if not rid:
            await self._reject(rid, "Invalid request ID.")
            return
        if not ((op == 1 and len(payload) == 25) or
                (op == 2 and 8 <= len(payload) <= 519) or
                (op in (3, 4, 5) and len(payload) == 5)):
            await self._reject(rid, "Invalid voice request.")
            return
        tx = self._tx
        if op == 1:
            if tx and rid == tx.rid:
                if payload != tx.begin:
                    await self._send(self._packet(rid, ERROR, "Request ID already used."))
                elif tx.state == RECEIVING:
                    await self._send(self._packet(rid, RECEIVING))
                else:
                    await self._reply(tx)
                return
            if rid in self._used:
                await self._reject(rid, "Stale voice request.")
                return
            if self.active:
                await self._reject(rid, "Another voice request is active.")
                return
            if len(self._used) >= MAX_REQUEST_IDS:
                await self._reject(rid, "Voice session limit reached. Reconnect device.")
                return
            _, _, target, samples = struct.unpack("<BI16sI", payload)
            tx = _Transaction(rid, str(UUID(bytes=target)), samples, payload)
            self._tx = tx
            self._used.add(rid)
            self._prune_targets(monotonic())
            if tx.target not in self._targets:
                self._fail(tx, "Task unavailable. Refresh the task list.")
            elif samples < 4800 or samples > 96000 or samples % 2:
                self._fail(tx, "Record between 0.3 and 6 seconds.")
            else:
                self._set(tx, RECEIVING)
            await self._reply(tx)
            return
        if not tx or rid != tx.rid:
            await self._reject(rid, "Unknown or stale voice request.")
            return
        if op == 2:
            block = struct.unpack_from("<H", payload, 5)[0]
            data = payload[7:]
            digest = hashlib.sha256(data).digest()
            if block < len(tx.blocks):
                if digest != tx.blocks[block]:
                    await self._reject(rid, "Audio block changed. Re-record.")
                elif tx.state == RECEIVING:
                    await self._send(self._packet(rid, RECEIVING, block=block))
                else:
                    await self._reply(tx)
                return
            if tx.state != RECEIVING:
                await self._reject(rid, "Audio is no longer being received.")
                return
            remaining = tx.samples // 2 - len(tx.audio)
            if block != len(tx.blocks) or remaining <= 0 or len(data) != min(512, remaining):
                await self._reject(rid, "Missing or invalid audio block. Re-record.")
                return
            tx.audio.extend(data)
            tx.blocks.append(digest)
            self._set(tx, RECEIVING, block=block)
            await self._reply(tx)
        elif tx.state in (SENT, ERROR):
            await self._reply(tx)
        elif op == 5:
            self._fail(tx, "Cancelled.")
            await self._reply(tx)
        elif op == 3:
            if tx.state == RECEIVING:
                if len(tx.audio) != tx.samples // 2:
                    await self._reject(rid, "Audio incomplete. Re-record.")
                    return
                self._set(tx, TRANSCRIBING)
                tx.task = asyncio.create_task(self._transcribe(tx))
            await self._reply(tx)
        elif op == 4:
            if tx.state == SENDING:
                await self._reply(tx)
            elif tx.state != REVIEW or not tx.review_sent:
                await self._reject(rid, "Review the transcript before confirming.")
            else:
                # Set state before yielding: concurrent physical confirms cannot resend.
                self._set(tx, SENDING)
                tx.task = asyncio.create_task(self._submit(tx))
                await self._reply(tx)

    async def _transcribe(self, tx: _Transaction):
        started, size = monotonic(), len(tx.audio)
        try:
            pcm = _decode_ima(tx.audio)
            tx.audio.clear()
            energy = sum(sample * sample for sample, in struct.iter_unpack("<h", pcm))
            if energy < SILENCE_RMS ** 2 * tx.samples:
                raise _VoiceError("No speech detected. Re-record.")
            with tempfile.TemporaryDirectory(prefix="passport-voice-") as folder:
                wav_path = Path(folder) / "audio.wav"
                with wave.open(str(wav_path), "wb") as wav:
                    wav.setparams((1, 2, 16000, tx.samples, "NONE", "not compressed"))
                    wav.writeframes(pcm)
                wav_path.chmod(0o600)
                del pcm
                raw = await transcribe(wav_path, model=self.model_path, whisper=self.whisper_path)
            text = _preview(raw)
            async with self._lock:
                if not self._closed and self._tx is tx and tx.state == TRANSCRIBING:
                    tx.text = text
                    self._set(tx, REVIEW, text)
                    await self._reply(tx)
        except asyncio.CancelledError:
            await self._transcription_error(tx, "Cancelled.")
        except asyncio.TimeoutError:
            await self._transcription_error(tx, "Transcription timed out. Re-record.")
        except _VoiceError as exc:
            await self._transcription_error(tx, str(exc))
        except Exception:
            await self._transcription_error(tx, "Transcription failed. Re-record.")
        finally:
            print(f"voice asr rid={tx.rid} duration_ms={int((monotonic() - started) * 1000)} bytes={size}")

    async def _transcription_error(self, tx: _Transaction, message: str):
        async with self._lock:
            if not self._closed and self._tx is tx and tx.state == TRANSCRIBING:
                self._fail(tx, message)
                await self._reply(tx)

    async def _submit(self, tx: _Transaction):
        started, size = monotonic(), len(tx.text.encode("utf-8"))
        state, message = ERROR, UNCERTAIN
        try:
            receipt = await self._deliver(tx.target, tx.text)
            if isinstance(receipt, str) and receipt in DELIVERY_RECEIPTS:
                state, message = SENT, receipt
        except (Exception, asyncio.CancelledError):
            pass
        finally:
            print(f"voice delivery rid={tx.rid} duration_ms={int((monotonic() - started) * 1000)} bytes={size}")
        async with self._lock:
            if not self._closed and self._tx is tx and tx.state == SENDING:
                self._set(tx, state, message)
                await self._reply(tx)

    async def close(self) -> None:
        """Stop work, reap any ASR child, and remove private audio/text on disconnect."""
        async with self._lock:
            self._closed = True
            self._targets.clear()
            self._used.clear()
            tx = self._tx
            if tx and tx.state < SENT:
                self._fail(tx, "Cancelled.")
                await self._reply(tx)
            pending = tx.task if tx else None
        if pending and not pending.done():
            await asyncio.gather(pending, return_exceptions=True)

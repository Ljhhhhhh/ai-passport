import struct
import sys
from pathlib import Path
import unittest
from uuid import UUID

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from passport_protocol import FrameReceiver, create_frames, pack_voice_messages


class VoiceFramesTests(unittest.TestCase):
    def test_small_mtu_large_sequence_and_crc(self):
        payload = struct.pack('<BIH', 2, 123, 40) + bytes(range(256)) * 2
        frames = create_frames(14, payload, 10)  # ATT MTU 23
        receiver = FrameReceiver()
        for frame in frames[:-1]:
            self.assertIsNone(receiver.feed(frame))
        self.assertEqual(receiver.feed(frames[-1]), (14, payload))
        corrupted = bytearray(frames[0])
        corrupted[-1] ^= 1
        with self.assertRaises(ValueError):
            receiver.feed(corrupted)
        with self.assertRaises(ValueError):
            FrameReceiver().feed(frames[1])
        receiver.feed(frames[0])
        changed_count = create_frames(14, payload + b'x', 10)[1]
        # Same count can be legitimate; explicitly modify and re-sign total instead.
        from passport_protocol import crc16_ccitt
        changed_count = bytearray(changed_count)
        changed_count[5] += 1
        changed_count[-2:] = crc16_ccitt(changed_count[:-2]).to_bytes(2, 'big')
        with self.assertRaises(ValueError):
            receiver.feed(changed_count)

    def test_identity_moves_with_card_not_position(self):
        first = dict(id='12345678-1234-4234-8234-123456789012', title='Same', status=1)
        second = dict(id='22345678-1234-4234-8234-123456789012', title='Same', status=1)
        for items in ([first, second], [second, first]):
            packet = pack_voice_messages(0, 1, items)
            self.assertEqual(len(packet), 342)
            self.assertEqual(packet[294:310], UUID(items[0]['id']).bytes)
            self.assertEqual(packet[310:326], UUID(items[1]['id']).bytes)
            self.assertEqual(packet[326:], bytes(16))


if __name__ == '__main__':
    unittest.main()

"""Offline tests. Never connect to the user's desktop or send real prompts.

Run: python3 -B -m unittest discover -s projects/codex-passport/tests -p test_codex_bridge.py
"""
import asyncio
import json
from pathlib import Path
import struct
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import codex_bridge as bridge

THREAD = "01a09396-fc0d-7aa2-bfcc-c4834d9072e2"
TEXT = "  精确文本\n$(never execute) `literal` 👋  "


def state(kind="idle", **extra):
    return {"id": THREAD, "hostId": "local", "resumeState": "resumed", "requests": [],
            "threadRuntimeStatus": {"type": kind, "activeFlags": []},
            "cwd": "/existing/task", **extra}


class FakeDesktop:
    snapshot_state = state()
    error = None
    calls = []
    closed = False

    def __init__(self, thread_id):
        self.thread_id = thread_id

    async def connect(self):
        pass

    async def snapshot(self):
        return self.snapshot_state

    async def request(self, method, params):
        self.calls.append((method, params))
        if self.error:
            raise self.error
        result = {"turnId": "existing-turn"} if method.endswith("steer-turn") else {"turn": {"id": "new-turn"}}
        return {"result": {"result": result}}

    async def close(self):
        type(self).closed = True


class BridgeTests(unittest.IsolatedAsyncioTestCase):
    def setUp(self):
        FakeDesktop.calls = []
        FakeDesktop.closed = False
        FakeDesktop.error = None
        FakeDesktop.snapshot_state = state()
        self.mock = patch.object(bridge, "_Desktop", FakeDesktop)
        self.mock.start()
        self.addCleanup(self.mock.stop)

    async def test_running_and_idle_keep_exact_text_and_settings(self):
        for kind in ("active", "idle"):
            FakeDesktop.snapshot_state = state(kind)
            receipt = json.loads(await bridge.deliver_receipt(THREAD, TEXT))
            method, params = FakeDesktop.calls[-1]
            self.assertEqual(method, "thread-follower-" + ("steer-turn" if kind == "active" else "start-turn"))
            request = params if kind == "active" else params["turnStart"]["request"]
            self.assertEqual(request["input"][0]["text"], TEXT)
            self.assertEqual(request["clientUserMessageId"], receipt["clientUserMessageId"])
            for setting in ("model", "effort", "serviceTier", "permissions", "sandboxPolicy",
                            "approvalPolicy", "environments", "collaborationMode"):
                self.assertNotIn(setting, request)
            self.assertEqual(receipt["status"], "accepted")
            self.assertFalse(receipt["queued"])
            self.assertFalse(receipt["consumed"])
            self.assertTrue(FakeDesktop.closed)

    async def test_pending_requests_and_unknown_states_never_send(self):
        blocked = [state(requests=[{"method": "item/tool/requestUserInput"}]),
                   state(requests=[{"method": "item/commandExecution/requestApproval"}]),
                   state(resumeState="needs_resume"), state("notLoaded"),
                   state(unconfirmedTurnSubmissions=[{}]), state(requests=None),
                   state(threadRuntimeStatus={"type": "active"})]
        for flag in ("waitingOnApproval", "waitingOnUserInput", "newUnknownFlag"):
            blocked.append(state(threadRuntimeStatus={"type": "active", "activeFlags": [flag]}))
        for snapshot in blocked:
            FakeDesktop.snapshot_state = snapshot
            with self.assertRaises(bridge.BridgeError):
                await bridge.deliver(THREAD, TEXT)
        self.assertEqual(FakeDesktop.calls, [])

    async def test_uncertain_write_is_not_retried(self):
        FakeDesktop.error = asyncio.TimeoutError()
        with self.assertRaises(bridge.DeliveryUncertain) as caught:
            await bridge.deliver(THREAD, TEXT)
        self.assertEqual(len(FakeDesktop.calls), 1)
        receipt = json.loads(caught.exception.receipt)
        self.assertEqual(receipt["status"], "unknown")
        self.assertEqual(receipt["threadId"], THREAD)
        self.assertNotIn(TEXT, caught.exception.receipt)

    async def test_invalid_input_never_connects(self):
        for task, text in (("task title", TEXT), (THREAD, " \n"), (THREAD, None)):
            with self.assertRaises(bridge.BridgeError):
                await bridge.deliver(task, text)
        self.assertEqual(FakeDesktop.calls, [])

    async def test_cancellation_keeps_attempt_id_without_retrying(self):
        FakeDesktop.error = asyncio.CancelledError()
        with self.assertRaises(asyncio.CancelledError) as caught:
            await bridge.deliver(THREAD, TEXT)
        self.assertEqual(json.loads(caught.exception.receipt)["status"], "unknown")
        self.assertEqual(len(FakeDesktop.calls), 1)
        self.assertTrue(FakeDesktop.closed)


class WireTests(unittest.IsolatedAsyncioTestCase):
    async def test_fragmented_frames_and_owner_pinning(self):
        client = bridge._Desktop(THREAD)
        client.client_id, client.owner = "bridge", "owner"
        client.reader = asyncio.StreamReader()
        written = []

        class Writer:
            def write(self, frame):
                size, = struct.unpack("<I", frame[:4])
                self_outer.assertEqual(size, len(frame) - 4)
                message = json.loads(frame[4:])
                written.append(message)
                if message["type"] == "request":
                    reply = {"type": "response", "requestId": message["requestId"],
                             "resultType": "success", "method": message["method"],
                             "handledByClientId": "owner", "result": {"ok": True}}
                    data = json.dumps(reply).encode()
                    frame = struct.pack("<I", len(data)) + data
                    client.reader.feed_data(frame[:2])
                    asyncio.get_running_loop().call_soon(client.reader.feed_data, frame[2:])

            async def drain(self):
                pass

        self_outer = self
        client.writer = Writer()
        response = await client.request("thread-owner-discovery", {"conversationId": THREAD})
        self.assertTrue(response["result"]["ok"])
        self.assertEqual(written[0]["targetClientId"], "owner")
        self.assertNotIn("hostId", written[0])
        self.assertEqual(written[0]["version"], 1)


if __name__ == "__main__":
    unittest.main()

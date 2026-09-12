"""Send exact text through an existing local Codex desktop task owner.

Importing this module performs no I/O. ``await deliver_receipt(thread_id, text)`` returns
a JSON receipt with status ``accepted`` after the owner's start/steer response.
Accepted means submitted to a turn, NOT consumed by the model or completed.
No CLI queue, daemon startup, task creation, resume, or approval replies occur.
An unloaded task must first be opened in the desktop. Do not automatically retry
DeliveryUncertain: reconcile its clientUserMessageId against task history first.

Private protocol verified against ChatGPT.app/Contents/Resources/app.asar:
  .vite/build/src-CCXHtyvY.js: nv (versions), jme/Fme (IPC router/client), y9
  .vite/build/main-DaMR-wdT.js: zge (follower requests), Ahe (snapshots)
  Desktop server 0.154.0-alpha.6.2; shell CLI 0.149.0 is a different server.
Protocol drift is rejected before submitting task input.
"""

import asyncio
import json
import os
from pathlib import Path
import stat
import struct
import time
import uuid


_TIMEOUT = 20
_MAX_FRAME = 32 * 1024 * 1024  # Bound memory used by a full task snapshot.
_VERSIONS = {"initialize": 0, "thread-owner-discovery": 1,
             "thread-follower-start-turn": 2, "thread-follower-steer-turn": 1}


class BridgeError(RuntimeError):
    """No delivery receipt was obtained; see DeliveryUncertain before retrying."""


class DeliveryUncertain(BridgeError):
    """A write may have reached the owner. ``receipt`` identifies that attempt."""

    def __init__(self, receipt):
        self.receipt = receipt
        super().__init__("Delivery outcome unknown; do not auto-retry. " + receipt)


class _Desktop:
    def __init__(self, thread_id):
        self.thread_id = thread_id
        self.client_id = "initializing-client"
        self.owner = None
        self.writer = None

    async def connect(self):
        root = Path(os.environ.get("CODEX_HOME", str(Path.home() / ".codex")))
        path = root / "ipc" / "ipc.sock"
        for entry, is_socket in ((path.parent, False), (path, True)):
            info = entry.lstat()
            correct_type = stat.S_ISSOCK(info.st_mode) if is_socket else stat.S_ISDIR(info.st_mode)
            if not correct_type or info.st_uid != os.getuid() or info.st_mode & 0o077:
                raise BridgeError("Desktop IPC endpoint must be private and owned by this user")
        self.reader, self.writer = await asyncio.open_unix_connection(str(path))
        initialized = await self.request("initialize", {"clientType": "codex-passport-bridge"})
        self.client_id = initialized["result"]["clientId"]
        response = await self.request("thread-owner-discovery", {
            "hostId": "local", "conversationId": self.thread_id,
        })
        self.owner = response.get("handledByClientId")
        if not isinstance(self.owner, str) or not self.owner:
            raise BridgeError("Desktop did not identify an existing task owner")

    async def write(self, message):
        data = json.dumps(message, ensure_ascii=False).encode("utf-8")
        if len(data) > _MAX_FRAME:
            raise BridgeError("Desktop IPC message exceeds the frame limit")
        self.writer.write(struct.pack("<I", len(data)) + data)
        await self.writer.drain()

    async def read(self):
        while True:
            size, = struct.unpack("<I", await self.reader.readexactly(4))
            if not 0 < size <= _MAX_FRAME:
                raise BridgeError("Invalid desktop IPC frame length")
            message = json.loads(await self.reader.readexactly(size))
            if not isinstance(message, dict):
                raise BridgeError("Invalid desktop IPC message")
            if message.get("type") == "client-discovery-request":
                # Never offer to handle incoming task actions or approval requests.
                await self.write({"type": "client-discovery-response",
                                  "requestId": message["requestId"],
                                  "response": {"canHandle": False}})
                continue
            return message

    async def request(self, method, params):
        request_id = str(uuid.uuid4())
        request = {"type": "request", "requestId": request_id,
                   "sourceClientId": self.client_id, "version": _VERSIONS[method],
                   "method": method, "params": params, "timeoutMs": 15000}
        if self.owner:
            request["targetClientId"] = self.owner
        # No top-level hostId: local follower requests use their base version.
        await self.write(request)
        while True:
            response = await self.read()
            if response.get("type") != "response" or response.get("requestId") != request_id:
                continue
            if response.get("resultType") != "success":
                error = response.get("error", "unknown-error")
                if error == "no-client-found":
                    raise BridgeError("No existing desktop owner; open this task in the desktop first")
                raise BridgeError(f"Desktop rejected {method}: {error}")
            if response.get("method") != method or (
                self.owner and response.get("handledByClientId") != self.owner
            ):
                raise BridgeError("Desktop response method/owner mismatch")
            return response

    async def snapshot(self):
        await self.write({"type": "broadcast", "method": "thread-stream-following-changed",
                          "sourceClientId": self.client_id, "targetClientIds": [self.owner],
                          "version": 1, "params": {"conversationId": self.thread_id,
                                                    "hostId": "local", "following": True}})
        while True:
            message = await self.read()
            params = message.get("params", {})
            if (message.get("type") != "broadcast"
                    or message.get("method") != "thread-stream-state-changed"
                    or message.get("sourceClientId") != self.owner
                    or params.get("conversationId") != self.thread_id
                    or params.get("hostId") != "local"):
                continue
            if message.get("version") != 11:
                raise BridgeError("Unsupported desktop snapshot protocol; revalidate the installed app")
            change = params.get("change", {})
            if change.get("type") == "snapshot":
                state = change["conversationState"]
                if state.get("id") != self.thread_id or state.get("hostId") != "local":
                    raise BridgeError("Desktop snapshot task/host mismatch")
                return state

    async def close(self):
        # The router removes this client's follower subscriptions on disconnect.
        if self.writer:
            self.writer.close()
            try:
                await self.writer.wait_closed()
            except (OSError, ConnectionError):
                pass


def _submission(thread_id, text, client_id, state):
    if state.get("resumeState") != "resumed" or state.get("unconfirmedTurnSubmissions"):
        raise BridgeError("Task is not ready or has an unconfirmed submission")
    status = state.get("threadRuntimeStatus", {})
    if not isinstance(state.get("requests"), list) or (
        status.get("type") == "active" and not isinstance(status.get("activeFlags"), list)
    ):
        raise BridgeError("Task pending-request state is unavailable")
    if state["requests"] or status.get("activeFlags"):
        raise BridgeError("Task has a pending approval/input request; answer it separately in the desktop")
    if status.get("type") not in ("active", "idle"):
        raise BridgeError("Task runtime state is unknown or unavailable")
    inputs = [{"type": "text", "text": text, "text_elements": []}]
    if status["type"] == "idle":
        return "thread-follower-start-turn", {
            "conversationId": thread_id,
            "turnStart": {"request": {"threadId": thread_id, "input": inputs,
                                       "clientUserMessageId": client_id},
                          "context": {"inheritThreadSettings": True}},
        }
    cwd = state.get("cwd")
    if not isinstance(cwd, str) or not os.path.isabs(cwd):
        raise BridgeError("Task cwd is unavailable")
    return "thread-follower-steer-turn", {
        "conversationId": thread_id, "clientUserMessageId": client_id, "input": inputs,
        "attachments": [],
        "restoreMessage": {"id": client_id, "text": text, "cwd": cwd,
                           "createdAt": int(time.time() * 1000),
                           "context": {"prompt": text, "addedFiles": [], "fileAttachments": [],
                                       "ideContext": None, "imageAttachments": [],
                                       "workspaceRoots": [cwd]}},
    }


async def deliver_receipt(thread_uuid: str, exact_text: str) -> str:
    """Return an accepted-turn JSON receipt, never a queued/consumed claim.

    A pending tool request is rejected, never answered or approved. All model,
    permission, environment and runtime settings are inherited by the owner.
    Cancellation after dispatch is uncertain too; CancelledError.receipt contains
    the attempt identifiers. Do not automatically resend.
    """
    try:
        thread_id = str(uuid.UUID(thread_uuid))
    except (ValueError, TypeError, AttributeError) as exc:
        raise BridgeError("A task UUID is required, not a title or provisional client ID") from exc
    if not isinstance(exact_text, str) or not exact_text.strip():
        raise BridgeError("Message text must not be empty")
    client_id = str(uuid.uuid4())
    receipt = {"status": "unknown", "transport": "desktop-ipc", "hostId": "local",
               "threadId": thread_id, "clientUserMessageId": client_id,
               "queued": False, "consumed": False}
    desktop = _Desktop(thread_id)
    dispatched = False
    try:
        await asyncio.wait_for(desktop.connect(), _TIMEOUT)
        state = await asyncio.wait_for(desktop.snapshot(), _TIMEOUT)
        method, params = _submission(thread_id, exact_text, client_id, state)
        dispatched = True
        response = await asyncio.wait_for(desktop.request(method, params), _TIMEOUT)
        result = response["result"]["result"]
        turn_id = result.get("turnId") if method.endswith("steer-turn") else result.get("turn", {}).get("id")
        if not isinstance(turn_id, str) or not turn_id:
            raise BridgeError("Desktop did not return a turn receipt")
        receipt.update(status="accepted", turnId=turn_id)
        return json.dumps(receipt)
    except asyncio.CancelledError as exc:
        if dispatched:
            exc.receipt = json.dumps(receipt)
        raise
    except Exception as exc:
        if dispatched:
            raise DeliveryUncertain(json.dumps(receipt)) from exc
        if isinstance(exc, BridgeError):
            raise
        raise BridgeError("Could not read the existing desktop task; no prompt was sent") from exc
    finally:
        await desktop.close()


async def deliver(thread_uuid: str, exact_text: str) -> str:
    receipt = json.loads(await deliver_receipt(thread_uuid, exact_text))
    if receipt.get("status") != "accepted" or not receipt.get("turnId"):
        raise BridgeError("No accepted-turn receipt")
    print(f"[+] Voice accepted: thread={receipt['threadId']} turn={receipt['turnId']} "
          f"message={receipt['clientUserMessageId']}")
    return "Message accepted by Codex."

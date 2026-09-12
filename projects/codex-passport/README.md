<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Codex Passport (`codex-passport`)

A personal digital identity and activity companion for OpenAI Codex on the ESP32-C3 FoloToy AI Passport (240 × 320).

## Message List and Status

The device starts on **MESSAGES**. UP/DOWN cycles Home, Quota, Messages and Settings. On Messages they also move between cards and pages, including back to the first card. Double-click OK on the highlighted card to speak. On Settings, OK changes alert volume; on Home and Quota it opens QR. A changed message snapshot wakes a sleeping screen for 30 seconds.

Restart the Mac service after upgrading the firmware: `projects/codex-passport/tools/passport-sync restart`. `passport-sync logs` reports **Projects ACK** only after the device acknowledges a message page. An older firmware produces an explicit upgrade message.

Messages display running, unread completed, waiting input, and failed tasks, each showing the latest prompt or task title, status, and project name. Tasks in the same project are listed individually. Outstanding input questions take precedence as waiting input. Waiting input and failed tasks are not limited to today's records; unread completed tasks are backfilled using desktop unread IDs. Interrupted tasks are excluded. Sorted by status update time descending; empty list shows "No Messages".

Device-side read-receipt marking is not included. Short ID is displayed if title is missing. Usage statistics still come from the existing usage collector.

## Screens

- **Home**: Name, last BLE sync time, today's tokens, plus lifetime / last 7 days / streak.
- **Quota**: Three Codex login accounts, each with 5-hour and weekly limit percent.
- **Messages**: Running, unread completed, waiting input, and failed tasks, three per page.
- **Settings**: Saved alert enable and volume control.
- **QR** (OK on other pages): GitHub homepage.

Status bar: BLE, Codex state (`IDLE` / `RUN` / `WAIT` / `DONE` / `ERR`), battery. While a session is active, a live line shows the active task's project and duration.

## Buttons

| Button | Action |
| :--- | :--- |
| `UP` | Previous card on Messages; previous screen at the first card |
| `DOWN` | Next card on Messages; next screen at the last card |
| `OK` | Change volume on Settings; toggle QR on Home/Quota |
| Double-click `OK` | Start a voice reply on the highlighted message |

The screen stays on while the Codex app has unread tasks across hosts and projects. Open the corresponding tasks in Codex to clear them; device buttons do not mark tasks read. Once the unread count reaches zero, the backlight turns off after 30 seconds idle. `OK` wakes it without toggling QR. `UP` and `DOWN` do not wake the screen. Until the unread count is available, or if BLE disconnects or the app state cannot be read, the screen stays on.

The Mac reads `electron-thread-read-state-v1` (with fallback to legacy `unread-thread-ids-by-host-v1`) from `$CODEX_HOME/.codex-global-state.json` (default `~/.codex`), syncing changes every two seconds.

## Flash

Merged image (flash at `0x0`):

`build/codex-passport-full.bin`

Web flasher: connect the ESP32-C3 USB JTAG port, start address `0x0`, baud `460800`.

Or from an ESP-IDF 5.5.3 shell:

```bash
idf.py -C projects/codex-passport flash
```

Flashing the merged image from `0x0` reloads factory profile defaults.

## Sync from the Mac

The device advertises as `Codex-Passport`. Install the login agent once:

```bash
projects/codex-passport/tools/passport-sync install
```

That starts BLE sync now, again at login, and after crashes. Control it with `start` / `stop` / `restart` / `status` / `logs`, or from Raycast: Settings → Extensions → Script Commands → Add Directories → `projects/codex-passport/tools/raycast`. Search `Codex Passport`.

Foreground (no login agent): `python3 projects/codex-passport/tools/assistant.py --sync --interval 60` after `pip install bleak`. Optional: `--device <address>`, `--config path/to/config.json`, `--interval 120`.

The device is BLE-only. Wi-Fi is not used: the ESP32-C3 already runs LVGL and NimBLE without PSRAM, and the Mac must be nearby to read `~/.opencodex`.

This prefers local `~/.opencodex/usage.jsonl` (the same ledger as the OpenCodex dashboard at `/#usage`), deduplicates by `requestId`, and maps the last 30 days' top models onto the directions page. If that file is absent, it falls back to `~/.codex/sessions` and `response_id`.

A sanitized template is `projects/codex-passport/config.example.json`. Keep a real profile out of git.

## Validation

### Voice replies

Keep the Mac awake with Codex open. Set `voice_device` in the private profile JSON to the Passport BLE address shown by `passport-sync logs`, then restart the service. For a foreground session, pass `--voice-device <address>` to `assistant.py --sync`. Unbound devices retain display-only synchronization. This binding uses the Mac's BLE device identity; it is not BLE encryption or protection against a malicious paired host.

The Mac uses the installed `whisper-cli` and local Whisper model. Profile keys `whisper_path` and `whisper_model` override the defaults in `tools/passport_voice.py:transcribe`. Run a transcription check with that function before testing the device. No audio is sent to a speech cloud service; temporary audio and transcription files are removed after transcription or cancellation. Confirmed text is sent to the selected Codex task, using its existing model and permissions.

1. On Messages, use UP/DOWN to highlight a card, then double-click OK to start a voice reply.
2. Speak when the screen shows **Speak now**. Click OK to stop, or pause for five seconds. Recording also stops at the six-second recording limit. Do not keep holding OK.
3. Wait for the recognized text. UP/DOWN scrolls it; click OK to send; long OK cancels.
4. After sending, wait for the host receipt, then click OK to close. “Message accepted by Codex” means the desktop accepted the message. Follow its status on Messages and read the response in Codex.

Audio is uploaded after recording ends; recognized text appears after local transcription finishes. Live transcription and displaying the assistant's response body on the device are not available.

The desktop bridge uses Codex's private local IPC protocol. Open the target task in the desktop first; it must not be waiting for an approval or structured input. Incompatible desktop updates are rejected until the protocol is revalidated. The bridge never launches another CLI process or changes task permissions.

Voice entry sends ordinary follow-up text. It does not approve commands or answer structured approval dialogs. A disconnected session discards unconfirmed recordings. The screen remains awake throughout capture, upload, review and receipt display. A press that wakes the screen never also starts recording.

For device acceptance, test a spoken follow-up in the intended task, confirm the exact text appears once in Codex, and observe its subsequent response. Also test cancellation, silence, maximum recording duration, BLE loss and repeated recordings while checking serial heap/stack metrics. Automated host tests and firmware builds do not substitute for these checks.

### Automated checks

```bash
./tools/validate.sh --static
./tools/validate.sh --firmware codex-passport
```

## Design

[docs/software-design/codex-passport-design.md](../../docs/software-design/codex-passport-design.md)

## Text and notification behavior

The bundled OFL font covers 30,440 printable BMP characters. Unsupported characters (including emoji outside the BMP) display as `?`; UTF-8 fields are truncated only at character boundaries. Regenerate using Python with Pillow and fonttools: `python3 projects/codex-passport/tools/make_fonts.py`. The source font and license are in `assets/fonts/`. The font requires LVGL large font descriptors and a 6 MB application partition.

A new waiting-input, completed-unread, or failed task event chimes once after message synchronization. Running tasks, read/removal, paging, unchanged messages, initial connection, reconnect, and recovery from unread-source errors are silent. Events received in one poll share a chime. Audio runs separately from button processing and uses a soft C5/E5 chime with a 15 ms fade-in and decaying tail. Upgrade both firmware and the Mac sync service for event notifications.

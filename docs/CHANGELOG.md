<p align="right">
  <a href="CHANGELOG.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Changelog

## Unreleased
- Fixed Codex Passport missing and distorted glyphs with a bundled OFL font, continuous 4bpp packing, and UTF-8-safe field encoding. Message sounds now follow new actionable task events once, with silent initial/reconnect snapshots and no periodic reminders.
- Fixed Codex Passport unread state tracking by reading `electron-thread-read-state-v1` (with fallback to legacy `unread-thread-ids-by-host-v1`) and mapping unread subagents to parent tasks, preventing completed unread tasks from being prematurely removed from the device message list.
- Codex Passport quota page skips the duplicate OpenCodex `main` identity and shows the three pool accounts by alias.
- Replaced Codex Passport project summary with a message list: shows unread completed, waiting input, and failed tasks with real task title, status, and project name; updated unread matching, session backfill, empty list, and sync error status.
- Keep Codex Passport awake while the Codex app has unread tasks; start the 30-second idle timeout when the count reaches zero. Unknown unread state keeps the screen on.
- Fixed Codex Passport Projects navigation and connected transcript aggregation, BLE capability detection, acknowledged project-page delivery and on-device rendering. Added three-project paging and wake-on-change; restored collection-aware validation after the validation script had reverted to removed root paths.
- Added `projects/codex-passport`, a Codex companion on the FoloToy AI Passport: four-page LVGL UI (home with today's tokens, mileage, 13-week gold heatmap, 30-day directions) plus OK-toggle GitHub QR; NimBLE BLE sync with fragmented CRC16 frames; NVS persistence; local session analytics deduplicated by `response_id`; realtime status with auto-idle timeout.
- Codex Passport host assistant prefers `~/.opencodex/usage.jsonl` when present so OpenCodex proxy usage (including non-Codex models) syncs to the device.
- Codex Passport home page now shows lifetime / last 7 days / streak; page 2 lists three Codex login accounts' 5-hour and weekly quotas from the local OpenCodex API.
- Codex Passport host assistant `--sync` now refreshes usage and quotas on an interval and reconnects after BLE drops.
- Codex Passport home page shows the last BLE sync time (`MM-DD HH:MM`) from the host.
- Codex Passport BLE sync can run as a macOS login agent (`tools/passport-sync`) and from Raycast script commands.
- Codex Passport home page no longer shows the signature; today's token number and unit align to the left and right of the today block. Quota 5-hour percent and remaining sit in the left column above the 5-hour bar.
- Codex Passport turns the backlight off after 10 seconds idle; `OK` wakes it without toggling QR.
- Added `projects/hanzi-cards`, an offline 50-card toddler flashcard app: cream picture-book layout, wrap-around paging, 60-second sleep with wake-without-turn, I4 illustrations, Tingting IMA ADPCM speech, and host tests for the card model, codec, and assets.
- Upgraded `projects/animal-hanzi-story` into the 3-day toddler literacy MVP game ("Hanzi World"): direct 2-choice card interface for UP/DOWN buttons, multi-stage cognitive tracking (Seen/Prompted/Unprompted/Detached), day-based progressive storyline introducing `shui` (water) with a permanent river world effect, 60-second sandbox magic mode, and host-tested state and persistence logic.
- Fixed the generated LVGL sparse font cmaps so all configured Chinese UI and 80 px focus glyphs render instead of missing-glyph boxes; static validation now checks generated glyph coverage.
- Refactored the repository into an AI Passport Project Collection (monorepo project set): moved individual applications into `projects/` subdirectories (`projects/animal-hanzi-story` and `projects/bsp-demo`), added `tools/create_project.py` for scaffolding new projects, and updated build and validation tooling.
- Replaced the wooden-fish demo with the offline two-episode Animal Hanzi Story app, including three-button learning flows, unlock persistence, embedded Mandarin prompts, IMA ADPCM streaming, pixel-art scenes, and host-tested story/save/codec logic.
- Simplified the tracked repository root: moved GitHub-recognized community documents into `.github/`, moved the changelog into `docs/`, updated every reference, and added a root-document allowlist to repository checks.
- Repository-wide language policy: every maintained Markdown default `.md` file is English, Simplified Chinese uses a paired `.zh_CN.md`, and both provide language switches. Static checks reject missing peers, missing switches, and Chinese prose in English defaults.
- Phase one of the AI development workflow: streamlined task-based context routing, unified local/CI validation, added PR checks and a template, and committed the dependency lock for reproducible builds.
- PR review fixes: pinned GitHub Actions to full commit SHAs, split build/release jobs by least privilege, disabled persisted sync checkout credentials, added Feature Request and Usage Question forms, clarified private security-report fallback, and corrected stale README, CI-trigger, and branch descriptions.
- Changed commit titles, PR titles, and PR bodies from Chinese-default to English; updated the Chinese punctuation rule so it no longer applies to PR descriptions.
- Reworked `build-firmware.yml` to pass `SDKCONFIG_DEFAULTS=sdkconfig.defaults`, enable `partitions.csv`, preserve the 8 MB image header, merge a flashable `FoloToy-AI-Passport-full.bin`, publish only that artifact, and use Actions cache v5.
- Integrated upstream PR #6 to resolve PR #4 conflicts: Wi-Fi, Bluetooth LE, radio lifecycle, and low-power demos; a 3 MB factory partition; build/menu/configuration updates; hardware-guide coverage; and bilingual capability tables.
- Defined English imperative Conventional Commit formatting for both commits and PR titles.
- Removed stale sync-workflow template comments and generalized an irrelevant Redis TTL rule to cache components.
- Added Chinese punctuation, credential safety, and recoverable file-deletion conventions.
- Expanded source-comment requirements for functions, state, ownership, concurrency, timing, registers, and magic values.
- Removed AI execution instructions from product READMEs so they remain human-facing product and repository overviews.
- Added `docs/development/agent-guide.md` as the focused AI workflow guide.
- Updated `AGENTS.md`, `docs/INDEX.md`, and the development index for the agent guide.
- Documented why the root README path is reserved for fork owners and how GitHub README precedence supports it.
- Created `main-update` from the upstream-aligned baseline and combined the repository-structure, firmware-CI, and upstream-sync work.
- Corrected the merged documentation index, workflow path, project tree, and CI references.
- Moved CI documentation from software design to `docs/development/`.
- Moved fork-only documentation assets from `assets/docs/` to `docs/assets/`.
- Moved the upstream English/Chinese project READMEs under `docs/` and renamed the documentation catalog to `docs/INDEX.md`.
- Initialized `AGENTS.md`, `CLAUDE.md`, and `CHANGELOG.md`.
- Standardized the initial project README language filenames.
- Added the `docs/`, `assets/`, and `skills/` directory structure.
- Moved the upstream hardware guide into `docs/hardware-design/`.
- Standardized subdirectory README capitalization and introduced fork conventions.
- Allowed fork-owned root README and supplemental documentation content on fork `main`.
- Added and documented the fork-only supplemental-document directory.
- Moved the build CI document to its dedicated CI branch before consolidation.
- Documented clean-`main` reasons, the direct-development exception, and Actions enablement for forks.
- Split the original agent rules into contribution, development, and fork documents with a compact root index.
- Updated software-design and project README references for the new documentation structure.
- Added the documentation catalog and task-triggered routing based on the earlier repository model.
- Added bilingual contribution, code-of-conduct, security, and support documents tailored to this ESP-IDF and fork workflow.

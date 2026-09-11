<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Hanzi World (3-Day MVP Literacy Adventure)

An interactive literacy adventure for FoloToy AI Passport (ESP32-C3) designed for toddlers aged 2-3.

## Core Features

- **Characters Change the World**: Characters serve as interactive world rules (e.g. learning `shui` (water) summons a permanent river in the little world).
- **Direct 2-Choice Interaction for Toddlers**: UP button selects the top card, DOWN selects the bottom card, OK replays audio or triggers action. No multi-step cursor confirmation.
- **3-Day Structured Path**:
  - **Day 1**: Familiar characters (`da` / `xiao` / `tiao`) and button action mapping.
  - **Day 2**: Introduce new character (`shui`) and permanently reshape the world with a flowing river.
  - **Day 3**: Detached context validation and 60-second sandbox magic mode.
- **Multi-Stage Cognitive Tracking**: Mastery levels tracked across `Seen` -> `Prompted` -> `Unprompted` -> `Detached`.
- **Lightweight Vector UI and IMA ADPCM Audio**: Optimized for ESP32-C3 with zero PSRAM requirements.

## Building and Flashing

From the repository root:

```bash
# Build firmware
idf.py -C projects/animal-hanzi-story build

# Flash to connected device and monitor
idf.py -C projects/animal-hanzi-story flash monitor
```

## Running Host Tests

```bash
cc -std=c11 -Wall -Wextra -Werror -Iprojects/animal-hanzi-story/main \
    projects/animal-hanzi-story/tests/test_hanzi_story.c \
    projects/animal-hanzi-story/main/hanzi_story.c \
    projects/animal-hanzi-story/main/hanzi_save.c \
    -o /tmp/test_hanzi_story && /tmp/test_hanzi_story

cc -std=c11 -Wall -Wextra -Werror -Iprojects/animal-hanzi-story/main \
    projects/animal-hanzi-story/tests/test_hanzi_adpcm.c \
    projects/animal-hanzi-story/main/hanzi_adpcm.c \
    projects/animal-hanzi-story/main/hanzi_player.c \
    -o /tmp/test_hanzi_adpcm && /tmp/test_hanzi_adpcm
```

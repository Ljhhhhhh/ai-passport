<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Hanzi Cards

Offline flashcards for a three-year-old on FoloToy AI Passport. The device boots to the first card, reads it aloud, and otherwise only turns pages. It does not include find-the-character, quizzes, scores, levels, learning history, a parent page, networking, or settings.

## Interaction

- Boot shows `ba` / `baba` and plays `ba. baba.`
- UP previous card, DOWN next card, OK replay. Fifty cards wrap around.
- No index, group labels, or on-screen hints.
- After 60 seconds idle the backlight turns off and audio stops. The first key after sleep only wakes and rereads the current card.
- Progress is not stored. A power cycle always starts at card 1.

## Layout

240 x 320 cream picture-book page: a 200 x 164 illustration, an 88 px character, and a 22 px phrase. No chrome, buttons, or animation.

## Build and flash

```bash
idf.py -C projects/hanzi-cards build
idf.py -C projects/hanzi-cards flash monitor
```

## Host tests

```bash
cc -std=c11 -Wall -Wextra -Werror -Iprojects/hanzi-cards/main \
    projects/hanzi-cards/tests/test_cards_model.c \
    projects/hanzi-cards/main/cards_model.c \
    -o /tmp/test_cards_model && /tmp/test_cards_model
```

`./tools/validate.sh --static` also runs the ADPCM player tests and `tests/test_cards_assets.py`.

## Regenerating assets

`assets/cards/cards.json` is the only content list. Firmware does not parse JSON.

```bash
python3 projects/hanzi-cards/tools/make_cards.py --font /path/to/NotoSansSC-Regular.otf
```

The tool uses the Python standard library, Pillow, and macOS `say` / `afconvert`. It writes I4 images, IMA ADPCM clips, sparse LVGL fonts, and `main/cards_catalog.c`. Generated fonts are committed so a firmware build does not need a local TTF. See [assets/fonts/README.md](assets/fonts/README.md).

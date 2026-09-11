<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 汉字卡片

面向 3 岁儿童的离线认字卡片，运行在 FoloToy AI Passport 上。开机即显示第一张并朗读，之后只翻页。不含找一找、测验、评分、等级、学习记录、家长页、网络或设置。

## 操作

- 开机显示「爸 / 爸爸」，朗读「爸。爸爸。」
- 上键上一张，下键下一张，确认键重播。50 张首尾循环。
- 不显示序号、分组和操作提示。
- 60 秒无操作后熄屏并停止音频。熄屏后第一次按键只唤醒并朗读当前卡。
- 不保存进度。断电后仍从第 1 张开始。

## 界面

240×320 奶油色绘本页：上方 200×164 插画，中部 88px 单字，底部 22px 短语。无边框、按钮和动画。

## 编译与烧录

```bash
idf.py -C projects/hanzi-cards build
idf.py -C projects/hanzi-cards flash monitor
```

## 主机测试

```bash
cc -std=c11 -Wall -Wextra -Werror -Iprojects/hanzi-cards/main \
    projects/hanzi-cards/tests/test_cards_model.c \
    projects/hanzi-cards/main/cards_model.c \
    -o /tmp/test_cards_model && /tmp/test_cards_model
```

`./tools/validate.sh --static` 还会跑 ADPCM 播放器测试和 `tests/test_cards_assets.py`。

## 再生成资源

`assets/cards/cards.json` 是唯一内容清单。固件不解析 JSON。

```bash
python3 projects/hanzi-cards/tools/make_cards.py --font /path/to/NotoSansSC-Regular.otf
```

生成工具只用 Python 标准库、Pillow 和 macOS 的 `say` / `afconvert`，输出 I4 插画、IMA ADPCM 语音、稀疏 LVGL 字体和 `main/cards_catalog.c`。生成的字体随项目提交，固件构建不依赖本机 TTF。说明见 [assets/fonts/README.zh_CN.md](assets/fonts/README.zh_CN.md)。

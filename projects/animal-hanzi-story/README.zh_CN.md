<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 汉字小世界 (3天 MVP 幼儿识字游戏)

专为 FoloToy AI Passport (ESP32-C3) 打造的幼儿离线互动识字掌机体验（面向快 3 岁儿童）。

## 核心特性

- **汉字重塑世界**：汉字即世界规则（例如学会「水」后，世界中奔流出现一条永久常驻的清泉河流）。
- **极简直接二选一**：针对幼儿空间认知优化，按 UP 直接选上方卡片，按 DOWN 直接选下方卡片，按 OK 重播语音/触发动作，杜绝多级光标确认。
- **3 天渐进实验路径**：
  - **第 1 天**：熟字操作验证（大/小/跳），建立实体三键与动作映射。
  - **第 2 天**：引入新字「水」，二选一辨识成功后永久生成常驻河流。
  - **第 3 天**：白底脱离场景验证识字率，达成后开放 60 秒小世界自由施法沙盒。
- **多阶段认知掌握度追踪**：清晰记录 `已见 (Seen)` $\rightarrow$ `提示认出 (Prompted)` $\rightarrow$ `自主认出 (Unprompted)` $\rightarrow$ `次日脱离认出 (Detached)`。
- **轻量矢量渲染与 ADPCM 音频**：适配无 PSRAM 硬件环境，Flash 占用低、帧率稳定。

## 编译与烧录

在仓库根目录下运行：

```bash
# 编译固件
idf.py -C projects/animal-hanzi-story build

# 烧录到设备并查看日志
idf.py -C projects/animal-hanzi-story flash monitor
```

## 运行主机测试

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

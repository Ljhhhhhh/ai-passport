<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# Codex 伴侣护照 (`codex-passport`)

运行在 ESP32-C3 FoloToy AI Passport（240 × 320）上的个人证件与 Codex 用量伴侣。

## 消息列表与状态

设备开机进入 **MESSAGES**（消息列表页）。UP/DOWN 在首页、额度、消息和设置之间切换；消息页上它们还会在卡片和分页之间来回移动，可以从第 2 页回到第 1 条。双击 OK 向高亮任务录音回复。设置页按 OK 调整提示音音量；首页和额度页按 OK 打开二维码。消息列表内容发生变化时会唤醒已熄灭的屏幕，亮屏 30 秒。

更新固件后重启 Mac 服务：`projects/codex-passport/tools/passport-sync restart`。只有设备确认收到消息页后，`passport-sync logs` 才会显示 **Projects ACK**。旧固件会产生明确的升级提示。

消息列表展示运行中、未读的已完成、待回复以及失败任务，每条显示最新提示词或任务标题、当前状态及所属项目。同一项目的不同任务分别展示，存在未处理问题时优先显示待回复。待回复和失败不受仅当天记录限制，已完成任务按桌面真实未读 ID 补读历史记录。主动中断的任务不展示。列表按状态最新更新时间倒序排列，空列表显示“暂无消息”。

本项目状态页尚未启用设备端标记已读操作。缺失标题时显示短 ID。用量统计仍由原有统计采集器提供。

## 界面

- **首页**：姓名、上次 BLE 同步时间、今日 Token，底部为终身 / 近 7 天 / 连续天数。
- **额度**：三个 Codex 登录账号各自的五小时与周限额占用。
- **消息**：运行中、未读已完成、待回复、失败任务，每页三条。
- **设置**：保存提示音开关和音量。
- **二维码**（其他页按 OK）：GitHub 主页。

状态栏：蓝牙、Codex 状态（`IDLE` / `RUN` / `WAIT` / `DONE` / `ERR`）、电量。会话进行中会抽出一行当前活跃任务所属项目名和时长。

## 按键

| 按键 | 作用 |
| :--- | :--- |
| `UP` | 消息页选择上一条；在第一条处切换到上一屏 |
| `DOWN` | 消息页选择下一条；在最后一条处切换到下一屏 |
| `OK` | 设置页调整音量；首页/额度页切换二维码 |
| 双击 `OK` | 向高亮任务启动语音回复 |

Codex 应用跨主机、跨项目有未读任务时，屏幕持续常亮。在 Codex 中打开对应任务后清除未读，设备按键不会标记已读。未读数归零后，无操作 30 秒才熄灭背光。确认键亮屏且不切换二维码。`UP` / `DOWN` 不唤醒。尚未取得未读数、蓝牙断连或无法读取应用状态时，屏幕保持亮屏。

Mac 从 `$CODEX_HOME/.codex-global-state.json`（默认 `~/.codex`）读取 `electron-thread-read-state-v1`（兼容回退至旧版 `unread-thread-ids-by-host-v1`），每两秒同步变化。

## 烧录

合并固件（地址 `0x0`）：

`build/codex-passport-full.bin`

网页烧录：连接 ESP32-C3 USB JTAG，起始地址 `0x0`，波特率 `460800`。

或在 ESP-IDF 5.5.3 环境中：

```bash
idf.py -C projects/codex-passport flash
```

从 `0x0` 烧录合并镜像会写入出厂默认资料。

## 从电脑同步

设备广播名为 `Codex-Passport`。安装一次登录项：

```bash
projects/codex-passport/tools/passport-sync install
```

安装后立即开始 BLE 同步，登录时自动拉起，进程退出也会拉起。日常用 `start` / `stop` / `restart` / `status` / `logs`。或在 Raycast：Settings → Extensions → Script Commands → Add Directories → `projects/codex-passport/tools/raycast`，搜索 `Codex Passport`。

前台运行（不装登录项）：先 `pip install bleak`，再 `python3 projects/codex-passport/tools/assistant.py --sync --interval 60`。可选 `--device <地址>`、`--config 配置.json`、`--interval 120`。

设备只走 BLE，不上 Wi-Fi：ESP32-C3 无 PSRAM，已经同时跑 LVGL 和 NimBLE；用量账本也在电脑本机。

助手优先读取本机 `~/.opencodex/usage.jsonl`（与 OpenCodex 仪表盘 `/#usage` 同一份账本），按 `requestId` 去重，把最近 30 天 Token 最多的模型映射到方向页。若该文件不存在，则回退到 `~/.codex/sessions` 与 `response_id`。

无隐私示例：`projects/codex-passport/config.example.json`。真实个人配置不要提交 Git。

## 验收

### 语音回复

保持 Mac 唤醒且 Codex 已打开。在私有配置 JSON 中，将 `voice_device` 设为 `passport-sync logs` 显示的设备 BLE 地址，然后重启服务。前台运行时可向 `assistant.py --sync` 传入 `--voice-device <address>`。未绑定设备继续只同步显示。这项绑定使用 Mac 的 BLE 设备身份，不提供蓝牙加密，也不能防御恶意的已配对主机。

Mac 使用已安装的 `whisper-cli` 和本地 Whisper 模型。配置项 `whisper_path`、`whisper_model` 可覆盖 `tools/passport_voice.py:transcribe` 中的默认路径。测试设备前，先用该函数检查转写。音频不上传云端语音服务，临时音频和转写文件在完成转写或取消后清除。确认的文字会发送到所选 Codex 任务，沿用任务的模型和权限。

1. 在消息页用 UP/DOWN 高亮卡片，再双击 OK 启动语音回复。
2. 屏幕出现 **Speak now** 后再说话。短按 OK 结束录音，或静音五秒自动收音。达到六秒录音上限也会停止，无需一直按住。
3. 等待识别文字。UP/DOWN 滚动，短按 OK 发送，长按 OK 取消。
4. 发送后等待主机回执，再短按 OK 关闭。“Message accepted by Codex”表示桌面已接收消息。可在消息列表关注任务状态，在 Codex 中阅读回复。

录音结束后才上传音频，本地转写完成后才显示识别文字。目前不支持实时转写，也不在设备上显示助手回复正文。

桌面桥接使用本机 Codex 的私有 IPC 协议；目标任务须先在桌面打开，不能处于等待审批或结构化输入的状态。桌面升级造成协议不兼容时会拒绝投递，需要重新核对协议；不会另起 CLI 进程或改写任务权限。

语音输入发送普通后续消息，不批准命令，也不代答结构化审批弹窗。断连会丢弃未确认录音。录音、上传、预览和回执显示期间保持亮屏。用于亮屏的按键动作不会同时开始录音。

设备验收需向指定任务说一句后续指令，确认相同文字在 Codex 中只出现一次，并观察任务的后续响应。还需测试取消、静音、最长录音、蓝牙断连和连续录音，检查串口中的堆与栈余量。主机测试和固件编译不能代替这些实机检查。

### 自动检查

```bash
./tools/validate.sh --static
./tools/validate.sh --firmware codex-passport
```

## 设计说明

[docs/software-design/codex-passport-design.zh_CN.md](../../docs/software-design/codex-passport-design.zh_CN.md)

## 文字与提示音行为

内置 OFL 字体覆盖 30,440 个可打印 BMP 字符。不支持的字符（包括 BMP 以外的表情）显示为 `?`；UTF-8 字段仅在完整字符边界截断。安装 Pillow 与 fonttools 后运行 `python3 projects/codex-passport/tools/make_fonts.py` 可重新生成。字体源文件与许可位于 `assets/fonts/`，需要 LVGL 大字体描述符及 6 MB 应用分区。

新的待回复、未读已完成或失败任务事件在消息同步后提示一次。运行中、已读或移除、翻页、消息未变化、首次连接、重连及未读数据源故障恢复均静默。同一轮检测到的事件共用一次提示音。播放与按键处理分别运行，使用轻柔的 C5/E5 双音，带 15 毫秒渐入和衰减尾音。事件提醒需要同时升级固件与 Mac 同步服务。

# Raspberry Pi + Pure Data + Arduino Trackball MIDI Setup

本文档记录本项目在树莓派上运行 Arduino trackball MIDI 控制 Pure Data 的完整顺序。

## 目标链路

```text
Arduino + trackball
-> USB Serial
-> Python serial_midi_bridge.py
-> ALSA virtual MIDI
-> Pure Data [ctlin 1]
-> 第 1 个声音音量
```

Arduino 发送 MIDI CC：

```text
control_change channel=0 control=1 value=0-127
```

Pure Data 补丁中接收：

```text
[ctlin 1]
|
[/ 127]
|
[pack f 30]
|
[line]
|
第 1 个声音 volume slider
```

## 1. 插好硬件

```text
Arduino + trackball -> 树莓派 USB
USB 声卡/音箱 -> 树莓派 USB
```

## 2. 确认 Arduino 串口

运行：

```bash
ls /dev/ttyACM* /dev/ttyUSB*
```

本项目当前识别为：

```text
/dev/ttyACM0
```

所以 Python bridge 中应使用：

```python
SERIAL_PORT = "/dev/ttyACM0"
BAUD_RATE = 115200
```

## 3. 启动 Python MIDI bridge

运行：

```bash
python3 /home/chengming1/serial_midi_bridge.py
```

保持这个终端开着。拨动 trackball 时，应该看到类似输出：

```text
control_change channel=0 control=1 value=64
control_change channel=0 control=1 value=71
```

如果没有输出，先检查 Arduino、串口路径、trackball 接线和 Arduino 程序。

## 4. 测试音频设备

查看声卡：

```bash
aplay -l
```

本项目中 USB Audio 是：

```text
card 2: USB2.0 Device
```

测试 USB 声卡：

```bash
speaker-test -D hw:2,0 -c 2 -t sine
```

有声音后按 `Ctrl+C` 停止。

## 5. 启动 Pure Data

新开一个终端：

```bash
pd -alsa -alsamidi -audiooutdev 3 -audiobuf 200 /home/chengming1/Desktop/Puredataforerhu/erhu-dmajor.pd
```

参数说明：

```text
-alsa          使用 ALSA 音频
-alsamidi      使用 ALSA MIDI
-audiooutdev 3 使用第 3 个 PD 音频输出设备，对应 card 2 USB Audio
-audiobuf 200  增大音频缓冲，减少 xrun
```

如果仍然出现 audio xrun，可以把 buffer 增大：

```bash
pd -alsa -alsamidi -audiooutdev 3 -audiobuf 300 /home/chengming1/Desktop/Puredataforerhu/erhu-dmajor.pd
```

## 6. 设置 Pure Data MIDI 端口

在 PD 中打开：

```text
Media -> MIDI Settings
```

设置：

```text
In Ports: 1
Out Ports: 0
```

然后点击：

```text
Apply -> OK
```

Linux 版 PD 没有 Windows 那种设备下拉框是正常的。这里只是让 PD 创建 1 个 MIDI 输入端口。

## 7. 查看 ALSA MIDI 端口

新开一个终端：

```bash
aconnect -l
```

应该看到类似：

```text
client 128: 'RtMidiOut Client' [type=user]
    0 'Arduino'

client 129: 'Pure Data' [type=user]
    0 'Pure Data Midi-In 1'
```

## 8. 连接 Python bridge 到 Pure Data

如果 Python bridge 是 `128:0`，Pure Data 是 `129:0`，运行：

```bash
aconnect 128:0 129:0
```

如果编号变了，以 `aconnect -l` 中实际显示的编号为准：

```bash
aconnect Python编号:0 PureData编号:0
```

## 9. 确认连接成功

再次运行：

```bash
aconnect -l
```

应看到类似：

```text
client 128: 'RtMidiOut Client'
    0 'Arduino'
        Connecting To: 129:0
```

或者：

```text
client 129: 'Pure Data'
    0 'Pure Data Midi-In 1'
        Connected From: 128:0
```

## 10. 在 Pure Data 中测试

在 PD 中：

```text
DSP On
点击第 1 个 toggle
拨动 trackball X 轴
```

预期效果：

```text
trackball 向右 -> 第 1 个声音音量变大
trackball 向左 -> 第 1 个声音音量变小
```

注意：trackball 现在只改变第 1 个声音的音量。如果第 1 个声音没有播放，音量变化不会明显听出来。

## 常见问题

### PD MIDI Settings 没有设备选项

Linux 版 PD 这里通常只有：

```text
In Ports
Out Ports
```

这是正常的。设置 `In Ports: 1` 后，用 `aconnect` 连接。

### aconnect 中看不到 Pure Data

关闭 PD，用 ALSA MIDI 模式重新启动：

```bash
pd -alsa -alsamidi -audiooutdev 3 -audiobuf 200 /home/chengming1/Desktop/Puredataforerhu/erhu-dmajor.pd
```

然后在 PD MIDI Settings 中设置：

```text
In Ports: 1
Out Ports: 0
```

### aconnect 中看不到 Arduino

确认 Python bridge 正在运行：

```bash
python3 /home/chengming1/serial_midi_bridge.py
```

你的 Python 代码中实际创建的 MIDI port 名称是：

```python
outport = mido.open_output("Arduino", virtual=True)
```

所以在 `aconnect -l` 中会显示：

```text
client 128: 'RtMidiOut Client'
    0 'Arduino'
```

### 出现 ALSA xrun

如果出现：

```text
alsa xrun recovery apparently failed
snd_pcm_recover failed
```

通常是音频设备或 buffer 问题。优先使用：

```bash
pd -alsa -alsamidi -audiooutdev 3 -audiobuf 200 /home/chengming1/Desktop/Puredataforerhu/erhu-dmajor.pd
```

如果仍然不稳定，改成：

```bash
pd -alsa -alsamidi -audiooutdev 3 -audiobuf 300 /home/chengming1/Desktop/Puredataforerhu/erhu-dmajor.pd
```

### 确认 PD 补丁是新版

运行：

```bash
grep -n "ctlin 1" /home/chengming1/Desktop/Puredataforerhu/erhu-dmajor.pd
```

如果没有输出，说明树莓派上的 PD 文件不是已加入 MIDI 控制的版本。


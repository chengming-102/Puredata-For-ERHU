---
name: pure-data-midi
description: Use when configuring Pure Data to receive MIDI from Arduino, serial bridges, Hairless MIDI, loopMIDI, ALSA MIDI, Raspberry Pi, or trackball/sensor controllers, especially when diagnosing ctlin, MIDI port selection, aconnect, or PD audio/MIDI startup issues.
---

# Pure Data MIDI

Use this skill to help a user connect sensor data from Arduino or another serial device into Pure Data as MIDI control messages.

## Core Model

Think in four separate links:

```text
sensor/Arduino -> serial bridge -> virtual MIDI port -> Pure Data MIDI input
```

For this project:

```text
Arduino trackball -> Python serial_midi_bridge.py -> ALSA virtual MIDI -> PD [ctlin 1]
```

On Windows, Hairless MIDI and loopMIDI usually replace the Python/ALSA bridge:

```text
Arduino -> Hairless MIDI -> loopMIDI Port -> Pure Data
```

## Pure Data Patch Pattern

For one MIDI CC controlling one audio parameter:

```text
[ctlin CC_NUMBER]
|
[/ 127]
|
[pack f 30]
|
[line]
|
target GUI/audio control
```

Use `[line]` for GUI/control-rate smoothing. Use `[line~]` only when the signal chain specifically needs audio-rate ramps.

For MIDI CC 1 controlling note 1 volume:

```text
[ctlin 1]
|
[/ 127]
|
[pack f 30]
|
[line]
|
note 1 volume slider
```

## Windows Workflow

1. Upload Arduino sketch before opening Hairless. Hairless occupies the serial port.
2. Start loopMIDI and create a virtual port, for example `loopMIDI Port`.
3. Start Hairless MIDI.
4. Set Hairless:

```text
Serial Port: Arduino COM port
Baud Rate: 115200
MIDI Out: loopMIDI Port
```

5. In Pure Data:

```text
Media -> MIDI Settings -> Input Device: loopMIDI Port
```

If PD does not show the loopMIDI port, keep loopMIDI open, restart PD, and check MIDI settings again.

## Linux/Raspberry Pi Workflow

Start the serial-to-MIDI bridge first:

```bash
python3 /home/chengming1/serial_midi_bridge.py
```

Start Pure Data with ALSA MIDI:

```bash
pd -alsa -alsamidi -audiooutdev 3 -audiobuf 200 /home/chengming1/Desktop/Puredataforerhu/erhu-dmajor.pd
```

In PD:

```text
Media -> MIDI Settings
In Ports: 1
Out Ports: 0
Apply -> OK
```

On Linux, PD may not show a device dropdown. This is normal. Use ALSA `aconnect`.

List MIDI ports:

```bash
aconnect -l
```

Expected shape:

```text
client 128: 'RtMidiOut Client'
    0 'Arduino'

client 129: 'Pure Data'
    0 'Pure Data Midi-In 1'
```

Connect the bridge to PD:

```bash
aconnect 128:0 129:0
```

If client numbers differ, use the actual numbers shown by `aconnect -l`.

Verify connection by running:

```bash
aconnect -l
```

Look for `Connecting To:` or `Connected From:` between the bridge and Pure Data.

## Audio Device Notes

Check audio devices:

```bash
aplay -l
```

If USB Audio is `card 2`, PD commonly uses:

```bash
pd -alsa -alsamidi -audiooutdev 3 -audiobuf 200 PATCH.pd
```

PD audio device numbering can be one-based relative to ALSA card order:

```text
audiooutdev 1 -> ALSA card 0
audiooutdev 2 -> ALSA card 1
audiooutdev 3 -> ALSA card 2
```

If audio underruns occur, increase buffer:

```bash
pd -alsa -alsamidi -audiooutdev 3 -audiobuf 300 PATCH.pd
```

## Debug Checklist

If MIDI has no effect:

1. Confirm the bridge prints MIDI messages when the sensor moves.
2. Confirm the PD patch contains the expected `[ctlin CC_NUMBER]`.
3. Confirm PD has `In Ports: 1`.
4. Confirm `aconnect -l` shows both bridge and Pure Data.
5. Confirm `aconnect` connects bridge output to PD input.
6. Confirm the target sound is actually playing before judging volume changes.

Useful commands:

```bash
ls /dev/ttyACM* /dev/ttyUSB*
aconnect -l
grep -n "ctlin 1" /home/chengming1/Desktop/Puredataforerhu/erhu-dmajor.pd
```

If Arduino upload fails with a port error, close Hairless, Serial Monitor, or any program occupying the serial port.


# Pure Data Erhu MIDI Controller

This project runs an erhu sample performance system on a Raspberry Pi. An Arduino reads sensor input and sends MIDI bytes over USB serial. A Python bridge converts the serial MIDI stream into an ALSA virtual MIDI port, and a Pure Data patch receives the MIDI messages to trigger and control erhu samples.

## Signal Flow

```text
Arduino sensors
-> USB Serial
-> Python MIDI bridge
-> ALSA virtual MIDI
-> Pure Data
-> USB audio output
```

## Project Files

```text
erhu-dmajor.pd                 Main Pure Data patch
1.WAV - 8.WAV                  Erhu sample audio files
mpr121.ino                     Arduino sensor and MIDI output sketch
serial_midi_bridge_notes_cc.py Serial MIDI to ALSA virtual MIDI bridge
README.md                      Project setup and usage guide
```

## Hardware

- Raspberry Pi
- Arduino
- MPR121 capacitive touch sensor
- IR sensor
- USB audio interface or USB speaker

Connect the hardware as follows:

```text
Arduino -> Raspberry Pi USB
USB audio device -> Raspberry Pi USB
```

## MIDI Mapping

The Arduino sends standard MIDI bytes over serial at 115200 baud.

Touch inputs are mapped to six Pure Data sound toggles:

```text
MPR121 pad 1  -> MIDI note 60 -> sound 1
MPR121 pad 3  -> MIDI note 61 -> sound 2
MPR121 pad 5  -> MIDI note 62 -> sound 3
MPR121 pad 6  -> MIDI note 63 -> sound 4
MPR121 pad 8  -> MIDI note 64 -> sound 5
MPR121 pad 10 -> MIDI note 65 -> sound 6
```

Continuous controllers:

```text
CC 11 -> bow / volume control
CC 12 -> IR sensor state
CC 1  -> legacy sound 1 volume control in Pure Data
```

Main MIDI receivers in the Pure Data patch:

```text
[notein]   -> route 60 61 62 63 64 65
[ctlin 11] -> bow / volume control
[ctlin 1]  -> sound 1 volume control
```

## Dependencies

Install these on the Raspberry Pi:

- Pure Data
- ALSA MIDI tools
- Python 3
- Python packages: `pyserial`, `mido`, `python-rtmidi`

Example installation:

```bash
sudo apt update
sudo apt install puredata alsa-utils python3-pip
pip3 install pyserial mido python-rtmidi
```

## Quick Start

### 1. Upload the Arduino Sketch

Upload `mpr121.ino` to the Arduino.

Default settings:

```text
Serial baud: 115200
MIDI channel: 1
IR pin: 6
Bow CC: 11
IR state CC: 12
```

### 2. Check the Arduino Serial Port

```bash
ls /dev/ttyACM* /dev/ttyUSB*
```

Common result:

```text
/dev/ttyACM0
```

If the port is different, update `serial_midi_bridge_notes_cc.py`:

```python
SERIAL_PORT = "/dev/ttyACM0"
BAUD_RATE = 115200
```

### 3. Start the Python MIDI Bridge

```bash
python3 serial_midi_bridge_notes_cc.py
```

Keep this terminal running. When you touch the MPR121 pads or trigger the IR sensor, you should see output similar to:

```text
note_on channel=0 note=60 velocity=100
note_off channel=0 note=60 velocity=0
control_change channel=0 control=11 value=64
```

The script creates this ALSA virtual MIDI output port:

```text
ArduinoSensors
```

### 4. Test the Audio Output

List audio devices:

```bash
aplay -l
```

Test the USB audio device:

```bash
speaker-test -D hw:2,0 -c 2 -t sine
```

Press `Ctrl+C` once you hear sound. If your USB audio device is not `card 2`, replace `hw:2,0` with the actual device number.

### 5. Start Pure Data

```bash
pd -alsa -alsamidi -audiooutdev 3 -audiobuf 200 /home/chengming1/Desktop/Puredataforerhu/erhu-dmajor.pd
```

Parameter notes:

```text
-alsa          Use ALSA audio
-alsamidi      Use ALSA MIDI
-audiooutdev 3 Pure Data audio output device number
-audiobuf 200  Increase audio buffer size to reduce xrun errors
```

If the audio is unstable, increase the buffer:

```bash
pd -alsa -alsamidi -audiooutdev 3 -audiobuf 300 /home/chengming1/Desktop/Puredataforerhu/erhu-dmajor.pd
```

### 6. Configure Pure Data MIDI

In Pure Data, open:

```text
Media -> MIDI Settings
```

Set:

```text
In Ports: 1
Out Ports: 0
```

Then click:

```text
Apply -> OK
```

On Linux, Pure Data may not show the same MIDI device dropdowns as the Windows version. This is normal. The setting above creates one MIDI input port for Pure Data.

### 7. Connect MIDI Ports

List ALSA MIDI ports:

```bash
aconnect -l
```

You should see something like:

```text
client 128: 'RtMidiOut Client' [type=user]
    0 'ArduinoSensors'

client 129: 'Pure Data' [type=user]
    0 'Pure Data Midi-In 1'
```

Connect the Python bridge to Pure Data:

```bash
aconnect 128:0 129:0
```

If the client numbers are different, use the actual numbers shown by `aconnect -l`.

### 8. Test Performance

In Pure Data:

```text
DSP On
Touch an MPR121 pad
Trigger the IR sensor
```

Expected behavior:

```text
MPR121 touch -> toggles the corresponding sample sound
IR movement  -> sends CC 11 to control bow / volume behavior
```

## Troubleshooting

### The Python bridge prints no MIDI messages

Check:

- `mpr121.ino` has been uploaded to the Arduino.
- `SERIAL_PORT` matches the actual Arduino serial port.
- The Arduino USB connection is working.
- The MPR121 and IR sensor wiring is correct.

### `aconnect -l` does not show Pure Data

Make sure Pure Data was started with ALSA MIDI enabled:

```bash
pd -alsa -alsamidi -audiooutdev 3 -audiobuf 200 /home/chengming1/Desktop/Puredataforerhu/erhu-dmajor.pd
```

Then configure MIDI settings in Pure Data:

```text
In Ports: 1
Out Ports: 0
```

### `aconnect -l` does not show ArduinoSensors

Make sure the Python bridge is running:

```bash
python3 serial_midi_bridge_notes_cc.py
```

The virtual MIDI port name is defined in the script:

```python
MIDI_PORT_NAME = "ArduinoSensors"
```

### ALSA xrun errors

If you see:

```text
alsa xrun recovery apparently failed
snd_pcm_recover failed
```

Increase the Pure Data audio buffer:

```bash
pd -alsa -alsamidi -audiooutdev 3 -audiobuf 300 /home/chengming1/Desktop/Puredataforerhu/erhu-dmajor.pd
```

If the problem continues, check the USB audio device number and close other programs that may be using the audio device.

### Confirm the Pure Data Patch Includes MIDI Control

```bash
grep -n "notein\|ctlin" /home/chengming1/Desktop/Puredataforerhu/erhu-dmajor.pd
```

The output should include `notein`, `ctlin 11`, and `ctlin 1`.

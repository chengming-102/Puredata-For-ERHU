import serial
import mido

SERIAL_PORT = "/dev/ttyACM0"
BAUD_RATE = 115200
MIDI_PORT_NAME = "ArduinoSensors"

print("Opening serial port:", SERIAL_PORT)
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

print("Creating virtual MIDI output:", MIDI_PORT_NAME)
outport = mido.open_output(MIDI_PORT_NAME, virtual=True)

running_status = None
data_bytes = []


def needed_data_bytes(status):
    message_type = status & 0xF0
    if message_type in (0x80, 0x90, 0xB0):
        return 2
    return None


def send_midi(status, data):
    channel = status & 0x0F
    message_type = status & 0xF0

    if message_type == 0x80:
        msg = mido.Message(
            "note_off",
            channel=channel,
            note=data[0],
            velocity=data[1],
        )
    elif message_type == 0x90:
        msg = mido.Message(
            "note_on",
            channel=channel,
            note=data[0],
            velocity=data[1],
        )
    elif message_type == 0xB0:
        msg = mido.Message(
            "control_change",
            channel=channel,
            control=data[0],
            value=data[1],
        )
    else:
        return

    outport.send(msg)
    print(msg)


while True:
    raw = ser.read(1)
    if not raw:
        continue

    byte = raw[0]

    if byte & 0x80:
        if needed_data_bytes(byte) is not None:
            running_status = byte
            data_bytes = []
        else:
            running_status = None
            data_bytes = []
        continue

    if running_status is None:
        continue

    data_bytes.append(byte)

    expected = needed_data_bytes(running_status)
    if expected is not None and len(data_bytes) >= expected:
        send_midi(running_status, data_bytes[:expected])
        data_bytes = data_bytes[expected:]

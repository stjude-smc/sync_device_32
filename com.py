import serial

import re
import ctypes
from ctypes import c_uint8
from ctypes import c_uint16
from ctypes import c_uint32
from ctypes import c_int32
from time import sleep
from rev_pin_map import rev_pin_map

UNIFORM_TIME_DELAY = 500


def cu8(value):
    assert value < 2**8, "value exceeds 8 bit"
    return c_uint8(value)


def cu16(value):
    assert value < 2**16, "value exceeds 16 bit"
    return c_uint16(value)


def cu32(value):
    assert value < 2**32, "value exceeds 32 bit"
    return c_uint32(value)


def c32(value):
    assert -(2**31) < value < 2**31, "value exceeds 32 bit"
    return c_int32(value)


def pad(data: bytearray, length=24):
    return bytearray(data + bytearray([0] * (length - len(data))))


def w(command, arg1=0, arg2=0, ts=0, N=0, interval=0):
    _command = pad(command.encode(), 4)
    if type(arg1) is str:
        _arg1 = pad(arg1.encode(), 4)
    else:
        _arg1 = bytearray(cu32(arg1))
    _arg2 = bytearray(cu32(arg2))
    _ts = bytearray(cu32(ts))
    _N = bytearray(cu32(N))
    _interval = bytearray(cu32(interval))
    c.write(pad(_command + _arg1 + _arg2 + _ts + _N + _interval))
    response = c.readall()
    try:
        return response.decode()
    except UnicodeDecodeError:
        return response


def w_buf(command, arg1=0, arg2=0, ts=0, N=0, interval=0):
    _command = pad(command.encode(), 4)
    if type(arg1) is str:
        _arg1 = pad(arg1.encode(), 4)
    else:
        _arg1 = bytearray(cu32(arg1))
    _arg2 = bytearray(cu32(arg2))
    _ts = bytearray(cu32(ts))
    _N = bytearray(cu32(N))
    _interval = bytearray(cu32(interval))
    return pad(_command + _arg1 + _arg2 + _ts + _N + _interval)


def s(command="STA"):
    c.write(pad(command.encode()))
    response = c.readall()
    if response:
        response = response.decode()
        cts = int(re.findall(r"Current system time:\s*(\d+)\s?cts", response)[0])
        p = get_prescaler()
        print(response)
        print(f"System time in seconds: {p*cts/84_000_000:.3f}")


class UInt32(ctypes.LittleEndianStructure):
    _fields_ = [("value", ctypes.c_uint32)]

class UInt64(ctypes.LittleEndianStructure):
    _fields_ = [("value", ctypes.c_uint64)]


def uint32_to_py(buf: bytearray):
    return ctypes.cast(buf, ctypes.POINTER(UInt32)).contents.value

def uint64_to_py(buf: bytearray):
    return ctypes.cast(buf, ctypes.POINTER(UInt64)).contents.value


class Event:
    def __init__(self, c_struct_data):
        self.func = uint32_to_py(c_struct_data[0:4])
        self.arg1 = uint32_to_py(c_struct_data[4:8])
        self.arg2 = uint32_to_py(c_struct_data[8:12])
        self.timestamp = uint64_to_py(c_struct_data[12:20])
        self.N = uint32_to_py(c_struct_data[20:24])
        self.interval = uint32_to_py(c_struct_data[24:28])
        self.unit = "cts"

    def __repr__(self):
        f = self.func
        arg1 = self.arg1
        if f in ["PIN", "TGL"]:
            arg1 = rev_pin_map[arg1]
        return f"{f}({arg1:<3}, {self.arg2}) at t={self.timestamp:>11}{self.unit}. Call {self.N:>4} times every {self.interval:>10} {self.unit}"

    def map_func(self, func_map):
        self.func = func_map[str(self.func)]


def get_function_addr():
    c.flushInput()
    c.write(pad("FUN".encode()))
    return {k: v for k, v in [l.split() for l in c.readall().decode().splitlines()]}


def get_prescaler():
    c.flushInput()
    c.write(pad("VER".encode()))
    r = c.readall().decode()
    return int(re.findall(r"prescaler=(\d+)", r)[0])


def us2cts(us, prescaler=0):
    if prescaler == 0:
        prescaler = get_prescaler()

    return us * 84_000_000 // prescaler // 1_000_000


def cts2us(cts, prescaler=0):
    if prescaler == 0:
        prescaler = get_prescaler()

    return cts * 1_000_000 * prescaler // 84_000_000


def get_events(unit="ms"):
    func_map = get_function_addr()

    if unit in ["us", "ms"]:
        prescaler = get_prescaler()

    c.reset_input_buffer()
    c.write(pad("QUE".encode()))
    r = c.readall()
    events = []
    for offset in range(0, len(r), 28):  # Event is 28 bytes
        e = Event(r[offset : offset + 28])
        e.map_func(func_map)
        e.timestamp -= us2cts(UNIFORM_TIME_DELAY)
        if unit in ["us", "ms"]:
            e.unit = unit
            e.timestamp = round(cts2us(e.timestamp) * (0.001 if unit == "ms" else 1))
            e.interval = round(cts2us(e.interval) * (0.001 if unit == "ms" else 1))
        events.append(e)
    return events


def go():
    c.write(pad("GO!".encode()))


def rst():
    c.write(pad("rst".encode()))


def r():
    return c.readall().decode()


# here "lasers" is a list with Arduino pin names, like ["A1", "A2"] for Cy3, Cy5
def run_ALEX(
    exposure_time,
    lasers,
    N_bursts,
    cam_readout=12_000,
    shutter_delay=1_000,
    burst_duration=0,
    fluidics=0,
):
    offset = 0 if fluidics >= 0 else -fluidics

    N_ch = len(lasers)
    frame_period = exposure_time + shutter_delay + cam_readout

    if burst_duration < frame_period * N_ch:
        burst_duration = frame_period * N_ch

    b = w_buf("STP")  # clear event queue

    for i, laser in enumerate(lasers):
        start_ts = i * frame_period

        # shutter
        b += w_buf(
            "PPL",
            arg1=laser,
            arg2=exposure_time,
            ts=start_ts + offset,
            N=N_bursts,
            interval=burst_duration,
        )
        # camera
        b += w_buf(
            "PPL",
            arg1="D12",
            arg2=exposure_time,
            ts=start_ts + offset + shutter_delay,
            N=N_bursts,
            interval=burst_duration,
        )

    if fluidics > 0:
        b += w_buf("PPL", arg1="D2", arg2=250_000, ts=fluidics, N=1, interval=0)
    elif fluidics < 0:
        b += w_buf("PPL", arg1="D2", arg2=250_000, ts=0, N=1, interval=0)
    return b + w_buf("GO!")

def a():
    m = run_ALEX(
        exposure_time=100,
        lasers=["A0", "A1", "A2"],
        N_bursts=0,
        cam_readout=150,
        shutter_delay=20,
        burst_duration=100,
        fluidics=0,
    )
    c.write(m + w_buf("go!"))


c = serial.Serial("COM3", baudrate=115200, timeout=0.01)


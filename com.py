import serial

import re
import ctypes
from ctypes import c_uint8
from ctypes import c_uint16
from ctypes import c_uint32
from ctypes import c_int32
from time import sleep

c = serial.Serial("COM4", baudrate=115200, timeout=0.01)


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
    print(
        f"""command: {_command}
arg1:    {_arg1}\t({arg1})
arg2:    {_arg2}\t({arg2})
ts:      {_ts}\t({ts}us = {int(ts*656250/1000000)}cts)
N:       {_N}\t({N})
interval:{_interval}\t({interval}us)
"""
    )
    response = c.readall()
    if response:
        print(f"RESPONSE: {response.decode()}")


def s(command="STA"):
    c.write(pad(command.encode()))
    response = c.readall()
    if response:
        print(response.decode())


class UInt32(ctypes.LittleEndianStructure):
    _fields_ = [("value", ctypes.c_uint32)]


def uint32_to_py(buf: bytearray):
    return ctypes.cast(buf, ctypes.POINTER(UInt32)).contents.value


class Event:
    def __init__(self, c_struct_data):
        self.func = uint32_to_py(c_struct_data[0:4])
        self.arg1 = uint32_to_py(c_struct_data[4:8])
        self.arg2 = uint32_to_py(c_struct_data[8:12])
        self.timestamp = uint32_to_py(c_struct_data[12:16])
        self.N = uint32_to_py(c_struct_data[16:20])
        self.interval = uint32_to_py(c_struct_data[20:24])
        self.unit = "cts"

    def __repr__(self):
        return f"{self.func}({self.arg1}, {self.arg2}) at t={self.timestamp}{self.unit}. Call {self.N} times every {self.interval} {self.unit}"

    def map_func(self, func_map):
        self.func = func_map[str(self.func)]


def get_function_addr():
    c.flushInput()
    c.write(pad("FUN".encode()))
    return {k: v for k, v in [l.split() for l in c.readall().decode().splitlines()]}


def get_prescaler():
    c.flushInput()
    c.write(pad("VER".encode()))
    c.readline()
    r = c.readline().decode()
    return int(re.findall(r"prescaler=(\d+)", r)[0])


def get_events(unit="us"):
    func_map = get_function_addr()

    if unit == "us":
        prescaler = get_prescaler()

    c.flushInput()
    c.write(pad("QUE".encode()))
    r = c.readall()
    events = []
    for offset in range(0, len(r), 24):  # Event is 24 bytes
        e = Event(r[offset : offset + 24])
        e.map_func(func_map)
        if unit == "us":
            e.unit = "us"
            e.timestamp = cts2us(e.timestamp)
            e.interval = cts2us(e.interval)
        events.append(e)
    return events


def us2cts(us, prescaler=0):
    if prescaler == 0:
        prescaler = get_prescaler()

    return us * 84_000_000 // prescaler // 1_000_000


def cts2us(cts, prescaler=0):
    if prescaler == 0:
        prescaler = get_prescaler()

    return cts * 1_000_000 * prescaler // 84_000_000

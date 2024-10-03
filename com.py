import serial

import ctypes
from ctypes import c_uint8
from ctypes import c_uint16
from ctypes import c_uint32
from ctypes import c_int32
from time import sleep

c = serial.Serial("COM4", baudrate=115200, timeout=0.1)


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




typedef struct Event
{
    EventFunc     func;      // pointer to function to coll
    uint32_t      arg1;      // first function argument
    uint32_t      arg2;      // second function argument
    uint32_t      timestamp; // timestamp for function call
    uint32_t      N;         // number of remaining calls
    uint32_t      interval;  // interval between the calls

    bool operator<(const Event& other) const {
        return this->timestamp > other.timestamp;
    }
} Event;  // 24 bytes


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


        
        

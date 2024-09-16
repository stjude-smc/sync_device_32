import serial

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


def pad(data: bytearray, length=16):
    return bytearray(data + bytearray([0] * (length - len(data))))


def w(command, arg1, arg2=0, ts=0):
    _command = pad(command.encode(), 4)
    if type(arg1) is str:
        _arg1 = pad(arg1.encode(), 4)
    else:
        _arg1 = bytearray(cu32(arg1))
    _arg2 = bytearray(cu32(arg2))
    _ts = bytearray(cu32(ts))
    c.write(pad(_command + _arg1 + _arg2 + _ts))
    print(
        f"""command: {_command}
arg1:    {_arg1}\t({arg1})
arg2:    {_arg2}\t({arg2})
ts:      {_ts}\t({ts}us = {int(ts*656250/1000000)}cts)"""
    )
    response = c.readall()
    if response:
        print(f"RESPONSE: {response.decode()}")

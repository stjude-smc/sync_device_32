import serial

from ctypes import c_uint8
from ctypes import c_uint16
from ctypes import c_uint32
from ctypes import c_int32


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


def pad(data: bytearray, length=5):
    return data + bytearray([0] * (length - len(data)))


c = serial.Serial("COM4", baudrate=115200, timeout=0.1)


def w(command, uni32_val):
    data = pad(command.encode() + c32(uni32_val))
    c.write(data)
    print(data)


r = lambda: c.readall()

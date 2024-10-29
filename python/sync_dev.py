from __version__ import __version__
from constants import ms, MHz, UNIFORM_TIME_DELAY
from ctypes import c_int32
from ctypes import c_uint16
from ctypes import c_uint32
from ctypes import c_uint8
from enum import Enum
from rev_pin_map import rev_pin_map
from serial import Serial, SerialException
import ctypes
import datetime
import gc
import re
import time

####################################################################
#        INTEGER CONVERSION FUNCTIONS BETWEEN C AND PYTHON
####################################################################

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

class UInt32(ctypes.LittleEndianStructure):
    _fields_ = [("value", ctypes.c_uint32)]

class UInt64(ctypes.LittleEndianStructure):
    _fields_ = [("value", ctypes.c_uint64)]

def uint32_to_py(buf: bytearray):
    return ctypes.cast(buf, ctypes.POINTER(UInt32)).contents.value

def uint64_to_py(buf: bytearray):
    return ctypes.cast(buf, ctypes.POINTER(UInt64)).contents.value



####################################################################
#        HELPER FUNCTIONS
####################################################################
def _compare_versions(v1, v2):
    return {
        k: a == b
        for k, a, b in zip(["major", "minor", "patch"], v1.split("."), v2.split("."))
    }

def pad(data: bytearray, length=24):
    return bytearray(data + bytearray([0] * (length - len(data))))

def us2cts(us, prescaler=0):
    if prescaler == 0:
        prescaler = get_prescaler()

    return us * 84_000_000 // prescaler // 1_000_000


def cts2us(cts, prescaler=0):
    if prescaler == 0:
        prescaler = get_prescaler()

    return cts * 1_000_000 * prescaler // 84_000_000

def close_lost_serial_port(port_name):
    # Look through all objects and find serial.Serial instances
    for obj in gc.get_objects():
        if isinstance(obj, Serial):
            if obj.port == port_name and obj.is_open:
                obj.close()
                return True
    print(f"No open port found with name {port_name}")
    return False



####################################################################
#        SYNC DEVICE PROPERTIES (see props.h)
####################################################################

class props(Enum):
    ro_VERSION = 0
    ro_SYS_TIMER_STATUS = 1
    ro_SYS_TIMER_VALUE = 2
    ro_SYS_TIMER_OVF_COUNT = 3
    ro_SYS_TIME_s = 4
    ro_SYS_TIMER_PRESCALER = 5
    rw_DFLT_PULSE_DURATION_us = 6
    ro_WATCHDOG_TIMEOUT_ms = 7
    ro_N_EVENTS = 8
    rw_INTLCK_ENABLED = 9
    # pTIRF extension
    rw_ENABLED_LASERS = 10
    wo_OPEN_SHUTTERS = 11
    wo_CLOSE_SHUTTERS = 12

####################################################################
#        LOGGING SERIAL PORT CLASS
####################################################################
class SyncDeviceError(ValueError):
    def __init__(self, reply):
        super().__init__(reply)

class LoggingSerial(Serial):
    def __init__(self, *args, log_file=None, **kwargs):
        super().__init__(*args, **kwargs)
        if log_file == "print":
            self.log_to_stdout = True
            self.log_file = None
        else:
            self.log_to_stdout = False
            self.log_file = open(log_file, 'a') if log_file else None

    def write(self, data):
        if self.log_file or self.log_to_stdout:
            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
            arguments = data[1:]
            argument_uint32 = int.from_bytes(arguments, byteorder='little', signed=False)
            log_entry = (f"{timestamp} TX: {str(data)}\n")
            if self.log_to_stdout:
                print(log_entry, end='')
            else:
                self.log_file.write(log_entry)
                self.log_file.flush()
        super().write(data)

    def readline(self, size=None):
        data = super().readline(size)
        if data and (self.log_file or self.log_to_stdout):
            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
            log_entry = f"{timestamp} RX: {data.decode('utf-8', errors='ignore')}\n"
            if self.log_to_stdout:
                print(log_entry, end='')
            else:
                self.log_file.write(log_entry)
                self.log_file.flush()
        return data

    def close(self):
        if self.log_file:
            self.log_file.close()
        super().close()

class Port(LoggingSerial):
    def __enter__(self):
        self.reset_input_buffer()
        return self

    def __exit__(self, *args, **kwargs):
        reply = self.readline().strip().decode()
        if reply.startswith("ERR"):
            raise SyncDeviceError(reply)



####################################################################
#        EVENT FROM DEVICE PRIORITY QUEUE
####################################################################

class Event:
    def __init__(self, c_struct_data):
        self.func = uint32_to_py(c_struct_data[0:4])
        self.arg1 = uint32_to_py(c_struct_data[4:8])
        self.arg2 = uint32_to_py(c_struct_data[8:12])
        self.ts = uint64_to_py(c_struct_data[12:20])
        self.N = uint32_to_py(c_struct_data[20:24])
        self.intvl = uint32_to_py(c_struct_data[24:28])
        self.unit = "cts"

    def __repr__(self):
        f = self.func
        arg1 = self.arg1
        if f in ["SET_PIN", "TGL_PIN"]:
            arg1 = rev_pin_map[arg1]
        return (f"{f}({arg1:<3}, {self.arg2:<3}) at " 
              + f"t={self.ts:>11}{self.unit}. Call "
              + f"{self.N:>6} times every {self.intvl:>10} {self.unit}")

    def map_func(self, func_map):
        self.func = func_map[str(self.func)]



####################################################################
#        SYNC DEVICE INTERFACE CLASS
####################################################################

class SyncDevice(object):
    """
    Python representation of 32-bit microscope synchronization
    device based on Arduino Due.
    """
    def __init__(self, port, log_file=None):
        self._pending_tx_ = bytearray()
        self._in_context = False

        try:
            self.com = Port(port, baudrate=115200, log_file=log_file)
        except SerialException as e:
            close_lost_serial_port(port)
            print(f"{port} is in use. Closing and re-opening...")
            self.com = Port(port, baudrate=115200, log_file=log_file)

        # Opening of the serial port resets MCU
        # We are going to wait for it to start up
        self.com.timeout = 1000 * ms
        msg = self.com.readline().strip().decode()
        self.com.timeout = 20 * ms
        time.sleep(0.025)

        # Ensure that firmware and driver have the same version
        msg_template = "Sync device is ready. Firmware version: "
        if msg.find(msg_template) != 0:
            raise ConnectionError(
                f"Could not connect to Arduino on port {port};\n"
                + f"Received message:\n{msg}\n"
                + f"Expected message:\n{msg_template + __version__}"
            )

        v = self.version
        version_match = _compare_versions(v, __version__)
        if not version_match["major"] or not version_match["minor"]:
            raise RuntimeWarning(
                f"Version mismatch: driver {__version__} != firmware {v}"
            )

        self.func_map = self.get_function_addr()


    def __enter__(self):
        self._in_context = True
        return self

    def __exit__(self, *args, **kwargs):
        self._in_context = False
        if(self._pending_tx_):
            self.com.write(self._pending_tx_)
        self._pending_tx_ = bytearray()

    def __del__(self):
        self.com.close()

    def __repr__(self):
        return self.get_status()

    def write(self, cmd: str, arg1=0, arg2=0, ts=0, N=0, interval=0):
        self.com.reset_input_buffer()
        _command = pad(cmd.encode(), 4)
        if type(arg1) is str:
            arg1 = pad(arg1.encode(), 4)
        else:
            arg1 = bytearray(cu32(arg1))

        data = (pad(cmd.encode(), 4)
            + arg1
            + bytearray(cu32(arg2))
            + bytearray(cu32(ts))
            + bytearray(cu32(N))
            + bytearray(cu32(interval)))

        if self._in_context:
            self._pending_tx_ += data
            return

        self.com.write(data)

    def query(self, cmd: str, arg1=0, arg2=0, ts=0, N=0, interval=0):
        if self._in_context:
            raise RuntimeError("Can't run queries inside of a context manager")

        self.write(cmd, arg1, arg2, ts, N, interval)

        reply = self.com.readline().strip().decode()
        if reply.startswith("ERR"):
            raise SyncDeviceError(reply)
        return reply

    def set_pin(self, pin, level, ts=0, N=0, interval=0):
        self.write("PIN", pin, level, ts, N, interval)

    def tgl_pin(self, pin, ts=0, N=0, interval=0):
        self.write("TGL", pin, 0, ts, N, interval)

    def pos_pulse(self, pin, duration=0, ts=0, N=0, interval=0):
        self.write("PPL", pin, duration, ts, N, interval)

    def neg_pulse(self, pin, duration=0, ts=0, N=0, interval=0):
        self.write("NPL", pin, duration, ts, N, interval)

    def pulse_train(self, period, duration=0, ts=0, N=0, interval=0):
        self.write("BST", period, duration, ts, N, interval)

    def enable_pin(self, pin, ts=0, N=0, interval=0):
        self.write("ENP", pin, 0, ts, N, interval)

    def disable_pin(self, pin, ts=0, N=0, interval=0):
        self.write("DSP", pin, 0, ts, N, interval)

    def go(self):
        """ Start sync device system timer """
        self.write("GO!")

    def stop(self):
        """ Stop sync device system timer """
        self.write("STP")

    def clear(self):
        """ Clear sync device event queue """
        self.write("CLR")

    def reset(self):
        """ Clear sync device event queue """
        self.write("RST")

    def get_status(self):
        self.write("STA")
        return self.com.readall().decode()

    def get_property(self, prop):
        if isinstance(prop, Enum):
            prop = prop.value
        return self.query("GET", prop)

    def set_property(self, prop, value):
        if isinstance(prop, Enum):
            prop = prop.value
        return self.write("SET", prop, value)

    @property
    def version(self):
        return self.get_property(props.ro_VERSION)

    @property
    def running(self):
        return self.get_property(props.ro_SYS_TIMER_STATUS) != '0'

    @property
    def sys_time_cts(self):
        cv = int(self.get_property(props.ro_SYS_TIMER_VALUE))
        ovf = int(self.get_property(props.ro_SYS_TIMER_OVF_COUNT))
        return (ovf << 32) & cv

    @property
    def sys_time_s(self):
        return float(self.get_property(props.ro_SYS_TIME_s))

    @property
    def prescaler(self):
        return int(self.get_property(props.ro_SYS_TIMER_PRESCALER))

    @property
    def pulse_duration_us(self):
        return int(self.get_property(props.rw_DFLT_PULSE_DURATION_us))

    @pulse_duration_us.setter
    def pulse_duration_us(self, value):
        self.set_property(props.rw_DFLT_PULSE_DURATION_us, value)

    @property
    def watchdog_timeout_ms(self):
        return int(self.get_property(props.ro_WATCHDOG_TIMEOUT_ms))

    @property
    def N_events(self):
        return int(self.get_property(props.ro_N_EVENTS))

    @property
    def interlock_enabled(self):
        return self.get_property(props.rw_INTLCK_ENABLED) != '0'
    
    @interlock_enabled.setter
    def interlock_enabled(self, value):
        self.set_property(props.rw_INTLCK_ENABLED, value)

    def get_function_addr(self):
        response = self.write("FUN")
        return {k: v for k, v in [l.split() for l in
            self.com.readall().decode().splitlines()]}

    def get_events(self, unit="ms"):
        presc = self.prescaler

        self.write("QUE")
        r = self.com.readall()
        events = []
        for offset in range(0, len(r), 28):  # Event is 28 bytes
            e = Event(r[offset : offset + 28])
            e.map_func(self.func_map)
            e.ts -= us2cts(UNIFORM_TIME_DELAY, presc)
            if unit in ["us", "ms"]:
                e.unit = unit
                e.ts = round(cts2us(e.ts, presc)*(0.001 if unit == "ms" else 1))
                e.intvl = round(cts2us(e.intvl, presc)*(0.001 if unit == "ms" else 1))
            events.append(e)
        return events

    ## pTIRF extension
    def open_shutters(self, mask=0):
        self.set_property(props.wo_OPEN_SHUTTERS, mask)

    def close_shutters(self, mask=0):
        self.set_property(props.wo_CLOSE_SHUTTERS, mask)

    @property
    def enabled_lasers(self):
        return int(self.get_property(props.rw_ENABLED_LASERS))

    @enabled_lasers.setter
    def enabled_lasers(self, mask):
        self.set_property(props.rw_ENABLED_LASERS, mask)


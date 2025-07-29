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
    rw_SELECTED_LASERS = 10
    wo_OPEN_SHUTTERS = 11
    wo_CLOSE_SHUTTERS = 12
    rw_SHUTTER_DELAY_us = 13
    rw_CAM_READOUT_us = 14

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
            self.log_file = log_file if log_file else None

    def _format_data(self, data):
        # Convert each byte to ASCII or hex notation
        formatted_data = ''.join(
            chr(byte) if 32 <= byte <= 126 else f'\\x{byte:02X}' 
            for byte in data
        )
        return formatted_data

    def write(self, data):
        if self.log_file or self.log_to_stdout:
            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
            log_entry = (f"{timestamp} TX: {self._format_data(data)}\n")
            if self.log_to_stdout:
                print(log_entry, end='')
            else:
                with open(self.log_file, 'a', encoding='utf-8') as f:
                    f.write(log_entry)
        super().write(data)

    def readline(self, size=None):
        data = super().readline(size)
        if data and (self.log_file or self.log_to_stdout):
            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
            log_entry = f"{timestamp} RX: {self._format_data(data)}\n"
            if self.log_to_stdout:
                print(log_entry, end='')
            else:
                with open(self.log_file, 'a', encoding='utf-8') as f:
                    f.write(log_entry)
        return data

    def readall(self):
        data = super().readall()
        if data and (self.log_file or self.log_to_stdout):
            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
            log_entry = f"{timestamp} RX: {self._format_data(data)}\n"
            if self.log_to_stdout:
                print(log_entry, end='')
            else:
                with open(self.log_file, 'a', encoding='utf-8') as f:
                    f.write(log_entry)
        return data

    def close(self):
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
    """
    Represents a scheduled event in the device's priority queue.
    
    This class contains all the information needed to execute an event,
    including the function to call, its arguments, timing, and repetition
    parameters.
    
    Attributes:
        func (str): Function name to execute
        arg1: First argument (usually pin number)
        arg2: Second argument (usually level or duration)
        ts (int): Timestamp when to execute (in specified units)
        N (int): Number of repetitions remaining
        intvl (int): Interval between repetitions
        unit (str): Time unit ("cts", "us", or "ms")
    """
    
    def __init__(self, c_struct_data):
        """
        Create an Event from raw C structure data.
        
        Args:
            c_struct_data (bytes): 28-byte C structure data from device
        """
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
    Python interface for 32-bit microscope synchronization device.
    
    This class provides a high-level API for controlling the Arduino Due-based
    synchronization device. It handles communication, event scheduling, and
    provides convenient methods for common microscope control tasks.
    
    The device supports microsecond-precision event scheduling with a priority
    queue system, laser shutter control, and various acquisition modes.
    
    Attributes:
        com: Serial communication interface
        func_map: Mapping of function addresses from device
        _pending_tx_: Buffer for context manager commands
        _in_context: Context manager state flag
    
    Example:
        >>> sd = SyncDevice("COM4")
        >>> sd.pos_pulse("A0", 1000, N=10, interval=50000)
        >>> sd.go()
    """
    
    def __init__(self, port, log_file=None):
        """
        Initialize connection to the synchronization device.
        
        Args:
            port (str): Serial port name (e.g., "COM4", "/dev/ttyUSB0")
            log_file (str, optional): Logging configuration:
                - None: No logging
                - "print": Print to terminal
                - filename: Save to file
        
        Raises:
            ConnectionError: If device connection fails
            RuntimeWarning: If firmware/driver version mismatch
        
        Example:
            >>> sd = SyncDevice("COM4", log_file="sync.log")
            >>> sd = SyncDevice("/dev/ttyUSB0", log_file="print")
        """
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
        """
        Enter context manager for batch command transmission.
        
        Returns:
            SyncDevice: Self for chaining
        
        Example:
            >>> with sd as dev:
            ...     dev.pos_pulse("A0", 1000, ts=0)
            ...     dev.pos_pulse("A1", 1000, ts=5000)
        """
        self._in_context = True
        return self

    def __exit__(self, *args, **kwargs):
        """
        Exit context manager and transmit batched commands.
        
        All commands collected within the context manager are sent as a single
        data packet to ensure precise timing and eliminate host OS jitter.
        """
        self._in_context = False
        if(self._pending_tx_):
            self.com.write(self._pending_tx_)
        self._pending_tx_ = bytearray()

    def __del__(self):
        self.com.close()

    def __repr__(self):
        return self.get_status()

    def write(self, cmd: str, arg1=0, arg2=0, ts=0, N=0, interval=0):
        """
        Send a command to the device.
        
        Args:
            cmd (str): 3-character command code
            arg1: First argument (can be string or integer)
            arg2 (int): Second argument
            ts (int): Timestamp (counter ticks)
            N (int): Number of repetitions
            interval (int): Interval between repetitions (counter ticks)
        
        Note:
            This is a low-level method. Use the high-level methods like
            pos_pulse(), tgl_pin(), etc. for most applications.
        """
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
        """
        Set a pin to a specific level.
        
        Args:
            pin (str): Pin name (e.g., "A0", "D13")
            level (int): Pin level (0=low, 1=high)
            ts (int): Timestamp (microseconds, relative to current time)
            N (int): Number of repetitions (0=infinite)
            interval (int): Interval between repetitions (microseconds)
        
        Example:
            >>> sd.set_pin("A0", 1, ts=1000)  # Set A0 high after 1ms
        """
        self.write("PIN", pin, level, ts, N, interval)

    def tgl_pin(self, pin, ts=0, N=0, interval=0):
        """
        Toggle a pin state.
        
        Args:
            pin (str): Pin name (e.g., "A0", "D13")
            ts (int): Timestamp (microseconds, relative to current time)
            N (int): Number of repetitions (0=infinite)
            interval (int): Interval between repetitions (microseconds)
        
        Example:
            >>> sd.tgl_pin("D13", N=1000, interval=1000)  # Toggle D13 every 1ms
        """
        self.write("TGL", pin, 0, ts, N, interval)

    def pos_pulse(self, pin, duration=0, ts=0, N=0, interval=0):
        """
        Generate a positive pulse on a pin.
        
        Args:
            pin (str): Pin name (e.g., "A0", "D13")
            duration (int): Pulse duration (microseconds)
            ts (int): Timestamp (microseconds, relative to current time)
            N (int): Number of repetitions (0=infinite)
            interval (int): Interval between repetitions (microseconds)
        
        Example:
            >>> sd.pos_pulse("A0", 1000, ts=5000, N=10, interval=50000)
        """
        self.write("PPL", pin, duration, ts, N, interval)

    def neg_pulse(self, pin, duration=0, ts=0, N=0, interval=0):
        """
        Generate a negative pulse on a pin.
        
        Args:
            pin (str): Pin name (e.g., "A0", "D13")
            duration (int): Pulse duration (microseconds)
            ts (int): Timestamp (microseconds, relative to current time)
            N (int): Number of repetitions (0=infinite)
            interval (int): Interval between repetitions (microseconds)
        
        Example:
            >>> sd.neg_pulse("A0", 1000, ts=5000, N=10, interval=50000)
        """
        self.write("NPL", pin, duration, ts, N, interval)

    def pulse_train(self, period, duration=0, ts=0, N=0, interval=0):
        """
        Generate a burst of pulses.
        
        Args:
            period (int): Period between pulses (microseconds)
            duration (int): Duration of each pulse (microseconds)
            ts (int): Timestamp (microseconds, relative to current time)
            N (int): Number of repetitions (0=infinite)
            interval (int): Interval between repetitions (microseconds)
        
        Example:
            >>> sd.pulse_train(1000, 100, ts=0, N=1000)  # 1000 pulses, 1kHz
        """
        self.write("BST", period, duration, ts, N, interval)

    def enable_pin(self, pin, ts=0, N=0, interval=0):
        """
        Enable a pin output.
        
        Args:
            pin (str): Pin name (e.g., "A0", "D13")
            ts (int): Timestamp (microseconds, relative to current time)
            N (int): Number of repetitions (0=infinite)
            interval (int): Interval between repetitions (microseconds)
        
        Example:
            >>> sd.enable_pin("A0", ts=1000)  # Enable A0 after 1ms
        """
        self.write("ENP", pin, 0, ts, N, interval)

    def disable_pin(self, pin, ts=0, N=0, interval=0):
        """
        Disable a pin output.
        
        Args:
            pin (str): Pin name (e.g., "A0", "D13")
            ts (int): Timestamp (microseconds, relative to current time)
            N (int): Number of repetitions (0=infinite)
            interval (int): Interval between repetitions (microseconds)
        
        Example:
            >>> sd.disable_pin("A0", ts=1000)  # Disable A0 after 1ms
        """
        self.write("DSP", pin, 0, ts, N, interval)

    def go(self):
        """
        Start the system timer and begin processing scheduled events.
        
        This method starts the hardware timer that processes the event queue.
        All scheduled events will begin executing at their specified timestamps.
        
        Example:
            >>> sd.pos_pulse("A0", 1000, ts=1000)
            >>> sd.go()  # Start processing events
        """
        self.write("GO!")

    def stop(self):
        """
        Stop the system timer and halt event processing.
        
        This method stops the hardware timer. No new events will be processed,
        but the event queue is preserved and can be resumed with go().
        
        Example:
            >>> sd.stop()  # Pause event processing
            >>> sd.go()    # Resume processing
        """
        self.write("STP")

    def clear(self):
        """
        Clear all scheduled events from the queue.
        
        This method removes all pending events from the event queue.
        The system timer is not affected - use stop() to halt processing.
        
        Example:
            >>> sd.clear()  # Remove all scheduled events
        """
        self.write("CLR")

    def reset(self):
        """
        Reset the device and clear all events.
        
        This method performs a soft reset of the device, clearing all events
        and resetting system parameters to default values.
        
        Example:
            >>> sd.reset()  # Reset device to default state
        """
        self.write("RST")

    def get_status(self):
        """
        Get detailed system status information.
        
        Returns:
            str: Multi-line status report including timer state, event count, etc.
        
        Example:
            >>> status = sd.get_status()
            >>> print(status)
        """
        self.write("STA")
        return self.com.readall().decode()

    def get_property(self, prop):
        """
        Get a device property value.
        
        Args:
            prop: Property enum or integer ID
        
        Returns:
            str: Property value as string
        
        Example:
            >>> version = sd.get_property(props.ro_VERSION)
        """
        if isinstance(prop, Enum):
            prop = prop.value
        return self.query("GET", prop)

    def set_property(self, prop, value):
        """
        Set a device property value.
        
        Args:
            prop: Property enum or integer ID
            value: New property value
        
        Example:
            >>> sd.set_property(props.rw_DFLT_PULSE_DURATION_us, 1000)
        """
        if isinstance(prop, Enum):
            prop = prop.value
        return self.write("SET", prop, value)

    @property
    def version(self):
        """
        Get the firmware version.
        
        Returns:
            str: Firmware version string (e.g., "2.3.0")
        """
        return self.get_property(props.ro_VERSION)

    @property
    def running(self):
        """
        Check if the system timer is running.
        
        Returns:
            bool: True if system timer is active, False otherwise
        """
        return self.get_property(props.ro_SYS_TIMER_STATUS) != '0'

    @property
    def sys_time_cts(self):
        """
        Get current system time in counter ticks.
        
        Returns:
            int: 64-bit system time in counter ticks
        """
        cv = int(self.get_property(props.ro_SYS_TIMER_VALUE))
        ovf = int(self.get_property(props.ro_SYS_TIMER_OVF_COUNT))
        return (ovf << 32) & cv

    @property
    def sys_time_s(self):
        """
        Get current system time in seconds.
        
        Returns:
            float: System time in seconds
        """
        return float(self.get_property(props.ro_SYS_TIME_s))

    @property
    def prescaler(self):
        """
        Get the timer prescaler value.
        
        Returns:
            int: Timer prescaler (2, 8, 32, or 128)
        """
        return int(self.get_property(props.ro_SYS_TIMER_PRESCALER))

    @property
    def pulse_duration_us(self):
        """
        Get the default pulse duration.
        
        Returns:
            int: Default pulse duration in microseconds
        """
        return int(self.get_property(props.rw_DFLT_PULSE_DURATION_us))

    @pulse_duration_us.setter
    def pulse_duration_us(self, value):
        """
        Set the default pulse duration.
        
        Args:
            value (int): Default pulse duration in microseconds
        """
        self.set_property(props.rw_DFLT_PULSE_DURATION_us, value)

    @property
    def watchdog_timeout_ms(self):
        """
        Get the watchdog timeout value.
        
        Returns:
            int: Watchdog timeout in milliseconds
        """
        return int(self.get_property(props.ro_WATCHDOG_TIMEOUT_ms))

    @property
    def N_events(self):
        """
        Get the number of scheduled events.
        
        Returns:
            int: Number of events currently in the queue
        """
        return int(self.get_property(props.ro_N_EVENTS))

    @property
    def interlock_enabled(self):
        """
        Check if the laser interlock is enabled.
        
        Returns:
            bool: True if interlock is active, False otherwise
        """
        return self.get_property(props.rw_INTLCK_ENABLED) != '0'
    
    @interlock_enabled.setter
    def interlock_enabled(self, value):
        """
        Enable or disable the laser interlock.
        
        Args:
            value (bool): True to enable interlock, False to disable
        """
        self.set_property(props.rw_INTLCK_ENABLED, value)

    def get_function_addr(self):
        """
        Get the function address mapping from the device.
        
        Returns:
            dict: Mapping of function names to their addresses
        
        Note:
            This is used internally to map function addresses to readable names.
        """
        response = self.write("FUN")
        return {k: v for k, v in [l.split() for l in
            self.com.readall().decode().splitlines()]}

    def get_events(self, unit="ms"):
        """
        Get all scheduled events from the device queue.
        
        Args:
            unit (str): Time unit for timestamps ("cts", "us", or "ms")
        
        Returns:
            list: List of Event objects representing scheduled events
        
        Example:
            >>> events = sd.get_events("us")
            >>> for event in events:
            ...     print(f"{event.func} at {event.ts} {event.unit}")
        """
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
        """
        Open laser shutters.
        
        Args:
            mask (int): Bitmask specifying which shutters to open:
            
                mask (int): Bitmask specifying which shutters to open:

                    * 0: Open all shutters
                    * 1: Cy2 only
                    * 2: Cy3 only
                    * 4: Cy5 only
                    * 8: Cy7 only
                    * Combinations: 1|2|4 = Cy2, Cy3, and Cy5
        
        Example:
            >>> sd.open_shutters()  # Open all shutters
            >>> sd.open_shutters(1 | 4)  # Open Cy2 and Cy5 only
        """
        self.set_property(props.wo_OPEN_SHUTTERS, mask)

    def close_shutters(self, mask=0):
        """
        Close laser shutters.
        
        Args:
            mask (int): Bitmask specifying which shutters to close:
                - 0: Close all shutters
                - 1: Cy2 only
                - 2: Cy3 only
                - 4: Cy5 only
                - 8: Cy7 only
                - Combinations: 1|2|4 = Cy2, Cy3, and Cy5
        
        Example:
            >>> sd.close_shutters()  # Close all shutters
            >>> sd.close_shutters(2)  # Close Cy3 only
        """
        self.set_property(props.wo_CLOSE_SHUTTERS, mask)

    @property
    def selected_lasers(self):
        """
        Get the currently selected laser channels.
        
        Returns:
            int: Bitmask of enabled laser channels
        
        Example:
            >>> mask = sd.selected_lasers
            >>> print(f"Enabled lasers: {bin(mask)}")
        """
        return int(self.get_property(props.rw_SELECTED_LASERS))

    @selected_lasers.setter
    def selected_lasers(self, mask):
        """
        Set which laser channels are enabled.
        
        Args:
            mask (int): Bitmask of laser channels to enable:
                - 0: No lasers
                - 1: Cy2 only
                - 2: Cy3 only
                - 4: Cy5 only
                - 8: Cy7 only
                - Combinations: 1|2|4 = Cy2, Cy3, and Cy5
        
        Example:
            >>> sd.selected_lasers = 0b0110  # Enable Cy3 and Cy5
        """
        self.set_property(props.rw_SELECTED_LASERS, mask)
    
    @property
    def cam_readout_us(self):
        """
        Get the camera readout time.
        
        Returns:
            int: Camera readout time in microseconds
        """
        return int(self.get_property(props.rw_CAM_READOUT_us))

    @cam_readout_us.setter
    def cam_readout_us(self, value):
        """
        Set the camera readout time.
        
        Args:
            value (int): Camera readout time in microseconds
        """
        self.set_property(props.rw_CAM_READOUT_us, value)

    @property
    def shutter_delay_us(self):
        """
        Get the laser shutter delay time.
        
        Returns:
            int: Shutter delay time in microseconds
        """
        return int(self.get_property(props.rw_SHUTTER_DELAY_us))

    @shutter_delay_us.setter
    def shutter_delay_us(self, value):
        """
        Set the laser shutter delay time.
        
        Args:
            value (int): Shutter delay time in microseconds
        """
        self.set_property(props.rw_SHUTTER_DELAY_us, value)

    def start_continuous_acq(self, exp_time, N_frames, ts=0):
        """
        Start continuous acquisition mode.
        
        In continuous mode, laser shutters remain open during the entire acquisition
        and the camera is triggered at precise intervals. The first frame is
        automatically discarded as it contains pre-acquisition noise.
        
        Args:
            exp_time (int): Exposure time in microseconds
            N_frames (int): Number of frames to acquire
            ts (int): Start time offset in microseconds
        
        Example:
            >>> sd.start_continuous_acq(200000, 15, ts=500000)
        """
        self.write("CON", arg1=exp_time, ts=ts, N=N_frames)

    def start_stroboscopic_acq(self, exp_time, N_frames, ts=0, frame_period=0):
        """
        Start stroboscopic/timelapse acquisition mode.
        
        In stroboscopic mode, the laser is briefly illuminated during each camera
        exposure, followed by a readout period. Optional timelapse periods can
        be added between frames.
        
        Args:
            exp_time (int): Exposure time in microseconds
            N_frames (int): Number of frames to acquire
            ts (int): Start time offset in microseconds
            frame_period (int): Time between frames for timelapse (microseconds)
        
        Example:
            >>> sd.start_stroboscopic_acq(200000, 15, frame_period=1500000)
        """
        self.write("STR", arg1=exp_time, ts=ts, N=N_frames, interval=frame_period)

    def start_ALEX_acq(self, exp_time, N_bursts, ts=0, burst_period=0):
        """
        Start ALEX (Alternating Laser Excitation) acquisition mode.
        
        In ALEX mode, frames are acquired in bursts, with each frame in a burst
        illuminated by a different laser channel. This enables multi-spectral imaging.
        
        Args:
            exp_time (int): Exposure time in microseconds
            N_bursts (int): Number of bursts to acquire
            ts (int): Start time offset in microseconds
            burst_period (int): Time between bursts for timelapse (microseconds)
        
        Example:
            >>> sd.start_ALEX_acq(50000, 9, burst_period=400000)
        """
        self.write("ALX", arg1=exp_time, ts=ts, N=N_bursts, interval=burst_period)

    def N_frames_left(self):
        """
        Get the number of camera frames remaining in the current acquisition.
        
        Returns:
            int: Number of camera frames left to acquire, or 0 if no acquisition is running
        
        Example:
            >>> frames_left = sd.N_frames_left()
            >>> print(f"Remaining frames: {frames_left}")
        """
        events = self.get_events()
        if events:
            for event in events:
                if rev_pin_map[event.arg1] == "A12":
                    return event.N
        return 0

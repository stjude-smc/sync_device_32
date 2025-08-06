# `microsync` — Microscope Control with Microsecond Precision

**Version:** 2.4.0  
**Author:** Roman Kiselev  
**License:** Apache 2.0  
**URL:** [https://github.com/stjude-smc/microsync](https://github.com/stjude-smc/microsync)

[![Documentation](https://img.shields.io/badge/docs-GitHub%20Pages-blue.svg)](https://stjude-smc.github.io/microsync/)
[![License](https://img.shields.io/badge/license-Apache%202.0-green.svg)](LICENSE)
[![Python](https://img.shields.io/badge/python-3.7+-blue.svg)](https://www.python.org/)

## 🚀 Quick Start

```python
from microsync import SyncDevice

# Connect to device
sd = SyncDevice("COM4")  # or "/dev/ttyUSB0" on Linux

# Schedule a laser pulse
sd.pos_pulse("A0", 1000, ts=1000)  # 1ms pulse on A0 after 1ms delay
sd.go()  # Start execution
```

**📖 [Full Documentation](https://stjude-smc.github.io/microsync/) | 📓 [Interactive Demo](python/sync%20device%20demo.ipynb)**

## Overview

This project provides firmware and a Python driver for a high-precision synchronization device based on a 32-bit ARM microcontroller (Arduino Due). The device is designed for advanced microscope control, enabling precise timing and coordination of lasers, shutters, cameras, and other peripherals.

- **Firmware:** C++ code for the Arduino Due, handling real-time event scheduling, pin control, and safety interlocks.
- **Python Driver:** A user-friendly API for communicating with the device, scheduling events, and running complex acquisition protocols.

### Evolution from Legacy `sync_device`
This is the **second generation** of the microscope synchronization device, based on a 32-bit ARM microcontroller (Arduino Due). The first generation was based on Arduino Mega2560 (8-bit ATMega2560) and had fundamental limitations:

**Key Changes and Improvements in 32-bit Version:**
- **3.3V** logic – Arduino Due uses 3.3V CMOS logic levels for digital I/O, whereas the Arduino Mega2560 uses 5V logic levels
- **Microsecond precision** (vs. 64µs steps in 8-bit version)
- **No 4.19s exposure time limit** (vs. 16-bit timer limitation)
- **Event-driven architecture** with priority queue (vs. fixed state machine)
- **Up to 450 scheduled events** (vs. limited to 4 hardware timers)
- **Advanced acquisition modes** with precise timing control
- **Safety interlocks** for laser protection
- **Modern Python API** with context management and logging

**Legacy Repository:** The original 8-bit version is available at [sync_device_8bit_legacy](https://github.com/stjude-smc/sync_device_8bit_legacy) for reference.

## ✨ Features

- **Microsecond-precision event scheduling** (pulses, toggles, pin set/reset, bursts)
- **Priority event queue** with hardware-timed execution
- **Laser shutter and interlock safety logic**
- **Support for advanced acquisition modes** (continuous, stroboscopic, ALEX)
- **Comprehensive Python API** with logging and context management
- **Extensive examples and documentation in Jupyter notebook**

## 🔧 Hardware

- **Platform:** Arduino Due (32-bit SAM3X8E ARM Cortex-M3 microcontroller)

<img src="https://store-usa.arduino.cc/cdn/shop/files/A000062_00.front_475x357.jpg" alt="Arduino Due Board">

*Arduino Due board - the hardware platform for microsync*

- **Key Pins:**
  - **Laser shutters:** A0 (Cy2), A1 (Cy3), A2 (Cy5), A3 (Cy7)
  - **Camera trigger:** A12
  - **Error indicator:** D53
  - **Interlock:** D12 (input), D13 (output)
  - **Burst pulse:** D5
- **See** `doc/Arduino Due pinout.pdf` and `microsync/src/globals.h` for full pin mapping.

### Wiring and Setup

**Default Connections:**
- **Laser shutters:** Arduino pins A0-A3 (configurable in `globals.h`)
- **Camera trigger:** Pin A12 (configurable)
- **USB connection:** Provides both power and host communication via the micro-USB port next to the power socket. The device is powered through this USB connection and is recognized by the host computer as a virtual COM port.

- **Interlock circuit:** D12 (input) and D13 (output) for laser safety

### Communication Protocol & Data Structure

The device communicates via UART at 115,200 baud using a fixed-length 24-byte packet format. Each packet contains a command and associated parameters for precise event scheduling. All 24 bytes of a packet must arrive together, with no delay longer than 25 ms between individual bytes; otherwise, the packet is considered incomplete and will be discarded.

<img src="doc/data%20packet%20structure.svg" alt="Data Packet Structure" width="100%">

**Packet Format:**
- **Command (4 bytes):** 3-character command string, null-terminated
- **Argument 1 (4 bytes):** First parameter (pin index, duration, etc.)
- **Argument 2 (4 bytes):** Second parameter (additional settings)
- **Timestamp (4 bytes):** Scheduled time point of function call, in microseconds
- **Count (4 bytes):** Number of repetitions (0 = infinite)
- **Interval (4 bytes):** Time between repetitions in microseconds



**Startup Message:**
Upon opening of the COM port, the device resets and sends the startup message the includes the firmware version.
```
Sync device is ready. Firmware version: 2.4.0
```

## 🐍 Python Driver

### Installation

Requires Python 3.7+ and `pyserial`.  
Install with:

```bash
pip install pyserial
```

Copy the `python/` directory or install as a package if desired.

### Python Module Structure

The Python driver consists of several modules:

- **`microsync.py`** - Main `SyncDevice` class and communication interface
- **`constants.py`** - Timing constants and system parameters
- **`props.py`** - Property definitions and system settings
- **`rev_pin_map.py`** - Hardware pin mapping (Arduino pin names to internal IDs)
- **`__version__.py`** - Version information and package metadata
- **`sync device demo.ipynb`** - Comprehensive Jupyter notebook with examples

### Basic Usage

```python
from microsync import SyncDevice

# Connect to the device (replace COM4 with your port)
sd = SyncDevice("COM4", log_file='print')  # log_file can be None, 'print', or a filename

# Get device status
print(sd.get_status())
print(sd.version)

# Set and get properties
print(sd.pulse_duration_us)
sd.pulse_duration_us = 1000

# Schedule a positive pulse on pin A0
sd.pos_pulse("A0", 8000, N=120, interval=50000)
sd.go()
```

### Logging

- Set `log_file='print'` to print all communication.
- Set `log_file='filename.log'` to save to a file.

### Context Manager

Batch multiple commands for precise timing: all commands within the context manager are collected and sent together as a single data packet over UART, ensuring they are processed together on the device. This approach eliminates timing jitter caused by delays or variability in the host operating system, resulting in highly accurate event scheduling.

```python
with sd as dev:
    dev.pos_pulse("A12", 100000, N=10, interval=500000, ts=0)
    dev.pos_pulse("A0", 100000, N=10, interval=500000, ts=5000)
```

### Event Scheduling

- **Pulse:** `sd.pos_pulse(pin, duration, ts, N, interval)`
- **Toggle:** `sd.tgl_pin(pin, ts, N, interval)`
- **Set/Reset:** `sd.set_pin(pin, level, ts, N, interval)`
- **Enable/Disable Pin:** `sd.enable_pin(pin)`, `sd.disable_pin(pin)`
- **Clear/Stop/Go:** `sd.clear()`, `sd.stop()`, `sd.go()`

#### Event Queue & Execution System

The device uses a **priority queue** to manage event scheduling with microsecond precision. Each data packet received from the host is converted into an internal event structure for scheduling. The most significant change is the conversion of timestamp from 4-byte to 8-byte integer, as well as mapping of pins from name to internal IOPORT index.

**Internal Event Structure (28 bytes):**
- **Function pointer** (4 bytes) — what action to execute (`EventFunc func`)
- **Argument 1** (4 bytes) — first parameter for the function (e.g., pin number)
- **Argument 2** (4 bytes) — second parameter for the function (e.g., duration)
- **Timestamp** (8 bytes) — 64-bit absolute time when to execute (`ts64_cts`)
- **Count** (4 bytes) — number of repetitions remaining (`N`)
- **Interval** (4 bytes) — time between repetitions (`interv_cts`)

**Queue Operation:**
1. **Packet Processing:** 24-byte data packets are converted to 28-byte event structures
2. **Sorting:** Events are automatically sorted by timestamp (earliest first)
3. **Execution:** System timer triggers the next event at its exact timestamp
4. **Repetition:** Events with `N > 1` are rescheduled with updated timestamps
5. **Precision:** Hardware timer ensures microsecond-accurate execution
6. **Capacity:** Up to 450 events can be queued simultaneously

**Example:** When you schedule multiple events, they're automatically ordered and executed in time sequence, regardless of the order they were submitted. If two events have exactly the same timestamp, their execution order is undefined.

### Laser Shutter and Interlock

- **Open/Close shutters:** `sd.open_shutters(mask)`, `sd.close_shutters(mask)`
- **Select lasers:** `sd.selected_lasers = 0b0110` (bitmask: Cy2, Cy3, Cy5, Cy7)
- **Interlock:** `sd.interlock_enabled = True/False`

#### Laser Safety Interlock System

The device includes a sophisticated laser safety interlock that monitors the integrity of the laser safety circuit:

**Hardware Setup:**
- **Output pin (D13):** Sends heartbeat pulses every 25ms
- **Input pin (D12):** Monitors return signal from interlock circuit
- **External circuit:** Required between D12 and D13 for proper operation

**Operation:**
1. **Heartbeat Generation:** System sends pulse train on D13 (HIGH for 1.56ms, LOW for 23.44ms)
2. **Signal Monitoring:** System checks D12 at two critical moments:
   - When output goes LOW (should detect LOW on input)
   - When output goes HIGH (should detect HIGH on input)
3. **Safety Logic:** 
   - **Normal:** Both conditions met → Lasers enabled
   - **Fault:** Either condition fails → Lasers immediately disabled
   - Detects open circuits, short circuits, and missing connections

**Runtime Control:**
```python
sd.interlock_enabled = True  # set to False to ignore the interlock circuit
```

In practice, you want to have normally closed magnetic reed switches (such as [Magnasphere](https://magnasphere.com/product/s-series/)) installed on the doors of the microscope enclosure, connected in series.

### Acquisition Modes

The following high-level acquisition modes are provided as convenience functions for our pTIRF microscopes. Internally, each mode schedules a sequence of low-level events using the common event execution engine described above. This allows you to easily run complex imaging protocols, while retaining precise timing and coordination under the hood:

#### Continuous Imaging
- **Use case:** Continuous illumination with synchronous camera readout
- **Method:** `sd.start_continuous_acq(exp_time, N_frames, ts=0)`
- **Behavior:** Laser shutters remain open during entire acquisition, camera triggered at precise intervals
- **First frame:** Automatically discarded as it contains pre-acquisition noise

#### Stroboscopic/Timelapse Imaging  
- **Use case:** Brief laser illumination during each camera exposure
- **Method:** `sd.start_stroboscopic_acq(exp_time, N_frames, ts=0, frame_period=0)`
- **Behavior:** Laser pulse synchronized with camera exposure, followed by readout period
- **Timelapse:** Optional waiting period between frames when `frame_period > 0`

#### ALEX (Alternating Laser Excitation)

> **Note:** For ALEX, select lasers by setting `sd.selected_lasers` in Python (e.g., `sd.selected_lasers = 0b0110`), or send a `SET` command with the `rw_SELECTED_LASERS` property ID (from the `SysProps` enum) over UART.
- **Use case:** Multi-spectral imaging with alternating laser illumination
- **Method:** `sd.start_ALEX_acq(exp_time, N_bursts, ts=0, burst_period=0)`
- **Behavior:** Bursts of frames, each illuminated by different laser channel
- **Timelapse:** Optional waiting period between bursts when `burst_period > 0`

### Status and Events

- **Get all scheduled events:** `sd.get_events(unit="us"|"ms")`
- **Check frames left:** `sd.N_frames_left()`

### Timing Configuration

**Note:** All timing parameters for `microsync` are specified in microseconds (µs).

Configure these timing parameters to match your hardware before starting acquisition:

```python
# Essential timing parameters (in microseconds)
sd.shutter_delay_us = 1300      # Laser shutter opening time
sd.cam_readout_us = 14000       # Camera readout duration (depends on ROI)
sd.pulse_duration_us = 800      # Default pulse duration

# Laser selection (bitmask: Cy2=bit0, Cy3=bit1, Cy5=bit2, Cy7=bit3)
sd.selected_lasers = 0b0110     # Enable Cy3 and Cy5 lasers
```

**Important Notes:**
- `shutter_delay_us` should match your laser shutter's actual opening time. Use an oscilloscope and a photodiode to measure it.
- `cam_readout_us` depends on camera ROI settings. See the camera manual to find out how to calculate or query it.
- Timing values persist between acquisitions until changed. They drop back to defaults after system reset.

## 📚 Examples

See [`python/sync device demo.ipynb`](python/sync%20device%20demo.ipynb) for a comprehensive, step-by-step tutorial with code, explanations, and advanced usage patterns.

## 📋 Properties

| Property                | Access     | Description                                 |
|-------------------------|------------|---------------------------------------------|
| `version`               | Read-only  | Firmware version                            |
| `running`               | Read-only  | System timer status                         |
| `sys_time_cts`          | Read-only  | System timer value (counter ticks)          |
| `sys_time_s`            | Read-only  | System time (seconds)                       |
| `prescaler`             | Read-only  | Timer prescaler                             |
| `pulse_duration_us`     | R/W        | Default pulse duration (microseconds)       |
| `watchdog_timeout_ms`   | Read-only  | Watchdog timeout (ms)                       |
| `N_events`              | Read-only  | Number of scheduled events                  |
| `interlock_enabled`     | R/W        | Laser interlock status                      |
| `selected_lasers`       | R/W        | Bitmask of enabled laser channels           |
| `shutter_delay_us`      | R/W        | Shutter delay (us)                          |
| `cam_readout_us`        | R/W        | Camera readout time (us)                    |

## 📖 Documentation

📖 **[Full Documentation](https://stjude-smc.github.io/microsync/)** - Complete API reference, user guides, and examples

- **Firmware:** See `microsync/src/` for C++ source and hardware logic.
- **Python API:** See `python/microsync.py` and the Jupyter notebook.
- **Data packet structure:** See `doc/data packet structure.xlsx`.

### Building Documentation

This project includes comprehensive Sphinx/Doxygen documentation that combines Python API documentation with C++ firmware documentation.

#### Prerequisites

**Doxygen:** Required for C++ documentation generation
   - **Windows:** Download and install from [Doxygen website](https://www.doxygen.nl/download.html)
   - **Linux:** `sudo apt-get install doxygen` (Ubuntu/Debian) or `sudo yum install doxygen` (CentOS/RHEL)
   - **macOS:** `brew install doxygen`

#### Building the Documentation

Run the automated build script from the project root:

```bash
python build_docs.py
```

The documentation will be available at `sphinx_docs/_build/html/index.html`. The generated documentation includes:
- **Python API Reference:** Auto-generated from docstrings in the Python modules
- **C++ API Reference:** Generated from firmware header files using Doxygen + Breathe
- **User Guide:** Manual documentation and examples
- **Firmware Architecture:** Detailed explanations of the C++ implementation

The Sphinx documentation automatically builds and deploys to GitHub pages on every push to branches `develop` and `master` via GitHub actions.

## 🔧 Firmware Development

### Building the Firmware

The firmware is built using Microchip Studio or compatible IDEs:

1. **Open the project:** Load `microsync.atsln` in Microchip Studio
2. **Build target:** Select "Release" configuration
3. **Upload:** Use the Atmel ICE debugger, connected via JTAG interface to the microcontroller board. Hit `Ctrl+Shift+P` to open the programming dialog.
4. **Debugging:** If you upload the "Debug" configuration, you can set a breakpoint and pause the code execution on Arduino Due when it's connected via JTAG to Atmel ICE. You will have a normal step-by-step debugging with the direct access to the device registers and memory via Microchip Studio . The caveat is that the hardware timers keep running while you're in the debug mode. Use an oscilloscope when verifying event order and timing in addition to the code debugging.

### Firmware Architecture

- **`main.cpp`:** Entry point and system initialization
- **`globals.h`:** Global definitions, pin mappings, and system settings
- **`events.h/cpp`:** Event scheduling and priority queue management
- **`uart_comm.h/cpp`:** Communication protocol implementation
- **`pins.h/cpp`:** Pin control and hardware abstraction
- **`interlock.h/cpp`:** Laser safety interlock logic (can be disabled)

### Features and Limitations

- **Event queue:** Maximum 450 scheduled events (vs. 4 hardware timers in legacy)
- **Jitter:** Events scheduled within ~10µs of each other may have timing jitter
- **Overload protection:** A watchdog timer automatically resets the system if event queue overflows
- **Interlock:** Requires external circuit between D12 and D13 for laser safety

**Note:** These limitations are significantly improved compared to the legacy 8-bit version, which had 4.19s exposure limits and 64µs timing resolution.

## 📝 License

Apache 2.0.  
(c) Roman Kiselev, St. Jude Children's Research Hospital

## 🙏 Acknowledgments

- Based on Atmel SAM3X8E microcontroller (Arduino Due board)
- Uses `ASF` (Atmel Software Framework)
- See `/doc` for datasheets and pinouts

---

**For questions or contributions, please open an issue or pull request on [GitHub](https://github.com/stjude-smc/microsync).** 
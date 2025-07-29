# Microscope Synchronization Device (`sync_device_32`)

**Version:** 2.3.0  
**Author:** Roman Kiselev  
**License:** Apache 2.0  
**URL:** [https://github.com/ximeg/sync_device_32](https://github.com/ximeg/sync_device_32)

## Overview

This project provides firmware and a Python driver for a high-precision synchronization device based on a 32-bit ARM microcontroller (Arduino Due). The device is designed for advanced microscope control, enabling precise timing and coordination of lasers, shutters, cameras, and other peripherals.

- **Firmware:** C++ code for the Arduino Due, handling real-time event scheduling, pin control, and safety interlocks.
- **Python Driver:** A user-friendly API for communicating with the device, scheduling events, and running complex acquisition protocols.

### Legacy `sync_device`
**Note:** this project is the second generation of microscope synchronization device, based on a 32-bit ARM microcontroller. You can find the firmware for first generation device based on Arduino Mega2560 in the [](https://github.com/stjude-smc/sync_device_8bit_legacy) repository.


---

## Features

- **Microsecond-precision event scheduling** (pulses, toggles, pin set/reset, bursts)
- **Priority event queue** with hardware-timed execution
- **Laser shutter and interlock safety logic**
- **Support for advanced acquisition modes** (continuous, stroboscopic, ALEX)
- **Comprehensive Python API** with logging and context management
- **Extensive examples and documentation in Jupyter notebook**

---

## Hardware

- **Platform:** Arduino Due (SAM3X8E ARM Cortex-M3)
- **Key Pins:**
  - **Laser shutters:** A0 (Cy2), A1 (Cy3), A2 (Cy5), A3 (Cy7)
  - **Camera trigger:** A12
  - **Error indicator:** D53
  - **Interlock:** D12 (input), D13 (output)
  - **Burst pulse:** D5
- **See** `doc/Arduino Due pinout.pdf` and `sync_device_32/src/globals.h` for full pin mapping.

---

## Python Driver

### Installation

Requires Python 3.7+ and `pyserial`.  
Install with:

```bash
pip install pyserial
```

Copy the `python/` directory or install as a package if desired.

### Basic Usage

```python
from sync_dev import SyncDevice

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

Batch multiple commands for precise timing:

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

### Laser Shutter and Interlock

- **Open/Close shutters:** `sd.open_shutters(mask)`, `sd.close_shutters(mask)`
- **Select lasers:** `sd.selected_lasers = 0b0110` (bitmask: Cy2, Cy3, Cy5, Cy7)
- **Interlock:** `sd.interlock_enabled = True/False`

### Acquisition Modes

- **Continuous:** `sd.start_continuous_acq(exp_time, N_frames, ts=0)`
- **Stroboscopic:** `sd.start_stroboscopic_acq(exp_time, N_frames, ts=0, frame_period=0)`
- **ALEX:** `sd.start_ALEX_acq(exp_time, N_bursts, ts=0, burst_period=0)`

### Status and Events

- **Get all scheduled events:** `sd.get_events(unit="us"|"ms")`
- **Check frames left:** `sd.N_frames_left()`

---

## Examples

See [`python/sync device demo.ipynb`](python/sync%20device%20demo.ipynb) for a comprehensive, step-by-step tutorial with code, explanations, and advanced usage patterns.

---

## Properties

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

---

## Advanced

- **Event queue:** Up to 450 events, microsecond-precision, priority-ordered.
- **Safety:** Watchdog timer, interlock, error pin.
- **Hardware abstraction:** All pins mapped by name (see `python/rev_pin_map.py`).

---

## Documentation

- **Firmware:** See `sync_device_32/src/` for C++ source and hardware logic.
- **Python API:** See `python/sync_dev.py` and the Jupyter notebook.
- **Data packet structure:** See `doc/data packet structure.xlsx`.

---

## License

Apache 2.0.  
(c) Roman Kiselev, St. Jude Children's Research Hospital

---

## Acknowledgments

- Based on Atmel SAM3X8E (Arduino Due)
- Uses ASF (Atmel Software Framework)
- See `/doc` for datasheets and pinouts

---

**For questions or contributions, please open an issue or pull request on [GitHub](https://github.com/ximeg/sync_device_32).** 
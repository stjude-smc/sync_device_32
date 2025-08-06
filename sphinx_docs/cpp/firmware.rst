Firmware Build and Deployment
=============================

Building the Firmware
---------------------

Prerequisites
^^^^^^^^^^^^^

- **Microchip Studio** (free download from Microchip)
- **Arduino Due board**
- **USB cable**
- **Atmel ICE debugger** (optional, for debugging)

Project Structure
^^^^^^^^^^^^^^^^^

.. code-block:: text

   microsync/
   ├── main.cpp                 # Main application entry point
   ├── src/
   │   ├── globals.h           # Global definitions and constants
   │   ├── events.h/cpp        # Event system implementation
   │   ├── uart_comm.h/cpp     # UART communication
   │   ├── interlock.h/cpp     # Laser safety interlock
   │   ├── pins.h/cpp          # Pin management
   │   ├── props.h/cpp         # System properties
   │   └── ASF/                # Atmel Software Framework
   └── microsync.atsln    # Microchip Studio solution

Build Configuration
^^^^^^^^^^^^^^^^^^^

**Debug Configuration:**
- Full debug symbols
- Optimization disabled
- Detailed error reporting
- Development and testing

**Release Configuration:**
- Optimized for performance (-O2)
- Minimal debug information
- Production deployment
- Reduced memory footprint

Build Steps
^^^^^^^^^^^

1. **Open Project:**
   - Launch Microchip Studio
   - Open `microsync.atsln`

2. **Select Configuration:**
   - Choose Debug or Release build
   - Verify target is Arduino Due

3. **Build Project:**
   - Press F7 or Build → Build Solution
   - Check output for errors

4. **Verify Build:**
   - Look for `.hex` file in output directory
   - Check build statistics

Upload Methods
--------------

Atmel ICE Debugger (Recommended)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Advantages:**
- Full debugging capabilities
- Breakpoint support
- Real-time variable inspection
- Reliable upload

**Setup:**
1. Connect Atmel ICE to Arduino Due via JTAG
2. Configure debugger settings in Microchip Studio
3. Use Debug → Start Debugging and Break

**Upload:**
1. Build the project
2. Debug → Start Debugging and Break
3. Debug → Stop Debugging (firmware uploaded)

Arduino Bootloader
^^^^^^^^^^^^^^^^^^

**Advantages:**
- No additional hardware required
- Simple USB connection
- Standard Arduino workflow

**Setup:**
1. Configure external tool in Microchip Studio
2. Set Arduino bootloader path
3. Configure COM port

**Upload:**
1. Build the project
2. Tools → External Tools → ArduinoBootloader
3. Verify upload completion

Verification
------------

Post-Upload Checks
^^^^^^^^^^^^^^^^^^

1. **LED Status:**
   - Power LED should be on
   - Status LED should blink periodically

2. **UART Communication:**
   - Connect via serial monitor
   - Check for startup messages
   - Verify version information

3. **Functionality Test:**
   - Test basic pin control
   - Verify event scheduling
   - Check interlock operation

Troubleshooting
---------------

Common Issues
^^^^^^^^^^^^^

**Build Errors:**
- Check ASF framework installation
- Verify include paths
- Ensure all source files are included

**Upload Failures:**
- Check USB connection
- Verify COM port settings
- Ensure Arduino Due is in programming mode

**Runtime Issues:**
- Check power supply
- Verify pin connections
- Monitor UART output for errors

Debugging
---------

Debug Features
^^^^^^^^^^^^^^

- **Breakpoints:** Set in Microchip Studio
- **Variable Watch:** Real-time value inspection
- **Call Stack:** Function call history
- **Memory Inspection:** RAM and register values

Debug Output
^^^^^^^^^^^^

The firmware provides extensive debug output via UART:

.. code-block:: cpp

   // Debug message examples
   printf("INFO: System initialized\n");
   printf("INFO: Event scheduled at %lu us\n", timestamp);
   printf("ERR: Invalid command received\n");

Version Management
------------------

Version Information
^^^^^^^^^^^^^^^^^^^

The firmware version is defined in `globals.h`:

.. code-block:: cpp

   #define FIRMWARE_VERSION_MAJOR 2
   #define FIRMWARE_VERSION_MINOR 3
   #define FIRMWARE_VERSION_PATCH 0

Version Compatibility
^^^^^^^^^^^^^^^^^^^^^

- **Python Driver**: Must match firmware version
- **Protocol**: Backward compatible within major versions
- **Features**: New features may require driver updates

Updating Firmware
^^^^^^^^^^^^^^^^^

1. **Backup Configuration:**
   - Save current settings
   - Note any custom configurations

2. **Upload New Firmware:**
   - Use standard upload procedure
   - Verify successful upload

3. **Restore Configuration:**
   - Reconfigure system settings
   - Test functionality 
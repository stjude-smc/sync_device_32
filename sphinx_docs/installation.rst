Installation Guide
=================

This guide covers the installation and setup of the sync_device_32 project, including both the Python driver and firmware.

Python Driver Installation
--------------------------

Prerequisites
^^^^^^^^^^^^

* Python 3.7 or higher
* pip package manager
* Serial port access (for device communication)

Install Dependencies
^^^^^^^^^^^^^^^^^^^

Install the required Python packages:

.. code-block:: bash

   pip install pyserial

Install the sync_device_32 Driver
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Option 1: Copy the Python directory**

.. code-block:: bash

   # Clone or download the repository
   git clone https://github.com/ximeg/sync_device_32.git
   cd sync_device_32
   
   # Copy the python directory to your project
   cp -r python/ /path/to/your/project/
   
   # Or add to your Python path
   export PYTHONPATH="${PYTHONPATH}:/path/to/sync_device_32/python"

**Option 2: Install as a package (recommended for development)**

.. code-block:: bash

   # From the project root directory
   pip install -e .

Verify Installation
^^^^^^^^^^^^^^^^^^

Test that the driver can be imported:

.. code-block:: python

   from sync_dev import SyncDevice
   print("Installation successful!")

Firmware Installation
---------------------

Prerequisites
^^^^^^^^^^^^

* Microchip Studio (free download from Microchip)
* Arduino Due board
* USB cable
* Atmel ICE debugger (optional, for debugging)

Build the Firmware
^^^^^^^^^^^^^^^^^

1. **Open the project in Microchip Studio:**

   .. code-block:: bash

      # Open the solution file
      sync_device_32.atsln

2. **Configure build settings:**

   - Set build target to "Release"
   - Ensure all dependencies are resolved

3. **Build the project:**

   - Press F7 or use Build → Build Solution
   - Verify successful compilation

Upload to Arduino Due
^^^^^^^^^^^^^^^^^^

**Method 1: Using Atmel ICE Debugger (Recommended)**

1. Connect Atmel ICE to Arduino Due via JTAG interface
2. In Microchip Studio, go to Tools → External Tools
3. Configure ArduinoBootloader tool:

   .. code-block:: text

      Executable: C:\Program Files (x86)\Arduino\hardware\tools\avr\bin\avrdude.exe
      Arguments: -C"C:\Program Files (x86)\Arduino\hardware\tools\avr\etc\avrdude.conf" -v -patmega2560 -cwiring -PCOM11 -b115200 -D -Uflash:w:"$(ProjectDir)Release\$(TargetName).hex":i

4. Run the tool to upload firmware

**Method 2: Using Arduino IDE (Alternative)**

1. Open Arduino IDE
2. Set board to "Arduino Due (Programming Port)"
3. Upload the compiled .hex file

Hardware Setup
--------------

Required Connections
^^^^^^^^^^^^^^^^^^^

* **USB Connection:** Connect Arduino Due to host computer
* **Laser Shutters:** Connect to pins A0-A3 (configurable)
* **Camera Trigger:** Connect to pin A12
* **Interlock Circuit:** Connect between D12 (input) and D13 (output)

Power Supply
^^^^^^^^^^^

The device is powered through the USB connection. No external power supply is required.

Driver Installation (Windows)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Download Arduino drivers from `arduino.cc <https://www.arduino.cc/en/software>`_
2. Install the drivers when prompted
3. The device will appear as "Arduino Due" in Device Manager

Verification
-----------

Test the Complete Setup
^^^^^^^^^^^^^^^^^^^^^^

1. **Connect the device:**

   .. code-block:: python

      from sync_dev import SyncDevice
      
      # Connect to device (replace COM4 with your port)
      sd = SyncDevice("COM4")
      print(f"Connected to device version: {sd.version}")

2. **Test basic functionality:**

   .. code-block:: python

      # Test event scheduling
      sd.pos_pulse("A0", 1000, N=5, interval=10000)
      sd.go()
      
      # Check status
      print(sd.get_status())

3. **Verify communication:**

   .. code-block:: python

      # Test property access
      print(f"System timer running: {sd.running}")
      print(f"Number of events: {sd.N_events}")

Troubleshooting
--------------

Common Issues
^^^^^^^^^^^^

**Connection Error:**
- Verify correct COM port
- Check USB cable connection
- Ensure drivers are installed

**Version Mismatch:**
- Update firmware to match Python driver version
- Check version compatibility

**Permission Errors:**
- Run as administrator (Windows)
- Check serial port permissions (Linux/Mac)

**Build Errors:**
- Verify Microchip Studio installation
- Check ASF (Atmel Software Framework) installation
- Ensure all dependencies are resolved

Getting Help
^^^^^^^^^^^

* Check the :doc:`troubleshooting` guide
* Review the :doc:`hardware` documentation
* Open an issue on `GitHub <https://github.com/ximeg/sync_device_32>`_

Next Steps
----------

After successful installation:

1. Read the :doc:`quickstart` guide
2. Explore the :doc:`api/index` documentation
3. Try the examples in :doc:`examples`
4. Review the :doc:`hardware` setup guide 
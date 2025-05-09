# Broadcast Audio Source with Multi-Subgroups
This application is a Zephyr-based Bluetooth LE Audio Broadcast Source application supporting multiple subgroups.


The application is a slightly modified version of the [BAP Broadcast Source sample](https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/bluetooth/bap_broadcast_source).


# Getting started...
If you haven't done it yet, first go to [The Zephyr getting started guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) and install all dependencies (I'd recommend following the path with the virtual python environment).

# For development
For developers of the application, first do a fork of the repo.  Then do the following:

Make a local workspace folder (to hold the repo, zephyr and west modules):

```
mkdir my-workspace
cd my-workspace
```

Clone the repo:

```
git clone git@github.com:<your handle>/multisubgroup-source.git
```

Initialize west and update dependencies:

```
west init -l multisubgroup-source
west update
```

# For normal use (non-development)
This repo contains a stand alone Zephyr application that can be fetched and initialized like this:

```
west init -m https://github.com/astraeuslabs/multisubgroup-source --mr main my-workspace
```

Then use west to fetch dependencies:

```
cd my-workspace
west update
```

# Add streams for each subgropu to the project
To prepare and add stream files to the project, follow these steps:
1. **Prepare the Audio Files**: Obtain or create audio files in `.wav` format (with a sample rate of 16, 24, or 48 kHz, in either Mono or Stereo channels).
2. **Encode with LC3**: Use the LC3 encoder tool (available at https://github.com/google/liblc3/), to encode the `.wav` files into LC3 format.
3. **Add Encoded Files to the Project**: Copy the encoded `.lc3` files into the `app/src/lc3/` directory of the project.
4. **Modify `lc3_audio.h`**: Edit the `lc3_audio.h` file, to include the names of the stream files and update the streams metadata accordingly. Also, adjust the `NUM_STREAM_FILES` value, add or remove the `stream_data` definitions, and update the corresponding array sizes and data in the `lc3_audio.h` file.


# Update configurations parameters
To modify the number of subgroups, streams per subgroup, sample rate, or audio channel mode:

- **Adjustments in `prj.conf`**
  - **Subgroup and stream configuration**
    - Set `CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT` to the desired number of subgroups.
    - Set `CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT` to the total number of streams (subgroups × streams per subgroup).

  - **ISO channel and buffer configuration** (if total streams > 6)
    - Set `CONFIG_BT_ISO_MAX_CHAN` to at least the total number of streams.
    - Set `CONFIG_BT_ISO_TX_BUF_COUNT` to at least three times the total number of streams.

  - **Audio sample rate** (if not using the default 16 kHz)
    - Uncomment `CONFIG_BT_BAP_AUDIO_SR_24K=y` for a 24 kHz sample rate.
    - Uncomment `CONFIG_BT_BAP_AUDIO_SR_48K=y` for a 48 kHz sample rate.

  - **Audio channel mode** (if not using the default Mono)
    - Uncomment `CONFIG_BT_BAP_AUDIO_CH_STEREO=y` for "Stereo".
    - Uncomment `CONFIG_BT_BAP_AUDIO_CH_LCR=y` for "LCR" (Left-Center-Right).

    NOTE: For Mono, Stereo, and LCR audio channel modes, the number of streams per subgroup is 1, 2, and 3, respectively.

- **Adjustments to controller configuration in `overlay-bt_ll_sw_split.conf`** (if needed)
  - **Stream configuration for controller** (if total streams > 6)
    - Set `CONFIG_BT_CTLR_ADV_ISO_STREAM_MAX` to at least the total number of streams.
    - Set `CONFIG_BT_CTLR_ISOAL_SOURCES` to at least the total number of streams.


# Build and flash

Navigate to the repository folder:

```
cd multisubgroup-source
```

## Build and flash nRF52840 dongle

### Build
To build the application for the nRF52840 dongle, run the following command (ensure you are in the virtual environment):

```
west build -b nrf52840dongle -d build/nrf52840dongle/app app --pristine -- "-DEXTRA_CONF_FILE=overlay-bt_ll_sw_split.conf"
```

### Create firmware package
After building, create the firmware package with::

```
nrfutil pkg generate --hw-version 52 --sd-req=0x00 --application build/nrf52840dongle/app/zephyr/zephyr.hex --application-version 1 build/build_dongle.zip
```

### Flash dongle
To flash the created package to the dongle:
- Enter DFU Mode: Press the small side button on the dongle using a nail or similar tool. The dongle's red LED should begin fading in and out, indicating it is in DFU mode.
- Flash the Package: Run the following command, replacing `/dev/ttyACMx` with the appropriate device (e.g., `/dev/ttyACM0`, `/dev/ttyACM1`, etc.):


```
nrfutil dfu serial -pkg build/build_dongle.zip -p /dev/ttyACMx
```

NOTE: To identify the correct device, you can list active serial devices before and after connecting the dongle:

```
ls /dev/ttyACM*
```

# Modify project config, build and flash nRF52840 dongle using script (alternative method)
As an alternative to manual configuration, you can use the provided script to automatically generate the configuration, build the firmware, and flash it to the nRF52840 dongle.
Run the following commands in sequence:
Alternatively, you can use the provided scripts to modify project config, build and flash the nRF52840 dongle.
Execute them in the following order:

```shell
./config_build_flash.sh -g -sg <number of desired subgroups> -sr <16k/24k/48k> -ch <mono/stereo/lcr>
./config_build_flash.sh -b
./config_build_flash.sh -p
./config_build_flash.sh -f -dd /dev/ttyACMx
```

- Replace `/dev/ttyACMx` with the correct device identifier.
- Ensure the dongle is in DFU mode as described above.

NOTE: You can also combine multiple steps into a single command.
For full usage instructions, run:

```shell
./config_build_flash.sh -h
```

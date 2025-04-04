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
4. **Update Configuration Settings (if needed)**: If the sample rate or channel mode of the new audio files differs from the default (16kHz, Mono), update the relevant configuration parameters in the `prj.conf` file. For example, adjust settings such as `CONFIG_BT_BAP_AUDIO_SR_24K` for a 24kHz sample rate or `CONFIG_BT_BAP_AUDIO_CH_STEREO` for stereo audio.
5. **Modify `lc3_audio.h`**: Edit the `lc3_audio.h` file, to include the names of the stream files and update the streams metadata accordingly.

# Change the number of subgroups/streams
To change the number of subgroups/streams, follow these steps:
1. **Update configuration parameters in `prj.conf`**: Modify the relavant configuration parameters in the `prj.conf` file.
   - **Mandatory Adjustments**:
     - Set `CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT` to the desired number of subgroups.
     - Set `CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT` to the desired number of streams.

     **Note**: In this project, each subgroup contains one stream. Therefore, the values for `CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT` and `CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT` should be identical.

   - **Additional Adjustments** (if increasing the number of subgroups/streams beyond three):
     - Set `CONFIG_BT_ISO_MAX_CHAN` to at least the total number of streams.
     - Set `CONFIG_BT_ISO_TX_BUF_COUNT` to at least three times the total number of streams.
 
 2. **Modify `lc3_audio.h`**: Adjust the `NUM_STREAM_FILES` value, add or remove the `stream_data` definitions, and update the corresponding array sizes and data in the `lc3_audio.h` file.


# Build and flash

Navigate to the repository folder:

```
cd multisubgroup-source
```

## Build and flash nRF52840 dongle

### Build
To build the application for the nRF52840 dongle, run the following command (ensure you are in the virtual environment):

```
west build -b nrf52840dongle -d build/nrf52840dongle/app app --pristine -- "-DOVERLAY_CONFIG=overlay-bt_ll_sw_split.conf"
```

### Create flash package
After building, create the flash package with::

```
nrfutil pkg generate --hw-version 52 --sd-req=0x00 --application build/nrf52840dongle/app/zephyr/zephyr.hex --application-version 1 build_dongle.zip
```

### Flash dongle
To flash the created package to the dongle:
- Enter DFU Mode: Press the small side button on the dongle using a nail or similar tool. The dongle's red LED should begin fading in and out, indicating it is in DFU mode.
- Flash the Package: Run the following command, replacing `/dev/ttyACMx` with the appropriate device (e.g., `/dev/ttyACM0`, `/dev/ttyACM1`, etc.):


```
nrfutil dfu serial -pkg build_dongle.zip -p /dev/ttyACMx
```

NOTE: To identify the correct device, you can list active serial devices before and after connecting the dongle:

```
ls /dev/ttyACM*
```

## Build and flash nRF52840 dongle using scripts (alternative method)
Alternatively, you can use the provided scripts to build and flash the nRF52840 dongle. Execute them in the following order:

```shell
compile_app.sh
create_flash_package.sh
flash_dongle.sh /dev/ttyACMx
```

Ensure the dongle is in DFU mode as described above. Also, replace `/dev/ttyACMx` with the correct device identifier.


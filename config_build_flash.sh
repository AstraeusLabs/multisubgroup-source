#!/bin/bash

# This script generates an overlay configuration file for the nRF52840 dongle
# and builds the project using the Zephyr build system. It also creates a firmware
# package and flashes it to the dongle if specified.
# The script accepts various command-line options to customize the configuration
# and build process.


subgroups=""
samplerate="16k"
channels="mono"
channel_count=1
samplerate_define=""
channel_define=""
generate_config=false
outfile_name="overlay-52840dongle.conf"
outfile="app/${outfile_name}"
package_file="build/build_dongle.zip"
dongle_device="/dev/ttyACM0"
do_build=false
create_package=false
do_flash=false


print_help() {
    echo "Usage: $0 [OPTIONS]"
    echo
    echo "Options:"
    echo "  -sg, --subgroups N            Number of subgroups (required, 1-9)"
    echo "  -sr, --sample-rate SR         Sample rate: 16k (default), 24k, or 48k"
    echo "  -ch, --audio-channels MODE    Audio channels: mono (default), stereo, or lcr"
    echo "  -g,  --generate-config        Generate the config file"
    echo "  -b,  --build                  Build the project (based on the generated config)"
    echo "  -p,  --create-package         Create a firmware package"
    echo "  -dd, --doungle-device NAME    TTY device name of the dongle (e.g. ttyACM0)"
    echo "  -f,  --flash                  Flash the firmware to the dongle"
    echo "  -bf, --build-flash            Build, create package, and flash (shorthand for -b -p -f)"
    echo "  -a,  --all                    Generate config, build, create package, and flash"
    echo "  -cl, --clean                  Clean the generated config file, build directory and fw package"
    echo "  -h,  --help                   Show this help message and exit"
    echo
    echo "Examples:"
    echo "  $0 -g -sg 2 -sr 24k -ch stereo"
    echo "  $0 --all --subgroups 3 --sample-rate 48k --audio-channels lcr -dd ttyACM3"
    echo "  $0 -bf -dd ttyACM1"
}

generate_overlay_conf() {
    if [[ -z "$subgroups" ]]; then
        echo "Error: Missing required option --subgroups or -sg"
        exit 1
    fi

    case "$samplerate" in
        16k)
            samplerate_define="CONFIG_BT_BAP_AUDIO_SR_16K=y"
            ;;
        24k)
            samplerate_define="CONFIG_BT_BAP_AUDIO_SR_24K=y"
            ;;
        48k)
            samplerate_define="CONFIG_BT_BAP_AUDIO_SR_48K=y"
            ;;
        *)
            echo "Error: Invalid sample rate. Choose from 16k, 24k, 48k."
            exit 1
            ;;
    esac

    case "$channels" in
        mono)
            channel_count=1
            channel_define="CONFIG_BT_BAP_AUDIO_CH_MONO=y"
            ;;
        stereo)
            channel_count=2
            channel_define="CONFIG_BT_BAP_AUDIO_CH_STEREO=y"
            ;;
        lcr)
            channel_count=3
            channel_define="CONFIG_BT_BAP_AUDIO_CH_LCR=y"
            ;;
        *)
            echo "Error: Invalid audio channel type. Choose from mono, stereo, lcr."
            exit 1
            ;;
    esac

    stream_count=$((subgroups * channel_count))
    iso_tx_buf_count=$((3 * stream_count))

    # Generate config file
    dir_path="$(dirname "$outfile")"
    if [ ! -d "$dir_path" ]; then
        echo "Error: Directory '$dir_path' does not exist!"
        exit 1
    fi

    {
        echo "# Overlay Configuration for nRF52840-dongle"
        echo "# Broadcast Audio Source"
        echo ""
        echo "# Subgroup and stream configuration"
        echo "CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT=$subgroups"
        echo "CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT=$stream_count"
        echo ""
        echo "# ISO channel & buffer configuration"
        echo "CONFIG_BT_ISO_MAX_CHAN=$stream_count"
        echo "CONFIG_BT_ISO_TX_BUF_COUNT=$iso_tx_buf_count"
        echo ""
        echo "# Audio configuration"
        echo "$samplerate_define"
        echo "$channel_define"
        echo ""
        echo "# Stream configuration for controller"
        echo "CONFIG_BT_CTLR_ADV_ISO_STREAM_MAX=$stream_count"
        echo "CONFIG_BT_CTLR_ISOAL_SOURCES=$stream_count"
        echo ""
    } > "$outfile"

    echo "Generated config: $outfile"
}

clean_all() {
    echo "Cleaning generated overlay config file and build directory..."
    rm -rf build ${outfile}

    if [ $? -ne 0 ]; then
	echo "❌ Failed to clean overlay config and/or build directory. Exiting."
	exit 1
    fi

    echo "✅ Cleaned successfully."
}


while [[ $# -gt 0 ]]; do
    case "$1" in
        -sg|--subgroups)
            subgroups="$2"
            if ! [[ "$subgroups" =~ ^[1-9][0-9]*$ ]]; then
                echo "Error: Invalid subgroup count. Must be a positive integer."
                exit 1
            fi
            shift 2
            ;;
        -sr|--sample-rate)
            samplerate="$2"
            shift 2
            ;;
        -ch|--audio-channels)
            channels="$2"
            shift 2
            ;;
        -g|--generate-config)
            generate_config=true
            shift
            ;;
        -b|--build)
            do_build=true
            shift
            ;;
        -p|--create-package)
            create_package=true
            shift
            ;;
        -dd|--doungle-device)
            if [[ "$2" =~ ^/dev/ttyACM[0-9]+$ ]]; then
                dongle_device="$2"
            elif [[ "$2" =~ ^ttyACM[0-9]+$ ]]; then
                dongle_device="/dev/$2"
            else
                echo "Error: Invalid dongle device name. Expected ttyACMx where x is a number."
                exit 1
            fi
            shift 2
            ;;
        -f|--flash)
            do_flash=true
            shift
            ;;
        -bf|--build-flash)
            do_build=true
            create_package=true
            do_flash=true
            shift
            ;;
        -a|--all)
	    generate_config=true
            do_build=true
            create_package=true
            do_flash=true
            shift
            ;;
	-cl|--clean)
            clean_all
	    exit 0
	    ;;
        -h|--help)
            print_help
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

if ! $generate_config && ! $do_build && ! $create_package && ! $do_flash; then
    echo "Error: No action specified. Use -g to generate config, -b to build, -p to create package, or -f to flash."
    exit 1
fi

# Generate overlay configuration
if $generate_config; then
    echo "Generating overlay configuration..."
    generate_overlay_conf

    if [ $? -ne 0 ]; then
	echo "❌ Failed to generate overlay configuration. Exiting."
	exit 1
    fi

    echo "✅ Overlay configuration generated successfully."
fi

# Build and flash options
if $do_build; then
    echo "Running west build..."
    west build -b nrf52840dongle -d build/nrf52840dongle/app app --pristine -- "-DEXTRA_CONF_FILE=overlay-bt_ll_sw_split.conf;${outfile_name}"

    if [ $? -ne 0 ]; then
	echo "❌ Build failed. Exiting."
	exit 1
    fi

    echo "✅ Build completed successfully."
fi
if $create_package; then
    echo "Creating firmware package..."
    nrfutil pkg generate --hw-version 52 --sd-req=0x00 --application ./build/nrf52840dongle/app/zephyr/zephyr.hex --application-version 1 ${package_file}

    if [ $? -ne 0 ]; then
	echo "❌ Package generation failed. Exiting."
	exit 1
    fi

    echo "✅ Package created: ${package_file}"
fi
if $do_flash; then
    echo "Flashing the dongle..."
    echo "Note: The device must be set in DFU flashing mode by pressing the small side button (with a nail)"
    echo "Trying to flash ${package_file} to ${dongle_device}"
    nrfutil dfu serial -pkg ${package_file} -p ${dongle_device}

    if [ $? -ne 0 ]; then
	echo "❌ Flashing failed. Exiting."
	exit 1
    fi

    echo "✅ Flashing completed successfully."
fi

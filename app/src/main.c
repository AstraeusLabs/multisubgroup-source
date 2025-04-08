/*
 * Copyright (c) 2025 Demant A/S
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/drivers/hwinfo.h>

#include "lc3_audio.h"
#include "rgb_led.h"


#define NUM_SUBGROUPS	CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT
#define NUM_STREAMS     CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT

/* When BROADCAST_ENQUEUE_COUNT > 1 we can enqueue enough buffers to ensure that
 * the controller is never idle
 */
#define BROADCAST_ENQUEUE_COUNT	   3U
#define TOTAL_BUF_NEEDED           (BROADCAST_ENQUEUE_COUNT * NUM_STREAMS)

/* Zephyr Controller works best while Extended Advertising interval to be a multiple
 * of the ISO Interval minus 10 ms (max. advertising random delay). This is
 * required to place the AUX_ADV_IND PDUs in a non-overlapping interval with the
 * Broadcast ISO radio events.
 *
 * I.e. for a 7.5 ms ISO interval use 90 ms minus 10 ms ==> 80 ms advertising
 * interval.
 * And, for 10 ms ISO interval, can use 90 ms minus 10 ms ==> 80 ms advertising
 * interval.
 */
#define BT_LE_EXT_ADV_CUSTOM	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | \
						BT_LE_ADV_OPT_USE_IDENTITY, \
						0x0080, 0x0080, NULL)

/* The following section contains the LC3 broadcast preset configuration
 * based on the Kconfig options
 */
#if defined(CONFIG_BT_BAP_AUDIO_SR_16K)
#define BROADCAST_PRESET_ARG_NAME 	"16_2_1"
#elif defined(CONFIG_BT_BAP_AUDIO_SR_24K)
#define BROADCAST_PRESET_ARG_NAME 	"24_2_1";
#elif defined(CONFIG_BT_BAP_AUDIO_SR_48K)
#define BROADCAST_PRESET_ARG_NAME 	"48_2_1";
#endif

#if defined(CONFIG_BT_BAP_AUDIO_CH_MONO)
#define LOCATION                (BT_AUDIO_LOCATION_MONO_AUDIO)
#elif defined(CONFIG_BT_BAP_AUDIO_CH_STEREO)
#define LOCATION                (BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT)
#endif

#define CONTEXT                 (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED)


struct named_lc3_preset {
	const char *name;
	struct bt_bap_lc3_preset preset;
};

static const struct named_lc3_preset lc3_broadcast_presets[] = {
	{"8_1_1", BT_BAP_LC3_BROADCAST_PRESET_8_1_1(LOCATION, CONTEXT)},
	{"8_2_1", BT_BAP_LC3_BROADCAST_PRESET_8_2_1(LOCATION, CONTEXT)},
	{"16_1_1", BT_BAP_LC3_BROADCAST_PRESET_16_1_1(LOCATION, CONTEXT)},
	{"16_2_1", BT_BAP_LC3_BROADCAST_PRESET_16_2_1(LOCATION, CONTEXT)},
	{"24_1_1", BT_BAP_LC3_BROADCAST_PRESET_24_1_1(LOCATION, CONTEXT)},
	{"24_2_1", BT_BAP_LC3_BROADCAST_PRESET_24_2_1(LOCATION, CONTEXT)},
	{"32_1_1", BT_BAP_LC3_BROADCAST_PRESET_32_1_1(LOCATION, CONTEXT)},
	{"32_2_1", BT_BAP_LC3_BROADCAST_PRESET_32_2_1(LOCATION, CONTEXT)},
	{"441_1_1", BT_BAP_LC3_BROADCAST_PRESET_441_1_1(LOCATION, CONTEXT)},
	{"441_2_1", BT_BAP_LC3_BROADCAST_PRESET_441_2_1(LOCATION, CONTEXT)},
	{"48_1_1", BT_BAP_LC3_BROADCAST_PRESET_48_1_1(LOCATION, CONTEXT)},
	{"48_2_1", BT_BAP_LC3_BROADCAST_PRESET_48_2_1(LOCATION, CONTEXT)},
	{"48_3_1", BT_BAP_LC3_BROADCAST_PRESET_48_3_1(LOCATION, CONTEXT)},
	{"48_4_1", BT_BAP_LC3_BROADCAST_PRESET_48_4_1(LOCATION, CONTEXT)},
	{"48_5_1", BT_BAP_LC3_BROADCAST_PRESET_48_5_1(LOCATION, CONTEXT)},
	{"48_6_1", BT_BAP_LC3_BROADCAST_PRESET_48_6_1(LOCATION, CONTEXT)},
	/* High-reliability presets */
	{"8_1_2", BT_BAP_LC3_BROADCAST_PRESET_8_1_2(LOCATION, CONTEXT)},
	{"8_2_2", BT_BAP_LC3_BROADCAST_PRESET_8_2_2(LOCATION, CONTEXT)},
	{"16_1_2", BT_BAP_LC3_BROADCAST_PRESET_16_1_2(LOCATION, CONTEXT)},
	{"16_2_2", BT_BAP_LC3_BROADCAST_PRESET_16_2_2(LOCATION, CONTEXT)},
	{"24_1_2", BT_BAP_LC3_BROADCAST_PRESET_24_1_2(LOCATION, CONTEXT)},
	{"24_2_2", BT_BAP_LC3_BROADCAST_PRESET_24_2_2(LOCATION, CONTEXT)},
	{"32_1_2", BT_BAP_LC3_BROADCAST_PRESET_32_1_2(LOCATION, CONTEXT)},
	{"32_2_2", BT_BAP_LC3_BROADCAST_PRESET_32_2_2(LOCATION, CONTEXT)},
	{"441_1_2", BT_BAP_LC3_BROADCAST_PRESET_441_1_2(LOCATION, CONTEXT)},
	{"441_2_2", BT_BAP_LC3_BROADCAST_PRESET_441_2_2(LOCATION, CONTEXT)},
	{"48_1_2", BT_BAP_LC3_BROADCAST_PRESET_48_1_2(LOCATION, CONTEXT)},
	{"48_2_2", BT_BAP_LC3_BROADCAST_PRESET_48_2_2(LOCATION, CONTEXT)},
	{"48_3_2", BT_BAP_LC3_BROADCAST_PRESET_48_3_2(LOCATION, CONTEXT)},
	{"48_4_2", BT_BAP_LC3_BROADCAST_PRESET_48_4_2(LOCATION, CONTEXT)},
	{"48_5_2", BT_BAP_LC3_BROADCAST_PRESET_48_5_2(LOCATION, CONTEXT)},
	{"48_6_2", BT_BAP_LC3_BROADCAST_PRESET_48_6_2(LOCATION, CONTEXT)},
};						

BUILD_ASSERT(strlen(CONFIG_BROADCAST_CODE) <= BT_ISO_BROADCAST_CODE_SIZE,
	     "Invalid broadcast code");
	
BUILD_ASSERT(CONFIG_BT_ISO_TX_BUF_COUNT >= TOTAL_BUF_NEEDED,
	     "CONFIG_BT_ISO_TX_BUF_COUNT should be at least "
	     "BROADCAST_ENQUEUE_COUNT * CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT");

BUILD_ASSERT(NUM_STREAM_FILES == NUM_STREAMS,
	     "NUM_STREAM_FILES should be equal to NUM_STREAMS");


NET_BUF_POOL_FIXED_DEFINE(tx_pool,
			  TOTAL_BUF_NEEDED,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE,
			  NULL);

struct broadcast_source_stream {
	struct bt_bap_stream stream;
	uint32_t seq_num;
	uint32_t sent_count;
	uint8_t *data;
	uint8_t *start;
	uint8_t *end;
	uint16_t sdu_len;
	uint16_t ch_spf;
	uint8_t ch_count;
};		

static struct broadcast_source_stream streams[NUM_STREAMS];
static struct bt_bap_broadcast_source *broadcast_source;

const char *broadcast_name = BROADCAST_NAME;

static K_SEM_DEFINE(sem_started, 0, NUM_STREAMS);
static K_SEM_DEFINE(sem_stopped, 0, NUM_STREAMS);

#define BROADCAST_SOURCE_LIFETIME	120U /* seconds */

#define LC3_MIN_FRAME_BYTES	20
#define LC3_MAX_FRAME_BYTES     400

#define CHANNEL_COUNT 		1

uint8_t read_buffer[LC3_MAX_FRAME_BYTES * CHANNEL_COUNT];

/**
 * The following section contains the
 * LC3 binary format handler
 *
 * Rewritten handlers to work on uint8_t buffer from
 *
 * https://github.com/google/liblc3/tools/lc3bin.c
 *
 * Original license:
 */
/******************************************************************************
 *
 *  Copyright 2022 Google LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

struct lc3bin_header {
	uint16_t file_id;
	uint16_t header_size;
	uint16_t srate_100hz;
	uint16_t bitrate_100bps;
	uint16_t channels;
	uint16_t frame_10us;
	uint16_t rfu;
	uint16_t nsamples_low;
	uint16_t nsamples_high;
};

int lc3bin_read_header(uint8_t **data, int *frame_us, int *srate_hz,
		       int *nchannels, int *nsamples)
{
	struct lc3bin_header hdr;

	memcpy(&hdr, *data, sizeof(hdr));

	*nchannels = hdr.channels;
	*frame_us = hdr.frame_10us * 10;
	*srate_hz = hdr.srate_100hz * 100;
	*nsamples = hdr.nsamples_low | (hdr.nsamples_high << 16);

	*data += sizeof(hdr);

	return sizeof(hdr);
}

int lc3bin_read_data(struct broadcast_source_stream *source_stream, uint8_t **data, int nchannels, void *buffer)
{
	uint16_t nbytes;

	memcpy(&nbytes, *data, sizeof(nbytes));

	*data += sizeof(nbytes);

	memcpy(buffer, *data, nbytes);

	*data += nbytes;

	if (*data >= source_stream->end) {
		*data = source_stream->start;
		printk("End of LC3 array reached => looping., source stream> %p\n", source_stream);
	}

	return nbytes;
}
/****** end of lc3 handler code ******/

static void send_stream_data(struct broadcast_source_stream *source_stream)
{
	struct bt_bap_stream *stream = &source_stream->stream;
	struct net_buf *buf;
	int ret;

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	if (buf == NULL) {
		printk("Could not allocate buffer when sending on %p\n",
		       stream);
		return;
	}

	/* read one frame */
	ret = lc3bin_read_data(source_stream, &(source_stream->data), source_stream->ch_count, read_buffer);

	if (ret < 0) {
		printk("ERROR READING LC3 DATA!\n");
		return;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, read_buffer, source_stream->sdu_len);

	ret = bt_bap_stream_send(stream, buf, source_stream->seq_num++);
	if (ret < 0) {
		/* This will end broadcasting on this stream. */
		printk("Unable to broadcast data on %p: %d\n", stream, ret);
		net_buf_unref(buf);
		return;
	}

	source_stream->sent_count++;
	if ((source_stream->sent_count % 1000U) == 0U) {
		printk("Stream %p: Sent %u total ISO packets\n", stream, source_stream->sent_count);
	}
}

static void stream_started_cb(struct bt_bap_stream *stream)
{
	struct broadcast_source_stream *source_stream = CONTAINER_OF(stream, struct broadcast_source_stream, stream);

	source_stream->seq_num   = 0;
	source_stream->sent_count = 0;
	printk("Stream %p started (ready to send audio)\n", stream);

	k_sem_give(&sem_started);
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	printk("Stream %p stopped (reason 0x%02X)\n", stream, reason);

	k_sem_give(&sem_stopped);
}

static void stream_sent_cb(struct bt_bap_stream *stream)
{
	struct broadcast_source_stream *source_stream = CONTAINER_OF(stream, struct broadcast_source_stream, stream);

	send_stream_data(source_stream);
}

static struct bt_bap_stream_ops stream_ops = {
	.started = stream_started_cb,
	.stopped = stream_stopped_cb,
	.sent    = stream_sent_cb,
};

static int get_lc3_preset(struct bt_bap_lc3_preset *preset, const char *preset_arg)
{
	for (size_t i = 0U; i < ARRAY_SIZE(lc3_broadcast_presets); i++) {
		if (strcmp(preset_arg, lc3_broadcast_presets[i].name) == 0) {
			*preset = lc3_broadcast_presets[i].preset;
			return 0;
		}
	}

	return -EINVAL;
}

static int setup_broadcast_source(void)
{
	static struct bt_audio_codec_cfg subgroup_codec_cfg[NUM_SUBGROUPS];
	struct bt_bap_broadcast_source_stream_param stream_params[NUM_STREAMS];
	struct bt_bap_broadcast_source_subgroup_param subgroup_params[NUM_SUBGROUPS];
	struct bt_bap_broadcast_source_param create_param = { 0 };
	struct bt_bap_lc3_preset preset_lc3;
	int err;


	err = get_lc3_preset(&preset_lc3, BROADCAST_PRESET_ARG_NAME);
	if (err) {
		printk("Failed to get LC3 preset: %s\n", BROADCAST_PRESET_ARG_NAME);
		return err;
	}

	for (size_t i = 0; i < NUM_SUBGROUPS; i++) {
		memcpy(&subgroup_codec_cfg[i], &preset_lc3.codec_cfg,
		       sizeof(struct bt_audio_codec_cfg));

#if defined(CONFIG_BT_BAP_AUDIO_CH_MONO)
		/* MONO is implicit if omitted */
		bt_audio_codec_cfg_unset_val(&subgroup_codec_cfg[i],
			BT_AUDIO_CODEC_CFG_CHAN_ALLOC);
#endif

#if (SET_SUBGROUP_LANGUAGE_METADATA)
		if (strlen(subgroup_language[i]) == BT_AUDIO_LANG_SIZE) {
			bt_audio_codec_cfg_meta_set_lang(&subgroup_codec_cfg[i],
				subgroup_language[i]);
		}
#endif
#if (SET_SUBGROUP_PARENTAL_METADATA)
		if (subgroup_parental[i] <= BT_AUDIO_PARENTAL_RATING_AGE_18_OR_ABOVE) {
			bt_audio_codec_cfg_meta_set_parental_rating(&subgroup_codec_cfg[i],
				subgroup_parental[i]);
		}
#endif
#if (SET_SUBGROUP_BROADCASTNAME_METADATA)
		if (strlen(subgroup_broadcast_name[i]) > 0) {
			bt_audio_codec_cfg_meta_set_broadcast_name(&subgroup_codec_cfg[i],
				subgroup_broadcast_name[i], strlen(subgroup_broadcast_name[i]));
		}
#endif
#if (SET_SUBGROUP_POGRAMINFO_METADATA)
		if (strlen(subgroup_program_info[i]) > 0) {
			bt_audio_codec_cfg_meta_set_program_info(&subgroup_codec_cfg[i],
				subgroup_program_info[i], strlen(subgroup_program_info[i]));
		}
#endif

		subgroup_params[i].params_count = 1;
		subgroup_params[i].params = &stream_params[i];
		subgroup_params[i].codec_cfg = &subgroup_codec_cfg[i];
	}

	for (size_t j = 0; j < NUM_STREAMS; j++) {
		int frame_us;
		int srate_hz;
		int nchannels;
		int nsamples;

		streams[j].data = (uint8_t *)lc3_audio[j];

		printk("Reading LC3 audio header (%p)\n", streams[j].data);
		(void)lc3bin_read_header(&streams[j].data, &frame_us, &srate_hz, &nchannels, &nsamples);

		streams[j].ch_count = nchannels;
		streams[j].ch_spf = (srate_hz / 1000) * (frame_us / 1000);

		printk("======================\n");
		printk("Stream %d: Frame size: %dus\n", j, frame_us);
		printk("Stream %d: Sample rate: %dHz\n", j, srate_hz);
		printk("Stream %d: Number of channels: %d\n", j, nchannels);
		printk("Stream %d: Number of samples: %d\n", j, nsamples);
		printk("Stream %d: Sample per frame: %d\n", j, streams[j].ch_spf);

		streams[j].sdu_len = preset_lc3.qos.sdu;

		/* Store position of start and end+1 of frame blocks */
		streams[j].start = streams[j].data;
		streams[j].end = streams[j].start +
				 ((nsamples / streams[j].ch_spf) *
				  (streams[j].sdu_len + 2)); // TBD

		stream_params[j].stream = &streams[j].stream;
		stream_params[j].data = NULL;
		stream_params[j].data_len = 0;

		bt_bap_stream_cb_register(stream_params[j].stream, &stream_ops);
	}

	create_param.params_count = NUM_SUBGROUPS;
	create_param.params = subgroup_params;
	create_param.qos = &preset_lc3.qos;
	create_param.encryption = (strlen(CONFIG_BROADCAST_CODE) > 0);
	create_param.packing = BT_ISO_PACKING_SEQUENTIAL;

	printk("Creating broadcast source with %d subgroups...\n", NUM_SUBGROUPS);

	err = bt_bap_broadcast_source_create(&create_param, &broadcast_source);
	if (err) {
		printk("Failed to create broadcast source: %d\n", err);
		return err;
	}

	return 0;
}


int main(void)
{
	struct bt_le_ext_adv *adv;
	uint32_t broadcast_id = 0;
	int err;

	/* Check that the RGB PWM devices are present*/
	printk("Initialize RGB LED...\n");
	err = rgb_led_init();
	if (err) {
		printk("Error setting up RGB light!\n");
		return 0;
	}

	rgb_led_set(0, 0xff, 0);

	printk("Starting Auracast Broadcast Source demo (3-subgroups broadcast)\n");

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		rgb_led_set(0xff, 0, 0);

		return 0;
	}

	printk("Bluetooth initialized\n");

	/* Broadcast Audio Streaming Endpoint advertising data */
	NET_BUF_SIMPLE_DEFINE(ad_buf, BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE);
	NET_BUF_SIMPLE_DEFINE(base_buf, 256);

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CUSTOM, NULL, &adv);
	if (err) {
		printk("Failed to create extended advertising set (err %d)\n", err);
		rgb_led_set(0xff, 0, 0);

		return 0;
	}

	printk("Extended advertising created\n");

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(adv, BT_LE_PER_ADV_DEFAULT);
	if (err) {
		printk("Failed to set periodic advertising params (err %d)\n", err);
		rgb_led_set(0xff, 0, 0);

		return 0;
	}

	printk("Creating broadcast source\n");
	err = setup_broadcast_source();
	if (err) {
		printk("setup_broadcast_source failed (err %d)\n", err);
		rgb_led_set(0xff, 0, 0);

		return 0;
	}

	uint8_t hwid[3] = { 0 };
	if (hwinfo_get_device_id(hwid, sizeof(hwid)) == sizeof(hwid)) {
		broadcast_id |= (hwid[0] << 16) | (hwid[1] << 8) | hwid[2];
	} else {
		broadcast_id = 0xABCDEF;
	}

	/* Setup extended advertising data */
	net_buf_simple_add_le16(&ad_buf, BT_UUID_BROADCAST_AUDIO_VAL);
	net_buf_simple_add_le24(&ad_buf, broadcast_id);

	struct bt_data ext_ad[3];
	ext_ad[0] = (struct bt_data)BT_DATA(BT_DATA_BROADCAST_NAME, broadcast_name, strlen(broadcast_name));
	ext_ad[1].type = BT_DATA_SVC_DATA16;
	ext_ad[1].data_len = ad_buf.len;
	ext_ad[1].data = ad_buf.data;
	ext_ad[2] = (struct bt_data)BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
		sizeof(CONFIG_BT_DEVICE_NAME) - 1);

	err = bt_le_ext_adv_set_data(adv, ext_ad, ARRAY_SIZE(ext_ad), NULL, 0);
	if (err) {
		printk("Failed to set ext advertising data (err %d)\n", err);
		rgb_led_set(0xff, 0, 0);

		return 0;
	}

	/* Setup periodic advertising data */
	err = bt_bap_broadcast_source_get_base(broadcast_source, &base_buf);
	if (err) {
		printk("Failed to get BASE data (err %d)\n", err);
		rgb_led_set(0xff, 0, 0);

		return 0;
	}

	struct bt_data per_ad;
	per_ad.type = BT_DATA_SVC_DATA16;
	per_ad.data_len = base_buf.len;
	per_ad.data = base_buf.data;
	printk("PA data len = %d\n", per_ad.data_len);
	
	err = bt_le_per_adv_set_data(adv, &per_ad, 1);
	if (err) {
		printk("Failed to set periodic advertising data (err %d)\n", err);
		rgb_led_set(0xff, 0, 0);

		return 0;
	}

	/* Start extended advertising */
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising (err %d)\n", err);
		rgb_led_set(0xff, 0, 0);

		return 0;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(adv);
	if (err) {
		printk("Failed to start periodic advertising (err %d)\n", err);
		rgb_led_set(0xff, 0, 0);

		return 0;
	}

	printk("Advertising started (Extended + Periodic)\n");

	err = bt_bap_broadcast_source_start(broadcast_source, adv);
	if (err != 0) {
		printk("Unable to start broadcast source: %d\n", err);
		rgb_led_set(0xff, 0, 0);

		return 0;
	}

	printk("Broadcast source started, waiting for streams to be ready...\n");

	for (size_t i = 0; i < NUM_STREAMS; i++) {
		k_sem_take(&sem_started, K_FOREVER);
	}

	printk("All BIS streams started. Beginning audio transmission.\n");

	for (size_t i = 0; i < NUM_STREAMS; i++) {
		for (uint8_t j = 0; j < BROADCAST_ENQUEUE_COUNT; j++) {
			stream_sent_cb(&streams[i].stream);
		}
	}

	printk("Audio streaming in progress on all %d streams.\n", NUM_STREAMS);
	rgb_led_set(0, 0, 0xff);

	return 0;
}

/**
 * LC3 audio data
 */

#ifndef LC3_AUDIO_H
#define LC3_AUDIO_H


#define BROADCAST_NAME 		"Multisubgroup Broadcast"

#define NUM_STREAM_FILES	3

#define STREAM1_FILENAME 	"stream_filename_1.lc3.inc"
#define STREAM2_FILENAME 	"stream_filename_2.lc3.inc"
#define STREAM3_FILENAME 	"stream_filename_3.lc3.inc"

static const uint8_t stream1_data[] = {
	#include STREAM1_FILENAME
};

static const uint8_t stream2_data[] = {
	#include STREAM2_FILENAME
};

static const uint8_t stream3_data[] = {
	#include STREAM3_FILENAME
};

static const uint8_t *lc3_audio[] = {
	stream1_data,
	stream2_data,
	stream3_data
};


#define SET_SUBGROUP_LANGUAGE_METADATA		true
#define SET_SUBGROUP_PARENTAL_METADATA		true
#define SET_SUBGROUP_BROADCASTNAME_METADATA	true
#define SET_SUBGROUP_PROGRAMINFO_METADATA	true

const uint8_t *subgroup_language[] = {
	"eng",
	"fra",
	"spa"
};

enum bt_audio_parental_rating subgroup_parental[] = {
	BT_AUDIO_PARENTAL_RATING_AGE_ANY,
	BT_AUDIO_PARENTAL_RATING_AGE_5_OR_ABOVE,
	BT_AUDIO_PARENTAL_RATING_AGE_7_OR_ABOVE
};

const uint8_t *subgroup_broadcast_name[] = {
	"Broadcast 1",
	"Broadcast 2",
	"Broadcast 3"
};

const uint8_t *subgroup_program_info[] = {
	"Program info 1",
	"Program info 2",
	"Program info 3"
};

#endif /* LC3_AUDIO_H */

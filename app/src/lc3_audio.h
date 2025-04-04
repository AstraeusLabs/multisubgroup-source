/**
 * LC3 audio data
 */

#ifndef LC3_AUDIO_H
#define LC3_AUDIO_H


#define BROADCAST_NAME 		"Multi-Subgroup Broadcast"

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


#define SET_SUBGROUP_NAME_METADATA		true
#define SET_SUBGROUP_LANGUAGES_METADATA		true
#define SET_SUBGROUP_PARENTALSS_METADATA	true

const uint8_t *subgroup_names[] = {
	"Subgroup_Audio_1",
	"Subgroup_Audio_2",
	"Subgroup_Audio_3"
};

const uint8_t *subgroup_languages[] = {
	"eng",
	"fra",
	"spa"
};

enum bt_audio_parental_rating subgroup_parentals[] = {
	BT_AUDIO_PARENTAL_RATING_AGE_ANY,
	BT_AUDIO_PARENTAL_RATING_AGE_5_OR_ABOVE,
	BT_AUDIO_PARENTAL_RATING_NO_RATING
};

#endif /* LC3_AUDIO_H */

/*
 * Copyright (C) 2003-2009 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef MPD_AUDIO_FORMAT_H
#define MPD_AUDIO_FORMAT_H

#include <stdint.h>
#include <stdbool.h>

struct audio_format {
	uint32_t sample_rate;
	uint8_t bits;
	uint8_t channels;
};

static inline void audio_format_clear(struct audio_format *af)
{
	af->sample_rate = 0;
	af->bits = 0;
	af->channels = 0;
}

static inline bool audio_format_defined(const struct audio_format *af)
{
	return af->sample_rate != 0;
}

/**
 * Checks whether the sample rate is valid.
 *
 * @param sample_rate the sample rate in Hz
 */
static inline bool
audio_valid_sample_rate(unsigned sample_rate)
{
	return sample_rate > 0 && sample_rate < (1 << 30);
}

/**
 * Checks whether the sample format is valid.
 *
 * @param bits the number of significant bits per sample
 */
static inline bool
audio_valid_sample_format(unsigned bits)
{
	return bits == 16 || bits == 24 || bits == 32 || bits == 8;
}

/**
 * Checks whether the number of channels is valid.
 */
static inline bool
audio_valid_channel_count(unsigned channels)
{
	return channels >= 1 && channels <= 8;
}

/**
 * Returns false if the format is not valid for playback with MPD.
 * This function performs some basic validity checks.
 */
static inline bool audio_format_valid(const struct audio_format *af)
{
	return audio_valid_sample_rate(af->sample_rate) &&
		audio_valid_sample_format(af->bits) &&
		audio_valid_channel_count(af->channels);
}

static inline bool audio_format_equals(const struct audio_format *a,
				       const struct audio_format *b)
{
	return a->sample_rate == b->sample_rate &&
		a->bits == b->bits &&
		a->channels == b->channels;
}

/**
 * Returns the size of each (mono) sample in bytes.
 */
static inline unsigned audio_format_sample_size(const struct audio_format *af)
{
	if (af->bits <= 8)
		return 1;
	else if (af->bits <= 16)
		return 2;
	else
		return 4;
}

static inline unsigned
audio_format_frame_size(const struct audio_format *af)
{
	return audio_format_sample_size(af) * af->channels;
}

static inline double audio_format_time_to_size(const struct audio_format *af)
{
	return af->sample_rate * audio_format_frame_size(af);
}

static inline double audioFormatSizeToTime(const struct audio_format *af)
{
	return 1.0 / audio_format_time_to_size(af);
}

#endif

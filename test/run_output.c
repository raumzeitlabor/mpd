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

#include "output_plugin.h"
#include "output_internal.h"
#include "output_control.h"
#include "conf.h"
#include "audio_parser.h"

#include <glib.h>

#include <assert.h>
#include <string.h>
#include <unistd.h>

void pcm_convert_init(G_GNUC_UNUSED struct pcm_convert_state *state)
{
}

void notify_init(G_GNUC_UNUSED struct notify *notify)
{
}

static const struct config_param *
find_named_config_block(const char *block, const char *name)
{
	const struct config_param *param = NULL;

	while ((param = config_get_next_param(block, param)) != NULL) {
		const char *current_name =
			config_get_block_string(param, "name", NULL);
		if (current_name != NULL && strcmp(current_name, name) == 0)
			return param;
	}

	return NULL;
}

static bool
load_audio_output(struct audio_output *ao, const char *name)
{
	const struct config_param *param;
	bool success;
	GError *error = NULL;

	param = find_named_config_block(CONF_AUDIO_OUTPUT, name);
	if (param == NULL) {
		g_printerr("No such configured audio output: %s\n", name);
		return false;
	}

	success = audio_output_init(ao, param, &error);
	if (!success) {
		g_printerr("%s\n", error->message);
		g_error_free(error);
	}

	return success;
}

int main(int argc, char **argv)
{
	struct audio_output ao;
	struct audio_format audio_format = {
		.sample_rate = 44100,
		.bits = 16,
		.channels = 2,
	};
	bool success;
	GError *error = NULL;
	char buffer[4096];
	ssize_t nbytes;
	size_t frame_size, length = 0, play_length, consumed;

	if (argc < 3 || argc > 4) {
		g_printerr("Usage: run_output CONFIG NAME [FORMAT] <IN\n");
		return 1;
	}

	g_thread_init(NULL);

	/* read configuration file (mpd.conf) */

	config_global_init();
	config_read_file(argv[1]);

	/* initialize the audio output */

	if (!load_audio_output(&ao, argv[2]))
		return 1;

	/* parse the audio format */

	if (argc > 3) {
		success = audio_format_parse(&audio_format, argv[3], &error);
		if (!success) {
			g_printerr("Failed to parse audio format: %s\n",
				   error->message);
			g_error_free(error);
			return 1;
		}
	}

	/* open the audio output */

	success = ao_plugin_open(ao.plugin, ao.data, &audio_format, &error);
	if (!success) {
		g_printerr("Failed to open audio output: %s\n",
			   error->message);
		g_error_free(error);
		return 1;
	}

	g_printerr("audio_format=%u:%u:%u\n", audio_format.sample_rate,
		   audio_format.bits, audio_format.channels);

	frame_size = audio_format_frame_size(&audio_format);

	/* play */

	while (true) {
		if (length < sizeof(buffer)) {
			nbytes = read(0, buffer + length, sizeof(buffer) - length);
			if (nbytes <= 0)
				break;

			length += (size_t)nbytes;
		}

		play_length = (length / frame_size) * frame_size;
		if (play_length > 0) {
			consumed = ao_plugin_play(ao.plugin, ao.data,
						  buffer, play_length,
						  &error);
			if (consumed == 0) {
				g_printerr("Failed to play: %s\n",
					   error->message);
				g_error_free(error);
				return 1;
			}

			assert(consumed <= length);
			assert(consumed % frame_size == 0);

			length -= consumed;
			memmove(buffer, buffer + consumed, length);
		}
	}

	/* cleanup and exit */

	ao_plugin_close(ao.plugin, ao.data);
	ao_plugin_finish(ao.plugin, ao.data);
	g_mutex_free(ao.mutex);

	config_global_finish();

	return 0;
}

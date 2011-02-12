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

#include "decoder_list.h"
#include "decoder_plugin.h"
#include "utils.h"
#include "config.h"
#include "conf.h"

#include <glib.h>

#include <string.h>

extern const struct decoder_plugin mad_decoder_plugin;
extern const struct decoder_plugin vorbis_decoder_plugin;
extern const struct decoder_plugin flac_decoder_plugin;
extern const struct decoder_plugin oggflac_decoder_plugin;
extern const struct decoder_plugin audiofile_decoder_plugin;
extern const struct decoder_plugin mp4ff_decoder_plugin;
extern const struct decoder_plugin faad_decoder_plugin;
extern const struct decoder_plugin mpcdec_decoder_plugin;
extern const struct decoder_plugin wavpack_decoder_plugin;
extern const struct decoder_plugin modplug_decoder_plugin;
extern const struct decoder_plugin mikmod_decoder_plugin;
extern const struct decoder_plugin sidplay_decoder_plugin;
extern const struct decoder_plugin wildmidi_decoder_plugin;
extern const struct decoder_plugin fluidsynth_decoder_plugin;
extern const struct decoder_plugin ffmpeg_decoder_plugin;

static const struct decoder_plugin *const decoder_plugins[] = {
#ifdef HAVE_MAD
	&mad_decoder_plugin,
#endif
#ifdef ENABLE_VORBIS_DECODER
	&vorbis_decoder_plugin,
#endif
#if defined(HAVE_FLAC) || defined(HAVE_OGGFLAC)
	&oggflac_decoder_plugin,
#endif
#ifdef HAVE_FLAC
	&flac_decoder_plugin,
#endif
#ifdef HAVE_AUDIOFILE
	&audiofile_decoder_plugin,
#endif
#ifdef HAVE_FAAD
	&faad_decoder_plugin,
#endif
#ifdef HAVE_MP4
	&mp4ff_decoder_plugin,
#endif
#ifdef HAVE_MPCDEC
	&mpcdec_decoder_plugin,
#endif
#ifdef HAVE_WAVPACK
	&wavpack_decoder_plugin,
#endif
#ifdef HAVE_MODPLUG
	&modplug_decoder_plugin,
#endif
#ifdef ENABLE_MIKMOD_DECODER
	&mikmod_decoder_plugin,
#endif
#ifdef ENABLE_SIDPLAY
	&sidplay_decoder_plugin,
#endif
#ifdef ENABLE_WILDMIDI
	&wildmidi_decoder_plugin,
#endif
#ifdef ENABLE_FLUIDSYNTH
	&fluidsynth_decoder_plugin,
#endif
#ifdef HAVE_FFMPEG
	&ffmpeg_decoder_plugin,
#endif
};

enum {
	num_decoder_plugins = G_N_ELEMENTS(decoder_plugins),
};

/** which plugins have been initialized successfully? */
static bool decoder_plugins_enabled[num_decoder_plugins];

const struct decoder_plugin *
decoder_plugin_from_suffix(const char *suffix, unsigned int next)
{
	static unsigned i = num_decoder_plugins;

	if (suffix == NULL)
		return NULL;

	if (!next)
		i = 0;
	for (; i < num_decoder_plugins; ++i) {
		const struct decoder_plugin *plugin = decoder_plugins[i];
		if (decoder_plugins_enabled[i] &&
		    stringFoundInStringArray(plugin->suffixes, suffix)) {
			++i;
			return plugin;
		}
	}

	return NULL;
}

const struct decoder_plugin *
decoder_plugin_from_mime_type(const char *mimeType, unsigned int next)
{
	static unsigned i = num_decoder_plugins;

	if (mimeType == NULL)
		return NULL;

	if (!next)
		i = 0;
	for (; i < num_decoder_plugins; ++i) {
		const struct decoder_plugin *plugin = decoder_plugins[i];
		if (decoder_plugins_enabled[i] &&
		    stringFoundInStringArray(plugin->mime_types, mimeType)) {
			++i;
			return plugin;
		}
	}

	return NULL;
}

const struct decoder_plugin *
decoder_plugin_from_name(const char *name)
{
	for (unsigned i = 0; i < num_decoder_plugins; ++i) {
		const struct decoder_plugin *plugin = decoder_plugins[i];
		if (decoder_plugins_enabled[i] &&
		    strcmp(plugin->name, name) == 0)
			return plugin;
	}

	return NULL;
}

void decoder_plugin_print_all_decoders(FILE * fp)
{
	for (unsigned i = 0; i < num_decoder_plugins; ++i) {
		const struct decoder_plugin *plugin = decoder_plugins[i];
		const char *const*suffixes;

		if (!decoder_plugins_enabled[i])
			continue;

		fprintf(fp, "[%s]", plugin->name);

		for (suffixes = plugin->suffixes;
		     suffixes != NULL && *suffixes != NULL;
		     ++suffixes) {
			fprintf(fp, " %s", *suffixes);
		}

		fprintf(fp, "\n");
	}
}

/**
 * Find the "decoder" configuration block for the specified plugin.
 *
 * @param plugin_name the name of the decoder plugin
 * @return the configuration block, or NULL if none was configured
 */
static const struct config_param *
decoder_plugin_config(const char *plugin_name)
{
	const struct config_param *param = NULL;

	while ((param = config_get_next_param(CONF_DECODER, param)) != NULL) {
		const char *name =
			config_get_block_string(param, "plugin", NULL);
		if (name == NULL)
			g_error("decoder configuration without 'plugin' name in line %d",
				param->line);

		if (strcmp(name, plugin_name) == 0)
			return param;
	}

	return NULL;
}

void decoder_plugin_init_all(void)
{
	for (unsigned i = 0; i < num_decoder_plugins; ++i) {
		const struct decoder_plugin *plugin = decoder_plugins[i];
		const struct config_param *param =
			decoder_plugin_config(plugin->name);

		if (!config_get_block_bool(param, "enabled", true))
			/* the plugin is disabled in mpd.conf */
			continue;

		if (decoder_plugin_init(plugin, param))
			decoder_plugins_enabled[i] = true;
	}
}

void decoder_plugin_deinit_all(void)
{
	for (unsigned i = 0; i < num_decoder_plugins; ++i) {
		const struct decoder_plugin *plugin = decoder_plugins[i];

		if (decoder_plugins_enabled[i])
			decoder_plugin_finish(plugin);
	}
}

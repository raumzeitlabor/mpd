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

#include "cmdline.h"
#include "path.h"
#include "log.h"
#include "conf.h"
#include "decoder_list.h"
#include "config.h"
#include "output_list.h"
#include "ls.h"

#ifdef ENABLE_ARCHIVE
#include "archive_list.h"
#endif

#include <glib.h>

#include <stdio.h>
#include <stdlib.h>

#define SYSTEM_CONFIG_FILE_LOCATION	"/etc/mpd.conf"
#define USER_CONFIG_FILE_LOCATION1	".mpdconf"
#define USER_CONFIG_FILE_LOCATION2	".mpd/mpd.conf"

G_GNUC_NORETURN
static void version(void)
{
	puts(PACKAGE " (MPD: Music Player Daemon) " VERSION " \n"
	     "\n"
	     "Copyright (C) 2003-2007 Warren Dukes <warren.dukes@gmail.com>\n"
	     "Copyright (C) 2008 Max Kellermann <max@duempel.org>\n"
	     "This is free software; see the source for copying conditions.  There is NO\n"
	     "warranty; not even MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
	     "\n"
	     "Supported decoders:\n");

	decoder_plugin_init_all();
	decoder_plugin_print_all_decoders(stdout);

	puts("\n"
	     "Supported outputs:\n");
	audio_output_plugin_print_all_types(stdout);

#ifdef ENABLE_ARCHIVE
	puts("\n"
	     "Supported archives:\n");
	archive_plugin_init_all();
	archive_plugin_print_all_suffixes(stdout);
#endif

	puts("\n"
	      "Supported protocols:\n");
	print_supported_uri_schemes_to_fp(stdout);

	exit(EXIT_SUCCESS);
}

#if GLIB_CHECK_VERSION(2,12,0)
static const char *summary =
	"Music Player Daemon - a daemon for playing music.";
#endif

void parseOptions(int argc, char **argv, Options *options)
{
	GError *error = NULL;
	GOptionContext *context;
	bool ret;
	static gboolean option_version,
		option_create_db, option_no_create_db, option_no_daemon,
		option_no_config;
	const GOptionEntry entries[] = {
		{ "create-db", 0, 0, G_OPTION_ARG_NONE, &option_create_db,
		  "force (re)creation of database", NULL },
		{ "kill", 0, 0, G_OPTION_ARG_NONE, &options->kill,
		  "kill the currently running mpd session", NULL },
		{ "no-config", 0, 0, G_OPTION_ARG_NONE, &option_no_config,
		  "don't read from config", NULL },
		{ "no-create-db", 0, 0, G_OPTION_ARG_NONE, &option_no_create_db,
		  "don't create database, even if it doesn't exist", NULL },
		{ "no-daemon", 0, 0, G_OPTION_ARG_NONE, &option_no_daemon,
		  "don't detach from console", NULL },
		{ "stdout", 0, 0, G_OPTION_ARG_NONE, &options->stdOutput,
		  "print messages to stderr", NULL },
		{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &options->verbose,
		  "verbose logging", NULL },
		{ "version", 'V', 0, G_OPTION_ARG_NONE, &option_version,
		  "print version number", NULL },
		{ .long_name = NULL }
	};

	options->kill = false;
	options->daemon = true;
	options->stdOutput = false;
	options->verbose = false;
	options->createDB = 0;

	context = g_option_context_new("[path/to/mpd.conf]");
	g_option_context_add_main_entries(context, entries, NULL);

#if GLIB_CHECK_VERSION(2,12,0)
	g_option_context_set_summary(context, summary);
#endif

	ret = g_option_context_parse(context, &argc, &argv, &error);
	g_option_context_free(context);

	if (!ret) {
		g_error("option parsing failed: %s\n", error->message);
		exit(1);
	}

	if (option_version)
		version();

	/* initialize the logging library, so the configuration file
	   parser can use it already */
	log_early_init(options->verbose);

	if (option_create_db && option_no_create_db)
		g_error("Cannot use both --create-db and --no-create-db\n");

	if (option_no_create_db)
		options->createDB = -1;
	else if (option_create_db)
		options->createDB = 1;

	options->daemon = !option_no_daemon;

	if (option_no_config) {
		g_debug("Ignoring config, using daemon defaults\n");
	} else if (argc <= 1) {
		/* default configuration file path */
		char *path1;
		char *path2;

		path1 = g_build_filename(g_get_home_dir(),
					USER_CONFIG_FILE_LOCATION1, NULL);
		path2 = g_build_filename(g_get_home_dir(),
					USER_CONFIG_FILE_LOCATION2, NULL);
		if (g_file_test(path1, G_FILE_TEST_IS_REGULAR))
			config_read_file(path1);
		else if (g_file_test(path2, G_FILE_TEST_IS_REGULAR))
			config_read_file(path2);
		else if (g_file_test(SYSTEM_CONFIG_FILE_LOCATION,
				     G_FILE_TEST_IS_REGULAR))
			config_read_file(SYSTEM_CONFIG_FILE_LOCATION);
		g_free(path1);
		g_free(path2);
	} else if (argc == 2) {
		/* specified configuration file */
		config_read_file(argv[1]);
	} else
		g_error("too many arguments");
}

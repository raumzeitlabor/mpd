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

#ifndef MPD_ARCHIVE_API_H
#define MPD_ARCHIVE_API_H

/*
 * This is the public API which is used by archive plugins to
 * provide transparent archive decompression layer for mpd
 *
 */

#include "archive_internal.h"
#include "input_stream.h"

#include <stdbool.h>

struct archive_file;

struct archive_plugin {
	const char *name;

	/**
	 * optional, set this to NULL if the archive plugin doesn't
	 * have/need one this must false if there is an error and
	 * true otherwise
	 */
	bool (*init)(void);

	/**
	 * optional, set this to NULL if the archive plugin doesn't
	 * have/need one
	 */
	void (*finish)(void);

	/**
	 * tryes to open archive file and associates handle with archive
	 * returns pointer to handle used is all operations with this archive
	 * or NULL when opening fails
	 */
	struct archive_file *(*open)(char * pathname);

	/**
	 * reset routine will move current read index in archive to default
	 * position and then the filenames from archives can be read
	 * via scan_next routine
	 */
	void  (*scan_reset)(struct archive_file *);

	/**
	 * the read method will return corresponding files from archive
	 * (as pathnames) and move read index to next file. When there is no
	 * next file it return NULL.
	 */
	char *(*scan_next)(struct archive_file *);

	/**
	 * Opens an input_stream of a file within the archive.
	 *
	 * If this function succeeds, then the #input_stream "owns"
	 * the archive file and will automatically close it.
	 *
	 * @param path the path within the archive
	 */
	bool (*open_stream)(struct archive_file *, struct input_stream *is,
			    const char *path);

	/**
	 * closes archive file.
	 */
	void (*close)(struct archive_file *);

	/**
	 * suffixes handled by this plugin.
	 * last element in these arrays must always be a NULL
	 */
	const char *const*suffixes;
};

bool archive_lookup(char *pathname, char **archive, char **inpath, char **suffix);

#endif


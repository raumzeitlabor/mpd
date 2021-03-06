/*
 * Copyright (C) 2003-2010 The Music Player Daemon Project
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

/*
 * Maps directory and song objects to file system paths.
 */

#include "config.h"
#include "mapper.h"
#include "directory.h"
#include "song.h"
#include "path.h"

#include <glib.h>

#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

static char *music_dir;
static size_t music_dir_length;

static char *playlist_dir;

/**
 * Duplicate a string, chop all trailing slashes.
 */
static char *
strdup_chop_slash(const char *path_fs)
{
	size_t length = strlen(path_fs);

	while (length > 0 && path_fs[length - 1] == G_DIR_SEPARATOR)
		--length;

	return g_strndup(path_fs, length);
}

static void
check_directory(const char *path)
{
	struct stat st;
	if (stat(path, &st) < 0) {
		g_warning("Failed to stat directory \"%s\": %s",
			  path, g_strerror(errno));
		return;
	}

	if (!S_ISDIR(st.st_mode)) {
		g_warning("Not a directory: %s", path);
		return;
	}

#ifndef WIN32
	char *x = g_build_filename(path, ".", NULL);
	if (stat(x, &st) < 0 && errno == EACCES)
		g_warning("No permission to traverse (\"execute\") directory: %s",
			  path);
	g_free(x);
#endif

	DIR *dir = opendir(path);
	if (dir == NULL && errno == EACCES)
		g_warning("No permission to read directory: %s", path);
	else
		closedir(dir);
}

static void
mapper_set_music_dir(const char *path)
{
	check_directory(path);

	music_dir = strdup_chop_slash(path);
	music_dir_length = strlen(music_dir);
}

static void
mapper_set_playlist_dir(const char *path)
{
	check_directory(path);

	playlist_dir = g_strdup(path);
}

void mapper_init(const char *_music_dir, const char *_playlist_dir)
{
	if (_music_dir != NULL)
		mapper_set_music_dir(_music_dir);

	if (_playlist_dir != NULL)
		mapper_set_playlist_dir(_playlist_dir);
}

void mapper_finish(void)
{
	g_free(music_dir);
	g_free(playlist_dir);
}

bool
mapper_has_music_directory(void)
{
	return music_dir != NULL;
}

const char *
map_to_relative_path(const char *path_utf8)
{
	return music_dir != NULL &&
		memcmp(path_utf8, music_dir, music_dir_length) == 0 &&
		G_IS_DIR_SEPARATOR(path_utf8[music_dir_length])
		? path_utf8 + music_dir_length + 1
		: path_utf8;
}

char *
map_uri_fs(const char *uri)
{
	char *uri_fs, *path_fs;

	assert(uri != NULL);
	assert(*uri != '/');

	if (music_dir == NULL)
		return NULL;

	uri_fs = utf8_to_fs_charset(uri);
	if (uri_fs == NULL)
		return NULL;

	path_fs = g_build_filename(music_dir, uri_fs, NULL);
	g_free(uri_fs);

	return path_fs;
}

char *
map_directory_fs(const struct directory *directory)
{
	assert(music_dir != NULL);

	if (directory_is_root(directory))
		return g_strdup(music_dir);

	return map_uri_fs(directory_get_path(directory));
}

char *
map_directory_child_fs(const struct directory *directory, const char *name)
{
	char *name_fs, *parent_fs, *path;

	assert(music_dir != NULL);

	/* check for invalid or unauthorized base names */
	if (*name == 0 || strchr(name, '/') != NULL ||
	    strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return NULL;

	parent_fs = map_directory_fs(directory);
	if (parent_fs == NULL)
		return NULL;

	name_fs = utf8_to_fs_charset(name);
	if (name_fs == NULL) {
		g_free(parent_fs);
		return NULL;
	}

	path = g_build_filename(parent_fs, name_fs, NULL);
	g_free(parent_fs);
	g_free(name_fs);

	return path;
}

char *
map_song_fs(const struct song *song)
{
	assert(song_is_file(song));

	if (song_in_database(song))
		return map_directory_child_fs(song->parent, song->uri);
	else
		return utf8_to_fs_charset(song->uri);
}

char *
map_fs_to_utf8(const char *path_fs)
{
	if (music_dir != NULL &&
	    strncmp(path_fs, music_dir, music_dir_length) == 0 &&
	    G_IS_DIR_SEPARATOR(path_fs[music_dir_length]))
		/* remove musicDir prefix */
		path_fs += music_dir_length + 1;
	else if (G_IS_DIR_SEPARATOR(path_fs[0]))
		/* not within musicDir */
		return NULL;

	while (path_fs[0] == G_DIR_SEPARATOR)
		++path_fs;

	return fs_charset_to_utf8(path_fs);
}

const char *
map_spl_path(void)
{
	return playlist_dir;
}

char *
map_spl_utf8_to_fs(const char *name)
{
	char *filename_utf8, *filename_fs, *path;

	if (playlist_dir == NULL)
		return NULL;

	filename_utf8 = g_strconcat(name, PLAYLIST_FILE_SUFFIX, NULL);
	filename_fs = utf8_to_fs_charset(filename_utf8);
	g_free(filename_utf8);
	if (filename_fs == NULL)
		return NULL;

	path = g_build_filename(playlist_dir, filename_fs, NULL);
	g_free(filename_fs);

	return path;
}

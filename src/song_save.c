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

#include "song_save.h"
#include "song.h"
#include "tag_save.h"
#include "directory.h"
#include "tag.h"

#include <glib.h>

#include <stdlib.h>

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "song"

#define SONG_KEY	"key: "
#define SONG_MTIME	"mtime: "

static void
song_save_url(FILE *fp, struct song *song)
{
	if (song->parent != NULL && song->parent->path != NULL)
		fprintf(fp, SONG_FILE "%s/%s\n",
			directory_get_path(song->parent), song->url);
	else
		fprintf(fp, SONG_FILE "%s\n",
			song->url);
}

static int
song_save(struct song *song, void *data)
{
	FILE *fp = data;

	fprintf(fp, SONG_KEY "%s\n", song->url);

	song_save_url(fp, song);

	if (song->tag != NULL)
		tag_save(fp, song->tag);

	fprintf(fp, SONG_MTIME "%li\n", (long)song->mtime);

	return 0;
}

void songvec_save(FILE *fp, struct songvec *sv)
{
	fprintf(fp, "%s\n", SONG_BEGIN);
	songvec_for_each(sv, song_save, fp);
	fprintf(fp, "%s\n", SONG_END);
}

static void
insertSongIntoList(struct songvec *sv, struct song *newsong)
{
	struct song *existing = songvec_find(sv, newsong->url);

	if (!existing) {
		songvec_add(sv, newsong);
		if (newsong->tag)
			tag_end_add(newsong->tag);
	} else { /* prevent dupes, just update the existing song info */
		if (existing->mtime != newsong->mtime) {
			if (existing->tag != NULL)
				tag_free(existing->tag);
			if (newsong->tag)
				tag_end_add(newsong->tag);
			existing->tag = newsong->tag;
			existing->mtime = newsong->mtime;
			newsong->tag = NULL;
		}
		song_free(newsong);
	}
}

static char *
matchesAnMpdTagItemKey(char *buffer, enum tag_type *itemType)
{
	int i;

	for (i = 0; i < TAG_NUM_OF_ITEM_TYPES; i++) {
		size_t len = strlen(tag_item_names[i]);

		if (0 == strncmp(tag_item_names[i], buffer, len) &&
		    buffer[len] == ':') {
			*itemType = i;
			return g_strchug(buffer + len + 1);
		}
	}

	return NULL;
}

void readSongInfoIntoList(FILE *fp, struct songvec *sv,
			  struct directory *parent)
{
	enum {
		buffer_size = 32768,
	};
	char *buffer = g_malloc(buffer_size);
	struct song *song = NULL;
	enum tag_type itemType;
	const char *value;

	while (fgets(buffer, buffer_size, fp) &&
	       !g_str_has_prefix(buffer, SONG_END)) {
		g_strchomp(buffer);

		if (0 == strncmp(SONG_KEY, buffer, strlen(SONG_KEY))) {
			if (song)
				insertSongIntoList(sv, song);

			song = song_file_new(buffer + strlen(SONG_KEY),
					     parent);
		} else if (*buffer == 0) {
			/* ignore empty lines (starting with '\0') */
		} else if (song == NULL) {
			g_error("Problems reading song info");
		} else if (0 == strncmp(SONG_FILE, buffer, strlen(SONG_FILE))) {
			/* we don't need this info anymore */
		} else if ((value = matchesAnMpdTagItemKey(buffer,
							   &itemType)) != NULL) {
			if (!song->tag) {
				song->tag = tag_new();
				tag_begin_add(song->tag);
			}

			tag_add_item(song->tag, itemType, value);
		} else if (0 == strncmp(SONG_TIME, buffer, strlen(SONG_TIME))) {
			if (!song->tag) {
				song->tag = tag_new();
				tag_begin_add(song->tag);
			}

			song->tag->time = atoi(&(buffer[strlen(SONG_TIME)]));
		} else if (0 == strncmp(SONG_MTIME, buffer, strlen(SONG_MTIME))) {
			song->mtime = atoi(&(buffer[strlen(SONG_MTIME)]));
		}
		else
			g_error("unknown line in db: %s", buffer);
	}

	g_free(buffer);

	if (song)
		insertSongIntoList(sv, song);
}

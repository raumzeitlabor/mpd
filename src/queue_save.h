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

/*
 * This library saves the queue into the state file, and also loads it
 * back into memory.
 */

#ifndef QUEUE_SAVE_H
#define QUEUE_SAVE_H

#include <stdio.h>

struct queue;

void
queue_save(FILE *fp, const struct queue *queue);

/**
 * Loads one song from the state file line and returns its number.
 * Returns -1 on failure.
 */
int
queue_load_song(struct queue *queue, const char *line);

#endif

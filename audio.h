/*-
 * audio.h - interface to the low-level audio structure
 * Copyright (C) 2012  University Radio York Computing Team
 *
 * This file is a part of playslave.
 *
 * playslave is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * playslave is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with playslave; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#ifndef AUDIO_H
#define AUDIO_H

#include "errors.h"

/* The audio structure contains all state pertaining to the currently
 * playing audio file.
 *
 * struct audio is an opaque structure; only audio.c knows its true
 * definition.
 */
struct audio;

/* Loads a file and constructs an audio structure to hold the playback
 * state.
 */
enum audio_init_err
audio_load(struct audio **au,	/* Location for the audio struct pointer */
	   const char *path,	/* File to load into the audio struct */
	   int device_id);	/* ID of the device to play out on */

void		audio_unload(struct audio *au);	/* Frees an audio struct */
enum error	audio_start(struct audio *au);	/* Starts playback */
enum error	audio_stop(struct audio *au);	/* Stops playback */

#endif				/* !AUDIO_H */

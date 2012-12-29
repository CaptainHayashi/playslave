/*
 * =============================================================================
 *
 *       Filename:  audio_av.h
 *
 *    Description:  Interface to ffmpeg-specific code
 *
 *        Version:  1.0
 *        Created:  23/12/2012 01:19:42
 *       Revision:  none
 *       Compiler:  clang
 *
 *         Author:  Matt Windsor (CaptainHayashi), matt.windsor@ury.york.ac.uk
 *        Company:  University Radio York Computing Team
 *
 * =============================================================================
 */

/*-
 * Copyright (C) 2012  University Radio York Computing Team
 *
 * This file is a part of playslave.
 *
 * playslave is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * playslave is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * playslave; if not, write to the Free Software Foundation, Inc., 51 Franklin
 * Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef AUDIO_AV_H
#define AUDIO_AV_H

/**  INCLUDES  ****************************************************************/

#include <stdint.h>		/* uint64_t */

#include <libavformat/avformat.h>
#include <portaudio.h>		/* PaStreamParameters */

#include "cuppa/errors.h"	/* enum error */

/**  DATA TYPES  **************************************************************/

/* The audio input structure (thusly named in case we ever generalise
 * away from ffmpeg), containing all state pertaining to the input
 * decoder for a file.
 *
 * struct au_in is an opaque structure; only audio_av.c knows its true
 * definition.
 */
struct au_in;

/* A structure containing a lump of decoded frame data.
 *
 * struct au_frame is an opaque structure; only audio_av.c knows its true
 * definition.
 */
struct au_frame;

/**  FUNCTIONS ****************************************************************/

/* Attempts to set ffmpeg up for reading the file in 'path', placing
 * the resulting au_in structure pointer in the location pointed to by
 * 'av'.
 */
enum error	audio_av_load(struct au_in **av, const char *path);
void		audio_av_unload(struct au_in *av);

/* Populates the given PortAudio parameter variables with information
 * from the ffmpeg audio context.
 *
 * The parameter variables MUST already have been allocated.
 */
enum error
audio_av_pa_config(struct au_in *av,	/* ffmpeg audio structure */
		   int device,	/* PortAudio device */
		   PaStreamParameters *params,
		   size_t *samples_per_buf);

enum error	audio_av_decode(struct au_in *av, char **buf, size_t *n);
double		audio_av_sample_rate(struct au_in *av);

enum error	audio_av_seek(struct au_in *av, uint64_t usec);

/* Unit conversion */
uint64_t	audio_av_samples2usec(struct au_in *av, size_t samples);
size_t		audio_av_usec2samples(struct au_in *av, uint64_t usec);
size_t		audio_av_bytes2samples(struct au_in *av, size_t bytes);
size_t		audio_av_samples2bytes(struct au_in *av, size_t samples);

#endif				/* not AUDIO_AV_H */

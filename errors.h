/*-
 * errors.h - error constants
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
#ifndef ERRORS_H
#define ERRORS_H

/* Categories of error. */
enum error {
	E_OK = 0,		/* No error */
	/* User errors */
	E_NO_FILE,		/* Tried to read nonexistent file */
	E_BAD_STATE,		/* State transition not allowed */
	/* Environment errors */
	E_BAD_FILE,		/* Tried to read corrupt file */
	E_BAD_CONFIG,		/* Program improperly configured */
	/* System errors */
	E_AUDIO_INIT_FAIL,	/* Couldn't open audio backend */
	E_INTERNAL_ERROR,	/* General system error, usually fatal */
	E_NO_MEM,		/* Allocation of memory failed */
	/* Misc */
	E_EOF,			/* Reached end of file while reading */
	E_UNKNOWN,		/* Unknown error */
	NUM_ERRORS,		/* Number of items in enum */
};

enum audio_init_err {
	E_AINIT_OK = 0,
	E_AINIT_OPEN_INPUT,
	E_AINIT_FIND_STREAM_INFO,
	E_AINIT_DEVICE_OPEN_FAIL,
	E_AINIT_NO_STREAM,
	E_AINIT_NO_CODEC,
	E_AINIT_CANNOT_ALLOC_AUDIO,
	E_AINIT_CANNOT_ALLOC_PACKET,
	E_AINIT_CANNOT_ALLOC_FRAME,
	E_AINIT_BAD_RATE,
};

enum audio_play_err {
	E_PLAY_OK = 0,
	E_PLAY_EOF,
	E_PLAY_DECODE_ERR,
};

#endif				/* !ERRORS_H */

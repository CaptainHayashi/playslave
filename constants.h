/*
 * =============================================================================
 *
 *       Filename:  constants.h
 *
 *    Description:  Forward declarations of general constants
 *
 *        Version:  1.0
 *        Created:  24/12/2012 18:52:46
 *       Revision:  none
 *       Compiler:  clang
 *
 *         Author:  Matt Windsor (CaptainHayashi), matt.windsor@ury.org.uk
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

#ifndef CONSTANTS_H
#define CONSTANTS_H

/**  INCLUDES  ****************************************************************/

#include <stdint.h>		/* int64_t */

/**  MACROS  ******************************************************************/

/* HOUSEKEEPING: Only put things in macros if they have to be constant at
 * compile-time (for example, array sizes).
 */

#define WORD_LEN 5		/* Length of command words in bytes plus \0 */

/**  CONSTANTS  ***************************************************************/

/* All of these are defined in constants.c.
 *
 * HOUSEKEEPING: Keeping these grouped in ASCIIbetical order by type first and
 * name second (eg by running them through sort) in both .h and .c would be nice.
 */

const long	LOOP_NSECS;	/* Number of nanoseconds between main loops */
const size_t	BUFFER_SIZE;	/* Number of bytes in decoding buffer */
const size_t    SPINUP_SIZE;    /* Number of bytes to load before playing */
const size_t	RINGBUF_SIZE;	/* Number of samples in ring buffer */
const uint64_t	TIME_USECS;	/* Number of microseconds between TIME pulses */
const uint64_t	USECS_IN_SEC;	/* Number of microseconds in a second */

#endif				/* not CONSTANTS_H */

/*
 * =============================================================================
 *
 *       Filename:  constants.c
 *
 *    Description:  General constants
 *
 *        Version:  1.0
 *        Created:  24/12/2012 18:55:23
 *       Revision:  none
 *       Compiler:  clang
 *
 *         Author:  Matt Windsor (CaptainHayashi), matt.windsor@ury.org.uk
 *        Company:  University Radio York Computing Team
 *
 * =============================================================================
 */

/**  INCLUDES  ****************************************************************/

#include <stdint.h>

#include <libavcodec/avcodec.h>

/**  GLOBAL VARIABLES  ********************************************************/

/* See constants.c for more constants (especially macro-based ones) */
const long	LOOP_NSECS = 1000;
const size_t	BUFFER_SIZE = (size_t)FF_MIN_BUFFER_SIZE;
const size_t    SPINUP_SIZE = 2 * BUFFER_SIZE;
const size_t	RINGBUF_SIZE = (size_t)(1 << 16);
const uint64_t	TIME_USECS = 1000000;
const uint64_t	USECS_IN_SEC = 1000000;

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
 *        Company:  University Radio York
 *
 * =============================================================================
 */

#include <stdint.h>

#include <libavcodec/avcodec.h>

/**  GLOBAL VARIABLES  ********************************************************/

const size_t	BUFFER_SIZE = (size_t)FF_MIN_BUFFER_SIZE;
const size_t	RINGBUF_SIZE = (size_t)(1 << 16);

const uint64_t	MSECS_IN_SEC = 1000000;
const uint64_t	TIME_MSECS = 1000000;

#ifndef AUDIO_H
#define AUDIO_H

#include "player.h"
#include "errors.h"

struct audio;

enum audio_init_err
audio_load(struct audio **au, const char *filename,
	   int device_id);

void		audio_unload(struct audio *au);
enum error	audio_start(struct audio *au);
enum error	audio_stop(struct audio *au);


#endif /* !AUDIO_H */

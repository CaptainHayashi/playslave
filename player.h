#ifndef PLAYER_H
#define PLAYER_H

#include <ao/ao.h>

struct player_context;
enum player_state {
        VOID,
        EJECTED,
        STOPPED,
        PLAYING,
        SHUTTING_DOWN
};

int player_init(struct player_context **play,
                int ao_driver_id,
                ao_option *ao_options);

/* Player commands */
enum error player_eject(struct player_context *play);
enum error player_play(struct player_context *play);
enum error player_stop(struct player_context *play);
enum error player_load(struct player_context *play, const char *filename);
enum error player_shutdown(struct player_context *play);

void player_update(struct player_context *play);

enum player_state player_state(struct player_context *play);
#endif /* !PLAYER_H */

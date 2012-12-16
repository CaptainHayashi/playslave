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

int player_init(struct player_context **context,
                int ao_driver_id,
                ao_option *ao_options);

void player_eject(struct player_context *context);
void player_play(struct player_context *context);
void player_stop(struct player_context *context);
void player_load(struct player_context *context, const char *filename);
void player_update(struct player_context *context);
void player_shutdown(struct player_context *context);

enum player_state player_state(struct player_context *context);
#endif // PLAYER_H

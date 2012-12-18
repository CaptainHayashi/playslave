#ifndef PLAYER_H
#define PLAYER_H

#include <pthread.h>
#include <ao/ao.h>

struct player_context;
enum player_state {
        VOID,
        EJECTED,
        LOADING,
        STOPPED,
        PLAYING,
        SHUTTING_DOWN
};

enum audio_init_err;

int player_init(struct player_context **play,
                int ao_driver_id,
                ao_option *ao_options);
void player_free(struct player_context *play);

void player_on_state_change(struct player_context *play, void (*cb)(struct player_context *));

/* Player commands */
enum error player_eject(struct player_context *play);
enum error player_play(struct player_context *play);
enum error player_stop(struct player_context *play);
enum error player_load(struct player_context *play, const char *filename);
enum error player_shutdown(struct player_context *play);

enum error player_do_load(struct player_context *play);

void player_update(struct player_context *play);

enum player_state player_state(struct player_context *play);
#endif /* !PLAYER_H */

#define _POSIX_C_SOURCE 200809
#include <stdio.h>
#include <libavformat/avformat.h>
#include <ao/ao.h>
#include <stdio.h>
#include <string.h>
#include "io.h"
#include "player.h"

void check_commands(struct player_context *context)
{
        if (input_waiting()) {
                char *buffer = NULL;
                char *argument = NULL;
                size_t num_bytes = 0;
                ssize_t length = getline(&buffer, &num_bytes, stdin); 

                /* Find start of argument(s) */
                ssize_t i;
                for (i = 4; i < length && argument == NULL; i++) {
                        if (!isspace(buffer[i])) {
                                /* Assume this is where the arg is */
                                argument = buffer + i;
                                break;
                        }
                }
                
                /* Strip any whitespace out of the argument
                 * (by setting it to the null character, thus
                 * null-terminating the argument)
                 */
                ssize_t j;
                for (j = length - 1; isspace(buffer[j]); i--) {
                    buffer[j] = '\0';
                }

                if (argument == NULL) {
                        if (strncmp("PLAY", buffer, 4) == 0) {
                                player_play(context);
                        } else if (strncmp("STOP", buffer, 4) == 0) {
                                player_stop(context);
                        } else if (strncmp("EJCT", buffer, 4) == 0) {
                                player_eject(context);
                        } else if (strncmp("QUIT", buffer, 4) == 0) {
                                player_shutdown(context);
                        } else {
                            printf("WHAT Not expecting argument here\n");
                        }
                } else if (strncmp("LOAD", buffer, 4) == 0) {
                        player_load(context, argument);
                } else {
                        printf("WHAT Not a valid command\n");
                }

                free(buffer);
        }
}
 
void main_loop(struct player_context *context)
{
        while (player_state(context) != SHUTTING_DOWN
                        && !feof(stdin) ) {
                check_commands(context);
                player_update(context);
        }
}
 
int main(int argc, char *argv[])
{
        if (argc < 2) {
                error(3, "specify as argument to command");
                exit(3); 
        }

        ao_initialize();
        int driver_id = ao_default_driver_id();
        if (driver_id < 0) {
                error(3, "couldn't get default libao driver");
                exit(3);
        }
        ao_option *options = NULL;
        ao_append_option(&options, "id", argv[1]);

        struct player_context *context = NULL;

        av_register_all();

        if (player_init(&context, driver_id, options) < 0) {
                error(2, "maybe out of memory?");
                exit(1);
        }

        main_loop(context);

        ao_shutdown();

        return 0;
}

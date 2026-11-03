#include "logic_thread.h"
#include <pthread.h>

#include "Render_wrapper.h"

#include <time.h>
#include <stdint.h>
#include <unistd.h>

#include "menu_state.h"
#include "play_defines.h"

extern volatile struct input_s user_inputs;
extern volatile char window_active;

#define LOGIC_LOOP_HZ 120
#define PERIOD_NS (1000000000L / LOGIC_LOOP_HZ)   // 8,333,333 ns

volatile uint game_state;

void logic_thread(void){
    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    static uint prev_state;

    while(window_active){

        // START OF LOGIC
        button_poll();

        if (prev_state != game_state){
            switch (prev_state) { 
                case MENU_STATE:
                    menu_remove();
                    break;

                default:
                    break;
            }

            
            switch (game_state) { 
                case MENU_STATE:
                    menu_init();
                    break;

                default:
                    printf("Game state not recognised - This is critical to game function... defaulting to menu_state\n");
                    menu_init();
                    game_state = MENU_STATE;
                    break;
            }
        }

        switch (game_state) { 
            case MENU_STATE:
                menu_poll();
                break;

            default:
                break;
        }
        

        // END OF LOGIC
        user_inputs.update = 1;

        // Advance the deadline by one period
        next.tv_nsec += PERIOD_NS;
        if(next.tv_nsec >= 1000000000L){
            next.tv_nsec -= 1000000000L;
            next.tv_sec++;
        }
        // Sleep until that moment (returns imassed)
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
        prev_state = game_state;
    }
}


void logic_thread_init(){
    // LOGIC INIT
    game_state = MENU_STATE;
    menu_init();
    logic_thread();
}

#include "first.h"
#include <stdio.h>
#include <SDL/SDL.h>
#include <plib/sg.h>

#include "globals.h"
//#include "rocket.h"
#include "input.h"

unsigned mouse_buttons;

motion_cb_t	motion_callback;
keydown_cb_t	keydown_callback;
keyup_cb_t	keyup_callback;

void	        *motion_callback_data;
void	        *keydown_callback_data;
void		*keyup_callback_data;

SDL_GrabMode	grab_mode = SDL_GRAB_OFF;

char		key_table[SDLK_LAST];

void
reset_keys()
{
    for (int i = 0; i < SDLK_LAST; i++)
	key_table[i] = 0;
}

void
init_input()
{
    reset_keys();
    motion_callback = NULL;
    keydown_callback = NULL;
    keyup_callback = NULL;
}

bool 
mouse_button_down(unsigned mask)
{
    return (mouse_buttons & mask) != 0;
}


// set which function to call on mouse events
void
set_motion_callback(motion_cb_t cb, void *data)
{
    motion_callback = cb;
    motion_callback_data = data;
}

// set which function to call when a key is pressed
void
set_keydown_callback(keydown_cb_t cb, void *data)
{
    keydown_callback = cb;
    keydown_callback_data = data;
}

// set which function to call when a key is released
void
set_keyup_callback(keyup_cb_t cb, void *data)
{
    keyup_callback = cb;
    keyup_callback_data = data;
}



// check if key is pressed down
char
key_down(int key)
{
    if (key < SDLK_LAST)
	return key_table[key];
    else
	return 0;	    
}

void 
update_input( void )
{
    /* Our SDL event placeholder. */
    SDL_Event event;

    /* Grab all the events off the queue. */
    while (SDL_PollEvent(&event)) {

        switch(event.type) {
        case SDL_KEYDOWN:
            key_table[event.key.keysym.sym] = 1;

	    if (key_down(SDLK_F1)) {
		grab_mode = SDL_WM_GrabInput(grab_mode == SDL_GRAB_ON ? 
					     SDL_GRAB_OFF : SDL_GRAB_ON);
		if ( grab_mode == SDL_GRAB_ON ) {
		    printf("Grab is now on\n");
		    SDL_ShowCursor(SDL_DISABLE);
		} else {
		    printf("Grab is now off\n");
		    SDL_ShowCursor(SDL_ENABLE);
		}
	    }

            break;
        case SDL_KEYUP:
            key_table[event.key.keysym.sym] = 0;

	    if (keyup_callback != NULL)
	    {
		keyup_callback(keyup_callback_data, event.key.keysym.sym);
	    }           
            break;

        case SDL_MOUSEBUTTONDOWN:
            switch (event.button.button) {
            case SDL_BUTTON_LEFT:   mouse_buttons |= LEFT_MOUSE_BUTTON; break;
            case SDL_BUTTON_MIDDLE: mouse_buttons |= MIDDLE_MOUSE_BUTTON; break;
            case SDL_BUTTON_RIGHT:  mouse_buttons |= RIGHT_MOUSE_BUTTON; break;
            }
            break;

        case SDL_MOUSEBUTTONUP:
            switch (event.button.button) {
            case SDL_BUTTON_LEFT:   mouse_buttons &= ~LEFT_MOUSE_BUTTON; break;
            case SDL_BUTTON_MIDDLE: mouse_buttons &= ~MIDDLE_MOUSE_BUTTON; break;
            case SDL_BUTTON_RIGHT:  mouse_buttons &= ~RIGHT_MOUSE_BUTTON; break;
            }
            break;



	case SDL_MOUSEMOTION:
	    
	    if (motion_callback != NULL &&
		ABS(event.motion.xrel) < 100 &&
		ABS(event.motion.yrel) < 100) 
	    {
		motion_callback(motion_callback_data,
				event.motion.x, event.motion.y);
	    }
//	    printf("x=%d, y=%d, xrel=%d, yrel=%d\n", 
//		   event.motion.x, event.motion.y,
//		   event.motion.xrel, event.motion.yrel);
	    if (grab_mode == SDL_GRAB_ON) {
		if (event.motion.x < 10 || 
		    event.motion.x > (globals.win_width - 10) ||
		    event.motion.y < 10 ||
		    event.motion.x > (globals.win_width - 10))
		{
		    SDL_WarpMouse(globals.win_width / 2,
				     globals.win_height / 2);
		}
	    }
	    
	    break;	    
        case SDL_QUIT:
	    printf("SDL_QUIT\n");
            break;
        }
    }
}

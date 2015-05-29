
#ifndef INPUT_H
#define INPUT_H

const unsigned LEFT_MOUSE_BUTTON   = 1;
const unsigned MIDDLE_MOUSE_BUTTON = 2;
const unsigned RIGHT_MOUSE_BUTTON  = 4;

typedef void (*motion_cb_t)(void *data, int x, int y);
typedef void (*keydown_cb_t)(void *data, int key);
typedef void (*keyup_cb_t)(void *data, int key);

void init_input();
void reset_keys();
char key_down(int key);
bool mouse_button_down(unsigned mask);
void update_input(void);


void set_motion_callback(motion_cb_t cb, void *data);
void set_keydown_callback(keydown_cb_t cb, void *data);
void set_keyup_callback(keyup_cb_t cb, void *data);

#endif

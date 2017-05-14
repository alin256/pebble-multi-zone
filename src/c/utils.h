#ifndef URILS_H
#define URILS_H


#include <pebble.h>

#ifndef STRING_CONSTS
//string constants

// const char weekdays[16] = "MoTuWeThFrSaSu";
// const char question[1] = "?";

#define WEEKDAYS_STRING "MoTuWeThFrSaSu"
#define QUESTION_STRING "?"

#define STRING_CONSTS
#endif


void layer_set_center(Layer* layer, int x, int y);

#endif
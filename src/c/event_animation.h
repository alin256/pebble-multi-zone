#ifdef ENABLE_EVENT_ANIMATION
#ifndef EVENT_ANIMATION_H
#define EVENT_ANIMATION_H
#include <pebble.h>



struct UnobstructedAreaContext{
  Layer *map_layer;
  Layer *place2;
  int16_t map_init_pos;
  int16_t map_new_pos;
  int16_t place_init_pos;
  int16_t place_new_pos;  
  int16_t old_height;
};

UnobstructedAreaHandlers get_unobstructed_handlers();

#endif
#endif

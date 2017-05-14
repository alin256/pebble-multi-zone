#ifndef PLACE_LAYER_H
#define PLACE_LAYER_H

#include <pebble.h>
#include "settings.h"


#ifdef PBL_PLATFORM_BASALT
//Rounded edges for Basalt
#define SCREEN_RADIUS 8
#else
#define SCREEN_RADIUS 0
#endif

struct place_visualization{
  Layer *place_layer;
  TextLayer *place_name_layer;
  TextLayer *place_time_layer;
  char watch_str[8];
  char place_str[100];
  struct place_descrition *place;
  Settings *settings;
  int radius;
  //GColor color;  
};

typedef struct place_visualization place_layer;

void create_place_layer_default(place_layer *place, 
                                struct place_descrition *description, 
                                int16_t top, 
                                Settings *settings,
                                Layer *parent);
void update_place(struct place_descrition *place_d, Tuple *city_t, Tuple *offset_t, Tuple* x_t, Tuple* y_t);
void destroy_place_layer(place_layer *place);
void update_place_layer(place_layer *place);

#endif
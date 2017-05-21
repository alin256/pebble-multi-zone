#ifndef TODAY_LAYER_H
#define TODAY_LAYER_H


#include <pebble.h>
#include "settings.h"

//overlay over map layer
struct date_layer{
  Layer *date_root_layer;
  Settings *settings;
};


Layer* date_layer_create(GRect frame, struct date_layer *date_l);
void destroy_date_layer(struct date_layer *date_l);
void date_layer_handle_minute_tick(struct date_layer *date_l, struct tm *tick_time, TimeUnits units_changed);
void date_layer_handle_update_settings(struct date_layer *date_l);
void date_layer_handle_night_pos_update(struct date_layer *date_l, 
                                       struct tm *tick_time, 
                                       TimeUnits units_changed);
//void update_time(Layer* today_layer);
//void update_time(Layer* today_layer);

#endif
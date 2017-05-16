#ifndef TODAY_LAYER_H
#define TODAY_LAYER_H


#include <pebble.h>
#include "settings.h"

//overlay over map layer
struct date_layer{
  Layer *date_root_layer;
  Layer *floating_layer;
  BitmapLayer *today_layer;
  BitmapLayer *sun_layer;
  GBitmap *today_pic;
  GBitmap *cur_today_pic;
  //GBitmap *sun_pic;
  Layer *ceparator_layer;
  TextLayer *date_left;
  TextLayer *dow_left;
  TextLayer *date_right;
  TextLayer *dow_right;
  TextLayer *local_time;
  Settings *settings;
};


Layer* date_layer_create(GRect frame, struct date_layer *date_l);
void destroy_date_layer(struct date_layer *date_l);

#endif
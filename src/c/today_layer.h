#ifndef TODAY_LAYER_H
#define TODAY_LAYER_H


#include <pebble.h>

struct date_layer{
//layer with date
  Layer *date_root_layer;
  Layer *floating_layer;
  BitmapLayer *today_layer;
  GBitmap *today_pic;
  Layer *ceparator_layer;
//add weekday
  TextLayer *date_left;
  TextLayer *date_right;
};


Layer* date_layer_create(GRect frame, struct date_layer *date_l);
void destroy_date_layer(struct date_layer *date_l);

#endif
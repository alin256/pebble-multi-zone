#ifndef ARROWS_LAYER_H
#define ARROWS_LAYER_H

#include <pebble.h>
#include "place_layer.h"
#include "settings.h"

typedef GPoint GetPointOnMap(int32_t x, int32_t y);

struct ArrowLayerParams{
  GetPointOnMap* get_point_on_map;
  place_layer* my_place_layer;
  Settings *settings;
  bool direction_down;
};

void draw_arrows(struct Layer *layer, GContext *ctx);

struct Layer* arrows_layer_create(GRect frame,
                                 place_layer* my_place_layer, 
                                 Settings* settings, 
                                 GetPointOnMap* get_point_on_map,
                                 bool direction_down);



#endif
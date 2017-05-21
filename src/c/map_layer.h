#ifndef MAP_LAYER_H
#define MAP_LAYER_H

#include <pebble.h>



struct MapLayer{
  //map things
  BitmapLayer *map_layer;
  GBitmap *three_worlds;
  GBitmap *image;  
  int32_t redraw_counter;
  //BitmapLayer *tmp_layer;
};

GPoint get_point_on_map(int32_t x, int32_t y);
GPoint get_dark_point_map(int time);

Layer* map_leyer_create(GPoint origin, struct MapLayer* map_layer);
void map_layer_destroy(struct MapLayer* map_layer);

//void draw_map(struct Layer *layer, GContext *ctx);
//void draw_earth(GBitmap* three_worlds, struct Layer* map_layer);

bool map_layer_redraw_required_minute(struct MapLayer *map_layer_struct);

void map_layer_handle_night_pos_update(time_t now, struct MapLayer *map_layer_struct);

GRect map_leyer_get_frame(struct MapLayer* map_layer);

void map_leyer_set_frame(struct MapLayer* map_layer, GRect frame);

#endif

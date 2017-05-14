#pragma once
#include <pebble.h>

struct MapLayer{
  //map things
  BitmapLayer *map_layer;
  GBitmap *three_worlds;
  GBitmap *image;
  //BitmapLayer *tmp_layer;
};

GPoint get_point_on_map(int32_t x, int32_t y, GSize bounds);
  
void get_dark_point_map(int time, int32_t* x, int32_t* y);

//void draw_map(struct Layer *layer, GContext *ctx);
void draw_earth(GBitmap* three_worlds, struct Layer* map_layer);

BitmapLayer* map_leyer_create(GPoint origin, struct MapLayer* map_layer);

void map_layer_destroy(struct MapLayer* map_layer);

GRect map_leyer_get_frame(struct MapLayer* map_layer){
  return layer_get_frame(bitmap_layer_get_layer(map_layer->map_layer));
}

void map_leyer_set_frame(struct MapLayer* map_layer, GRect frame){
  return layer_set_frame(bitmap_layer_get_layer(map_layer->map_layer), frame);
}

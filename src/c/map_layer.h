#pragma once
#include <pebble.h>

//const uint16_t 
#define WIDTH 144
//const uint16_t 
#define HEIGHT 72


GPoint get_point_on_map(int32_t x, int32_t y, GSize bounds);
  
void get_dark_point_map(int time, int32_t* x, int32_t* y);

//void draw_map(struct Layer *layer, GContext *ctx);
void draw_earth(GBitmap* three_worlds, struct Layer* map_layer);


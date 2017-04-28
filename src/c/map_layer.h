#pragma once
#include <pebble.h>

//const uint16_t 
#define WIDTH 144
//const uint16_t 
#define HEIGHT 72

struct my_point{
  int32_t x; //0 .. trig max angle
  int32_t y; //-trig max angle/2 .. trig max angle/2
};


//void draw_map(struct Layer *layer, GContext *ctx);
void draw_earth(GBitmap* three_worlds, struct Layer* map_layer);

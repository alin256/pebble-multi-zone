#pragma once
#include <pebble.h>

//const uint16_t 
#define WIDTH 144
//const uint16_t 
#define HEIGHT 72

//void draw_map(struct Layer *layer, GContext *ctx);
void draw_earth(GBitmap* three_worlds, struct Layer* map_layer);

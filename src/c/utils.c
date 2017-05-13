#include <pebble.h>
#include "utils.h"


void layer_set_center(Layer* layer, int x, int y){
  GSize size = layer_get_frame(layer).size;
  int w = size.w;
  int h = size.h;
  GRect new_frame = GRect(x - w/2, y-h/2, w, h);  
  layer_set_frame(layer, new_frame);
}

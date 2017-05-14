#include <pebble.h>
#include "utils.h"


void layer_set_center(Layer* layer, int x, int y){
  GSize size = layer_get_frame(layer).size;
  int w = size.w;
  int h = size.h;
  GRect new_frame = GRect(x - w/2, y-h/2, w, h);  
  layer_set_frame(layer, new_frame);
}

// static void draw_number(GContext *ctx, GPoint middle, int16_t num){
//   GRect box =  GRect(middle.x - radius*3/2, middle.y - radius, 
//                                 radius*3, radius*2);
//   graphics_fill_rect(ctx, box, radius, GCornersAll);
//   graphics_context_set_text_color(ctx, settings.TextColor);
//   char *text = "00";
//   snprintf(text, 2, "%d", (int) num);
//   graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), box, 
//                      GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
// }

#include <pebble.h>
#include "today_layer.h"
#include "map_layer.h"
#include "utils.h"

const int date_width =  WIDTH / 4;
const int date_height =  HEIGHT / 3;



TextLayer* date_text_layer_create_with_font(GRect rect, GFont font, Layer *root){
  TextLayer *cur = text_layer_create(rect);
  text_layer_set_text_color(cur, GColorWhite);
  text_layer_set_font(cur, font);
  text_layer_set_text_alignment(cur, GTextAlignmentCenter);
  text_layer_set_background_color(cur, GColorClear);
  //TODO remove
  text_layer_set_text(cur, "26");
  layer_add_child(root, text_layer_get_layer(cur));
  layer_set_frame(text_layer_get_layer(cur), rect);
  return cur;
}

void ceparator_layer_update(struct Layer *layer, GContext *ctx){
  GSize size = layer_get_bounds(layer).size;
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_antialiased(ctx, false);
  for(int i = 0; i+1<size.h/5; ++i){
    graphics_fill_rect(ctx, GRect(0, i*5, size.w, 3), 0, GCornerNone);
  }
}

Layer* create_date_layer(struct date_layer *date_l){
  date_l->date_root_layer = layer_create(GRect(0, 0, date_width*2+3, HEIGHT));
  
  
  //today
  //image
  date_l->today_pic = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TODAY_7);
  //layer
  //TODO correct rect
  date_l->today_layer = bitmap_layer_create(GRect(0, 0, 7, gbitmap_get_bounds(date_l->today_pic).size.h));
  bitmap_layer_set_bitmap(date_l->today_layer, date_l->today_pic);
  bitmap_layer_set_background_color(date_l->today_layer, GColorClear);
  bitmap_layer_set_compositing_mode(date_l->today_layer, GCompOpSet);
  layer_set_center(bitmap_layer_get_layer(date_l->today_layer), date_width/2, HEIGHT/2);
  layer_add_child(date_l->date_root_layer, bitmap_layer_get_layer(date_l->today_layer));
  
  //left
  date_l->date_left = date_text_layer_create_with_font(
    GRect(0, HEIGHT/2, date_width, date_height), fonts_get_system_font(FONT_KEY_GOTHIC_14),date_l->date_root_layer);
  text_layer_set_text_alignment(date_l->date_left, GTextAlignmentRight);
  
  //right
  date_l->date_right = date_text_layer_create_with_font(
    GRect(date_width+3, HEIGHT/2, date_width, date_height), fonts_get_system_font(FONT_KEY_GOTHIC_14), date_l->date_root_layer);
  text_layer_set_text_alignment(date_l->date_right, GTextAlignmentLeft);
  
  //ceparator
  date_l->ceparator_layer = layer_create(GRect(0, 0, 1, HEIGHT));
  layer_set_update_proc(date_l->ceparator_layer, ceparator_layer_update);
  layer_add_child(date_l->date_root_layer, date_l->ceparator_layer);
  layer_set_frame(date_l->ceparator_layer, GRect(date_width+1, 0, 1, HEIGHT));
  
  //layer_add_child(date_l->date_root_layer, text_layer_get_layer(date_l->date_left));
  //layer_add_child(date_l->date_root_layer, text_layer_get_layer(date_l->date_left));  
  return date_l->date_root_layer;
}

void destroy_date_layer(struct date_layer *date_l){
  bitmap_layer_destroy(date_l->today_layer);
  gbitmap_destroy(date_l->today_pic);
  text_layer_destroy(date_l->date_left);
  text_layer_destroy(date_l->date_right);
  
  layer_destroy(date_l->ceparator_layer);
  
  layer_destroy(date_l->date_root_layer);
}

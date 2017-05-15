#include <pebble.h>
#include "today_layer.h"
#include "map_layer.h"
#include "utils.h"
#include "map_layer.h"

//TODO remove defines from here
//const uint16_t 
#ifndef WIDTH
#define WIDTH 144
#endif
//const uint16_t 
#ifndef HEIGHT
#define HEIGHT 72
#endif

#define date_width 41
#define date_height 49

#define DATE_TEXT_WIDTH 20
#define DATE_TEXT_HEIGHT 20

TextLayer* date_text_layer_create_with_font(GRect rect, GFont font, Layer *root){
  TextLayer *cur = text_layer_create(rect);
  text_layer_set_text_color(cur, GColorWhite);
  text_layer_set_font(cur, font);
  text_layer_set_text_alignment(cur, GTextAlignmentCenter);
  text_layer_set_background_color(cur, GColorClear);
  //TODO remove
  text_layer_set_text(cur, "26");
  //layer_add_child(root, text_layer_get_layer(cur));
  layer_set_frame(text_layer_get_layer(cur), rect);
  return cur;
}

struct RootLayerData{
  Layer *floating_layer;
};

void update_location(struct Layer *layer){
  layer_set_center(layer, get_dark_point_map(time(NULL)));
}

void move_floating_layer(struct Layer* root_layer, GContext *ctx){
  struct RootLayerData* my_data = layer_get_data(root_layer);
  update_location(my_data->floating_layer);
}

void draw_cross(struct Layer *layer, GContext *ctx){
  GSize size = layer_get_bounds(layer).size;
  graphics_context_set_stroke_color(ctx, GColorRed);
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_context_set_antialiased(ctx, false);
  graphics_draw_line(ctx, GPoint(0, 0), GPoint(size.w, size.h));
  graphics_draw_line(ctx, GPoint(0, size.h), GPoint(size.w, 0)); 
  for(int i = 0; i<=size.w; i+=2){
    graphics_fill_rect(ctx, GRect(i, 0, 1, size.h), 0, GCornerNone);
  }  
}


void ceparator_layer_update(struct Layer *layer, GContext *ctx){
  GSize size = layer_get_bounds(layer).size;
  graphics_context_set_stroke_color(ctx, GColorMagenta);
  graphics_context_set_fill_color(ctx, GColorMagenta);
  graphics_context_set_antialiased(ctx, false);
  for(int i = 0; i+1<size.h/5; ++i){
    graphics_fill_rect(ctx, GRect(0, i*5, size.w, 3), 0, GCornerNone);
  }
}


Layer* date_layer_create(GRect frame, struct date_layer *date_l){
  //floating layer
  date_l->floating_layer = layer_create(GRect(0, 0, date_width, date_height));
  layer_set_update_proc(date_l->floating_layer, draw_cross);
  //root_layer
  date_l->date_root_layer = layer_create_with_data(frame, sizeof(struct RootLayerData));
  struct RootLayerData* root_data = layer_get_data(date_l->date_root_layer);
  root_data->floating_layer = date_l->floating_layer; 
  layer_add_child(date_l->date_root_layer, date_l->floating_layer);
  layer_set_update_proc(date_l->date_root_layer, move_floating_layer);
  
  //update_location(date_l->floating_layer);
  //layer_set_center(date_l->date_root_layer, );
  //date_l->date_root_layer = layer_create(GRect(0, 0, date_width*2+3, HEIGHT));
  //today
  //image
  date_l->today_pic = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TODAY_7);
  //layer
  //TODO correct rect
  date_l->today_layer = bitmap_layer_create(GRect(0, 0, 7, gbitmap_get_bounds(date_l->today_pic).size.h));
  bitmap_layer_set_bitmap(date_l->today_layer, date_l->today_pic);
  bitmap_layer_set_background_color(date_l->today_layer, GColorClear);
  bitmap_layer_set_compositing_mode(date_l->today_layer, GCompOpSet);
  //layer_set_center(bitmap_layer_get_layer(date_l->today_layer), date_width/2, HEIGHT/2);
  //layer_add_child(date_l->date_root_layer, bitmap_layer_get_layer(date_l->today_layer));
  
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
  layer_add_child(date_l->floating_layer, date_l->ceparator_layer);
  layer_set_frame(date_l->ceparator_layer, GRect((date_width+1)/2, 0, 1, HEIGHT));
  //layer_set_frame(date_l->ceparator_layer, GRect(date_width+1, 0, 1, HEIGHT));
  
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

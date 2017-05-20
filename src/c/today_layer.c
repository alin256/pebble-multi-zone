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

#define LOCAL_TIME_HIGHT 20
#define LOCAL_TIME_WIDTH 5*12

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
  struct date_layer *today_root_layer;
  char dow1[5];
  char dow2[5];
  char month1[5];
  char month2[5];
  char date1[5];
  char date2[5];
  char time[10];
  int old_pos;
  
  //bitmap pointers
  GBitmap *today_pic;
  GBitmap *cur_today_pic;
  //GBitmap *sun_pic;
  
  //Layer pointers
  Layer *floating_layer;
  BitmapLayer *today_bitmap_layer;
  BitmapLayer *sun_layer;
  Layer *ceparator_layer;
  TextLayer *date_left;
  //TextLayer *dow_left;
  TextLayer *date_right;
  //TextLayer *dow_right;
  TextLayer *local_time;
  TextLayer *local_time_root;
};

void date_layer_handle_minute_tick(struct date_layer *date_l, 
                                   struct tm *tick_time, 
                                   TimeUnits units_changed){
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Updateing current time");
  struct RootLayerData *data = layer_get_data(date_l->date_root_layer);
  strftime(data->time, sizeof(data->time), 
           clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  layer_mark_dirty(text_layer_get_layer(data->local_time));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done Updateing current time");
}

void date_layer_handle_update_settings(struct date_layer *date_l){
  struct RootLayerData *data = layer_get_data(date_l->date_root_layer);
  text_layer_set_text_color(data->local_time, date_l->settings->HighlightColor);
  //text_layer_set_text_color(data->local_time, GColorWhite);
  layer_mark_dirty(date_l->date_root_layer);
}


void update_location(struct Layer *layer){
  layer_set_center(layer, get_dark_point_map(time(NULL)));
}

void update_overlay_layer(struct Layer* root_layer, GContext *ctx){
  struct RootLayerData *data = layer_get_data(root_layer);
  update_location(data->floating_layer);
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

TextLayer* local_time_create_with_color(struct RootLayerData* data, GColor color){
  TextLayer* tl = text_layer_create(GRect(0, 0, LOCAL_TIME_WIDTH, LOCAL_TIME_HIGHT));
  text_layer_set_font(tl, fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS));
  text_layer_set_text(tl, data->time);
  text_layer_set_text_color(tl, color);
  text_layer_set_text_alignment(tl, GTextAlignmentCenter);
  text_layer_set_background_color(tl, GColorClear);
  return tl;
}

Layer* date_layer_create(GRect frame, struct date_layer *date_l){
  //root_layer
  date_l->date_root_layer = layer_create_with_data(frame, sizeof(struct RootLayerData));
  layer_set_update_proc(date_l->date_root_layer, update_overlay_layer);
  
  //get the internal data structure
  struct RootLayerData* data = layer_get_data(date_l->date_root_layer);  
  
  //back refrence
  data->today_root_layer = date_l; 
  
  //floating layer
  data->floating_layer = layer_create(GRect(0, 0, date_width, date_height));
  layer_set_update_proc(data->floating_layer, draw_cross);
  layer_add_child(date_l->date_root_layer, data->floating_layer);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Root layer added");
  
  //today
  //image
  data->today_pic = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TODAY_7);
  //layer
  data->today_bitmap_layer = bitmap_layer_create(GRect(0, 0, 
                                                  7, gbitmap_get_bounds(data->today_pic).size.h));
  bitmap_layer_set_bitmap(data->today_bitmap_layer, data->today_pic);
  bitmap_layer_set_background_color(data->today_bitmap_layer, GColorClear);
  bitmap_layer_set_compositing_mode(data->today_bitmap_layer, GCompOpSet);
  //layer_add_child(date_l->floating_layer, bitmap_layer_get_layer(date_l->today_layer));
  //TODO check set center
  //layer_set_center(bitmap_layer_get_layer(date_l->today_layer), GPoint((date_width+1)/2+1, (date_height+1)/2));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Today bitmap layer created");

  
  //left
  data->date_left = date_text_layer_create_with_font(
    GRect(0, HEIGHT/2, date_width, date_height), fonts_get_system_font(FONT_KEY_GOTHIC_14),date_l->date_root_layer);
  text_layer_set_text_alignment(data->date_left, GTextAlignmentRight);
  
  //right
  data->date_right = date_text_layer_create_with_font(
    GRect(date_width+3, HEIGHT/2, date_width, date_height), fonts_get_system_font(FONT_KEY_GOTHIC_14), date_l->date_root_layer);
  text_layer_set_text_alignment(data->date_right, GTextAlignmentLeft);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Date texts created");

  //ceparator
  data->ceparator_layer = layer_create(GRect(0, 0, 1, HEIGHT));
  layer_set_update_proc(data->ceparator_layer, ceparator_layer_update);
  layer_add_child(data->floating_layer, data->ceparator_layer);
  layer_set_frame(data->ceparator_layer, GRect((date_width+1)/2, 0, 1, HEIGHT));
  //layer_set_frame(date_l->ceparator_layer, GRect(date_width+1, 0, 1, HEIGHT));
  //layer_add_child(date_l->date_root_layer, text_layer_get_layer(date_l->date_left));
  //layer_add_child(date_l->date_root_layer, text_layer_get_layer(date_l->date_left));  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Cepartor layer added completely");
  
  //current time
  data->time[0] = '\0'; 
  data->local_time_root = local_time_create_with_color(data, GColorBlack);
  layer_add_child(date_l->date_root_layer, text_layer_get_layer(data->local_time_root));
  //data->local_time = local_time_create_with_color(data, GColorWhite);
  data->local_time = local_time_create_with_color(data, date_l->settings->HighlightColor);
  layer_add_child(text_layer_get_layer(data->local_time_root), text_layer_get_layer(data->local_time));
  layer_set_frame(text_layer_get_layer(data->local_time), GRect(-1, -1, LOCAL_TIME_WIDTH, LOCAL_TIME_HIGHT));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Local time layer added completely");
  
  return date_l->date_root_layer;
}

// data to destroy
//   //bitmap pointers
//   GBitmap *today_pic;
//   GBitmap *cur_today_pic;
//   //GBitmap *sun_pic;
  
//   //Layer pointers
//   Layer *floating_layer;
//   BitmapLayer *today_bitmap_layer;
//   BitmapLayer *sun_layer;
//   Layer *ceparator_layer;
//   TextLayer *date_left;
//   //TextLayer *dow_left;
//   TextLayer *date_right;
//   //TextLayer *dow_right;
//   TextLayer *local_time;
// };


void destroy_date_layer(struct date_layer *date_l){
  //get the internal data structure
  struct RootLayerData* data = layer_get_data(date_l->date_root_layer);  
  
  gbitmap_destroy(data->today_pic);
  gbitmap_destroy(data->cur_today_pic);
  
  text_layer_destroy(data->date_left);
  text_layer_destroy(data->date_right);
  
  text_layer_destroy(data->local_time); 
  text_layer_destroy(data->local_time_root);
  
  layer_destroy(data->ceparator_layer);
  
  bitmap_layer_destroy(data->today_bitmap_layer);
  bitmap_layer_destroy(data->sun_layer);
  
  layer_destroy(data->floating_layer);
  
  layer_destroy(date_l->date_root_layer);
}

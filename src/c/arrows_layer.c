#include <pebble.h>
#include "arrows_layer.h"
#include "map_layer.h"
#include "settings.h"

int16_t get_x_within_bounds(int16_t old_x, GRect bounds, int16_t radius){
  int16_t x = old_x;
  if (x<=bounds.origin.x+radius*2){
    x = bounds.origin.x+radius*2;
  }
  if (x+radius>=bounds.origin.x+bounds.size.w-radius*2){
    x = bounds.origin.x+bounds.size.w-radius*3;
  }
  return x;
}


void graphics_draw_lines(GContext *gtx, GPoint start, GPoint end, int16_t end_x_range){
  for (int16_t i = 0 ;i<=end_x_range; i+=2){
    graphics_draw_line(gtx, start, GPoint(end.x + i, end.y));
  }
}


void draw_arrows(struct Layer *layer, GContext *ctx){
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Drawing an arrow");
  GRect bounds = layer_get_bounds(layer);
  struct ArrowLayerParams *params = layer_get_data(layer);
  graphics_context_set_antialiased(ctx, true);
  //Settings *settings = layer_get_data(layer);
  graphics_context_set_stroke_color(ctx, params->settings->ForegroundColor);
  
  //set up coords
  GPoint p1_m = get_point_on_map(params->my_place_layer->place->x, 
                                 params->my_place_layer->place->y);
  //GPoint p2_m = get_point_on_map(place2.place->x, place2.place->y, bounds.size);

  int16_t y1_con = 0;
  //int16_t y2_con = bounds.size.h;
  if (params->direction_down){
    //y2_con = 0;
    y1_con = bounds.size.h;
  }
  
  int16_t x1_con = get_x_within_bounds(p1_m.x, 
                                       layer_get_frame(params->my_place_layer->place_layer),
                                      params->my_place_layer->radius);
  //int16_t x2_con = get_x_within_bounds(p2_m.x, layer_get_frame(place2.place_layer));

  //draw lines 1
  graphics_draw_lines(ctx, p1_m, GPoint(x1_con, y1_con), params->my_place_layer->radius);
  //draw lines 2
  //graphics_draw_lines(ctx, p2_m, GPoint(x2_con, y2_con), radius);
  
  //TODO consider drawing multiple bubbles
  //draw time
  //draw_number(ctx, GPoint(layer_get_frame(current.place_layer).origin.x, HEIGHT-radius*2), 12); 
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done drawing an arrow");
}

struct Layer* arrows_layer_create(GRect frame,
                                       place_layer* my_place_layer, 
                                       Settings* settings, 
                                       GetPointOnMap* get_point_on_map,
                                 bool direction_down){
  struct Layer *arrow_layer = layer_create_with_data(frame, sizeof(struct ArrowLayerParams));
  struct ArrowLayerParams* params = layer_get_data(arrow_layer);
  params->get_point_on_map = get_point_on_map;
  params->my_place_layer = my_place_layer;
  params->settings = settings;
  params->direction_down = direction_down;
  layer_set_update_proc(arrow_layer, draw_arrows);
  return arrow_layer;
}

#include <pebble.h>
#include "map_layer.h"


//const uint16_t 
#define WIDTH 144
//const uint16_t 
#define HEIGHT 72
//
#define REDRAW_INTERVAL_MINUTES 15

//#define MAP_DRAW_CEPARATOR


struct my_point{
  int32_t x; //0 .. trig max angle
  int32_t y; //-trig max angle/2 .. trig max angle/2
};


GRect map_leyer_get_frame(struct MapLayer* map_layer){
  return layer_get_frame(bitmap_layer_get_layer(map_layer->map_layer));
}

void map_leyer_set_frame(struct MapLayer* map_layer, GRect frame){
  layer_set_frame(bitmap_layer_get_layer(map_layer->map_layer), frame);
}

struct my_point get_sun_point(int time){
  float day_of_year; // value from 0 to 1 of progress through a year
  float time_of_day; // value from 0 to 1 of progress through a day
  // approx number of leap years since epoch
  // = now / SECONDS_IN_YEAR * .24; (.24 = average rate of leap years)
  int leap_years = (int)((float)time / 131487192.0);
  // day_of_year is an estimate, but should be correct to within one day
  day_of_year = time - (((int)((float)time / 31556926.0) * 365 + leap_years) * 86400);
  day_of_year = day_of_year / 86400.0;
  time_of_day = day_of_year - (int)day_of_year;
  day_of_year = day_of_year / 365.0;
  struct my_point sun_point;
  // ##### calculate the position of the sun
  // left to right of world goes from 0 to 65536
  sun_point.x = (int)((float)TRIG_MAX_ANGLE * (1.0 - time_of_day));
  // bottom to top of world goes from -32768 to 32768
  // 0.2164 is march 20, the 79th day of the year, the march equinox
  // Earth's inclination is 23.4 degrees, so sun should vary 23.4/90=.26 up and down
  sun_point.y = -sin_lookup((day_of_year - 0.2164) * TRIG_MAX_ANGLE) * .26 * .25;
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sun point: %d, %d", sun_point.x, sun_point.y);
  return sun_point;
}

struct my_point get_dark_point(int time){
  struct my_point dark_point = get_sun_point(time);
  dark_point.x = (dark_point.x + TRIG_MAX_ANGLE/2) % TRIG_MAX_ANGLE;
  dark_point.y = - dark_point.y;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Dark point: %d, %d", dark_point.x, dark_point.y);
  return dark_point;
}

// void convert_to_map(struct my_point p, int32_t* x, int32_t* y){

// }

GPoint get_point_on_map(int32_t x, int32_t y){
  //GSize bounds = GSize(WIDTH, HEIGHT);
  GPoint dark;
  dark.x = (x*WIDTH+TRIG_MAX_ANGLE/2)/TRIG_MAX_ANGLE;
  dark.y = ((TRIG_MAX_ANGLE/2+y)*HEIGHT+TRIG_MAX_ANGLE/2)/TRIG_MAX_ANGLE;
  //dark.x = x*WIDTH/TRIG_MAX_ANGLE;
  //dark.y = y*HEIGHT*2/TRIG_MAX_ANGLE; //up to pi
  APP_LOG(APP_LOG_LEVEL_DEBUG, "coordinates on map: %d %d", dark.x, dark.y);
  return dark;
}


GPoint get_dark_point_map(int time){
  struct my_point p = get_dark_point(time);
  //   GPoint dark;
  //   dark.x = p.x*WIDTH/TRIG_MAX_ANGLE;
  //   dark.y = (TRIG_MAX_ANGLE/2+p.y)*HEIGHT/TRIG_MAX_ANGLE;
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "coordinates on map: %d %d", dark.x, dark.y);
  return get_point_on_map(p.x, p.y);
}

//piece of code modified from:
//github: davidfg4/pebble-day-night
void draw_earth(time_t now, GBitmap* three_worlds, Layer* map_layer) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Appdating map image");
  // ##### calculate the time
  //int now = (int)time(NULL);
  //get sun
  struct my_point sun_point = get_sun_point(now);
  int sun_x = sun_point.x;
  int sun_y = sun_point.y;
  //get dark
  #ifdef MAP_DRAW_CEPARATOR
    struct my_point dark_point = get_dark_point(now);
    int32_t dark_x_map = dark_point.x * WIDTH / TRIG_MAX_ANGLE;
  #endif
  // draw the bitmap
  int x, y;
  for(x = 0; x < WIDTH; x++) {
    int x_angle = (int)((float)TRIG_MAX_ANGLE * (float)x / (float)(WIDTH));
    for(y = 0; y < HEIGHT; y++) {
      //divider between day and night
      #ifdef MAP_DRAW_CEPARATOR
            if (dark_x_map == x && y%5 > 1){
              #ifdef PBL_BW
              #else
              int byte_tmp = y * gbitmap_get_bytes_per_row(three_worlds) + x;
              ((int8_t *)gbitmap_get_data(three_worlds))[byte_tmp] = -1;
              continue;
              #endif
            }
      #endif
      int y_angle = (int)((float)TRIG_MAX_ANGLE * (float)y / (float)(HEIGHT * 2)) - TRIG_MAX_ANGLE/4;
      // spherical law of cosines
      float angle = ((float)sin_lookup(sun_y)/(float)TRIG_MAX_RATIO) * ((float)sin_lookup(y_angle)/(float)TRIG_MAX_RATIO);
      angle = angle + ((float)cos_lookup(sun_y)/(float)TRIG_MAX_RATIO) * ((float)cos_lookup(y_angle)/(float)TRIG_MAX_RATIO) * ((float)cos_lookup(sun_x - x_angle)/(float)TRIG_MAX_RATIO);
#ifdef PBL_BW
      //this would not work right now
      int byte = y * gbitmap_get_bytes_per_row(image) + (int)(x / 8);
      if ((angle < 0) ^ (0x1 & (((char *)gbitmap_get_data(world_bitmap))[byte] >> (x % 8)))) {
        // white pixel
        ((char *)gbitmap_get_data(image))[byte] = ((char *)gbitmap_get_data(image))[byte] | (0x1 << (x % 8));
      } else {
        // black pixel
        ((char *)gbitmap_get_data(image))[byte] = ((char *)gbitmap_get_data(image))[byte] & ~(0x1 << (x % 8));
      }
#else
      int byte = y * gbitmap_get_bytes_per_row(three_worlds) + x;
      if (angle < 0) { // dark pixel
        ((char *)gbitmap_get_data(three_worlds))[byte] = ((char *)gbitmap_get_data(three_worlds))[WIDTH*HEIGHT + byte];
      } else { // light pixel
        ((char *)gbitmap_get_data(three_worlds))[byte] = ((char *)gbitmap_get_data(three_worlds))[WIDTH*HEIGHT*2 + byte];
      }
#endif
    }
  }
  layer_mark_dirty(map_layer);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done updating map image");
}


void map_layer_handle_night_pos_update(time_t now, struct MapLayer *map_layer_struct){
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Drawing map on layer");
  draw_earth(now, map_layer_struct->three_worlds, bitmap_layer_get_layer(map_layer_struct->map_layer));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done drawing map on layer");
}

bool map_layer_redraw_required_minute(struct MapLayer *map_layer_struct){
  map_layer_struct->redraw_counter++;
  if (map_layer_struct->redraw_counter >= REDRAW_INTERVAL_MINUTES) {
    map_layer_struct->redraw_counter = 0;
    return true;
  }else{
    return false;
  }
}

Layer* map_leyer_create(GPoint origin, struct MapLayer* map_layer){
  // Load the image
  map_layer->three_worlds = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_3_WORLDS);
  map_layer->image = gbitmap_create_as_sub_bitmap(map_layer->three_worlds, GRect(0, 0, WIDTH, HEIGHT));
  
  GRect map_bounds = GRect(origin.x, origin.y, WIDTH, HEIGHT);
  map_layer->map_layer = bitmap_layer_create(map_bounds);
  bitmap_layer_set_bitmap(map_layer->map_layer, map_layer->image);
  time_t now = time(NULL);
  draw_earth(now, map_layer->three_worlds, bitmap_layer_get_layer(map_layer->map_layer));
  map_layer->redraw_counter = 0; //large number
  
  //(map_layer, draw_map);

  return bitmap_layer_get_layer(map_layer->map_layer);
  //draw_earth();
  
  //tmp_layer = bitmap_layer_create(GRect(0,0, map_bounds.size.w, map_bounds.size.h));
  //bitmap_layer_set_bitmap(tmp_layer, three_worlds);
  //layer_add_child(map_layer, bitmap_layer_get_layer(tmp_layer));
}

void map_layer_destroy(struct MapLayer* map_layer){
  bitmap_layer_destroy(map_layer->map_layer);
  gbitmap_destroy(map_layer->three_worlds);
  gbitmap_destroy(map_layer->image);
}







//void draw_map(struct Layer *layer, GContext *ctx) {
//  graphics_draw_bitmap_in_rect(ctx, image, gbitmap_get_bounds(image));
//}
//end of borrowed code

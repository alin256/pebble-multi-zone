#include <pebble.h>
#include "map_layer.h"


//const uint16_t 
#define WIDTH 144
//const uint16_t 
#define HEIGHT 72

static int redraw_counter = 500;

struct my_point{
  int32_t x; //0 .. trig max angle
  int32_t y; //-trig max angle/2 .. trig max angle/2
};

BitmapLayer* map_leyer_create(GPoint origin, struct MapLayer* map_layer){
  // Load the image
  map_layer->three_worlds = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_3_WORLDS);
  map_layer->image = gbitmap_create_as_sub_bitmap(map_layer->three_worlds, GRect(0, 0, WIDTH, HEIGHT));
  
  GRect map_bounds = GRect(origin.x, origin.y, WIDTH, HEIGHT);
  map_layer->map_layer = bitmap_layer_create(map_bounds);
  bitmap_layer_set_bitmap(map_layer->map_layer, map_layer->image);
  //(map_layer, draw_map);

  return map_layer->map_layer;
  //draw_earth();
  
  //tmp_layer = bitmap_layer_create(GRect(0,0, map_bounds.size.w, map_bounds.size.h));
  //bitmap_layer_set_bitmap(tmp_layer, three_worlds);
  //layer_add_child(map_layer, bitmap_layer_get_layer(tmp_layer));
}

void map_layer_destroy(struct MapLayer* map_layer){
  bitmap_layer_destroy(map_layer->map_layer);
  gbitmap_destroy(map_layer->three_worlds);
}

GPoint get_point_on_map(int32_t x, int32_t y, GSize bounds){
  int16_t x_m = x*bounds.w/TRIG_MAX_ANGLE;
  int16_t y_m = y*bounds.h*2/TRIG_MAX_ANGLE; //up to pi
  return GPoint(x_m, y_m);
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
  
  return sun_point;
}

struct my_point get_dark_point(int time){
  struct my_point dark_point = get_sun_point(time);
  dark_point.x = (dark_point.x + TRIG_MAX_ANGLE/2) % TRIG_MAX_ANGLE;
  dark_point.y = - dark_point.y;
  return dark_point;
}

// void convert_to_map(struct my_point p, int32_t* x, int32_t* y){

// }

void get_dark_point_map(int time, int32_t* x, int32_t* y){
  struct my_point p = get_dark_point(time);
  *x = p.x*WIDTH/TRIG_MAX_ANGLE;
  *y = (TRIG_MAX_ANGLE/2+p.y)*HEIGHT/TRIG_MAX_ANGLE;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "coordinates on map: %d %d", *x, *y);
}


//piece of code modified from:
//davidfg4/pebble-day-night
void draw_earth(GBitmap* three_worlds, struct Layer* map_layer) {
  // ##### calculate the time
#ifdef PBL_SDK_2
  int now = (int)time(NULL) + time_offset;
#else
  int now = (int)time(NULL);
#endif
  //get sun
  struct my_point sun_point = get_sun_point(now);
  int sun_x = sun_point.x;
  int sun_y = sun_point.y;
  //get dark
  struct my_point dark_point = get_dark_point(now);
  int32_t dark_x_map = dark_point.x * WIDTH / TRIG_MAX_ANGLE;
  // ##### draw the bitmap
  int x, y;
  for(x = 0; x < WIDTH; x++) {
    int x_angle = (int)((float)TRIG_MAX_ANGLE * (float)x / (float)(WIDTH));
    for(y = 0; y < HEIGHT; y++) {
      //divider between day and night
      if (dark_x_map == x && y%5 > 1){
        #ifdef PBL_BW
        #else
        int byte_tmp = y * gbitmap_get_bytes_per_row(three_worlds) + x;
        ((int8_t *)gbitmap_get_data(three_worlds))[byte_tmp] = -1;
        continue;
        #endif
      }
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
}

//void draw_map(struct Layer *layer, GContext *ctx) {
//  graphics_draw_bitmap_in_rect(ctx, image, gbitmap_get_bounds(image));
//}
//end of borrowed code

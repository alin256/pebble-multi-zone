#pragma once
#include <pebble.h>

struct place_descrition{
  int32_t offset;
  int32_t x, y;
  char place_name[80];
};

void update_place_partial(struct place_descrition *place_d, Tuple* x_t, Tuple* y_t);

void update_place(struct place_descrition *place_d, Tuple *city_t, Tuple *offset_t, Tuple* x_t, Tuple* y_t){
  if (!(city_t && offset_t && x_t && y_t))
    return;
  //TODO update only on substansial cahnges; make ifs
  update_place_partial(place_d, x_t, y_t);
  place_d->offset = offset_t->value->int32;
  strncpy(place_d->place_name, city_t->value->cstring, sizeof(place_d->place_name));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Place %s changed to: x: %d, y: %d", place_d->place_name, place_d->x, place_d->y); 
}
#ifndef PLACE_DESCRIPTION_H
#define PLACE_DESCRIPTION_H

#include <pebble.h>

struct place_descrition{
  int32_t offset;
  int32_t x, y;
  char place_name[80];
};

void update_place_partial(struct place_descrition *place_d, Tuple* x_t, Tuple* y_t);

void update_place(struct place_descrition *place_d, Tuple *city_t, Tuple *offset_t, Tuple* x_t, Tuple* y_t);

#endif

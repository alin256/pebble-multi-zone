#include <pebble.h>
#include "my_types.h"


void update_place_partial(struct place_descrition *place_d, Tuple* x_t, Tuple* y_t){
  if (!(x_t && y_t))
    return;
  //TODO update only on substansial cahnges; make ifs
  place_d->x = x_t->value->int32;
  place_d->y = y_t->value->int32;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Place %s changed to: x: %d, y: %d", place_d->place_name, place_d->x, place_d->y); 
}

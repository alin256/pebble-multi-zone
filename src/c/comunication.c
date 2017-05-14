#include <pebble.h>
#include "comunication.h"

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {  
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
}


// // Write message to buffer & send
// static void send_position_request(void){
//   if (settings.show_local_time){
//     DictionaryIterator *iter;
  
//     app_message_outbox_begin(&iter);
//     dict_write_int16(iter, MESSAGE_KEY_Request, 1);
//     dict_write_end(iter);
//     app_message_outbox_send();
//   }
// }

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
  Tuple *reason_t = dict_find(received, MESSAGE_KEY_UpdateReason);
  
  Tuple *show_local_t = dict_find(received, MESSAGE_KEY_ShowLocalTime);
  if (show_local_t){
    settings.show_local_time = show_local_t->value->int16;
  }
  
  {
    Tuple *back_color_t = dict_find(received, MESSAGE_KEY_BackgroundColor);
    if (back_color_t){
      settings.BackgroundColor = GColorFromHEX(back_color_t->value->int32);
    }
    else{
      settings.BackgroundColor = GColorBlack;
    }
  }
  
  {
    Tuple *front_color_t = dict_find(received, MESSAGE_KEY_ForegroundColor);
    if (front_color_t){
      settings.ForegroundColor = GColorFromHEX(front_color_t->value->int32);
    }
    else{
      settings.ForegroundColor = GColorOrange;
    }
  }

  {
    Tuple *text_color_t = dict_find(received, MESSAGE_KEY_TextColor);
    if (text_color_t){
      settings.TextColor = GColorFromHEX(text_color_t->value->int32);
    }
    else{
      settings.TextColor = GColorWhite;
    }
    text_layer_set_text_color(place1.place_time_layer, settings.TextColor);
    text_layer_set_text_color(place1.place_name_layer, settings.TextColor);

    text_layer_set_text_color(place2.place_time_layer, settings.TextColor);
    text_layer_set_text_color(place2.place_name_layer, settings.TextColor);

    text_layer_set_text_color(current.place_time_layer, settings.TextColor);
    text_layer_set_text_color(current.place_name_layer, settings.TextColor);

  }
  
  if (reason_t){
    uint32_t reason_id = reason_t->value->uint32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Status: %d", (int) reason_id);
    {
      Tuple *city_t;
      Tuple *offset_t;
      Tuple *x_t;
      Tuple *y_t;

      //updated place 1
      city_t = dict_find(received, MESSAGE_KEY_Place1);
      offset_t = dict_find(received, MESSAGE_KEY_ZoneOffset1);
      x_t = dict_find(received, MESSAGE_KEY_P1X);
      y_t = dict_find(received, MESSAGE_KEY_P1Y);
      update_place(&settings.place1, city_t, offset_t, x_t, y_t);
      update_place_layer(&place1);

      //updated place 2
      city_t = dict_find(received, MESSAGE_KEY_Place2);
      offset_t = dict_find(received, MESSAGE_KEY_ZoneOffset2);
      x_t = dict_find(received, MESSAGE_KEY_P2X);
      y_t = dict_find(received, MESSAGE_KEY_P2Y);
      update_place(&settings.place2, city_t, offset_t, x_t, y_t);
      update_place_layer(&place2);

      switch_panels_if_required();
      layer_mark_dirty(window_get_root_layer(s_window));
      
      if (reason_id == 42){//GPS
        //updated place Curent
        //city_t = dict_find(received, MESSAGE_KEY_CurPlace);
        //offset_t = dict_find(received, MESSAGE_KEY_ZoneOffsetCur);
        x_t = dict_find(received, MESSAGE_KEY_P_CUR_X);
        y_t = dict_find(received, MESSAGE_KEY_P_CUR_Y);
        //update_place(&settings.place_cur, &settings.place_cur.place_name, offset_t, x_t, y_t);
        update_place_partial(&settings.place_cur, x_t, y_t);
        update_floating_place(&current);
        settings.last_update = time(NULL);
        
      }
      else if  (reason_id == 43){
        //position_known = false;         
      }
    }
    prv_save_settings(&settings);
  }
  
  //send_message();
}

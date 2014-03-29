#include <pebble.h>

// persistence keys
#define STROKE_SELECTED 0
#define CURRENT_STROKE 1
#define BACKSTROKE_LENGTH_COUNT 2
#define BREASTSTROKE_LENGTH_COUNT 3
#define BUTTERFLY_LENGTH_COUNT 4
#define FREESTYLE_LENGTH_COUNT 5

// hack
char *itoa(int num)
{
  static char buff[15] = {};
  int i = 0, temp_num = num, length = 0;
  char *string = buff;
  
  if(num >= 0) {
    // count how many characters in the number
    while(temp_num) {
      temp_num /= 10;
      length++;
    }
    
    // assign the number to the buffer starting at the end of the 
    // number and going to the begining since we are doing the
    // integer to character conversion on the last number in the
    // sequence
    for(i = 0; i < length; i++) {
      buff[(length-1)-i] = '0' + (num % 10);
      num /= 10;
    }
    buff[i] = '\0'; // can't forget the null byte to properly end our string
  }
  else
    return "Unsupported Number";
  
  return string;
}
// hack

static const int RESOURCE_IDS[7] = {
  RESOURCE_ID_IMAGE_ACTION_UP,
  RESOURCE_ID_IMAGE_ACTION_DOWN,
  RESOURCE_ID_IMAGE_ACTION_CHANGE_STROKE,
  RESOURCE_ID_IMAGE_STROKE_BACK,
  RESOURCE_ID_IMAGE_STROKE_BREAST,
  RESOURCE_ID_IMAGE_STROKE_BUTTERFLY,
  RESOURCE_ID_IMAGE_STROKE_FREESTYLE
};

typedef struct {
  GBitmap *bitmap;
  char* text;
  int count;
} StrokeData;

static StrokeData s_stroke_datas[4];
static uint32_t i_current_stroke;
static bool b_stroke_selected;

static Window *window;
static ActionBarLayer *action_bar_layer;

static TextLayer *intro_up_help;
static TextLayer *intro_select_help;
static TextLayer *intro_down_help;

static BitmapLayer *stroke_image_layer;
static TextLayer *stroke_name_layer;
static TextLayer *stroke_count_layer;


static void init_intro_layers() {
  intro_up_help = text_layer_create(GRect(0, 19, 120, 20));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(intro_up_help));

  intro_select_help = text_layer_create(GRect(0, 59, 120, 40));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(intro_select_help));  

  intro_down_help = text_layer_create(GRect(0, 114, 120, 40));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(intro_down_help));  

  text_layer_set_text(intro_up_help, "Increase count");
  text_layer_set_text(intro_select_help, "Toggle stroke\n(Hold to reset)");
  text_layer_set_text(intro_down_help, "Decrease count");

  text_layer_set_text_alignment(intro_up_help, GTextAlignmentCenter);
  text_layer_set_text_alignment(intro_select_help, GTextAlignmentCenter);
  text_layer_set_text_alignment(intro_down_help, GTextAlignmentCenter);  
}

static void init_stroke_layers() {
  StrokeData *stroke_data = &s_stroke_datas[i_current_stroke];

  text_layer_destroy(intro_up_help);
  text_layer_destroy(intro_select_help);
  text_layer_destroy(intro_down_help);

  stroke_image_layer = bitmap_layer_create(GRect(0, 10, 120, 50));
  stroke_name_layer = text_layer_create(GRect(0, 65, 120, 30));
  stroke_count_layer = text_layer_create(GRect(0, 105, 120, 30));

  bitmap_layer_set_bitmap(stroke_image_layer, stroke_data->bitmap);
  bitmap_layer_set_alignment(stroke_image_layer, GAlignCenter);

  text_layer_set_text_alignment(stroke_name_layer, GTextAlignmentCenter);
  text_layer_set_font(stroke_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  
  text_layer_set_text_alignment(stroke_count_layer, GTextAlignmentCenter);
  text_layer_set_font(stroke_count_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));

  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(stroke_image_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(stroke_name_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(stroke_count_layer));
}
static void init_stroke_datas(Window *window) {
  for(int i = 0; i < 4; i++) {
    StrokeData *stroke_data = &s_stroke_datas[i];

    stroke_data->count = 0;
    stroke_data->bitmap = gbitmap_create_with_resource(RESOURCE_IDS[i + 3]);

    switch(i) {
      case 0:
        stroke_data->text = "Backstroke";
        stroke_data->count = persist_read_int(BACKSTROKE_LENGTH_COUNT);
        break;
      case 1:
        stroke_data->text = "Breaststroke";
        stroke_data->count = persist_read_int(BREASTSTROKE_LENGTH_COUNT);
        break;
      case 2:
        stroke_data->text = "Butterfly";
        stroke_data->count = persist_read_int(BUTTERFLY_LENGTH_COUNT);
        break;
      case 3:
        stroke_data->text = "Freestyle";
        stroke_data->count = persist_read_int(FREESTYLE_LENGTH_COUNT);
        break;
    }
  }
}
static void deinit_stroke_datas(void) {
  for(int i = 0; i < 4; i++) {
    StrokeData *stroke_data = &s_stroke_datas[i];

    switch(i) {
      case 0:
        persist_write_int(BACKSTROKE_LENGTH_COUNT, stroke_data->count);
        break;
      case 1:
        persist_write_int(BREASTSTROKE_LENGTH_COUNT, stroke_data->count);
        break;
      case 2:
        persist_write_int(BUTTERFLY_LENGTH_COUNT, stroke_data->count);
        break;
      case 3:
        persist_write_int(FREESTYLE_LENGTH_COUNT, stroke_data->count);
        break;
    }

    gbitmap_destroy(stroke_data->bitmap);
  }
}
static void update_stroke_text(StrokeData *stroke_data) {
  if(stroke_data->count == 0) {
    text_layer_set_text(stroke_count_layer, "0");
  } else {
    text_layer_set_text(stroke_count_layer, itoa(stroke_data->count));  
  }
}
static void change_stroke(uint32_t stroke_id) {
  if(stroke_id == 4) stroke_id = 0; // cycling

  i_current_stroke = stroke_id;

  StrokeData *stroke_data = &s_stroke_datas[i_current_stroke];

  bitmap_layer_set_bitmap(stroke_image_layer, stroke_data->bitmap);
  text_layer_set_text(stroke_name_layer, stroke_data->text);
  update_stroke_text(stroke_data);
}

static void begin_counting() {
  b_stroke_selected = true;
  init_stroke_layers();
  change_stroke(0);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(b_stroke_selected) {
    change_stroke(i_current_stroke + 1);
  } else {
    begin_counting();   
  }
}
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(b_stroke_selected) {
    StrokeData *stroke_data = &s_stroke_datas[i_current_stroke];

    if(stroke_data->count == 500) {
      vibes_short_pulse();
    } else {
      stroke_data->count++;
      update_stroke_text(stroke_data);  
    }
  } else {
    begin_counting();
  }
}
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(b_stroke_selected) {
    StrokeData *stroke_data = &s_stroke_datas[i_current_stroke];

    if(stroke_data->count == 0) {
      vibes_short_pulse();
    } else {
      stroke_data->count--;
      update_stroke_text(stroke_data);  
    }
  } else {
    begin_counting();
  }
}
static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(b_stroke_selected) {
    for(int i = 0; i < 4; i++) {
      StrokeData *stroke_data = &s_stroke_datas[i];
      stroke_data->count = 0;
    }

    StrokeData *current_stroke = &s_stroke_datas[i_current_stroke];
    update_stroke_text(current_stroke);
  } else {
    begin_counting();
  }
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, select_long_click_handler, NULL);
}
static void action_bar_init(Window *window) {
  GBitmap *add_icon = gbitmap_create_with_resource(RESOURCE_IDS[0]);
  GBitmap *subtract_icon = gbitmap_create_with_resource(RESOURCE_IDS[1]);
  GBitmap *stroke_icon = gbitmap_create_with_resource(RESOURCE_IDS[2]);

  action_bar_layer = action_bar_layer_create();
  action_bar_layer_add_to_window(action_bar_layer, window);
  action_bar_layer_set_click_config_provider(action_bar_layer, click_config_provider);
  action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_UP, add_icon);
  action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_SELECT, stroke_icon);
  action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_DOWN, subtract_icon);
}

static void on_window_load(Window *window) {
  init_stroke_datas(window);
  action_bar_init(window);

  b_stroke_selected = false;
  i_current_stroke = persist_read_int(CURRENT_STROKE);

  if(b_stroke_selected) {
    init_stroke_layers();
    change_stroke(i_current_stroke);
  } else {
    init_intro_layers();
  }
}
static void on_window_unload(Window *window) {
  deinit_stroke_datas();
  action_bar_layer_destroy(action_bar_layer);

  if(b_stroke_selected) {
    bitmap_layer_destroy(stroke_image_layer);
    text_layer_destroy(stroke_name_layer);
    text_layer_destroy(stroke_count_layer);
  } else {
    text_layer_destroy(intro_up_help);
    text_layer_destroy(intro_select_help);
    text_layer_destroy(intro_down_help);  
  }
  
  persist_write_bool(STROKE_SELECTED, b_stroke_selected);
  persist_write_int(CURRENT_STROKE, i_current_stroke);
}

static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = on_window_load,
    .unload = on_window_unload,
  });
  window_stack_push(window, true);
}
static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  app_event_loop();

  deinit();
}
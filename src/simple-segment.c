#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

// MY_UUID for convenience: 5901001137E544309F415422187B20D9
#define MY_UUID { 0x59, 0x01, 0x00, 0x11, 0x37, 0xE5, 0x44, 0x30, 0x9F, 0x41, 0x54, 0x22, 0x18, 0x7B, 0x20, 0xD9 }

// Shorthand for debug
#define D(fmt, args...)                                \
  app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ## args)

#define SEGMENT_RESOLUTION 20

// extends it out a little further to chop off edge of circle
#define EDGE_OF_SEGMENT_COMPENSATION 3

PBL_APP_INFO(MY_UUID,
             "Simple Segment", "Tim Clipsham",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_STANDARD_APP);

Window window;
Layer segment_display_layer;
GPath triangle_path;

int angle_to_test = 45;

GPathInfo triangle_path_info = {
  .num_points = SEGMENT_RESOLUTION,
  .points = (GPoint []) {}
};

// void log_path_info(GPathInfo path_info) {
//   GPoint point;

//   D("GPathInfo.num_points: %i", path_info.num_points);

//   for (int i = 0; i < path_info.num_points; ++i) {
//     point = path_info.points[i];
//     D("  %i => [%i, %i]", i, point.x, point.y);
//   }
// }

GPoint point_on_circle(int radius, int angle) {
  return (GPoint) {
    .x = 0 + (radius + EDGE_OF_SEGMENT_COMPENSATION) *
    cos_lookup(TRIG_MAX_ANGLE * angle / 360) / TRIG_MAX_RATIO,

    .y = 0 + (radius + EDGE_OF_SEGMENT_COMPENSATION) *
    sin_lookup(TRIG_MAX_ANGLE * angle / 360) / TRIG_MAX_RATIO
  };
}

void render_segment(GContext *ctx, GPoint position, int radius, int rotation, int angle) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, position, radius);

  triangle_path_info.points[1] = point_on_circle(radius, rotation);

  int partial_angle; // float??

  for (int i = 1; i <= SEGMENT_RESOLUTION - 2; ++i) {
    partial_angle = angle / (SEGMENT_RESOLUTION - 2) * i;
    triangle_path_info.points[i+1] = point_on_circle(radius, partial_angle);
  }

  graphics_context_set_fill_color(ctx, GColorBlack);
  gpath_move_to(&triangle_path, position);
  // gpath_rotate_to(&triangle_path, -(TRIG_MAX_ANGLE / 4));
  gpath_draw_filled(ctx, &triangle_path);
}

void segment_display_layer_update_callback(Layer *me, GContext *ctx) {
  // PblTm time;
  // get_time(&time);
  render_segment(ctx, grect_center_point(&me->frame), 35, 0, angle_to_test);
}

// temporary function for testing
void button_up_click_handler(ClickRecognizerRef recognizer, Window *window) {
  angle_to_test += 45;
  layer_mark_dirty(&segment_display_layer);
}

// temporary function for testing
void button_down_click_handler(ClickRecognizerRef recognizer, Window *window) {
  angle_to_test -= 45;
  layer_mark_dirty(&segment_display_layer);
}

void config_provider(ClickConfig **config, Window *window) {
  config[BUTTON_ID_UP]->click.handler = (ClickHandler) button_up_click_handler;
  config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) button_down_click_handler;
}

void handle_init(AppContextRef ctx) {
  window_init(&window, "Window Name");
  window_stack_push(&window, true);
  window_set_background_color(&window, GColorBlack);
  window_set_click_config_provider(&window, (ClickConfigProvider) config_provider);

  gpath_init(&triangle_path, &triangle_path_info);

  layer_init(&segment_display_layer, window.layer.frame);
  segment_display_layer.update_proc = &segment_display_layer_update_callback;
  layer_add_child(&window.layer, &segment_display_layer);
}

void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {
  layer_mark_dirty(&segment_display_layer);
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .tick_info = {
      .tick_handler = &handle_minute_tick,
      .tick_units = MINUTE_UNIT
    }
  };
  app_event_loop(params, &handlers);
}

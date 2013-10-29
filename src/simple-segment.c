#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

// MY_UUID for convenience: 5901001137E544309F415422187B20D9
#define MY_UUID { 0x59, 0x01, 0x00, 0x11, 0x37, 0xE5, 0x44, 0x30, 0x9F, 0x41, 0x54, 0x22, 0x18, 0x7B, 0x20, 0xD9 }

// Shorthand for debug
#define D(fmt, args...) \
  app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ## args)

#define SEGMENT_RESOLUTION 20
#define RADIUS 50
#define TIME_STRING_BUFFER_SIZE 6

// extends it out a little further to chop off edge of circle
#define EDGE_OF_SEGMENT_COMPENSATION 4

PBL_APP_INFO(MY_UUID,
             "Simple Segment", "Tim Clipsham",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

Window window;
TextLayer time_display;
Layer segment_display_layer;
GPath triangle_path;

int twelve_oclock_rotation = -90,
  angle_hour_interval = 360 / 12,
  angle_minute_interval = 360 / 60;

char time_string[TIME_STRING_BUFFER_SIZE];

GPathInfo triangle_path_info = {
  .num_points = SEGMENT_RESOLUTION,
  .points = (GPoint []) {}
};

// parametric equation for a circle
//   x = cx + r * cos(a)
//   y = cy + r * sin(a)
GPoint point_on_circle(int radius, int angle) {
  return (GPoint) {
    .x = 0 + (radius + EDGE_OF_SEGMENT_COMPENSATION) *
    cos_lookup(TRIG_MAX_ANGLE * angle / 360) / TRIG_MAX_RATIO,

    .y = 0 + (radius + EDGE_OF_SEGMENT_COMPENSATION) *
    sin_lookup(TRIG_MAX_ANGLE * angle / 360) / TRIG_MAX_RATIO
  };
}

void render_segment(GContext *ctx, GPoint position, int radius, int rotation, int angle) {
  angle = (360 - angle) % 360;
  rotation = rotation % 360;

  triangle_path_info.points[0] = GPointZero;
  triangle_path_info.points[1] = point_on_circle(radius, 0);

  int partial_angle; // float??

  for (int i = 1; i <= SEGMENT_RESOLUTION - 2; ++i) {
    partial_angle = angle / (SEGMENT_RESOLUTION - 2) * i;
    triangle_path_info.points[i + 1] = point_on_circle(radius, -partial_angle);
  }

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, position, radius);
  graphics_context_set_fill_color(ctx, GColorBlack);
  gpath_move_to(&triangle_path, position);
  gpath_rotate_to(&triangle_path, TRIG_MAX_ANGLE * rotation / 360);
  gpath_draw_filled(ctx, &triangle_path);
}

void render_clock(GContext *ctx, GPoint position, PblTm time) {
  int hours, minutes, rotation, angle, hours_clock_angle_deg, minutes_clock_angle_deg;

  angle = 0;
  rotation = 0;
  hours = time.tm_hour;
  minutes = time.tm_min;

  hours_clock_angle_deg = angle_hour_interval * hours % 360;
  minutes_clock_angle_deg = angle_minute_interval * minutes % 360;

  if (hours % 2 == 0 && minutes_clock_angle_deg >= hours_clock_angle_deg) {
    rotation = hours_clock_angle_deg;
    angle = minutes_clock_angle_deg - hours_clock_angle_deg;
  } else if (hours % 2 == 0 && minutes_clock_angle_deg < hours_clock_angle_deg) {
    rotation = minutes_clock_angle_deg;
    angle = hours_clock_angle_deg - minutes_clock_angle_deg;
  } else if (hours % 2 != 0 && minutes_clock_angle_deg >= hours_clock_angle_deg) {
    rotation = minutes_clock_angle_deg;
    angle = 360 + hours_clock_angle_deg - minutes_clock_angle_deg;
  } else if (hours % 2 != 0 && minutes_clock_angle_deg < hours_clock_angle_deg) {
    rotation = hours_clock_angle_deg;
    angle = 360 - hours_clock_angle_deg + minutes_clock_angle_deg;
  }

  rotation += twelve_oclock_rotation;
  render_segment(ctx, position, RADIUS, rotation, angle);
}

void segment_display_layer_update_callback(Layer *me, GContext *ctx) {
  PblTm time;
  GPoint center;

  get_time(&time);
  center = grect_center_point(&me->frame);

  render_clock(ctx, center, time);

  string_format_time(time_string, sizeof(time_string), "%I:%M", &time);
  text_layer_set_text(&time_display, time_string);
}

void handle_init(AppContextRef ctx) {
  window_init(&window, "Window Name");
  window_stack_push(&window, true);
  window_set_background_color(&window, GColorBlack);

  text_layer_init(&time_display, window.layer.frame);
  text_layer_set_background_color(&time_display, GColorClear);
  text_layer_set_text_color(&time_display, GColorWhite);
  text_layer_set_text_alignment(&time_display, GTextAlignmentCenter);
  text_layer_set_font(&time_display, fonts_get_system_font(FONT_KEY_GOTHIC_24));

  gpath_init(&triangle_path, &triangle_path_info);

  layer_init(&segment_display_layer, window.layer.frame);
  segment_display_layer.update_proc = &segment_display_layer_update_callback;

  layer_add_child(&window.layer, &segment_display_layer);
  layer_add_child(&window.layer, &time_display.layer);
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

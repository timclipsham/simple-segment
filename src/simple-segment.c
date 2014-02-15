#include <pebble.h>
#include <time.h>

// Shorthand for debug
#define D(fmt, args...) \
  app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ## args)

#define SEGMENT_RESOLUTION 20
#define RADIUS 50
#define TIME_STRING_BUFFER_SIZE 6

// extends it out a little further to chop off edge of circle
#define EDGE_OF_SEGMENT_COMPENSATION 5

Window *window;
TextLayer *time_display;
Layer *segment_display_layer;
GPath *triangle_path;

int twelve_oclock_rotation = -90,
  angle_hour_interval = 360 / 12,
  angle_minute_interval = 360 / 60;

char time_string[TIME_STRING_BUFFER_SIZE];

GPoint triangle_path_points[SEGMENT_RESOLUTION];

GPathInfo triangle_path_info = {
  .num_points = SEGMENT_RESOLUTION,
  .points = triangle_path_points
};

// parametric equation for a circle
//   x = cx + r * cos(a)
//   y = cy + r * sin(a)
GPoint point_on_circle(int radius, int angle) {
  return (GPoint) {
    .x = 0 + (radius + EDGE_OF_SEGMENT_COMPENSATION) *
    cos_lookup(angle) / TRIG_MAX_RATIO,

    .y = 0 + (radius + EDGE_OF_SEGMENT_COMPENSATION) *
    sin_lookup(angle) / TRIG_MAX_RATIO
  };
}

void render_segment(GContext *context, GPoint position, int radius, int rotation, int angle) {
  angle = ((360 - angle) % 360) / 360.0 * TRIG_MAX_ANGLE;
  rotation = (rotation % 360) / 360.0 * TRIG_MAX_ANGLE;

  triangle_path_info.points[0] = GPointZero;
  triangle_path_info.points[1] = point_on_circle(radius, 0);

  int partial_angle;

  for (int i = 1; i <= SEGMENT_RESOLUTION - 2; ++i) {
    partial_angle = angle / (SEGMENT_RESOLUTION - 2) * i;
    triangle_path_info.points[i + 1] = point_on_circle(radius, -partial_angle);
  }

  graphics_context_set_fill_color(context, GColorWhite);
  graphics_fill_circle(context, position, radius);
  graphics_context_set_fill_color(context, GColorBlack);
  gpath_move_to(triangle_path, position);
  gpath_rotate_to(triangle_path, rotation);
  gpath_draw_filled(context, triangle_path);
}

void render_clock(GContext *context, GPoint position, struct tm *time_info) {
  int hours, minutes, rotation, angle, hours_clock_angle_deg, minutes_clock_angle_deg;

  angle = 0;
  rotation = 0;
  hours = time_info->tm_hour;
  minutes = time_info->tm_min;

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
  render_segment(context, position, RADIUS, rotation, angle);
}

void segment_display_layer_update_callback(Layer *layer, GContext *context) {
  time_t raw_time = time(NULL);
  struct tm *time_info = localtime(&raw_time);
  GRect bounds = layer_get_bounds(layer);

  GPoint center = grect_center_point(&bounds);
  center.y += 20;  // HACK to better position the clock

  render_clock(context, center, time_info);

  strftime(time_string, sizeof(time_string), "%I:%M", time_info);
  text_layer_set_text(time_display, time_string);
}

void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(segment_display_layer);
}

void window_load(Window *window) {
  Layer *layer = window_get_root_layer(window);
  GRect layer_bounds = layer_get_bounds(layer);

  window_set_background_color(window, GColorBlack);

  time_display = text_layer_create(layer_bounds);
  text_layer_set_background_color(time_display, GColorClear);
  text_layer_set_text_color(time_display, GColorWhite);
  text_layer_set_text_alignment(time_display, GTextAlignmentCenter);
  text_layer_set_font(time_display, fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS));

  triangle_path = gpath_create(&triangle_path_info);

  segment_display_layer = layer_create(layer_bounds);
  layer_set_update_proc(segment_display_layer, segment_display_layer_update_callback);

  layer_add_child(layer, segment_display_layer);
  layer_add_child(layer, text_layer_get_layer(time_display));

  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
}

void window_unload(Window *window) {
  tick_timer_service_unsubscribe();
  layer_destroy(segment_display_layer);
  gpath_destroy(triangle_path);
  text_layer_destroy(time_display);
}

void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_stack_push(window, true);
}

void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  D("Done initializing, pushed window: %p", window);
  app_event_loop();
  deinit();
}

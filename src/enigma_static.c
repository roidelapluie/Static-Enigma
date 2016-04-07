#include <pebble.h>

static Window *window;

static bool savedConnectionState;

//Setup Layers.
static Layer *bottom_layer;
static Layer *time_layer;
static Layer *date_layer;
static Layer *top_random;
static Layer *bottom_random;

static GSize textSize;

enum {
  bg = 0,
  fg = 1
};

static int32_t fg_color;
static int32_t bg_color;

#define FONT_PADDING 6
#define NUMBER_GAP 10
#ifdef PBL_ROUND
#define PADDING 17
#else
#define PADDING 0
#endif

static int getSmallRandNumber() {
    return rand() % 5;
}

static int getRandNumber() {
    return rand() % 9;
}

static void battery_handler(BatteryChargeState charge) {
   layer_mark_dirty(bottom_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & MINUTE_UNIT) {
    layer_mark_dirty(time_layer);
  }
  if (units_changed & DAY_UNIT) {
      layer_mark_dirty(date_layer);
      layer_mark_dirty(bottom_layer);
  }
}


static void bluetooth_handler(bool connected) {
    if (connected) {
        bg_color = (int) 0xAA0000;
    } else {
        bg_color = (int) 0x555555;
    }
    savedConnectionState = connected;
    vibes_short_pulse();
    layer_mark_dirty(time_layer);
}

#ifdef PBL_ROUND
static void draw_round_numbers(GContext *ctx, GRect bounds, int height) {
    char tmp[2];
    snprintf(tmp, 2, "%d", getRandNumber());
    graphics_draw_text(ctx, tmp, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS),
            GRect(-textSize.w/2, height, textSize.w, bounds.size.h),
            GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    snprintf(tmp, 2, "%d", getRandNumber());
    graphics_draw_text(ctx, tmp, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS),
            GRect(bounds.size.w-textSize.w/2, height, textSize.w, bounds.size.h),
            GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}
#endif

static void update_time(Layer *layer, GContext *ctx) {
    // APP_LOG(APP_LOG_LEVEL_INFO, "Drawing Time Layer.");
    GRect bounds = layer_get_bounds(layer);

    graphics_context_set_text_color(ctx, PBL_IF_COLOR_ELSE(GColorFromHEX(fg_color), GColorWhite));
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorFromHEX(bg_color), GColorLightGray));
    graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, bounds.size.h-1),0,GCornerNone);

    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    static char s_buffer[5][5];
    strftime(s_buffer[0], sizeof(s_buffer[0]), "%H%M", tick_time);

    int n = 0;
    for (size_t i = 1; i < 5; i++) {
        s_buffer[i][0] = s_buffer[0][n];
        s_buffer[i][1] = '\n';
        graphics_draw_text(ctx, s_buffer[i], fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS),
                GRect((NUMBER_GAP * n) + (textSize.w * n) + FONT_PADDING + PADDING, -7, textSize.w, bounds.size.h),
                GTextOverflowModeFill, GTextAlignmentCenter, NULL);
        n++;
    }

#ifdef PBL_ROUND
    draw_round_numbers(ctx, bounds, -7);
#endif
}

static void update_date(Layer *layer, GContext *ctx) {
    // APP_LOG(APP_LOG_LEVEL_INFO, "Drawing Date Layer.");
    GRect bounds = layer_get_bounds(layer);
    graphics_context_set_text_color(ctx, GColorWhite);

    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    static char s_buffer[5][5];
    strftime(s_buffer[0], sizeof(s_buffer), "%d%m", tick_time);

    int n = 0;
    for (size_t i = 1; i < 5; i++) {
        s_buffer[i][0] = s_buffer[0][n];
        s_buffer[i][1] = '\n';
        graphics_draw_text(ctx, s_buffer[i], fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS),
                GRect((NUMBER_GAP * n) + (textSize.w * n) + FONT_PADDING + PADDING, 0, textSize.w, bounds.size.h),
                GTextOverflowModeFill, GTextAlignmentCenter, NULL);
        n++;
    }

#ifdef PBL_ROUND
    draw_round_numbers(ctx, bounds, 0);
#endif
}

static void update_bottom(Layer *layer, GContext *ctx) {
    // APP_LOG(APP_LOG_LEVEL_INFO, "Drawing Year Layer.");
    GRect bounds = layer_get_bounds(layer);
    graphics_context_set_text_color(ctx, GColorWhite);

    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    static char s_buffer[5][5];
    strftime(s_buffer[0], sizeof(s_buffer[0]), "%W", tick_time);

    BatteryChargeState state = battery_state_service_peek();
    int pc = (int)state.charge_percent;
    if (pc > 99) {
        s_buffer[0][2] = '9';
        s_buffer[0][3] = '9';
    }else{
        s_buffer[0][2] = getSmallRandNumber() + '0';
        s_buffer[0][3] = pc/10+'0';
    }

    int n = 0;
    for (size_t i = 1; i < 5; i++) {
        s_buffer[i][0] = s_buffer[0][n];
        s_buffer[i][1] = '\n';
        graphics_draw_text(ctx, s_buffer[i], fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS),
                GRect((NUMBER_GAP * n) + (textSize.w * n) + FONT_PADDING  + PADDING, 0, textSize.w, bounds.size.h),
                GTextOverflowModeFill, GTextAlignmentCenter, NULL);
        n++;
    }

#ifdef PBL_ROUND
    draw_round_numbers(ctx, bounds, 0);
#endif

}

static void generate_random(Layer *layer, GContext *ctx) {
    // APP_LOG(APP_LOG_LEVEL_INFO, "Drawing Random Number Layers.");
    GRect bounds = layer_get_bounds(layer);
    graphics_context_set_text_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkGray, GColorWhite));
    char tmp[2];
    for (size_t i = 0; i < 4; i++) {
        snprintf(tmp, 2, "%d", getRandNumber());
        graphics_draw_text(ctx, tmp, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS),
                GRect((NUMBER_GAP * i) + (textSize.w * i) + FONT_PADDING + PADDING, 0, textSize.w, bounds.size.h),
                GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    }
}

static void window_load(Window *window) {
    textSize = graphics_text_layout_get_content_size("0",
            fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS),
            GRect(0, 0, 50, 42), GTextOverflowModeFill,
            GTextAlignmentCenter);

    bool connectionState = connection_service_peek_pebble_app_connection();
    if (connectionState) {
        bg_color = (int) 0xAA0000;
    } else {
        bg_color = (int) 0x555555;
    }

    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    window_set_background_color(window, GColorBlack);

    //draw random numbers
    top_random = layer_create(GRect(0, PBL_IF_RECT_ELSE(-32, -26), bounds.size.w, textSize.h));
    layer_add_child(window_layer, top_random);
    layer_set_update_proc(top_random, generate_random);

    bottom_random = layer_create(GRect(0, PBL_IF_RECT_ELSE(144, 150), bounds.size.w, textSize.h));
    layer_add_child(window_layer, bottom_random);
    layer_set_update_proc(bottom_random, generate_random);

    date_layer = layer_create(GRect(0, PBL_IF_RECT_ELSE(12, 18), bounds.size.w, textSize.h));
    layer_add_child(window_layer, date_layer);
    layer_set_update_proc(date_layer, update_date);

    time_layer = layer_create(GRect(0, PBL_IF_RECT_ELSE(63, 69), bounds.size.w, textSize.h));
    layer_add_child(window_layer,time_layer);
    layer_set_update_proc(time_layer, update_time);

    bottom_layer = layer_create(GRect(0, PBL_IF_RECT_ELSE(100, 106), bounds.size.w, textSize.h));
    layer_add_child(window_layer, bottom_layer);
    layer_set_update_proc(bottom_layer, update_bottom);

    if (savedConnectionState != connectionState){
        savedConnectionState = connectionState;
        vibes_short_pulse();
    }
}

static void window_unload(Window *window) {
    layer_destroy(top_random);
    layer_destroy(bottom_random);
    layer_destroy(bottom_layer);
    layer_destroy(time_layer);
    layer_destroy(date_layer);
}

static void init(void) {
    window = window_create();
    savedConnectionState = connection_service_peek_pebble_app_connection();

    if (savedConnectionState) {
        bg_color = (int) 0xAA0000;
    } else {
        bg_color = (int) 0x555555;
    }
    fg_color = 16777215;

    window_set_window_handlers(window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload,
            });

    srand(time(NULL)); //seed random number generator
    //subscribe to multiple tick events to update layers accordingly.
    tick_timer_service_subscribe(MINUTE_UNIT | DAY_UNIT, tick_handler);
    battery_state_service_subscribe(battery_handler);
    connection_service_subscribe((ConnectionHandlers) { .pebble_app_connection_handler = bluetooth_handler });
    window_stack_push(window, false);
}

static void deinit(void) {
    tick_timer_service_unsubscribe();
    connection_service_unsubscribe();
    window_destroy(window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}

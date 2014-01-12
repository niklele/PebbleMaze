#include <pebble.h>

#define ACCEL_RATIO 0.05
#define TICK 80 /* milliseconds */

static Window *window;
static GRect window_frame;

// static TextLayer *text_layer;
static AppTimer *timer;
static Layer *graphics_layer;

static GBitmap *box;


typedef struct Vec2d {
    double x;
    double y;
} Vec2d;

#define RADIUS 5
#define MASS 30

typedef struct Ball {
    Vec2d pos;
    Vec2d vel;
    double mass;
    double radius;
} Ball;

static Ball *ball;

void ball_init() {
    ball = (Ball *) malloc(sizeof(Ball));
    ball->pos.x = window_frame.size.w/2;
    ball->pos.y = window_frame.size.h/2;
    ball->vel.x = 0;
    ball->vel.y = 0;
    ball->radius = RADIUS;
    ball->mass = 25;
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    // text_layer_set_text(text_layer, "Select");
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    // text_layer_set_text(text_layer, "Up");
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    // text_layer_set_text(text_layer, "Down");
}

static void ball_apply_force(Vec2d force) {
    ball->vel.x += force.x / ball->mass;
    ball->vel.y += force.y / ball->mass;
}

static void ball_apply_accel(AccelData accel) {
    Vec2d force;
    force.x = accel.x * ACCEL_RATIO;
    force.y = -accel.y * ACCEL_RATIO;
    ball_apply_force(force);
}

static void ball_update() {
    const GRect frame = window_frame;
    double e = 0.5;
    if ((ball->pos.x - ball->radius < 0 && ball->vel.x < 0) ||
        (ball->pos.x + ball->radius > frame.size.w && ball->vel.x > 0)) {
        ball->vel.x = -ball->vel.x * e;
    }
    if ((ball->pos.y - ball->radius < 0 && ball->vel.y < 0) ||
        (ball->pos.y + ball->radius > frame.size.h && ball->vel.y > 0)) {
        ball->vel.y = -ball->vel.y * e;
    }
    ball->pos.x += ball->vel.x;
    ball->pos.y += ball->vel.y;
}

static void ball_draw(GContext *ctx) {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_circle(ctx, GPoint(ball->pos.x, ball->pos.y), ball->radius);
}

static void draw_callback(Layer *me, GContext *ctx) {
    GRect bounds = box->bounds;
    graphics_draw_bitmap_in_rect(ctx, box, (GRect) { .origin = { 10, 20 }, .size = bounds.size });
    ball_draw(ctx);
}

static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void timer_callback(void *data) {
    AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };

    accel_service_peek(&accel);
    ball_apply_accel(accel);
    ball_update();

    layer_mark_dirty(graphics_layer);

    timer = app_timer_register(TICK /* milliseconds */, timer_callback, NULL);
}

static void handle_accel(AccelData *accel_data, uint32_t num_samples) {
    // do nothing
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    window_frame = layer_get_frame(window_layer);

    graphics_layer = layer_create(window_frame);
    layer_set_update_proc(graphics_layer, draw_callback);
    layer_add_child(window_layer, graphics_layer);
}

static void window_unload(Window *window) {
    // text_layer_destroy(text_layer);
    layer_destroy(graphics_layer);
}

static void init(void) {
    window = window_create();
    window_set_click_config_provider(window, click_config_provider);
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    window_stack_push(window, true /* animated */);

    accel_data_service_subscribe(0, handle_accel);

    box = gbitmap_create_with_resource(RESOURCE_ID_BOX);

    ball_init();

    timer = app_timer_register(TICK /* milliseconds */, timer_callback, NULL);
}

static void deinit(void) {
    accel_data_service_unsubscribe();
    window_destroy(window);
    free(ball);
    gbitmap_destroy(box);
}

int main(void) {
    init();

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

    app_event_loop();
    deinit();
}

/*
 * Irish Fuzzy Time
 * jim.lawton@gmail.com
 *
 * Inspired by Fuzzy Time +, Fuzzy Time.
 *
 * Define AM_REVERSE to turn on reverse highlight during a.m. hours.
 * Define HOURLY_VIBE to enable the hourly vibration.
 */

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "fuzzy_time.h"

/* Configuration */
//#define AM_REVERSE
#define HOURLY_VIBE
//#define LINE1_FONT FONT_KEY_GOTHIC_28
#define LINE1_FONT FONT_KEY_BITHAM_42_LIGHT
//#define LINE2_NORMAL_FONT FONT_KEY_GOTHIC_28
#define LINE2_NORMAL_FONT FONT_KEY_BITHAM_42_LIGHT
#define LINE2_BOLD_FONT FONT_KEY_BITHAM_42_BOLD
#define LINE3_FONT FONT_KEY_BITHAM_42_BOLD
/* End configuration. */

//#define MY_UUID { 0x39, 0x5F, 0x05, 0x28, 0x48, 0x9B, 0x40, 0xAC, 0xB4, 0xF1, 0x10, 0x3F, 0x8D, 0xE4, 0x66, 0x9E }
#define MY_UUID { 0x5f, 0x58, 0x20, 0x2c, 0xa1, 0x7c, 0x49, 0xaa, 0xa5, 0xd9, 0xae, 0x68, 0x35, 0xf0, 0xda, 0xc3 }

PBL_APP_INFO(MY_UUID,
             "Irish Fuzzy Time",
             "jim.lawton@gmail.com",
             1, 3, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);

#define ANIMATION_DURATION 800
#define LINE_BUFFER_SIZE 50
#define WINDOW_NAME "fuzzy_time_irish"

Window window;

typedef struct {
    TextLayer layer[2];
    PropertyAnimation layer_animation[2];
} TextLine;

typedef struct {
    char line1[LINE_BUFFER_SIZE];
    char line2[LINE_BUFFER_SIZE];
    char line3[LINE_BUFFER_SIZE];
} TheTime;

TextLayer topbarLayer;
TextLayer bottombarLayer;
TextLine line1;
TextLine line2;
TextLine line3;

static TheTime cur_time;
static TheTime new_time;

static char str_topbar[LINE_BUFFER_SIZE];
static char str_bottombar[LINE_BUFFER_SIZE];
static bool busy_animating_in = false;
static bool busy_animating_out = false;
const int line1_y = 18;
const int line2_y = 60;
const int line3_y = 102;

void animationInStoppedHandler(struct Animation *animation, bool finished, void *context) {
    busy_animating_in = false;
    //reset cur_time
    cur_time = new_time;
}

void animationOutStoppedHandler(struct Animation *animation, bool finished, void *context) {
    //reset out layer to x=144
    TextLayer *outside = (TextLayer *) context;
    GRect rect = layer_get_frame(&outside->layer);
    rect.origin.x = 144;
    layer_set_frame(&outside->layer, rect);

    busy_animating_out = false;
}

void set_am_style(void) {
    text_layer_set_text_color(&line3.layer[0], GColorBlack);
    text_layer_set_background_color(&line3.layer[0], GColorWhite);
    text_layer_set_text_color(&line3.layer[1], GColorBlack);
    text_layer_set_background_color(&line3.layer[1], GColorWhite);
}

void set_pm_style(void) {
    text_layer_set_text_color(&line3.layer[0], GColorWhite);
    text_layer_set_background_color(&line3.layer[0], GColorClear);
    text_layer_set_text_color(&line3.layer[1], GColorWhite);
    text_layer_set_background_color(&line3.layer[1], GColorClear);
}

void set_line2_am(void) {
    GRect rect = layer_get_frame(&line2.layer[0].layer);
    if (rect.origin.x == 0) {
        text_layer_set_text_color(&line2.layer[1], GColorBlack);
        text_layer_set_background_color(&line2.layer[1], GColorWhite);
        text_layer_set_font(&line2.layer[1], fonts_get_system_font(LINE2_BOLD_FONT));
    } else {
        text_layer_set_text_color(&line2.layer[0], GColorBlack);
        text_layer_set_background_color(&line2.layer[0], GColorWhite);
        text_layer_set_font(&line2.layer[0], fonts_get_system_font(LINE2_BOLD_FONT));
    }
}

void set_line2_pm(void) {
    GRect rect = layer_get_frame(&line2.layer[0].layer);
    if (rect.origin.x == 0) {
        text_layer_set_font(&line2.layer[1], fonts_get_system_font(LINE2_BOLD_FONT));
    } else {
        text_layer_set_font(&line2.layer[0], fonts_get_system_font(LINE2_BOLD_FONT));
    }
}

void reset_line2(void) {
//  GRect rect = layer_get_frame(&line2.layer[0].layer);
//  if (rect.origin.x == 0) {
    text_layer_set_text_color(&line2.layer[1], GColorWhite);
    text_layer_set_background_color(&line2.layer[1], GColorBlack);
    text_layer_set_font(&line2.layer[1], fonts_get_system_font(LINE2_NORMAL_FONT));
//  } else {
    text_layer_set_text_color(&line2.layer[0], GColorWhite);
    text_layer_set_background_color(&line2.layer[0], GColorBlack);
    text_layer_set_font(&line2.layer[0], fonts_get_system_font(LINE2_NORMAL_FONT));
//  }  
}

void updateLayer(TextLine *animating_line, int line) {
    TextLayer *inside, *outside;
    GRect rect = layer_get_frame(&animating_line->layer[0].layer);

    inside = (rect.origin.x == 0) ? &animating_line->layer[0] : &animating_line->layer[1];
    outside = (inside == &animating_line->layer[0]) ? &animating_line->layer[1] : &animating_line->layer[0];

    GRect in_rect = layer_get_frame(&outside->layer);
    GRect out_rect = layer_get_frame(&inside->layer);

    in_rect.origin.x -= 144;
    out_rect.origin.x -= 144;

    //animate out current layer
    busy_animating_out = true;
    property_animation_init_layer_frame(&animating_line->layer_animation[1], &inside->layer, NULL, &out_rect);
    animation_set_duration(&animating_line->layer_animation[1].animation, ANIMATION_DURATION);
    animation_set_curve(&animating_line->layer_animation[1].animation, AnimationCurveEaseOut);
    animation_set_handlers(&animating_line->layer_animation[1].animation,
                           (AnimationHandlers) { .stopped = (AnimationStoppedHandler)animationOutStoppedHandler },
                           (void *)inside);
    animation_schedule(&animating_line->layer_animation[1].animation);

    if (line == 1) {
        text_layer_set_text(outside, new_time.line1);
        text_layer_set_text(inside, cur_time.line1);
    }

    if (line == 2) {
        text_layer_set_text(outside, new_time.line2);
        text_layer_set_text(inside, cur_time.line2);
    }

    if (line == 3) {
        text_layer_set_text(outside, new_time.line3);
        text_layer_set_text(inside, cur_time.line3);
    }

    //animate in new layer
    busy_animating_in = true;
    property_animation_init_layer_frame(&animating_line->layer_animation[0], &outside->layer, NULL, &in_rect);
    animation_set_duration(&animating_line->layer_animation[0].animation, ANIMATION_DURATION);
    animation_set_curve(&animating_line->layer_animation[0].animation, AnimationCurveEaseOut);
    animation_set_handlers(&animating_line->layer_animation[0].animation,
                           (AnimationHandlers ) { .stopped = (AnimationStoppedHandler) animationInStoppedHandler },
                           (void *)outside);
    animation_schedule(&animating_line->layer_animation[0].animation);
}

void update_watch(PblTm* t) {
    //Let's get the new time and date
    fuzzy_time(t->tm_hour, t->tm_min, new_time.line1, new_time.line2, new_time.line3);
    string_format_time(str_topbar, sizeof(str_topbar), "%A | %e %b", t);
    string_format_time(str_bottombar, sizeof(str_bottombar), " %H%M | CW: %V", t);


    //Let's update the top and bottom bar anyway - **to optimize later to only update top bar every new day.
    text_layer_set_text(&topbarLayer, str_topbar);
    text_layer_set_text(&bottombarLayer, str_bottombar);

    if (t->tm_min == 0) {
#ifdef HOURLY_VIBE
        /* No vibe during the night. */
        if (t->tm_hour == 0 || t->tm_hour > 7) {
            vibes_short_pulse();
        }
#endif
#ifdef AM_REVERSE
        if (t->tm_hour >= 12) {
            set_line2_pm();
        } else {
            set_line2_am();
        }
#else
        set_line2_pm();
#endif
    }

    if (t->tm_min > 1) {
        reset_line2();
    }

#ifdef AM_REVERSE
    if (t->tm_hour >= 12) {
        set_pm_style();
    } else {
        set_am_style();
    }
#else
    set_pm_style();
#endif

    // update hour only if changed
    if (strcmp(new_time.line1, cur_time.line1) != 0) {
        updateLayer(&line1, 1);
    }
    // update min1 only if changed
    if (strcmp(new_time.line2, cur_time.line2) != 0) {
        updateLayer(&line2, 2);
    }
    // update min2 only if changed happens on
    if (strcmp(new_time.line3, cur_time.line3) != 0) {
        updateLayer(&line3, 3);
    }
}

void init_watch(PblTm* t) {
    fuzzy_time(t->tm_hour, t->tm_min, new_time.line1, new_time.line2, new_time.line3);
    string_format_time(str_topbar, sizeof(str_topbar), " %A | %e %b", t);
    string_format_time(str_bottombar, sizeof(str_bottombar), " %H%M | CW: %V", t);

    text_layer_set_text(&topbarLayer, str_topbar);
    text_layer_set_text(&bottombarLayer, str_bottombar);

    strcpy(cur_time.line1, new_time.line1);
    strcpy(cur_time.line2, new_time.line2);
    strcpy(cur_time.line3, new_time.line3);

#ifdef AM_REVERSE
    if (t->tm_min == 0) {
        if (t->tm_hour > 12) {
            set_line2_pm();
        } else {
            set_line2_pm();
        }
    }
    if (t->tm_hour > 12) {
        set_pm_style();
    } else {
        set_am_style();
    }
#else
    set_line2_pm();
    set_pm_style();
#endif

    text_layer_set_text(&line1.layer[0], cur_time.line1);
    text_layer_set_text(&line2.layer[0], cur_time.line2);
    text_layer_set_text(&line3.layer[0], cur_time.line3);
}

// Handle the start-up of the app
void handle_init_app(AppContextRef app_ctx) {
    // Create our app's base window
    window_init(&window, WINDOW_NAME);
    window_stack_push(&window, true);
    window_set_background_color(&window, GColorBlack);


    // Init the text layers used to show the time

    // line1
    text_layer_init(&line1.layer[0], GRect(0, line1_y, 144, 50));
    text_layer_set_text_color(&line1.layer[0], GColorWhite);
    text_layer_set_background_color(&line1.layer[0], GColorClear);
    text_layer_set_font(&line1.layer[0], fonts_get_system_font(LINE1_FONT));
    text_layer_set_text_alignment(&line1.layer[0], GTextAlignmentLeft);

    text_layer_init(&line1.layer[1], GRect(144, line1_y, 144, 50));
    text_layer_set_text_color(&line1.layer[1], GColorWhite);
    text_layer_set_background_color(&line1.layer[1], GColorClear);
    text_layer_set_font(&line1.layer[1], fonts_get_system_font(LINE1_FONT));
    text_layer_set_text_alignment(&line1.layer[1], GTextAlignmentLeft);

    // line2
    text_layer_init(&line2.layer[0], GRect(0, line2_y, 144, 50));
    text_layer_set_text_color(&line2.layer[0], GColorWhite);
    text_layer_set_background_color(&line2.layer[0], GColorBlack);
    text_layer_set_font(&line2.layer[0], fonts_get_system_font(LINE2_NORMAL_FONT));
    text_layer_set_text_alignment(&line2.layer[0], GTextAlignmentLeft);

    text_layer_init(&line2.layer[1], GRect(144, line2_y, 144, 50));
    text_layer_set_text_color(&line2.layer[1], GColorWhite);
    text_layer_set_background_color(&line2.layer[1], GColorBlack);
    text_layer_set_font(&line2.layer[1], fonts_get_system_font(LINE2_NORMAL_FONT));
    text_layer_set_text_alignment(&line2.layer[1], GTextAlignmentLeft);

    // line3
    text_layer_init(&line3.layer[0], GRect(0, line3_y, 144, 50));
    //text_layer_set_text_color(&line3.layer[0], GColorWhite);
    //text_layer_set_background_color(&line3.layer[0], GColorClear);
    text_layer_set_font(&line3.layer[0], fonts_get_system_font(LINE3_FONT));
    text_layer_set_text_alignment(&line3.layer[0], GTextAlignmentLeft);

    text_layer_init(&line3.layer[1], GRect(144, line3_y, 144, 50));
    //text_layer_set_text_color(&line3.layer[1], GColorWhite);
    //text_layer_set_background_color(&line3.layer[1], GColorClear);
    text_layer_set_font(&line3.layer[1], fonts_get_system_font(LINE3_FONT));
    text_layer_set_text_alignment(&line3.layer[1], GTextAlignmentLeft);

    //text_layer_init(&line3_bg, GRect(144, line3_y, 144, 48));
    //text_layer_set_background_color(&line3_bg, GColorWhite);

    // date
    text_layer_init(&topbarLayer, GRect(0, 0, 144, 18));
    text_layer_set_text_color(&topbarLayer, GColorWhite);
    text_layer_set_background_color(&topbarLayer, GColorBlack);
    text_layer_set_font(&topbarLayer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(&topbarLayer, GTextAlignmentCenter);

    // day24week
    text_layer_init(&bottombarLayer, GRect(0, 150, 144, 18));
    text_layer_set_text_color(&bottombarLayer, GColorWhite);
    text_layer_set_background_color(&bottombarLayer, GColorBlack);
    text_layer_set_font(&bottombarLayer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(&bottombarLayer, GTextAlignmentCenter);

    // Ensures time is displayed immediately (will break if NULL tick event accessed).
    // (This is why it's a good idea to have a separate routine to do the update itself.)

    PblTm t;
    get_time(&t);
    init_watch(&t);

    layer_add_child(&window.layer, &line3.layer[0].layer);
    layer_add_child(&window.layer, &line3.layer[1].layer);
    layer_add_child(&window.layer, &line2.layer[0].layer);
    layer_add_child(&window.layer, &line2.layer[1].layer);
    layer_add_child(&window.layer, &line1.layer[0].layer);
    layer_add_child(&window.layer, &line1.layer[1].layer);
    layer_add_child(&window.layer, &bottombarLayer.layer);
    layer_add_child(&window.layer, &topbarLayer.layer);
}

// Called once per minute
void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {
    (void)ctx;

    if (busy_animating_out || busy_animating_in)
        return;

    update_watch(t->tick_time);
}

// The main event/run loop for our app
void pbl_main(void *params) {
    PebbleAppHandlers handlers = {
        // Handle app start
        .init_handler = &handle_init_app,
        // Handle time updates
        .tick_info = {
            .tick_handler = &handle_minute_tick,
            .tick_units = MINUTE_UNIT
        }
    };
    app_event_loop(params, &handlers);
}

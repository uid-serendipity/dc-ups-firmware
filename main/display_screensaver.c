#include "display_screensaver.h"

#include <stdint.h>

#include <esp_random.h>

#include "battery_gauge.h"
#include "event_bus.h"
#include "power_path.h"
#include "scheduler.h"
#include "util.h"

#define MOVE_INTERVAL_MS 5000

static gui_container_t screensaver;
static gui_label_t screensaver_soc_label;
static char screensaver_soc_text[10];
static gui_label_t screensaver_power_label;
static char screensaver_power_text[10];

static scheduler_task_t screensaver_move_task;

static gui_t *gui;

event_bus_handler_t event_hander_battery_gauge;
event_bus_handler_t event_hander_power_path;

static void on_battery_gauge_event(void *priv, void *data) {
	gui_t *gui = priv;
	unsigned int soc;

	gui_lock(gui);
	soc = battery_gauge_get_soc_percent();
	snprintf(screensaver_soc_text, sizeof(screensaver_soc_text), "%u%%", soc);
	gui_label_set_text(&screensaver_soc_label, screensaver_soc_text);
	gui_unlock(gui);
}

static void on_power_path_event(void *priv, void *data) {
	gui_t *gui = priv;
	unsigned long power_mw = power_path_get_output_power_consumption_mw();

	gui_lock(gui);
	snprintf(screensaver_power_text, sizeof(screensaver_power_text), "%.2fW", power_mw / 1000.f);
	gui_label_set_text(&screensaver_power_label, screensaver_power_text);
	gui_unlock(gui);
}

static void screensaver_move_cb(void *ctx);
static void screensaver_move_cb(void *ctx) {
	uint32_t x, y;

	gui_lock(gui);
	x = esp_random() % (64 - screensaver.element.area.size.x);
	y = esp_random() % (48 - screensaver.element.area.size.y);
	gui_element_set_position(&screensaver.element, x, y);
	gui_unlock(gui);

	scheduler_schedule_task_relative(&screensaver_move_task, screensaver_move_cb, NULL, MS_TO_US(MOVE_INTERVAL_MS));
}

static const display_screen_t screensaver_screen = {
	.name = NULL,
	.show = display_screensaver_show,
	.hide = display_screensaver_hide
};

const display_screen_t *display_screensaver_init(gui_t *gui_root) {
	gui = gui_root;

	gui_container_init(&screensaver);
	gui_element_set_size(&screensaver.element, 20, 12);
	gui_element_set_hidden(&screensaver.element, true);
	gui_element_add_child(&gui->container.element, &screensaver.element);

	gui_label_init(&screensaver_soc_label, "<SOC>");
	gui_label_set_text_alignment(&screensaver_soc_label, GUI_TEXT_ALIGN_CENTER);
	gui_element_set_size(&screensaver_soc_label.element, 20, 5);
	gui_element_add_child(&screensaver.element, &screensaver_soc_label.element);

	gui_label_init(&screensaver_power_label, "<POWER>");
	gui_label_set_text_alignment(&screensaver_power_label, GUI_TEXT_ALIGN_CENTER);
	gui_element_set_size(&screensaver_power_label.element, 20, 5);
	gui_element_set_position(&screensaver_power_label.element, 0, 7);
	gui_element_add_child(&screensaver.element, &screensaver_power_label.element);

	scheduler_task_init(&screensaver_move_task);
	scheduler_schedule_task_relative(&screensaver_move_task, screensaver_move_cb, NULL, MS_TO_US(MOVE_INTERVAL_MS));

	event_bus_subscribe(&event_hander_battery_gauge, "battery_gauge", on_battery_gauge_event, gui);
	event_bus_subscribe(&event_hander_power_path, "power_path", on_power_path_event, gui);
	return &screensaver_screen;
}

void display_screensaver_show() {
	gui_element_set_hidden(&screensaver.element, false);
	gui_element_show(&screensaver.element);
}

void display_screensaver_hide() {
	gui_element_set_hidden(&screensaver.element, true);
}

/*
 * Example of using esp-homekit library to control
 * a Gosund SP111 using HomeKit.
 * The esp-wifi-config library is also used in this
 * example. This means you don't have to specify
 * your network's SSID and password before building.
 *
 * In order to flash the gosund SP111 you will have to
 * have a 3,3v (logic level) FTDI adapter.
 *
 * To flash this example connect 3,3v, TX, RX, GND
 * by following this tutorial https://www.bastelbunker.de/gosund-sp111-mit-tasmota/
 *
 * WARNING: Do not connect the sonoff to AC while it's
 * connected to the FTDI adapter! This may fry your
 * computer and gosund outlet.
 *
 */

#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <wifi_config.h>

#include "button.h"

// The GPIO pin that is connected to the relay on the Gosund SP111.
const int relay_gpio = 15;

// The GPIO pin that is connected to the button on the Gosund SP111.
const int button_gpio = 13;

// The GPIO pin that is connected to the red LED on the Gosund SP111.
const int led_gpio_red = 0;

// The GPIO pin that is connected to the blue LED on the Gosund SP111.
const int led_gpio_blue = 2;

void switch_on_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context);
void button_callback(uint8_t gpio, button_event_t event);

void relay_write(bool on) {
    gpio_write(relay_gpio, on ? 1 : 0);
}

void led_blue_write(bool on) {
  gpio_write(led_gpio_blue, on ? 0 : 1);
}

void led_red_write(bool on) {
  gpio_write(led_gpio_red, on ? 0 : 1);
}

void blink_red_led() {
  for (int i=0; i<3; i++) {
      for (int j=0; j<2; j++) {
          led_red_write(true);
          vTaskDelay(100 / portTICK_PERIOD_MS);
          led_red_write(false);
          vTaskDelay(100 / portTICK_PERIOD_MS);
      }

      vTaskDelay(250 / portTICK_PERIOD_MS);
  }

  led_red_write(false);
  vTaskDelete(NULL);
}

void reset_configuration_task() {
    //Flash the LED first before we start the reset
    for (int i=0; i<3; i++) {
        led_blue_write(true);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        led_blue_write(false);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    printf("Resetting Wifi Config\n");

    wifi_config_reset();

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    printf("Resetting HomeKit Config\n");

    homekit_server_reset();

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    printf("Restarting\n");

    sdk_system_restart();

    vTaskDelete(NULL);
}

void reset_configuration() {
    printf("Resetting Gosund SP111 configuration\n");
    xTaskCreate(reset_configuration_task, "Reset configuration", 256, NULL, 2, NULL);
}

homekit_characteristic_t switch_on = HOMEKIT_CHARACTERISTIC_(
    ON, false, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(switch_on_callback)
);

void gpio_init() {
    gpio_enable(button_gpio, GPIO_INPUT);

    gpio_enable(led_gpio_blue, GPIO_OUTPUT);
    led_blue_write(true);

    gpio_enable(led_gpio_red, GPIO_OUTPUT);
    led_red_write(false);

    gpio_enable(relay_gpio, GPIO_OUTPUT);
    relay_write(switch_on.value.bool_value);
}

void switch_on_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
    led_blue_write(!switch_on.value.bool_value);
    led_red_write(switch_on.value.bool_value);
    relay_write(switch_on.value.bool_value);
}

void button_callback(uint8_t gpio, button_event_t event) {
    switch (event) {
        case button_event_single_press:
            printf("Toggling relay\n");
            switch_on.value.bool_value = !switch_on.value.bool_value;

            led_blue_write(!switch_on.value.bool_value);
            led_red_write(switch_on.value.bool_value);
            relay_write(switch_on.value.bool_value);

            homekit_characteristic_notify(&switch_on, switch_on.value);
            break;
        case button_event_long_press:
            blink_red_led();
            reset_configuration();
            break;
        default:
            printf("Unknown button event: %d\n", event);
    }
}

void switch_identify_task(void *_args) {
    // We identify the Sonoff by Flashing it's LED.
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            led_blue_write(true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            led_blue_write(false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    led_blue_write(false);

    vTaskDelete(NULL);
}

void switch_identify(homekit_value_t _value) {
    printf("Switch identify\n");
    xTaskCreate(switch_identify_task, "Switch identify", 128, NULL, 2, NULL);
}

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, "Gosund Outlet");

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_outlet, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            &name,
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Gosund"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "03A11122333"),
            HOMEKIT_CHARACTERISTIC(MODEL, "SP111"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, switch_identify),
            NULL
        }),
        HOMEKIT_SERVICE(OUTLET, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Gosund Outlet"),
            &switch_on,
	          HOMEKIT_CHARACTERISTIC(OUTLET_IN_USE, true),
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-22-333"
};

void on_wifi_ready() {
    homekit_server_init(&config);
}

void create_accessory_name() {
    uint8_t macaddr[6];
    sdk_wifi_get_macaddr(STATION_IF, macaddr);

    int name_len = snprintf(NULL, 0, "Gosund Outlet-%02X%02X%02X", macaddr[3], macaddr[4], macaddr[5]);
    char *name_value = malloc(name_len+1);
    snprintf(name_value, name_len+1, "Gosund Outlet-%02X%02X%02X", macaddr[3], macaddr[4], macaddr[5]);
    name.value = HOMEKIT_STRING(name_value);
}

void user_init(void) {
    uart_set_baud(0, 115200);

    create_accessory_name();

    wifi_config_init("gosund-outlet", NULL, on_wifi_ready);
    gpio_init();

    if (button_create(button_gpio, 0, 10000, button_callback)) {
        printf("Failed to initialize button\n");
    }
}

#ifndef LED_H
#define LED_H
 
#include <hardware/hardware.h>
#include <fcntl.h>
#include <errno.h>
#include <cutils/log.h>
#include <cutils/atomic.h>

/*定义模块ID*/
#define LED_HARDWARE_MODULE_ID "led"

/*硬件模块结构体*/ 
struct led_module_t {
	struct hw_module_t common;
};

/*硬件接口结构体*/
struct led_control_device_t {
	struct hw_device_t common;
	int fd;
 
	int (*set_on)(struct led_control_device_t *dev, int led);
	int (*set_off)(struct led_control_device_t *dev, int led);
 
};
#endif

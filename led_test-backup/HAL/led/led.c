
#define LOG_TAG "LedStub"
 
#include <hardware/hardware.h>
#include <hardware/led.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
 
#define LED_ON  1
#define LED_OFF 0

#define DEVICE_NAME "/dev/led"
 
int led_device_close(struct hw_device_t* device){
	struct led_control_device_t *led_dev = (struct led_control_device_t*)device;
	
	if(led_dev){
	  close(led_dev->fd);
	  free(led_dev);
	}
 
	return 0;
}
 
int led_on(struct led_control_device_t *dev, int led){
	ALOGI("---czd--- LED set %d on", led);
	ioctl(dev->fd, LED_ON, NULL);
	return 0;
}
 
int led_off(struct led_control_device_t *dev, int led){
	ALOGI("---czd--- LED set %d off", led);
	ioctl(dev->fd, LED_OFF, NULL);
	return 0;
}
 
static int led_device_open(const hw_module_t* module, const char* name,
			hw_device_t** device){
	struct led_control_device_t *dev;
 
	dev = calloc(1, sizeof(struct led_control_device_t));
	if(!dev){
		ALOGE("---czd--- led device open error");	
		return -ENOMEM;
	}
 
	dev->common.tag     = HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module  = (struct hw_module_t *)module;
	dev->common.close   = led_device_close;
 
	dev->set_on  = led_on;
	dev->set_off = led_off;
 
 
	dev->fd = open(DEVICE_NAME, O_RDWR);
	if(dev->fd <0){
		ALOGE("---czd--- open /dev/led error");
	}else
	  ALOGI("---czd--- open /dev/led success");
	
	*device = &dev->common;
	return 0;
}
 
static struct hw_module_methods_t led_module_methods = {
	.open = led_device_open,
};
 
struct led_module_t HAL_MODULE_INFO_SYM = {
	.common = {
		.tag = HARDWARE_MODULE_TAG,
		.version_major = 1,
		.version_minor = 0,
		.id = LED_HARDWARE_MODULE_ID,
		.name = "LED FLASH HW HAL",
		.author = "The Android Open Source Project",
		.methods = &led_module_methods,
	},
};

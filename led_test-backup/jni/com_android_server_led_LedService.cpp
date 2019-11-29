#define LOG_TAG "LedStub"
 
#include "jni.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"
 
#include <utils/misc.h>
#include <utils/Log.h>
#include <hardware/hardware.h>
#include <hardware/led.h>
 
#include <stdio.h>
 
namespace android {
	/*在硬件抽象层中定义的硬件访问结构体，参考<hardware/led_hal.h>*/	
	struct led_control_device_t *sLedDevice = NULL;
	/*通过调用硬件抽象层定义的硬件访问接口set_on来封装led_on设置打开led*/
	static jboolean led_on(JNIEnv *env, jobject clazz, jint led){
		ALOGI("---czd--- LedService JNI:led_on");
		if(sLedDevice==NULL){
			ALOGE("---czd--- LedService JNI: led_on() is invoked");
			return -1;
		}else
		  return sLedDevice->set_on(sLedDevice,led);
	}

	/*通过调用硬件抽象层定义的硬件访问接口set_off来封装led_off设置关闭led*/
	static jboolean led_off(JNIEnv *env, jobject clazz, jint led){
		ALOGI("---czd--- LedService JNI:led_off");
		if(sLedDevice==NULL){
			ALOGE("---czd--- LedService JNI: led_off() is invoked");
			return -1;
		}else
		  return sLedDevice->set_off(sLedDevice,led);
	}

	/*通过硬件抽象层定义的硬件模块打开接口打开硬件设备*/
	static inline int led_control_open(const struct hw_module_t *module, struct led_control_device_t** device){
		return module->methods->open(module, LED_HARDWARE_MODULE_ID, 
					(struct hw_device_t**)device);
	}

	/*通过硬件模块ID来加载指定的硬件抽象层模块并打开硬件*/
	static jboolean led_init(JNIEnv *env, jclass clazz){
		led_module_t* module;
		ALOGI("---czd--- led jni init......");
		if(hw_get_module(LED_HARDWARE_MODULE_ID, (const struct hw_module_t**)&module) == 0){
			ALOGI("---czd--- Led JNI: led stub found.");
			if(led_control_open(&module->common, &sLedDevice) == 0){
				ALOGI("---czd--- JNI: led device is open");
				return 0;
			}
			ALOGE("---czd--- LED JNI: failed to open led device");
			return -1;
		}
		ALOGE("---czd--- LED JNI: failed to get led stub module");
		return -1;
	}
 
	static const JNINativeMethod method_table[] = {
		{"init_native","()Z", (void*)led_init},
		{"led_on", "(I)Z", (void*)led_on},
		{"led_off", "(I)Z", (void*)led_off},
	};
 
	int register_android_server_LedService(JNIEnv *env){
	
		return jniRegisterNativeMethods(env, "com/android/server/led/LedService", method_table, NELEM(method_table));
	}
};

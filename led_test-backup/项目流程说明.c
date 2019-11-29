玩了安卓这么久了，你是否有好奇，它的底层到上层或者上层到底层的标准流程是怎么走的呢？这里通过操作一个GPIO控制led实现从上层到底层的完整流程
Android Version:v8.0.1
Linux Version:V4.4
Hardware:mtk6739

1、在kernel层,编写和添加驱动和编译文件Malefie，创建设备节点文件/dev/led

目录：kernel-4.4/drivers
新建led_test.c


2、在HAL层,增加模块接口,以访问驱动程序
主要是操作驱动程序,open,ioctl,close
生成led.default.so 在system/lib/hw/目录下

在hardware/libhardware/include/hardware目录新建led.h文件
hardware/libhardware/include/hardware/led.h

在modules目录新建led目录，添加以下文件：
hardware/libhardware/modules/led/led.c
hardware/libhardware/modules/led/Andorid.mk
执行编译命令：mmm hardware/libhardware/modules/led/,编译得到led.default.so文件

在 led_init 函数中，通过Android硬件抽象层提供的hw_get_module方法来加载模块 LED_HARDWARE_MODULE_ID 的硬件抽象层模块，
其中，LED_HARDWARE_MODULE_ID 是在<hardware/led.h>中定义的。Android硬件抽象层会根据 LED_HARDWARE_MODULE_ID 的值
在Android系统的/system/lib/hw目录中找到相应的模块，然后加载起来，并且返回hw_module_t接口给调用者使用。



3、JNI层,编写JNI方法,在应用程序框架层提供Java接口访问硬件
frameworks/base/services/core/jni/com_android_server_led_LedService.cpp
生成文件打包进system/lib/libandroid_servers.so
Android系统初始化时,会自动加载JNI方法.

在frameworks/base/services/core/jni/添加com_android_server_led_LedService.cpp文件
在com_android_server_led_LedService.cpp文件中，实现JNI方法。注意文件的命令方法，com_android_server_led前缀表示的是包名，
表示硬件服务LedService是放在frameworks/base/services/core/java/目录下的com/android/server/led目录的，即存在一个命令
为com.android.server.led.LedService的类。
简单地说，LedService是一个提供Java接口的硬件访问服务类。

然后修改同目录下的onload.cpp文件，首先在namespace android增加register_android_server_LedService函数声明，这样，
在Android系统初始化时，就会自动加载该JNI方法调用表。

然后修改同目录下的Android.mk文件，在LOCAL_SRC_FILES变量中增加一行：
com_android_server_led_LedService.cpp \


4、framework层,应用程序框架层,增加硬件服务接口
jni在注册方法时用到com/android/server/led/LedService
所以在frameworks/base/services/core/java/com/android/server/目录下新建led文件夹，添加LedService.java文件

在Android系统中，硬件服务一般是运行在一个独立的进程中为各种应用程序提供服务。
因此，调用这些硬件服务的应用程序与这些硬件服务之间的通信需要通过代理来进行创建aidl文件来描述通信接口
在frameworks/base/core/java/android/os/目录新建ILedService.aidl文件

然后修改frameworks/base/services/core/jni/Android.mk文件，在LOCAL_SRC_FILES += \ 增加
+       $(LOCAL_REL_DIR)/com_android_server_led_LedService.cpp \

在frameworks/base/Android.mk里增加对aidl的编译项
LOCAL_SRC_FILES += \
+core/java/android/os/ILedService.aidl \

编译后会在out目录下产生ILedService.java文件
out/target/common/obj/JAVA_LIBRARIES/framework_intermediates/src/core/java/android/os/ILedService.java


5、通过Manager调用Service
Manager管理service,在注册服务的时候就实例化一个Manager,App获取服务getService也就获取了这个对象
添加frameworks/base/core/java/android/os/LedManager.java文件

6、添加注册服务,addService开机服务就可以启动

通过service list 命令查看服务是否启动

# service list | grep led                                          
35    led: [android.os.ILedService]

具体文件修改如下：
--- a/frameworks/base/services/java/com/android/server/SystemServer.java
+++ b/frameworks/base/services/java/com/android/server/SystemServer.java
@@ -103,6 +103,7 @@ import com.android.server.usage.UsageStatsService;
 import com.android.server.vr.VrManagerService;
 import com.android.server.webkit.WebViewUpdateService;
 import com.android.server.wm.WindowManagerService;
+import com.android.server.led.LedService;
 
 import dalvik.system.VMRuntime;
 
@@ -516,6 +517,7 @@ public final class SystemServer {
      */
     private void startOtherServices() {
         final Context context = mSystemContext;
+               LedService led = null;
         VibratorService vibrator = null;
         IMountService mountService = null;
         NetworkManagementService networkManagement = null;
@@ -987,6 +989,15 @@ public final class SystemServer {
                     }
                     Trace.traceEnd(Trace.TRACE_TAG_SYSTEM_SERVER);
                 }
+                               
+                               try{
+                                       Slog.i(TAG,"Led Service");
+                                       led = new LedService(context);
+                                       ServiceManager.addService(Context.LED_SERVICE, led);
+                                       Slog.i(TAG,"Led Service-----");
+                               }catch(Throwable e){
+                                       Slog.e(TAG,"Failure starting LedService", e);
+                               }
 
--- a/frameworks/base/core/java/android/content/Context.java
+++ b/frameworks/base/core/java/android/content/Context.java
@@ -3431,6 +3431,15 @@ public abstract class Context {
     public static final String SERIAL_SERVICE = "serial";
 
     /**
+     * Use with {@link #getSystemService} to retrieve a {@link
+     * android.os.LedManager} for access to led ports.
+     *
+     * @see #getSystemService
+     *
+     * @hide
+     */
+    public static final String LED_SERVICE = "led";
 
注册服务
--- a/frameworks/base/core/java/android/app/SystemServiceRegistry.java
+++ b/frameworks/base/core/java/android/app/SystemServiceRegistry.java
@@ -127,6 +127,8 @@ import android.view.accessibility.AccessibilityManager;
 import android.view.accessibility.CaptioningManager;
 import android.view.inputmethod.InputMethodManager;
 import android.view.textservice.TextServicesManager;
+import android.os.LedManager;
+import android.os.ILedService;
 
 import java.util.HashMap;
 
@@ -480,6 +482,14 @@ final class SystemServiceRegistry {
                 return new SerialManager(ctx, ISerialManager.Stub.asInterface(b));
             }});
 
+        registerService(Context.LED_SERVICE, LedManager.class,
+                new CachedServiceFetcher<LedManager>() {
+            @Override
+            public LedManager createService(ContextImpl ctx) {
+                IBinder b = ServiceManager.getService(Context.LED_SERVICE);
+                return new LedManager(ctx, ILedService.Stub.asInterface(b));
+            }});
+


7、更新API
make update-api -j8


8、修改节点权限

Android在Selinux下要获取对内核节点访问的权限,需要修改.te文件(根据各个平台不同而异，这里是飞思卡尔平台的，我在其后添加mtk平台的权限)

1)为/dev/led节点定义一个名字led_device

--- a/device/fsl/imx6/sepolicy/file_contexts
+++ b/device/fsl/imx6/sepolicy/file_contexts
@@ -8,6 +8,7 @@
 /dev/ttymxc[0-9]*               u:object_r:tty_device:s0
 /dev/ttyUSB[0-9]*               u:object_r:tty_device:s0 
 /dev/mma8x5x                    u:object_r:sensors_device:s0
+/dev/led                        u:object_r:led_device:s0


2)将led_device声明为dev_type

--- a/device/fsl/imx6/sepolicy/device.te
+++ b/device/fsl/imx6/sepolicy/device.te
@@ -1,2 +1,3 @@
 type caam_device, dev_type;
 type pxp_device, dev_type;
+type led_device,dev_type;

3)修改读写权限

--- a/device/fsl/imx6/sepolicy/system_server.te
+++ b/device/fsl/imx6/sepolicy/system_server.te
@@ -4,3 +4,4 @@ allow system_server system_data_file:file {relabelto rw_file_perms};
 allow system_server system_data_file:dir {relabelto rw_dir_perms};
 allow system_server kernel:system { syslog_read };
 allow system_server debugfs_tracing:file { write };
+allow system_server led_device:chr_file { open read write ioctl };

4)修改linux下节点自身的权限

--- a/device/fsl/imx6/etc/ueventd.freescale.rc
+++ b/device/fsl/imx6/etc/ueventd.freescale.rc
@@ -1,3 +1,4 @@
+/dev/led                  0660   system     system

5)增加服务的权限【这个在mtk添加编译没通过，所以不这样做】

--- a/system/sepolicy/service_contexts
+++ b/system/sepolicy/service_contexts
@@ -116,6 +116,7 @@ scheduling_policy                         u:object_r:scheduling_policy_service:s
 search                                    u:object_r:search_service:s0
 sensorservice                             u:object_r:sensorservice_service:s0
 serial                                    u:object_r:serial_service:s0
+led                                       u:object_r:led_service:s0

--- a/system/sepolicy/service.te
+++ b/system/sepolicy/service.te
@@ -96,6 +96,7 @@ type rttmanager_service, app_api_service, system_server_service, service_manager
 type samplingprofiler_service, system_server_service, service_manager_type;
 type scheduling_policy_service, system_server_service, service_manager_type;
 type search_service, app_api_service, system_server_service, service_manager_type;
+type led_service, app_api_service, system_server_service, service_manager_type;


MTK平台的修改节点权限（android8.1）：

(1)修改linux下节点自身的权限
--- a/device/mediatek/mt6739/init.mt6739.rc
+++ b/device/mediatek/mt6739/init.mt6739.rc
@@ -807,7 +807,13 @@ on boot
 #added for sunwave finger tee hidl
    chown system system /dev/sunwave_fp
    chmod 0660          /dev/sunwave_fp
-   
+
+#added for led by czd
+    chown system system /dev/led
+    chmod 0660 /dev/led
+

(2)为/dev/led节点定义一个名字led_device
--- a/device/mediatek/mt6739/sepolicy/bsp/file_contexts
+++ b/device/mediatek/mt6739/sepolicy/bsp/file_contexts
@@ -9,4 +9,5 @@
 # Data files
 #
 /dev/goodix_fp     u:object_r:fingerprint_device:s0
+/dev/led                        u:object_r:led_device:s0

(3)将led_device声明为dev_type
--- a/device/mediatek/mt6739/sepolicy/basic/device.te
+++ b/device/mediatek/mt6739/sepolicy/basic/device.te
@@ -4,3 +4,4 @@
 
 type gps_emi_device, dev_type;
 type mntl_block_device, dev_type;
+type led_device,dev_type;

(4)修改读写权限
--- a/device/mediatek/mt6739/sepolicy/basic/system_server.te
+++ b/device/mediatek/mt6739/sepolicy/basic/system_server.te
@@ -5,4 +5,6 @@
 # Date : WK15.45
 # Operation : Migration
 # Purpose : for debug
-allow system_server gps_data_file:dir search;
\ No newline at end of file
+allow system_server gps_data_file:dir search;
+allow system_server led_device:chr_file { open read write ioctl getattr };

(5)增加服务的权限
sepolicy 部分添加
参考：
以下测试验证平台MSM8909 android8.1

一. sepolicy 部分添加

1. \system\sepolicy\public\service.te 定义服务名称和属性

type xxx_service,app_api_service, ephemeral_app_api_service, system_server_service, service_manager_type;

2.\system\sepolicy\private\service_contexts 添加服务名称

xxx                      u:object_r:xxx_service:s0

3.\system\sepolicy\private\compat\26.0\26.0.cil 文件最后添加

(typeattributeset xxx_service_26_0 (xxx_service))

4.\system\sepolicy\prebuilts\api\26.0\public\service.te 定义服务名称和属性

type xxx_service,app_api_service, ephemeral_app_api_service, system_server_service, service_manager_type;

5.\system\sepolicy\prebuilts\api\26.0\private\service_contexts 添加服务名称

xxx                      u:object_r:xxx_service:s0

6.\system\sepolicy\prebuilts\api\26.0\nonplat_sepolicy.cil 添加相应配置

6.1 typeattributeset system_server_service 在最后添加自定义的服务 xxx_service_26_0

6.2 typeattributeset app_api_service  在最后添加自定义的服务 xxx_service_26_0

6.3 typeattributeset ephemeral_app_api_service  在最后添加自定义的服务 xxx_service_26_0

6.4 typeattributeset service_manager_type  在最后添加自定义的服务 xxx_service_26_0

6.5 添加一对 配置（参考已有的例子）

(typeattribute xxx_service_26_0)
(roletype object_r xxx_service_26_0)





9、编译更新系统镜像到机器emmc（flash）
make -j8 2>&1 | tee make.log


10、android app 测试
10.1 eclipse做法：
将out/target/common/obj/JAVA_LIBRARIES/framework_intermediates/classes.jar里的
android/os/LedManager.class
导入eclipse 的sdk/platforms/android-xx/android.jar包中
的android/os目录

10.2 Android stadio做法：
将out/target/common/obj/JAVA_LIBRARIES/framework_intermediates/classes.jar导入lib目录

11、总结

APP调用了LedManager里的LedOn LedOff
LedManager服务通过aidl调用到LedService.java里的setOn setOff
LedService通过jni提供的接口调用led_on led_off
private static native boolean led_on(int led);
private static native boolean led_off(int led);
jni注册的方法表
{"led_on", "(I)Z", (void*)led_on},
{"led_off", "(I)Z", (void*)led_off},
jni层调用 led_on----->return sLedDevice->set_on(sLedDevice,led);
      led_off---->return sLedDevice->set_off(sLedDevice,led);

调用到硬件抽象层led.default.so里的
    dev->set_on  = led_on;
    dev->set_off = led_off;
led_on/led_off通过ioctl调用到驱动调置gpio的高低电平


参考来自网上:
http://www.cnblogs.com/hackfun/p/7418902.html
http://blog.csdn.net/baiduluckyboy/article/details/6973015
http://blog.csdn.net/luoshengyang/article/details/6567257
--------------------- 
https://blog.csdn.net/shui1025701856/article/details/78480776 











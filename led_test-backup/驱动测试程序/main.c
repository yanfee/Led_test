/*
 * main.c : test demo driver
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#define LED_ON 1
#define LED_OFF 0

#define LED_TEST_DEVICE "/dev/led_test"

int main()

{
      int i = 0;
      int fd;

      fd = open(LED_TEST_DEVICE,O_RDWR | O_NONBLOCK);//打开设备文件
      if ( fd == -1 ) {
               printf("Can't open file /dev/led_test\n");
               exit(1);
      }
      while(1)
      {
      ioctl(fd,LED_ON,0);//发送ioctl命令LED_ON
     printf("---czd--- led power on\n");
      sleep(1);
      ioctl(fd,LED_OFF,0); //发送ioctl命令LED_OFF
     printf("---czd--- led power off\n");
      sleep(1);
}
return 0;
}


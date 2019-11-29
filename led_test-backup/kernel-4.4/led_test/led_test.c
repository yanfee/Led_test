#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>

#define BUFSIZE  1024 

/*ioctl*/
#define LED_ON 1
#define LED_OFF 0

/*主设备和从设备号变量*/
static int test_major = 0;
static int test_minor = 0;

/*设备类别和设备变量*/
static struct cdev cdev;
static struct class *test_class = NULL;
static struct device *test_class_dev = NULL;

int number_of_devices = 1;
static dev_t dev = 0;
static char *buf;
static unsigned int len;
static unsigned int gpio_led;

/*从dts中解析出gpio口*/
static int get_gpio_number(void)
{
    struct device_node *node = NULL;
    int gpio = 0;
	
	printk("---czd--- Enter %s\n",__func__);
    node = of_find_compatible_node(NULL, NULL, "led_node");
    if (node == NULL) {
       printk("%s:node:led_node not find!\n", __func__);
       return -EFAULT;
    }
    gpio = of_get_named_gpio(node, "gpio_num", 0);
    return gpio;
}

/*打开设备方法*/
static int test_open(struct inode *inode, struct file *filp) {

	int ret;

	printk("---czd--- Enter %s\n",__func__);
	gpio_led = get_gpio_number();
	if (gpio_is_valid(gpio_led)){
	ret = gpio_request(gpio_led,"gpio_led");
	if(ret){
		printk("---czd--- failed to get %d gpio.\n", gpio_led);
		return -1;
		}
	}
	else gpio_led = -1;
    gpio_direction_output(gpio_led,1);
	printk("---czd--- gpio is valid and pull up now\n");

	return 0;
}

/*设备文件释放时调用*/
static int test_release(struct inode *inode, struct file *filp) {

	return 0;
}

/************************
 * file_operations->read
 * 可以在adb工具进入机器的pro目录，执行adb shell && cd proc && cat tets_rw，即可读出节点test_rw的内容是12345
 * 函数原型：static ssize_t read(struct file *file, char *buf, size_t count, loff_t *ppos)
 ************************/

static ssize_t test_read(struct file *file, char __user *buffer,size_t count, loff_t *f_pos) 
{
	if(*f_pos > 0)
		return 0;
 
	printk("---start read---\n");
	printk("the string is >>>>> %s\n", buf);
 
	if(copy_to_user(buffer, buf, len))
		return -EFAULT;
	*f_pos = *f_pos + len;
	return len;
}


/************************
 * file_operations->write
 * 可以在adb工具进入机器的pro目录，执行adb shell && cd proc && echo 12345 > tets_rw，即可把12345写入节点test_rw
 * 函数原型：static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *ppos)
 ************************/

static ssize_t test_write(struct file *file, const char __user *buffer,size_t count, loff_t *f_pos) 
{
	
	if(count <= 0)
		return -EFAULT;
	printk("---start write---\n");
	
	len = count > BUFSIZE ? BUFSIZE : count;
 
	// kfree memory by kmalloc before
	if(buf != NULL)
		kfree(buf);
	buf = (char*)kmalloc(len+1, GFP_KERNEL);
	if(buf == NULL)
	{
		printk("device_create kmalloc fail!\n");
		return -EFAULT;
	}
 
	//memset(buf, 0, sizeof(buf));
	memset(buf, 0, len+1);
 
	if(copy_from_user(buf, buffer, len))
		return -EFAULT;
	printk("device_create writing :%s",buf);
	return len;
}


static long test_ioctl(struct file *filp,unsigned int cmd,unsigned long arg)
{

    printk("---czd--- Enter %s\n",__func__);
	printk("-----czd--- kernel cmd is : %d\n",cmd);

    switch(cmd)
    {
    case LED_ON:
        printk("---czd--- you set gpio up\n");
//		gpio_set_value(gpio_led, 1);
        break;
    
	case LED_OFF:
        printk("---czd--- you set gpio down\n");
//		gpio_set_value(gpio_led, 0);
        break;
    default:
        return -EINVAL;
    }
    return 0;
}



/*设备文件操作方法表*/
static struct file_operations test_fops = {
	.owner = THIS_MODULE,
	.open = test_open,
	.release = test_release,
	.read = test_read,
	.write = test_write, 
	.unlocked_ioctl = test_ioctl,
};



static void test_setup_cdev(void)
	{
	int error;
	dev_t devno = MKDEV(test_major, test_minor);

	cdev_init (&cdev, &test_fops);
	cdev.owner = THIS_MODULE;
	cdev.ops = &test_fops;        
 
	/*注册字符设备*/
	error = cdev_add (&cdev, devno, 1);
	if(error)
	printk("Error %d adding test_setup_cdev", error);  

	}



static int __init test_init(void)
    {		
	int err = -1;
 
	printk("---czd--- Initializing test device.\n");        
 
	/*动态分配主设备和从设备号*/
	err = alloc_chrdev_region(&dev, 0, number_of_devices, "led");
	if(err < 0) {
		printk("Failed to alloc char dev region.\n");
		return err;
	}
 
	test_major = MAJOR(dev);
	test_minor = MINOR(dev);
	printk("---czd--- test_major ==> %d ; test_minor ==> %d \n",test_major,test_minor);


	/*初始化设备*/
	test_setup_cdev();       
 
	/*在/sys/class/目录下创建设备类test_class*/
	test_class = class_create(THIS_MODULE, "led");
	if(IS_ERR(test_class)) 
        {
            printk("Err: failed in creating class.\n");
            return -1; 
        }       


	/*在/dev/目录和/sys/class/test目录下分别创建设备文件test*/
	test_class_dev = device_create(test_class, NULL, MKDEV(test_major, 0), NULL, "led");
	if(IS_ERR(test_class_dev)) 
        {
		printk("Err: failed in creating device.\n");
		return -1; 
        }

	printk("---czd--- Succedded to initialize test device.\n");  
	return 0;
 
    }


/*模块卸载方法*/
static void __exit test_exit(void)
    {
	dev_t devno = MKDEV (test_major, test_minor);

	cdev_del(&cdev);
	device_destroy(test_class, devno); //delete device node under /dev
	class_destroy(test_class); //delete class created by us
	unregister_chrdev_region (devno, number_of_devices);
	gpio_free(gpio_led);
	printk ("Char driver test_exit.\n");

    }



module_init(test_init);
module_exit(test_exit);

MODULE_AUTHOR("caizhongding");
MODULE_DESCRIPTION("Device_create Driver");
MODULE_LICENSE("GPL");

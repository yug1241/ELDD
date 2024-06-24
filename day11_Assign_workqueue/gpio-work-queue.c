// Q.1. Blink LED when switch is pressed. Use work queues.
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include<linux/delay.h> //for msleep

#define LED_GPIO 	49
#define SWITCH_GPIO 	115

static ssize_t yug_GPIO_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t yug_GPIO_write(struct file *, const char __user *, size_t, loff_t *);
static int yug_GPIO_open(struct inode *, struct file *);
static int yug_GPIO_close(struct inode *, struct file *);

#define MAX 32
static int led_state;
static int major;
static dev_t devno;
static struct class *pclass;
static struct cdev cdev;
static int irq;
// static struct tasklet_struct tasklet;
static struct work_struct work_que;

static struct file_operations f_ops = {

	.owner = THIS_MODULE,
	.read = yug_GPIO_read,
	.write = yug_GPIO_write,
	.open = yug_GPIO_open,
	.release = yug_GPIO_close,
	//.llseek = yug_GPIO_lseek
};

/*static void tasklet_handler(unsigned long param){

led_state = !led_state;
gpio_set_value(LED_GPIO,led_state);
printk(kern_info"%s : switch_isr() called\n",THIS_MODULE->name);

}*/

static void work_que_handler(struct work_struct *param)
{

	int i, led_state = 1; // led chalu keli ki on mhanun sate 1 mg switch press vr blink
	for (i = 1; i <= 10; i++) // blink led for 10 sec sathi add kela
	{
		led_state = !led_state;
		gpio_set_value(LED_GPIO, led_state);
		//led_state = !led_state;
		msleep(100); 
	}
}
static irqreturn_t switch_isr(int irq, void *param)
{

	printk(KERN_INFO "%s : switch_isr() called\n", THIS_MODULE->name);
	// tasklet_schedule(&tasklet);
	schedule_work(&work_que);
	return IRQ_HANDLED;
}

static __init int yug_GPIO_init(void)
{

	int ret, minor;
	bool valid;
	static struct device *pdevice;

	printk(KERN_INFO "%s : yug_GPIO_init() called\n", THIS_MODULE->name);

	ret = alloc_chrdev_region(&devno, 0, 1, "pchar");
	if (ret < 0)
	{

		printk(KERN_ERR "%s : alloc_chrdev_region() failed\n", THIS_MODULE->name);
		goto alloc_chrdev_region_failed;
	}
	major = MAJOR(devno);
	minor = MINOR(devno);

	printk(KERN_INFO "%s : alloc_chrdev_region() is successful : device nummber = %d/%d\n", THIS_MODULE->name, major, minor);

	pclass = class_create(THIS_MODULE, "yug_GPIO_class");
	if (IS_ERR(pclass))
	{

		printk(KERN_ERR "%s : class_create() failed\n", THIS_MODULE->name);
		// unregister_chrdev_region(devno,1);
		ret = -1;
		goto class_create_failed;
	}
	printk(KERN_INFO "%s : class_create() is successful\n", THIS_MODULE->name);

	pdevice = device_create(pclass, NULL, devno, NULL, "yug_GPIO_device_%d", 0);
	if (IS_ERR(pdevice))
	{

		printk(KERN_ERR "%s : device_create() failed\n", THIS_MODULE->name);
		// class_destroy(pclass,devno);
		// unregister_chrdev_region(devno,1);
		ret = -1;
		goto device_create_failed;
	}

	cdev_init(&cdev, &f_ops);
	ret = cdev_add(&cdev, devno, 1);
	if (ret < 0)
	{

		printk(KERN_ERR "%s : cdev_add() is failed\n", THIS_MODULE->name);
		goto cdev_add_failed;
	}
	printk(KERN_INFO "%s : cdev_add() successful\n", THIS_MODULE->name);

	valid = gpio_is_valid(LED_GPIO);
	if (!valid)
	{

		printk(KERN_ERR "%s : GPIO pin %d dosent exists \n", THIS_MODULE->name, LED_GPIO);
		ret = -1;
		goto gpio_invalid;
	}
	printk(KERN_INFO "%s : GPIO pin %d exists\n", THIS_MODULE->name, LED_GPIO);

	ret = gpio_request(LED_GPIO, "yug-led");
	if (ret != 0)
	{

		printk(KERN_ERR "%s : gpio pin %d is busy \n", THIS_MODULE->name, LED_GPIO);
		goto gpio_invalid;
	}
	printk(KERN_INFO "%s : gpio pin %d is aquired \n", THIS_MODULE->name, LED_GPIO);

	led_state = 1;
	ret = gpio_direction_output(LED_GPIO, led_state);
	if (ret != 0)
	{

		printk(KERN_ERR "%s : GPIO pin %d direction is not set\n", THIS_MODULE->name, LED_GPIO);
		goto gpio_direction_failed;
	}
	printk(KERN_INFO "%s : GPIO pin %d direction is set to OUTPUT\n", THIS_MODULE->name, LED_GPIO);

	valid = gpio_is_valid(SWITCH_GPIO);
	if (!valid)
	{

		printk(KERN_INFO "%s : GPIO pin %d dosent exixt\n", THIS_MODULE->name, SWITCH_GPIO);
		ret = -1;
		goto switch_gpio_invalid;
	}
	printk(KERN_INFO "%s : GPIO Pin %d is valid and exists\n", THIS_MODULE->name, SWITCH_GPIO);

	ret = gpio_request(SWITCH_GPIO, "yug-SWITCH");
	if (ret != 0)
	{

		printk(KERN_INFO "%s : GPIO pin %d is busy\n", THIS_MODULE->name, SWITCH_GPIO);
		goto switch_gpio_invalid;
	}
	printk(KERN_INFO "%s : GPIO Pin %d is aquired\n", THIS_MODULE->name, SWITCH_GPIO);

	ret = gpio_direction_input(SWITCH_GPIO);
	if (ret != 0)
	{

		printk(KERN_INFO "%s : GPIO pin %d direction not set\n", THIS_MODULE->name, SWITCH_GPIO);
		goto switch_gpio_direction_failed;
	}
	printk(KERN_INFO "%s : GPIO pin %d direction set to input\n", THIS_MODULE->name, SWITCH_GPIO);

	irq = gpio_to_irq(SWITCH_GPIO);
	ret = request_irq(irq, switch_isr, IRQF_TRIGGER_RISING, "yug_SWITCH", NULL);
	if (ret != 0)
	{

		printk(KERN_ERR "%s : GPIO pin %d ISR registration failed\n", THIS_MODULE->name, SWITCH_GPIO);
		goto switch_gpio_direction_failed;
	}
	printk(KERN_INFO "%s : GPIO pin %d registered ISR on irq line %d\n", THIS_MODULE->name, SWITCH_GPIO, irq);

	// tasklet_init(&tasklet,tasklet_handler,0);
	// printk(KERN_INFO"%s : tasklet is initialised\n",THIS_MODULE->name);

	INIT_WORK(&work_que, work_que_handler);
	printk(KERN_INFO "%s : work_queue is initalised\n", THIS_MODULE->name);

	printk(KERN_INFO "%s : yug_GPIO_init() is finished\n", THIS_MODULE->name);

	return 0;

switch_gpio_direction_failed:
	gpio_free(SWITCH_GPIO);
switch_gpio_invalid:
gpio_direction_failed:
	gpio_free(LED_GPIO);
gpio_invalid:
	cdev_del(&cdev);
cdev_add_failed:
	device_destroy(pclass, devno);
device_create_failed:
	class_destroy(pclass);
class_create_failed:
	unregister_chrdev_region(devno, 1);
alloc_chrdev_region_failed:
	return ret;
}

static __exit void yug_GPIO_exit(void)
{

	printk(KERN_INFO "%s : yug_GPIO_exit() is called \n", THIS_MODULE->name);

	free_irq(irq, NULL);
	printk(KERN_INFO "%s : GPIO pin %d ISR released\n", THIS_MODULE->name, SWITCH_GPIO);

	gpio_free(SWITCH_GPIO);
	printk(KERN_INFO "%s : GPIO pin %d released\n", THIS_MODULE->name, SWITCH_GPIO);

	gpio_free(LED_GPIO);
	printk(KERN_INFO "%s : GPIO pin %d released\n", THIS_MODULE->name, LED_GPIO);

	cdev_del(&cdev);
	printk(KERN_INFO "%s : cdev_del() succesfull\n", THIS_MODULE->name);

	device_destroy(pclass, devno);
	printk(KERN_INFO "%s : device_destroy() successful\n", THIS_MODULE->name);

	class_destroy(pclass);
	printk(KERN_INFO "%s : class_destroy() successful\n", THIS_MODULE->name);

	unregister_chrdev_region(devno, 1);
	printk(KERN_INFO "%s : unregister_chrdev_region successful\n", THIS_MODULE->name);

	printk(KERN_INFO "%s : yug_GPIO_exit is finished\n", THIS_MODULE->name);
}

static int yug_GPIO_open(struct inode *pinode, struct file *pfile)
{

	printk(KERN_INFO "%s : yug_GPIO_open() is called \n", THIS_MODULE->name);
	return 0;
}

static ssize_t yug_GPIO_read(struct file *pfile, char __user *ubuf, size_t size, loff_t *poffset)
{

	int ret, switch_state;
	char kbuf[4];
	switch_state = gpio_get_value(SWITCH_GPIO);
	printk(KERN_INFO "%s : yug_GPIO_read() called\n", THIS_MODULE->name);
	sprintf(kbuf, "%d\n", switch_state);
	ret = 2 - copy_to_user(ubuf, kbuf, 2);
	if (ret == 0)
	{

		printk(KERN_ERR "%s : copy_to_user() failed to read two bytes in user space\n", THIS_MODULE->name);
		return -1;
	}
	printk(KERN_INFO "%s : GPIO PIN %d SWITCH state read = %d\n", THIS_MODULE->name, SWITCH_GPIO, switch_state);
	return ret;
}

static ssize_t yug_GPIO_write(struct file *pfile, const char __user *ubuf, size_t size, loff_t *poffset)
{

	int ret;
	char kbuf[2] = "";
	printk(KERN_INFO "%s : yug_GPIO_write() called \n", THIS_MODULE->name);
	ret = 1 - copy_from_user(kbuf, ubuf, 1);
	if (ret == 0)
	{

		printk(KERN_ERR "%s : copy_from_user() failed to write 1 bytes in kernel space\n", THIS_MODULE->name);
		return -1;
	}
	if (ret > 0)
	{
		if (kbuf[0] == '1')
		{

			led_state = 1;
			gpio_set_value(LED_GPIO, 1);
			printk(KERN_INFO "%s : GPIO_PIN %d LED ON \n", THIS_MODULE->name, LED_GPIO);
		}
		else if (kbuf[0] == '0')
		{

			led_state = 0;
			gpio_set_value(LED_GPIO, 0);
			printk(KERN_INFO "%s : GPIO_PIN %d LED OFF\n", THIS_MODULE->name, LED_GPIO);
		}
		else
			printk(KERN_INFO "%s : GPIO pin %d LED NO state changed\n", THIS_MODULE->name, LED_GPIO);
	}
	return size;
}

static int yug_GPIO_close(struct inode *pinode, struct file *pfile)
{

	printk(KERN_INFO "%s : yug_GPIO_close() is called\n", THIS_MODULE->name);
	return 0;
}

module_init(yug_GPIO_init);
module_exit(yug_GPIO_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yugandhar Anil Narkhede <yashnarkhede04@gmail.com>");
MODULE_DESCRIPTION("THIS IS SECOND PSEUDO CHARACTER DEVICE DRIVER CODE");

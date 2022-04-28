#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>


#include <linux/proc_fs.h>
#include <linux/slab.h>

#include <asm/io.h>

#define LLL_MAX_USER_SIZE 1024
#define GPIO_ADDRESS 0xFE200000

static struct proc_dir_entry* lll_proc = NULL;

static char data_buffer[LLL_MAX_USER_SIZE];

static unsigned int* gpio_registers = NULL;

ssize_t lll_read(struct file* file, char __user *user, size_t size, loff_t* offset) {
	copy_to_user(user, "Hello\n", 6);
	return 6;
}

static void gpio_on(unsigned int pin) {
	unsigned int fsel_index = pin/10;
	unsigned int fsel_bitpos = pin % 10;
	unsigned int* gpio_fsel = gpio_registers + fsel_index;
	unsigned int* gpio_on_register = (unsigned int*)((char*)gpio_registers + 0x1c);
	
	*gpio_fsel &= ~(7 << (fsel_bitpos * 3));
	*gpio_fsel |= (1 << (fsel_bitpos * 3));
	*gpio_on_register |= 1 << pin;
}

static void gpio_off(unsigned int pin) {
	unsigned int* gpio_off_reg = (unsigned int*)((char*)gpio_registers + 0x28);
	*gpio_off_reg |= 1 << pin;

}


ssize_t lll_write(struct file* file, const char __user* user, size_t size, loff_t* offset) {
	memset(data_buffer, 0, sizeof(data_buffer));
	if(size > LLL_MAX_USER_SIZE) {
		size = LLL_MAX_USER_SIZE;
	}

	if(copy_from_user(data_buffer, user, size))
		return 0;
	printk("LLL_GPIO: data buffer: %s\n", data_buffer);
	unsigned int pin = 0, value = 0;

	if(sscanf(data_buffer, "%d %d", &pin, &value) != 2) {
		printk("LLL_GPIO: inproper command\n");
		return size;
	}

	if(pin > 21 || value > 1)
		return size;
	if(value)
		gpio_on(pin);
	else
		gpio_off(pin);

	printk("message: %s\n", data_buffer);
	return size;
	
}

static const struct proc_ops lll_proc_fops = {
	.proc_read = lll_read,
	.proc_write = lll_write
};

static int __init gpio_driver_init(void) {
	printk("Hello from the kernel\n");


	gpio_registers = (int* ) ioremap(GPIO_ADDRESS, PAGE_SIZE);
	if(!gpio_registers) {
		printk("LLL_GPIO: could not map pins\n");
		return -1;
	}

	lll_proc = proc_create("lll-gpio", 0666, NULL, &lll_proc_fops);
	if(lll_proc == NULL) {
		return -1;
	}


	return 0;
}

static void __exit gpio_driver_exit(void) {
	printk("Goodbye !\n");
	proc_remove(lll_proc);
}


module_init(gpio_driver_init);
module_exit(gpio_driver_exit);

MODULE_LICENSE("GPL");


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>


#define MAJOR_NUMBER 61/* forward declaration */


int fourMB_open(struct inode *inode, struct file *filep);
int fourMB_release(struct inode *inode, struct file *filep);
ssize_t fourMB_read(struct file *filep, char *buf, size_t count, loff_t *f_pos);
ssize_t fourMB_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos);
static void fourMB_exit(void);


/* definition of file_operation structure */
struct file_operations fourMB_fops = {
	read: fourMB_read,
	write: fourMB_write,
	open: fourMB_open,
	release: fourMB_release
};

char *fourMB_data = NULL;

int fourMB_open(struct inode *inode, struct file *filep)
{
	return 0; // always successful
}
int fourMB_release(struct inode *inode, struct file *filep)
{
	return 0; // always successful
}
ssize_t fourMB_read(struct file *filep, char *buf, size_t count, loff_t *f_pos)
{
	int bytes_read = 0;
	
	/* Check if the buffer has been written */
	if(*buf != 0){
		return 0;	
	}
	copy_to_user(buf, fourMB_data, sizeof(char));
	
	bytes_read ++;
	return bytes_read;
}
ssize_t fourMB_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos)
{
	int bytes_write = 0;	
	copy_from_user(fourMB_data, buf, sizeof(char));

	/* Check the length of the bytes that have been written*/
	if(count > sizeof(char))
	{
		printk(KERN_ALERT "No space left on device\n");

		/*Return Linux System Error<28>: No space left on device */
		return -ENOSPC; 
	}

	bytes_write ++;
	return bytes_write;
}

static int fourMB_init(void)
{
	int result;
	// register the device
	result = register_chrdev(MAJOR_NUMBER, "fourMB", &fourMB_fops);
	if (result < 0) {
		return result;
	}
	// allocate one byte of memory for storage
	// kmalloc is just like malloc, the second parameter is
	// the type of memory to be allocated.
	// To release the memory allocated by kmalloc, use kfree.
	fourMB_data = kmalloc(sizeof(char), GFP_KERNEL);

	if (!fourMB_data) {
		fourMB_exit();
		// cannot allocate memory
		// return no memory error, negative signify a failure
		return -ENOMEM;
	}

	// initialize the value to be X
	*fourMB_data = 'X';

	printk(KERN_ALERT "This is a fourMB device module\n");
	return 0;
}


static void fourMB_exit(void)
{
	// if the pointer is pointing to something
	if (fourMB_data) {
		// free the memory and assign the pointer to NULL
		kfree(fourMB_data);
		fourMB_data = NULL;
	}

	// unregister the device
	unregister_chrdev(MAJOR_NUMBER, "fourMB");
	printk(KERN_ALERT "Onebyte device module is unloaded\n");
}

MODULE_LICENSE("GPL");
module_init(fourMB_init);
module_exit(fourMB_exit);

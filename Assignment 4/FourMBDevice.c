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

char *data_pointer = NULL;
size_t dataLength = 0;

int fourMB_open(struct inode *inode, struct file *filep)
{
	data_pointer = fourMB_data;
	dataLength = 0;
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
	if (*data_pointer == 0){
                return 0;
	}
	
	while (count && *data_pointer) {

                copy_to_user(buf++, data_pointer++, sizeof(char));

                count--;
                bytes_read++;
        }
	

	printk("R-Bytes read: %d ", bytes_read);
	printk("R-Count: %lu ", count);
	
	return bytes_read;
}
ssize_t fourMB_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos)
{	
	int bytes_write = 0;

	size_t pre_dataLength = dataLength;
	dataLength += count;
	
	if(dataLength < 1024*1024*4*sizeof(char)){
		copy_from_user(data_pointer, buf, count);
		data_pointer += count;
		buf += count;
		bytes_write += count;
	} else {
		copy_from_user(fourMB_data, buf, 1024*1024*4*sizeof(char) - pre_dataLength);
		bytes_write += 1024*1024*4*sizeof(char) - pre_dataLength;
	}

	printk("W-datalength: %lu ", dataLength);
	printk("W-RealLength: %lu ", strlen(fourMB_data));

	/* Check the length of the bytes that have been written*/
	if(dataLength > 1024*1024*4*sizeof(char))
	{
		printk(KERN_ALERT "No space left on device\n");

		/*Return Linux System Error<28>: No space left on device */
		return -ENOSPC; 
	}


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
	// allocate 4 MB of memory for storage
	// kmalloc is just like malloc, the second parameter is
	// the type of memory to be allocated.
	// To release the memory allocated by kmalloc, use kfree.
	fourMB_data = kmalloc(1024*1024*4*sizeof(char), GFP_KERNEL);


	if (!fourMB_data) {
		fourMB_exit();
		// cannot allocate memory
		// return no memory error, negative signify a failure
		return -ENOMEM;
	}

	// initialize the value to be X
	*fourMB_data = 'X';
	data_pointer = fourMB_data;

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
	printk(KERN_ALERT "FourMB device module is unloaded\n");
}

MODULE_LICENSE("GPL");
module_init(fourMB_init);
module_exit(fourMB_exit);

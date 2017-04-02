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
loff_t fourMB_lseek(struct file *file, loff_t offset, int orig);
static void fourMB_exit(void);


/* definition of file_operation structure */
struct file_operations fourMB_fops = {
	read: fourMB_read,
	write: fourMB_write,
	open: fourMB_open,
	release: fourMB_release,
	llseek: fourMB_lseek
};

char *fourMB_data = NULL;

char *data_pointer = NULL;
size_t data_length_written = 0;
size_t data_length_to_read = 0;
int bytes_written_total = 0;
int bytes_read_total = 0; 

loff_t fourMB_lseek(struct file *file, loff_t offset, int whence) {

	loff_t new_position = 0;

	switch(whence) {
		case SEEK_SET :
		    new_position = offset;
		    break;
		case SEEK_CUR : 
		    new_position = file->f_pos + offset;
		    break;
		case SEEK_END : 
		    new_position = data_length_written - offset;
		    break;
	}

	//check boundary
	if(new_position > data_length_written){
		new_position = data_length_written;
	}
	
	if(new_position < 0){
		new_position = 0;
	}
	
	file->f_pos = new_position;
	
	//reset data pointer
	data_pointer = fourMB_data;

	return new_position;
}


int fourMB_open(struct inode *inode, struct file *filep)
{
	//re-align data pointer to the start of data section 	
	data_pointer = fourMB_data;
	
	//need to set how many bytes to read for read operation
	data_length_to_read = data_length_written;

	bytes_written_total = 0;
	bytes_read_total = 0;

	return 0; // always successful
}


int fourMB_release(struct inode *inode, struct file *filep)
{
	return 0; // always successful
}


ssize_t fourMB_read(struct file *filep, char *buf, size_t count, loff_t *f_pos)
{
	int bytes_read = 0;

	/* Check if it the end of the data section */
	if (data_length_to_read == 0){
		printk("Total Bytes read: %d ", bytes_read_total);
                return 0;
	}
	
	data_pointer = *f_pos + data_pointer;
	data_length_to_read -= *f_pos;
	
	while (count && *data_pointer && data_length_to_read) {

                copy_to_user(buf++, data_pointer++, sizeof(char));

                count--;
                bytes_read++;
		bytes_read_total ++;
		data_length_to_read --;
        }
	
	return bytes_read;
}


ssize_t fourMB_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos)
{	
	int bytes_written = 0;	
	
	//if this function is called first time during current write operation
	//simple reset the written data length 
	if(data_pointer == fourMB_data){
		printk("Offset: %d ", *f_pos);
		data_length_written = *f_pos;
		data_pointer += *f_pos;
	}

	while (count && *buf) {
		//detect if have writen 4MB data
		if(data_length_written >= 1024*1024*4*sizeof(char)){
			break;
		}

                copy_from_user(data_pointer++, buf++, sizeof(char));

                count--;
                bytes_written++;
		bytes_written_total++;
		data_length_written ++;
        }
	
	printk("Total Bytes written so far: %d ", bytes_written_total);
	
	//check if data more than 4MB left to be written
	if(count > 0)
	{
		printk(KERN_ALERT "No space left on device\n");

		/*Return Linux System Error<28>: No space left on device */
		return -ENOSPC; 
	}
	
	return bytes_written;

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
	data_length_written ++;
	

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

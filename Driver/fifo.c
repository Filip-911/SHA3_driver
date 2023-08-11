#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/slab.h>
  
#define BUFF_SIZE 30
MODULE_LICENSE("Dual BSD/GPL");

dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static struct cdev *my_cdev;

struct mutex  mut;

unsigned char fifo[16];
unsigned char write_pos = 0;
int num = 1; //number of iterations for read fucntion
int i = 0;   //runtime iterations for read function
int endRead = 0;
unsigned char read_pos = 0;
unsigned char writeable_amount = 16;
unsigned char readable_amount = 0;

wait_queue_head_t readQ;
DECLARE_WAIT_QUEUE_HEAD (readQ);

wait_queue_head_t writeQ;
DECLARE_WAIT_QUEUE_HEAD(writeQ);
  
int fifo_open(struct inode *pinode, struct file *pfile);
int fifo_close(struct inode *pinode, struct file *pfile);
ssize_t fifo_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset);
ssize_t fifo_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset);

struct file_operations my_fops =
{
	.owner = THIS_MODULE,
	.open = fifo_open,
	.read = fifo_read,
	.write = fifo_write,
	.release = fifo_close,
};


int fifo_open(struct inode *pinode, struct file *pfile) 
{
		printk(KERN_INFO "Succesfully opened fifo\n");
		return 0;
}

int fifo_close(struct inode *pinode, struct file *pfile) 
{
		printk(KERN_INFO "Succesfully closed fifo\n");
		return 0;
}

ssize_t fifo_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset) 
{
        int ret;
        char buff[BUFF_SIZE];
        long int len = 0;
	
	if(endRead)
	  {
	    endRead = 0;
	    i=0;
	    return 0;
	  }
	//mutex lock
	if(mutex_lock_interruptible(&mut))
	  return -ERESTARTSYS;
	while (readable_amount == 0)
	  {
	    mutex_unlock(&mut);
	    if(wait_event_interruptible(readQ,(readable_amount > 0)))
	      return -ERESTARTSYS;
	    if(mutex_lock_interruptible(&mut))
	      return -ERESTARTSYS;
	  }
	//read while readable_amount > 0
	if(readable_amount > 0)
	  {
	    len = scnprintf(buff, BUFF_SIZE, "Value %d at position  %d; ", fifo[read_pos], read_pos);
	    printk(KERN_INFO "Value %d at position %d\n", fifo[read_pos], read_pos);
		
	    ret = copy_to_user(buffer, buff, len);
	    if(ret)
	      return -EFAULT;
		
	    read_pos++;
	    writeable_amount++;
	    readable_amount--;
	    i++;
		
	    if(read_pos == 16)
	      read_pos = 0;
		
	    printk(KERN_INFO "Succesfully read\n");
	  }
	  
	else
	  {
		    
	    printk(KERN_WARNING "Fifo is empty\n");
	    endRead = 1;
	  }
	
	mutex_unlock(&mut);
	//if all num positions are read terminate
	if(i == num)
	  {
	    wake_up_interruptible(&writeQ);
	    endRead = 1;
	  }
	    
	return len;
}
	    
ssize_t fifo_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset) 
{
    char* buff;
	char *jump;
	unsigned char partial,decimal;
	int i,j;
	int ret;
	long tmp;
	
	buff = (char*) kmalloc(length * sizeof(char), GFP_KERNEL);
	if(!buff)
	  return -EFAULT;
	
	ret = copy_from_user(buff, buffer, length);
	if(ret)
		return -EFAULT;
	buff[length-1] = '\0';

	jump = strchr(buff, 'b'); //jump to 'b'
	if(jump != NULL) // if 'b' is found 
	  {
	    jump++;
	    //mutex lock
	    if(mutex_lock_interruptible(&mut))
	      return -ERESTARTSYS;
	    while(writeable_amount == 0)
	      {
		mutex_unlock(&mut);
		if(wait_event_interruptible(writeQ,(writeable_amount > 0)))
		  return -ERESTARTSYS;
		if(mutex_lock_interruptible(&mut))
		  return -ERESTARTSYS;
	      }
	    // write until writeable_amount > 0
	    while(writeable_amount > 0)
	      {
		decimal = 0;
		if(write_pos == 16)
		  write_pos = 0;
	  
		ret = sscanf(jump,"%ld",&tmp);
		if(ret==1) //one parameter parsed in sscanf
		  {
		    //binary conversion to decimal without pow() function
		    if(tmp%10 == 1)
		      decimal++;
		    tmp = tmp/10;
	      
		    for(i = 1; i < 8;i++)
		      {
			if(tmp%10 == 1)
			  {
			    partial = 1;
			    for(j = 0; j < i; j++)
			      {					
				partial *=2;
			      }
			    decimal += partial;
			  }
			tmp = tmp/10;
		      }
		    
		    fifo[write_pos] = decimal; //write in the calculated value
		    printk(KERN_INFO "Successfully wrote value %d", fifo[write_pos], write_pos);
		    write_pos++;
		    writeable_amount--;
		    readable_amount++;
		  }	
		else
		  {
		    printk(KERN_WARNING "Wrong command format");
		  }
	  
		if(writeable_amount == 0)
		  {
		    printk(KERN_WARNING "Fifo is full");
		  }
		// if possible, jump to next number
		jump += 8;
		if(*(jump)== ';')
		  jump += 3;
		else
		  break;
	      }
	    wake_up_interruptible(&readQ);
	    mutex_unlock(&mut);
	  }
	else   //if 'b' is not found
	  {
	    jump = strchr(buff, 'n');  // jump to n
	
	    if(jump != NULL)
	      {
		jump += 4; 
		ret = sscanf(jump,"%d",&num); //change num
		printk(KERN_INFO "Successfully written value n=%d", num);
		i=0;
	      }
	    else
	  printk(KERN_WARNING "Wrong command format");
	  }
	kfree(buff);
	return length;
	  
	  
}

static int __init fifo_init(void)
{
   int ret = 0;
	int i=0;

	//Initialize array
	for (i=0; i<16; i++)
		fifo[i] = 0;
	
	//mutex and wait queues initalized
	init_waitqueue_head(&writeQ);
	init_waitqueue_head(&readQ);
        mutex_init(&mut);

   ret = alloc_chrdev_region(&my_dev_id, 0, 1, "fifo");
   if (ret){
      printk(KERN_ERR "failed to register char device\n");
      return ret;
   }
   printk(KERN_INFO "char device region allocated\n");

   my_class = class_create(THIS_MODULE, "fifo_class");
   if (my_class == NULL){
      printk(KERN_ERR "failed to create class\n");
      goto fail_0;
   }
   printk(KERN_INFO "class created\n");
   
   my_device = device_create(my_class, NULL, my_dev_id, NULL, "fifo");
   if (my_device == NULL){
      printk(KERN_ERR "failed to create device\n");
      goto fail_1;
   }
   printk(KERN_INFO "device created\n");

	my_cdev = cdev_alloc();	
	my_cdev->ops = &my_fops;
	my_cdev->owner = THIS_MODULE;
	ret = cdev_add(my_cdev, my_dev_id, 1);
	if (ret)
	{
      printk(KERN_ERR "failed to add cdev\n");
		goto fail_2;
	}
   printk(KERN_INFO "cdev added\n");
   printk(KERN_INFO "Hello world\n");

   return 0;

   fail_2:
      device_destroy(my_class, my_dev_id);
   fail_1:
      class_destroy(my_class);
   fail_0:
      unregister_chrdev_region(my_dev_id, 1);
   return -1;
}

static void __exit fifo_exit(void)
{
   cdev_del(my_cdev);
   device_destroy(my_class, my_dev_id);
   class_destroy(my_class);
   unregister_chrdev_region(my_dev_id,1);
   printk(KERN_INFO "Goodbye, cruel world\n");
}


module_init(fifo_init);
module_exit(fifo_exit);

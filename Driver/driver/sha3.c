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

MODULE_LICENSE("GPL");

dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static struct cdev *my_cdev;
  
int sha3_open(struct inode *pinode, struct file *pfile);
int sha3_close(struct inode *pinode, struct file *pfile);
ssize_t sha3_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset);
ssize_t sha3_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset);

#define WORD_SIZE (64/8)
#define RATE (1088/8)
#define CAPACITY (512/8)
#define SIZE (1600/8)
#define OUTPUT_SIZE (256/8)
#define BUFF_SIZE 65
#define KECCAKF_ROUNDS 24
#define HEX_AMOUNT OUTPUT_SIZE * 2 + 1
#define Z_AMOUNT OUTPUT_SIZE/8 + 1

uint64_t state[5][5];
uint64_t block[5][5];
unsigned char hex[HEX_AMOUNT];
bool result_ready = 0;
int pos = 0;
int endRead = 0;

void Pad(unsigned char *buff, int length);
void Keccak_f(void);
void Absorb(unsigned char *buff, int inputPos);
void Squeeze(void);
void Hardware_f(int blockCount, unsigned char* buff);

void Hex_value_2(uint64_t , unsigned char* hex1);
uint64_t Rotate_right(uint64_t a, const uint64_t rotc);

struct file_operations my_fops =
{
	.owner = THIS_MODULE,
	.open = sha3_open,
	.read = sha3_read,
	.write = sha3_write,
	.release = sha3_close,
};

int sha3_open(struct inode *pinode, struct file *pfile) 
{
    printk(KERN_INFO "Succesfully opened sha3\n");
    return 0;
}

int sha3_close(struct inode *pinode, struct file *pfile) 
{
    printk(KERN_INFO "Succesfully closed sha3\n");
    return 0;
}

ssize_t sha3_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset) 
{
    // squeeze into file
    char buff[BUFF_SIZE];
    int ret, len;

    if(result_ready == 0){
        printk(KERN_ERR "result not ready ");
        return 0;
    }

    if(endRead){
	endRead = 0;
        pos = 0;
        result_ready = 0;
        return 0;
    }
	
    if (pos < BUFF_SIZE){
	 len = scnprintf(buff, BUFF_SIZE, "%c", hex[pos++]);
         ret = copy_to_user(buffer, buff, HEX_AMOUNT);
    }
    else if(pos == BUFF_SIZE - 1){
	 len = scnprintf(buff, BUFF_SIZE, "%c\n", hex[pos++]);
         ret = copy_to_user(buffer, buff, HEX_AMOUNT);
    }
    else if(pos == BUFF_SIZE)  // 65
	 endRead = 1;
    
    if(ret){
        return -EFAULT;
    }
    
    return len;
}

ssize_t sha3_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset) 
{
    size_t i, j;
    int ret;
    char* buff;
    size_t len;
    int blockCount;
    result_ready = 0;
     
    //array init
    for (i = 0; i < 5; i++)
		for (j = 0; j < 5; j++)
			state[i][j] = 0x0000000000000000;

    if(length + 1 == RATE)
    {
        buff = (unsigned char*)kmalloc(RATE + 1, GFP_KERNEL);
        blockCount = 1;
    }
    else
    {
        blockCount = length/RATE;
        blockCount++;
        buff = (unsigned char*)kmalloc(blockCount*RATE + 1, GFP_KERNEL); 
        printk(KERN_INFO "Successfully allocated memory, blockCount: %d", blockCount);
    }
	if(!buff)
	  return -EFAULT;
	
    ret = copy_from_user(buff, buffer, length);
    if(ret)
	return -EFAULT;

    buff[length-1] = '\0';
    printk(KERN_INFO "Successfully wrote buff: %s", buff);


    // padding
    len = length;
    len--;
    Pad(buff, len);
    
    // do the actual hw function
    Hardware_f(blockCount, buff);
    kfree(buff);
    
    // squeeze with kernel print
    Squeeze();
    
    // global variable for reading set
    result_ready = 1;
  
    return length;
}

static int __init sha3_init(void)
{    int ret;
   ret = alloc_chrdev_region(&my_dev_id, 0, 1, "sha3");
   if (ret){
      printk(KERN_ERR "failed to register char device\n");
      return ret;
   }
   printk(KERN_INFO "char device region allocated\n");

   my_class = class_create(THIS_MODULE, "sha3_class");
   if (my_class == NULL){
      printk(KERN_ERR "failed to create class\n");
      goto fail_0;
   }
   printk(KERN_INFO "class created\n");
   
   my_device = device_create(my_class, NULL, my_dev_id, NULL, "sha3");
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

static void __exit sha3_exit(void)
{
   cdev_del(my_cdev);
   device_destroy(my_class, my_dev_id);
   class_destroy(my_class);
   unregister_chrdev_region(my_dev_id,1);
   printk(KERN_INFO "Goodbye, cruel world\n");
}

module_init(sha3_init);
module_exit(sha3_exit);

void Pad(unsigned char *buff, int length)
{    
    int a, i;
    if(length + 1 == RATE)
    {
      buff[RATE - 1] = 0x18;
      buff[RATE] = '\0';
    }
    else
    {
        a = (length)/RATE;
        a++;
        
        buff[length] = 0x06;
        for(i = length + 1; i < a*RATE - 1; i++)
        {
            buff[i] = 0x00;
        }
        buff[a*RATE - 1] = 0x80;
        buff[a*RATE] = '\0';
    }

}

void Hex_value_2(uint64_t dec_value, unsigned char* hex1)
{   
    uint64_t tmp = dec_value;
    uint64_t rem;
    int k = 1;
    size_t i ;
    for (i = 0; i < 16; i++)
    {
        rem = tmp % 16;
        if(rem < 10)
            hex1[i + k] = 48 + rem;
        else
            hex1[i + k] = 55 + rem;
        tmp /= 16;
        k = -k;
    }
}

uint64_t Rotate_right(uint64_t a, uint64_t rot)

{
     rot &= 0x3f;
   return (a << rot) | (a >> (64 - rot));
}

void Hardware_f(int blockCount, unsigned char* buff)
{
    int inputPos = 0;
    size_t i, j, k, z;
    for (i = 0; i < blockCount; i++)
    {
        for (j = 0; i < 5; i++)
            for (z = 0; j < 5; j++)
                block[j][z] = 0x0000000000000000;

        Absorb(buff, inputPos);
        inputPos += RATE;
        for (j = 0; j < 5; j++)
            for (k = 0; k < 5; k++)
                state[j][k] ^= block[j][k];

        Keccak_f();
    }
}

void Squeeze(void)
{   
    int p = 0 ;
    size_t i,j ;
    uint64_t z[Z_AMOUNT];

    //initialize Z to be the empty string
    for (i = 0; i < Z_AMOUNT; i++)
    {
        z[i] = 0x0000000000000000;
    }
    
    z[Z_AMOUNT -1] = '\0';


    //while the length of Z is less than OUTPUT_SIZE:
    //append the first RATE bytes of STATE to Z
    for (i = 0; i < 5; i++)
    {
        for (j = 0; j < 5; j++)
        {
            z[p++] = state[j][i];
            if(p == Z_AMOUNT)
                break;
        }
        if(p == Z_AMOUNT)
            break;
    }
    for (i = 0; i < Z_AMOUNT; i++)
        Hex_value_2(z[i], &hex[i * WORD_SIZE *2]);
     
   hex[HEX_AMOUNT - 1] = '\0';

    printk(KERN_INFO "\n\nSha3 digest: 0x%s\n\n", hex);
}

void Absorb(unsigned char *buff, int inputPos)        
{
   int p = 0;
   size_t i, j, k;
    for (i = 0; i < 5; i++)
        for (j = 0; j < 5; j++)
        {
            p++;
            if(p <= RATE/8) // if you reached 17th member of the matrix - done
            {
                for (k = 0; k < WORD_SIZE; k++) // put 8 bytes into one member of the matrix
                    { 
                        block[j][i] |= buff[i*5*WORD_SIZE + j*WORD_SIZE + WORD_SIZE - 1 - k + inputPos];
                        if(k != WORD_SIZE - 1)
                            block[j][i] <<= WORD_SIZE;
                    }
            }	
	}    
}


void Keccak_f(void)
{
    // constants
    const uint64_t keccakf_rndc[25] = {
        0x0000000000000001, 0x0000000000008082, 0x800000000000808a,
        0x8000000080008000, 0x000000000000808b, 0x0000000080000001,
        0x8000000080008081, 0x8000000000008009, 0x000000000000008a,
        0x0000000000000088, 0x0000000080008009, 0x000000008000000a,
        0x000000008000808b, 0x800000000000008b, 0x8000000000008089,
        0x8000000000008003, 0x8000000000008002, 0x8000000000000080,
        0x000000000000800a, 0x800000008000000a, 0x8000000080008081,
        0x8000000000008080, 0x0000000080000001, 0x8000000080008008
    };

    const uint64_t keccak_rotCM[5][5] =
    {
        {0, 36, 3, 41, 18},
        {1, 44, 10, 45, 2},
        {62, 6, 43, 15, 61},
        {28, 55, 25, 21, 56},
        {27, 20, 39, 8, 14}
    };

    uint64_t B[5][5];
    uint64_t C[5];
    uint64_t D[5];

    int n, x, y;
    for(n = 0; n < KECCAKF_ROUNDS; n++)
    {
        //theta
        for (x = 0; x < 5; x++)
        {
            C[x] = 0L;
            for ( y = 0; y < 5; y++)
                C[x] ^= state[x][y];
        }
        for (x = 0; x < 5; x++)
        {
            D[x] = C[(x + 4) % 5] ^Rotate_right(C[(x + 1) % 5], 1);
            for ( y = 0; y < 5; y++)
                state[x][y] ^= D[x];
        }

        //rho pi
        memset(B, 0, 5* 5* sizeof(uint64_t));

        for( x = 0; x < 5; x++)
            for( y = 0; y < 5; y++)
                B[y][(2*x + 3*y) % 5] = Rotate_right(state[x][y], keccak_rotCM[x][y]);

        //chi
        for ( x = 0; x < 5; x++)
            for ( y = 0; y < 5; y++)
                state[x][y] = B[x][y] ^ ((~B[(x + 1) % 5][y]) & B[(x + 2) % 5][y]);

        //iota
            state[0][0] ^= keccakf_rndc[n];
    }
}

#include "asm/uaccess.h"
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>

/**
 * 实验内容：
 * chrdevbase 设备有容量为100 字节缓冲区chrdevbuf。在应用程序中可以向 chrdevbase 设
备的缓冲区中写入数据，也可以从缓冲区中读取数据。 
 */

 #define dbg_info(...) printk(KERN_DEBUG "[drives] %s-%d-<%s>:", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__) ;

/*1、变量定义*/
static char chrdevbuf[100] ;
static int major = 200 ;
static char * chrdevname = "chrdevbase" ;


static int chrdev_open(struct inode *inode, struct file *filp) 
{
   //filp->private_data = NULL ;
   return 0 ;
}

static int chrdev_read(struct file *filp, char __user *buf, size_t len, loff_t * offset)
{
    return copy_to_user(buf, chrdevbuf, len) ;
}

static int chrdev_write(struct file *filp, const char __user *buf, size_t len, loff_t *offset)
{
    memset(chrdevbuf, 0 , 100) ;
    return copy_from_user(chrdevbuf, buf, len) ;
}

static int chrdev_close(struct inode *inode, struct file *filp)
{
   return 1 ;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = chrdev_open ,
    .read = chrdev_read ,
    .write = chrdev_write ,
    .release = chrdev_close
} ;

static int chrdevbase_init(void)
{
    dbg_info("chrdevbase_init") ;
   return register_chrdev(major, chrdevname, &fops) ;
}

static void chrdevbase_exit(void)
{
    unregister_chrdev(major, chrdevname) ;
}

module_init(chrdevbase_init);
module_exit(chrdevbase_exit) ;

MODULE_LICENSE("GPL") ;
MODULE_AUTHOR("WANGCHAO") ;
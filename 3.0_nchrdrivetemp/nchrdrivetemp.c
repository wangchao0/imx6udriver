#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/*1、定义全局变量和自定义结构体变量，统一管理驱动变量*/
const char* CHRDEV_NAME = "chrdev" ;
#define DEV_MAXNUM  1
struct chrdev_s
{
    unsigned int uiMajor ;   //主设备号
    unsigned int uiMinor ;   //子设备号
    dev_t uiDevid ;           //设备号
    struct cdev  sDev ;      //字符设备
    struct class *psClass ;  //类指针
    struct device *psDevice; //设备指针
};
static struct chrdev_s schrdev ;   //定义chrdev_s结构体变量schedev

/*2、实现fops中的函数；定义半初始化file_operations结构体变量*/
static int chrdev_open(struct inode* spInode, struct file* spFilp)
{
    spFilp->private_data = &schrdev ;   //设置私有数据
    return 0 ;
}
static struct file_operations ssFops = 
{
    .owner = THIS_MODULE,
    .open = chrdev_open,
};

/*3、实现驱动入口函数chardev_init*/
static int chrdev_init(void)
{
    int err ;
    /*3.1、分配设备号*/
    if( schrdev.uiMajor )   //定义主设备号
    {
        schrdev.uiDevid = MKDEV(schrdev.uiMajor, 0) ;
        register_chrdev_region(schrdev.uiDevid, DEV_MAXNUM, CHRDEV_NAME) ;
    }
    else
    {
        alloc_chrdev_region(&schrdev.uiDevid, 0, DEV_MAXNUM, CHRDEV_NAME) ;
        schrdev.uiMajor = MAJOR(schrdev.uiDevid) ;
        schrdev.uiMinor = MINOR(schrdev.uiDevid) ;
    }
    printk("major = %d, minor = %d .\n", schrdev.uiMajor, schrdev.uiMinor) ;

    /*3.2、初始化、ADD 字符设备cdev*/
    cdev_init(&schrdev.sDev, &ssFops) ;
    err = cdev_add(&schrdev.sDev, schrdev.uiDevid, DEV_MAXNUM) ;
    if( err != 0 )
    {
        printk("cdev_add fail.\n") ;
        goto cdev_add_fail ;
    }

    /*3.3、创建class*/
    schrdev.psClass = class_create(THIS_MODULE, CHRDEV_NAME) ;
    if( IS_ERR_OR_NULL(schrdev.psClass))
    {
        printk("class_create fail.\n") ;
        err = -1;
        goto class_create_fail ;
    }

    /*3.4、创建device*/
    schrdev.psDevice = device_create(schrdev.psClass, NULL, schrdev.uiDevid, NULL, CHRDEV_NAME) ;
    if( IS_ERR_OR_NULL(schrdev.psDevice))
    {
        printk("device_create fail.\n") ;
        err = -2;
        goto device_create_fail ;
    }
    return 0 ;
device_create_fail:
    class_destroy(schrdev.psClass) ;
class_create_fail:
    cdev_del(&schrdev.sDev) ;
cdev_add_fail:
    unregister_chrdev_region(schrdev.uiDevid, DEV_MAXNUM) ;
    return err ;
}

/*4、实现驱动出口函数*/
static void chrdev_exit(void)
{
    /*释放顺序与入口初始化顺序的逆序*/
    device_destroy(schrdev.psClass, schrdev.uiDevid) ;
    class_destroy(schrdev.psClass) ;
    cdev_del(&schrdev.sDev) ;
    unregister_chrdev_region(schrdev.uiDevid, DEV_MAXNUM) ;
}

/*5、确定驱动的出入口函数*/
module_init(chrdev_init) ;
module_exit(chrdev_exit) ;

/*6、添加licence 和 author信息*/
MODULE_LICENSE("GPL") ;
MODULE_AUTHOR("WANG")  ;

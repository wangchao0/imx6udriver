
#include "asm-generic/gpio.h"
#include "asm/gpio.h"
#include "linux/of.h"
#include "linux/of_gpio.h"
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
#include <linux/of.h>
#include <linux/of_address.h>

/*1、定义全局变量和自定义结构体变量，统一管理驱动变量*/
const char* CHRDEV_NAME = "chrdevled" ;
#define DEV_MAXNUM  1
struct chrdev_s
{
    unsigned int uiMajor ;   //主设备号
    unsigned int uiMinor ;   //子设备号
    dev_t uiDevid ;           //设备号
    struct cdev  sDev ;      //字符设备
    struct class *psClass ;  //类指针
    struct device *psDevice; //设备指针i
    struct device_node *np ; //设备节点指针
    int beep_gpio ;          //beep gpio 申请的GPIO编号
};
static struct chrdev_s schrdev ;   //定义chrdev_s结构体变量schedev


static void beep_switch(u8 status)
{
    switch(status)
    {
        case '1':
           printk("beep_switch to ON\n") ;
           gpio_set_value(schrdev.beep_gpio, 1 ) ;
           break ;
        case '0':
           printk("beep_switch to OFF\n") ;
           gpio_set_value(schrdev.beep_gpio, 0);
           break ;
        default:
           printk("beep_switch to default\n") ;
           break; 
    }
}



/*2、实现fops中的函数；定义半初始化file_operations结构体变量*/
static int chrdev_open(struct inode* spInode, struct file* spFilp)
{
    spFilp->private_data = &schrdev ;   //设置私有数据
    return 0 ;
}

static int chrdev_write(struct file *filp, const char __user *buf, size_t len, loff_t *offset)
{
    char data[2] = {0} ;
    unsigned long lenth = copy_from_user(data, buf, 1) ;
    printk("chrdevled_write beep status: %s\n", data) ;
    beep_switch(data[0]) ;
    return (int)lenth ;
}


static struct file_operations ssFops =
{
    .owner = THIS_MODULE,
    .open = chrdev_open,
    .write = chrdev_write,
};

/*3、实现驱动入口函数chardev_init*/
static int chrdev_init(void)
{
    int err, len ;
    const char *compa ;
    struct property *prop ;
    schrdev.np = of_find_node_by_name(NULL, "gpiobeep") ;
    if(schrdev.np == NULL)
    {
        printk("find gpiobeep node fail.\n");
        return -1 ;
    }
    else
    {
        printk( "find gpiobeep node success.\n") ;
    }

    err = of_property_read_string(schrdev.np, "compatible", &compa);
    if(err == 0 )
    {
        printk("read compatible success, compatible = %s \n", compa) ;
    }
    else
    {
        printk("read compatible fail.\n");
        return -1 ;
    }

    prop = of_find_property(schrdev.np, "status",&len) ;
    if(prop == NULL)
    {
        printk("find status fail.\n") ;
    }
    else
    {
        printk( "find status success, status = %s .\n", (char*)prop->value);
    }

    schrdev.beep_gpio = of_get_named_gpio(schrdev.np, "beep_gpio", 0);
    if( schrdev.beep_gpio >= 0 )
    {
        printk("beep gpio num = %d.\n", schrdev.beep_gpio);
    }
    else
    {
        printk("get gpio fail,\n") ;
        return -2 ;
    }

    err = gpio_request(schrdev.beep_gpio, "beepgpio") ;
    if(err == 0 )
    {
        printk("gpio request success.\n");
    }
    else
    {
        printk("gpio request fail.\n") ;
    }
err = gpio_direction_output(schrdev.beep_gpio, 1);
 if(err < 0) {
 printk("can't set gpio!\r\n");
 }
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
    gpio_free(schrdev.beep_gpio) ;
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

#include "asm-generic/gpio.h"
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <linux/irq.h>
#include <linux/of_irq.h>

/*1、定义全局变量和自定义结构体变量，统一管理驱动变量*/
const char* CHRDEV_NAME = "keydev" ;
#define DEV_MAXNUM  1
#define KEY_NUM 1
#define KEY0VALUE 0x01

struct irq_keydesc{
    int gpio ;     //gpio标号
    int irqnum ;   //中断号
    unsigned char value ; //按键对应的值
    char name[10] ;
    irqreturn_t (*handler)(int, void *) ; //中断服务函数
} ;

struct chrdev_s
{
    unsigned int uiMajor ;   //主设备号
    unsigned int uiMinor ;   //子设备号
    dev_t uiDevid ;           //设备号
    struct cdev  sDev ;      //字符设备
    struct class *psClass ;  //类指针
    struct device *psDevice; //设备指针
    struct device_node *np ; //指向设备节点的指针 
    int key0pinnum ;          //pin在kernel中分配的编号
    atomic_t key0value ; //key0的值
    struct irq_keydesc irqkeydesc ; //描述按键中断
    struct timer_list timer ; //定时器用于中断消抖
};
static struct chrdev_s schrdev ;   //定义chrdev_s结构体变量schedev

//按键中断处理函数
static irqreturn_t key0_handler(int irq, void *dev_id)
{
    printk("key0 interrupt. \n" ) ;
    struct chrdev_s *dev;
    dev = dev_id ;

 if( 0 ==  gpio_get_value(dev->key0pinnum))
    {
        printk("key0value = 0x%x.\n", dev->key0value) ;
        atomic_set(&dev->key0value, 0) ;
    }
    else
    {
        printk("key0value = 0x%x.\n", dev->key0value) ;
        atomic_set(&dev->key0value, 1) ;
    }
return IRQ_RETVAL(IRQ_HANDLED);
}

//timer服务函数
void timer_function(unsigned long arg)
{

}

/*2、实现fops中的函数；定义半初始化file_operations结构体变量*/
static int chrdev_open(struct inode* spInode, struct file* spFilp)
{
    spFilp->private_data = &schrdev ;   //设置私有数据
    return 0 ;
}

static ssize_t chrdev_read(struct file *filp, char __user *buff, size_t len, loff_t *offset)
{
    struct chrdev_s *dev = filp->private_data ;
    int value ;
    value = atomic_read(&dev->key0value) ;
    printk("key0 value =0x%x .\n", value) ;
    copy_to_user(buff,  (const char*)&value, 1) ;
    return 1 ;
}


static struct file_operations ssFops = 
{
    .owner = THIS_MODULE,
    .open = chrdev_open,
    .read = chrdev_read,
};

/*3、实现驱动入口函数chardev_init*/
static int chrdev_init(void)
{
    int err, gpiocount = 0, valuelen = 0 ;
    struct property *prop = NULL  ;
    /*在设备树中查找gpiokey节点*/
    schrdev.np = of_find_node_by_name(NULL, "gpiokey") ;
    if(schrdev.np == NULL )
    {
        printk("find node by name: [gpiokey] fail .\n") ;
        return -1 ;
    }
    else
    {
        printk("find_node_by_name: [gpiokey] success. \n") ;
    }
    /*在np节点中查找是否有key_gpio属性*/
    prop = of_find_property(schrdev.np, "key_gpio", &valuelen) ;
    if(prop == NULL )
    {
        printk("node:[gpiokey] no has property:[key_gpio].\n") ;
        return -2 ;
    }
    else
    {
        printk("node:[gpiokey] has property:[]key_gpio],valuelen = %d.\n", valuelen) ;
    }
    /*key_gpio属性中有几个gpio信息*/
    gpiocount = of_gpio_named_count(schrdev.np, "key_gpio") ;
    printk("node:[gpiokey], property:[key_gpio] has gpio cells = %d.\n ", gpiocount ) ;
    /*获取 key_gpio属性中的第一个cell进行gpio编号*/
    schrdev.key0pinnum = of_get_named_gpio(schrdev.np, "key_gpio", 0) ;
    if(schrdev.key0pinnum < 0)
    {
        printk("of_gpio_named_gpio failed. \n") ;
        return -3 ;
    }
    printk("key0pin num = %d .\n", schrdev.key0pinnum) ;
    /*为编号为schrdev.key0pinnum的pin分配一个GPIO管脚*/
    gpio_request(schrdev.key0pinnum, "key0") ;

    /*设置gpio方向为输入*/
    if( 0 == gpio_direction_input(schrdev.key0pinnum))
    {
        printk("set pin input success.\n") ;
    }
    else
    {
        printk("set pin input failed.\n" ) ;
    }

    //13_irq增加代码
#if 0
    schrdev.irqkeydesc.irqnum = irq_of_parse_and_map(schrdev.np, 0);
#else
    schrdev.irqkeydesc.irqnum = gpio_to_irq(schrdev.key0pinnum) ;
#endif 
    printk("gpio = %d, irqnum = %d .\n", schrdev.key0pinnum, schrdev.irqkeydesc.irqnum) ;
    
    schrdev.irqkeydesc.handler = key0_handler ;
    schrdev.irqkeydesc.value = KEY0VALUE ;

    err = request_irq(schrdev.irqkeydesc.irqnum, schrdev.irqkeydesc.handler,IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING|IRQF_SHARED, "key0irq", &schrdev) ;
    if(err < 0) 
    {
        printk("key0 request irq failed.\n") ;
        return -EFAULT ;
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
    free_irq(schrdev.irqkeydesc.irqnum, &schrdev) ;
    gpio_free(schrdev.key0pinnum) ;
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

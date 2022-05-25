#include "linux/of.h"
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
/**
 * 实验内容：
 * dtsled为开发板led(DS0)驱动，
 * 应用层程序可以通过向/dev/dtsled设备文件写入1、0控制led状态为亮或者灭。
 * 驱动程序：
*1、在imx6ull-alientek-emmc.dts设备树文件中添加alphaled子节点，在驱动入口函数中分别通过of_find_node_by_name、of_find_property、of_property_read_string、of_find_property、of_property_read_u32_array等of操作函数处理dtb中的alphaled子节点;通过ioremap或者of_iomap完成物理寄存的映射，得到相关寄存器的虚拟地址。
2、定义major，name、file_operations结构体变量fops等相关变量
 *  3、通过宏module_init和module_exit确定驱动注册和注销函数
 *  4、实现驱动注册函数
        a、register_chrdev实现驱动设备注册
        b、地址映射
        c、外部实现内部调用led初始化函数
        
 *  4、实现驱动注销函数
        a、使用iounmap释放所以映射的虚拟地址
        b、unregister_chrdev实现驱动设备注销
 *  5、实现fops内相关函数指针成员变量的函数
 */

 /*LED相关寄存器操作
@      1、通过设置CCM_CCGR1[26:27]寄存器(0x20c406c)，打开GPIO1的时钟，即或值 0x00c00000
@      2、设置GPIO1_GDIR(0x209c004)寄存器设置IO03模式为输出模式，即或值：0x00000008
@      3、通过IOMUX_SW_MUX_CTL_PAD_GPIO1_IO03寄存器(0x20E0068)，设置GPIO1_IO03引脚复用模式为ALT5，即寄存器值：0x00000005
@      4、通过IOMUX_SW_PAD_CTL_PAD_GPIO1_IO03寄存器(0x20e02f4), 设置为关闭Hyst(施密特触发器)=0、100K上拉=10、PUE=1、PKE=0
@         ODE=0、Res=000、SPEED=11、驱动能力DSE=111、Res=00,压摆率SRE=1；即寄存器值：0x000050f9
@      5、通过GPIO1_DR寄存器设置GPIO1_IO03（0x209c000）输出的电平为0,即与值为：0xFFFF7FFF
 */

 #define LED_CCM_CCGR1  0x20c406c
 #define LED_GPIO1_GDIR 0X209C004
 #define IOMUX_SW_MUX_CTL_PAD_GPIO1_IO03 0X20E0068
 #define IOMUX_SW_PAD_CTL_PAD_GPIO1_IO03 0x20e02f4
 #define LED_GPIO1_DR 0X209C000

 static void __iomem  *VIRLED_CCM_CCGR1 ;
 static void __iomem  *VIRLED_GPIO1_GDIR ;
 static void __iomem  *VIRIOMUX_SW_MUX_CTL_PAD_GPIO1_IO03 ;
 static void __iomem  *VIRIOMUX_SW_PAD_CTL_PAD_GPIO1_IO03 ;
 static void __iomem  *VIRLED_GPIO1_DR ;

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
    struct device *psDevice; //设备指针
    struct device_node *np ; //设备树节点
};
static struct chrdev_s schrdev ;   //定义chrdev_s结构体变量schedev
static void led_init(void)
{
     /*LED相关寄存器操作
@      1、通过设置CCM_CCGR1[26:27]寄存器(0x20c406c)，打开GPIO1的时钟，即或值 0x00c00000
@      2、设置GPIO1_GDIR(0x209c004)寄存器设置IO03模式为输出模式，即或值：0x00000008
@      3、通过IOMUX_SW_MUX_CTL_PAD_GPIO1_IO03寄存器(0x20E0068)，设置GPIO1_IO03引脚复用模式为ALT5，即寄存器值：0x00000005
@      4、通过IOMUX_SW_PAD_CTL_PAD_GPIO1_IO03寄存器(0x20e02f4), 设置为关闭Hyst(施密特触发器)=0、100K上拉=10、PUE=1、PKE=0
@         ODE=0、Res=000、SPEED=11、驱动能力DSE=111、Res=00,压摆率SRE=1；即寄存器值：0x000050f9
@      5、通过GPIO1_DR寄存器设置GPIO1_IO03（0x209c000）输出的电平为0,（ & ~(1<<3)）点亮LED；通过或（1<<3）点亮LED灯
 */
    u32 value = readl(VIRLED_CCM_CCGR1) ;
    value |= 0x00c00000 ;
    writel(value, VIRLED_CCM_CCGR1) ;

    value = readl(VIRLED_GPIO1_GDIR) ;
    value |= (1<<3) ;
    writel(value, VIRLED_GPIO1_GDIR) ;

    writel(0x05, VIRIOMUX_SW_MUX_CTL_PAD_GPIO1_IO03) ;
    writel(0x50f9, VIRIOMUX_SW_PAD_CTL_PAD_GPIO1_IO03) ;

    value = readl(VIRLED_GPIO1_DR) ;
    printk("gpio_dr1 = %x\n", value) ;
    value |= (1<<3) ;
    writel(value, VIRLED_GPIO1_DR) ;
    printk("gpio_dr2 = %x\n", readl(VIRLED_GPIO1_DR)) ;

    value &= ~(1<<3) ;
    writel(value, VIRLED_GPIO1_DR) ;
    printk("gpio_dr3 = %x\n", readl(VIRLED_GPIO1_DR)) ;
}

#define  LED_ON  0X30 
#define  LED_OFF 0X31 

static void led_switch(u8 status)
{
    u32 value = readl(VIRLED_GPIO1_DR) ;
    switch(status)
    {
        case '0':
           printk("led_switch to ON\n") ;
           value |= (1<<3) ;
           break ;
        case '1':
           printk("led_switch to OFF\n") ;
           value &=~(1<<3) ;
           break ;
        default:
           printk("led_switch to default\n") ;
           break ;       
    }
    writel(value, VIRLED_GPIO1_DR) ;
}

/*2、实现fops中的函数；定义半初始化file_operations结构体变量*/
static int chrdevled_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &schrdev ;   //设置私有数据
    return 0 ;
} 

static int chrdevled_close(struct inode *inode, struct file *filp)
{
    return 0 ;
}

static int chrdevled_write(struct file *filp, const char __user *buf, size_t len, loff_t *offset)
{  
    char data[2] = {0} ;
    unsigned long lenth = copy_from_user(data, buf, 1) ;
    printk("chrdevled_write led status: %s\n", data) ;
    led_switch(data[0]) ;
    return (int)lenth ;
}

static struct file_operations ssFops = 
{
    .owner = THIS_MODULE,
    .open = chrdevled_open,
    .release = chrdevled_close,
    .write = chrdevled_write
};

/*3、实现驱动入口函数chardev_init*/
static int chrdev_init(void)
{
   int err, ret, i; 
   struct property *prope ;
   u32 regdata[14] ;
   const char *str ; 
   //通过路径查找节点
   schrdev.np = of_find_node_by_path("/alphaled") ;
   if(schrdev.np == NULL )
   {
       printk("find /alphaled node fail.\n");
       return -1 ;
   }
   else
   {
       printk("find /alphaled success.\n" ) ;
   }
   //从节点中查找compatibel属性，输出属性值
   prope = of_find_property(schrdev.np, "compatible", NULL) ;
   if(prope == NULL )
   {
       printk("compatibel property find failed\n");
   }
   else
   {
       printk("compatibel = %s\n", (char*)prope->value) ;
   }
  //获取属性status属性值 
   ret = of_property_read_string(schrdev.np, "status", &str) ;
   if(ret < 0)
   {
       printk("status read fail\n");
   }
   else
   {
       printk("status = %s\n", str) ;
   }
  
   //获取reg属性及内容
   ret =  of_property_read_u32_array(schrdev.np, "reg", regdata, 10 );
   if(ret < 0 )
   {
       printk("reg property read fail\n") ;
   }
   else
   {
       printk("reg data:\n") ;
       for(i =0; i<10; i++ )
       {
           printk("%#x", regdata[i]) ;
       }
       printk( "\n") ;
   }
#if 0
   printk("chrdevled_init\n") ;
   VIRLED_CCM_CCGR1 = ioremap(regdata[0], regdata[1]) ;
   printk("LED_CCM_CCGR1 = %p\n", VIRLED_CCM_CCGR1);
   VIRLED_GPIO1_GDIR = ioremap(regdata[8], regdata[9]) ;
   printk("VIRLED_GPIO1_GDIR = %p\n", VIRLED_GPIO1_GDIR);
   VIRIOMUX_SW_MUX_CTL_PAD_GPIO1_IO03 = ioremap(regdata[2], regdata[3]) ;
   printk("VIRIOMUX_SW_MUX_CTL_PAD_GPIO1_IO03 = %p\n", VIRIOMUX_SW_MUX_CTL_PAD_GPIO1_IO03);
   VIRIOMUX_SW_PAD_CTL_PAD_GPIO1_IO03 = ioremap(regdata[4], regdata[5]) ;
   VIRLED_GPIO1_DR = ioremap(regdata[6], regdata[7]) ;
#else
    printk("chrdevled_init\n") ;
   VIRLED_CCM_CCGR1 = of_iomap(schrdev.np, 0) ;
   printk("LED_CCM_CCGR1 = %p\n", VIRLED_CCM_CCGR1);
   VIRLED_GPIO1_GDIR = of_iomap(schrdev.np, 4) ;
   printk("VIRLED_GPIO1_GDIR = %p\n", VIRLED_GPIO1_GDIR);
   VIRIOMUX_SW_MUX_CTL_PAD_GPIO1_IO03 = of_iomap(schrdev.np, 1) ;
   printk("VIRIOMUX_SW_MUX_CTL_PAD_GPIO1_IO03 = %p\n", VIRIOMUX_SW_MUX_CTL_PAD_GPIO1_IO03);
   VIRIOMUX_SW_PAD_CTL_PAD_GPIO1_IO03 = of_iomap(schrdev.np, 2) ;
   VIRLED_GPIO1_DR = of_iomap(schrdev.np, 3) ;

#endif
   led_init() ;
   led_switch(LED_ON) ;
   
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

#include "asm/io.h"
#include "asm/uaccess.h"
#include "linux/fs.h"
#include "linux/printk.h"
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>

/**
 * 实验内容：
 * chrdevled为开发板led(DS0)驱动，
 * 应用层程序可以通过向/dev/chrdevled设备文件写入1、0控制led状态为亮或者灭。
 * 驱动程序：
 *  1、定义led相关寄存物理地址宏和虚拟地址宏，定义major，name、file_operations结构体变量fops等相关变量
 *  2、通过宏module_init和module_exit确定驱动注册和注销函数
 *  3、实现驱动注册函数
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

 static int ledmajor = 202 ;
 static char* name = "chardevled" ;

 
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

static int chrdevled_open(struct inode *inode, struct file *filp)
{
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

 struct file_operations fops= {
     .owner = THIS_MODULE,
     .open = chrdevled_open ,
     .write = chrdevled_write,
     .release = chrdevled_close
 };

static int chrdevled_init(void)
{
   printk("chrdevled_init\n") ;
   VIRLED_CCM_CCGR1 = ioremap(LED_CCM_CCGR1, 4) ;
   printk("LED_CCM_CCGR1 = %lx\n", VIRLED_CCM_CCGR1);
   VIRLED_GPIO1_GDIR = ioremap(LED_GPIO1_GDIR, 4) ;
   printk("VIRLED_GPIO1_GDIR = %x\n", VIRLED_GPIO1_GDIR);
   VIRIOMUX_SW_MUX_CTL_PAD_GPIO1_IO03 = ioremap(IOMUX_SW_MUX_CTL_PAD_GPIO1_IO03, 4) ;
   printk("VIRIOMUX_SW_MUX_CTL_PAD_GPIO1_IO03 = %x\n", VIRIOMUX_SW_MUX_CTL_PAD_GPIO1_IO03);
   VIRIOMUX_SW_PAD_CTL_PAD_GPIO1_IO03 = ioremap(IOMUX_SW_PAD_CTL_PAD_GPIO1_IO03, 4) ;
   VIRLED_GPIO1_DR = ioremap(LED_GPIO1_DR, 4) ;
   led_init() ;
   led_switch(LED_ON) ;
   return register_chrdev(ledmajor, name, &fops) ;
}

static void chrdevled_exit(void)
{
   printk("chrdevled_exit\n") ;
   iounmap(VIRLED_CCM_CCGR1) ;
   iounmap(VIRLED_GPIO1_GDIR) ;
   iounmap(VIRIOMUX_SW_MUX_CTL_PAD_GPIO1_IO03) ;
   iounmap(VIRIOMUX_SW_PAD_CTL_PAD_GPIO1_IO03) ;
   iounmap(VIRLED_GPIO1_DR) ;
   unregister_chrdev(ledmajor, name) ;
}

 module_init(chrdevled_init) ;
 module_exit(chrdevled_exit) ;

 MODULE_LICENSE("GPL") ;
 MODULE_AUTHOR("WANG") ;
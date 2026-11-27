#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/pwm.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/miscdevice.h>

#define PWM_ENCODER_LOW_BUFFER          0x4
#define PWM_ENCODER_FULL_BUFFER         0x8
#define PWM_ENCODER_CTRL                0xC

#define PWM_ENCODER_EN          (1 << 0)        /* 计数器使能 */
#define PWM_ENCODER_OE          (1 << 3)        /* 脉冲输出使能控制,低有效 */
#define PWM_ENCODER_SINGLE      (1 << 4)        /* 单脉冲控制 */
#define PWM_ENCODER_INTE        (1 << 5)        /* 中断使能 */
#define PWM_ENCODER_INT         (1 << 6)        /* 中断位 */
#define PWM_ENCODER_RST         (1 << 7)        /* low_level和full_pluse计数器重置 */
#define PWM_ENCODER_CAPTE       (1 << 8)        /* 测量脉冲使能 */
#define PWM_ENCODER_INVERT      (1 << 9)        /* 输出翻转使能 */
#define PWM_ENCODER_DZONE       (1 << 10)       /* 防死区使能 */


#define MISCENCODER_MINOR   255
#define MISCENCODER_NAME    "encoder"
#define TIMER_EXPIRES       100



static void __iomem *pwm_base_address;

struct Encoder_dev{
    struct resource *res;
    struct timer_list timer;            /* 定时器 */
    int dir_gpio;

    void *private_data;

};

struct Encoder_dev encoder;

static int dir_gpio_init(struct Encoder_dev *dev)
{
    int ret;
    struct platform_device *pdev = (struct platform_device *)dev->private_data;

    dev->dir_gpio = of_get_named_gpio(pdev->dev.of_node, "encoder1-dir", 0);
    if(dev->dir_gpio < 0) {
        printk("no find gpio\r\n");
        ret = -ENOENT;
        goto fail_getgpio;
    }

    ret = gpio_request(dev->dir_gpio, "encoder-dir");
    if(ret) {
        printk("gpio request error\r\n");
        ret = -EBUSY;
        goto fail_gpiorequest;
    }

    ret = gpio_direction_input(dev->dir_gpio);
    if(ret) {
        printk("gpio direction input error\r\n");
        ret = -EINVAL;
        goto fail_input;
    }


    return 0;


fail_input:
    gpio_free(dev->dir_gpio);
fail_gpiorequest:
fail_getgpio:
    return ret;
}

static int pwm_encoder_init(struct Encoder_dev *dev)
{
    int ret;

    struct platform_device *pdev = (struct platform_device*)dev->private_data;
    dev->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if(dev->res == NULL) {
        printk("platform_get_resource error\r\n");
        ret = -ENOENT;
        goto fail_get;
    }
    pwm_base_address = ioremap(dev->res->start, resource_size(dev->res));

    return 0;

fail_get:
    return ret;
}

static int encoder_open (struct inode *inode, struct file *file)
{
    u16 val;
    val = readl(pwm_base_address + PWM_ENCODER_CTRL);       //获取控制寄存器

    val |= PWM_ENCODER_EN;
    val |= PWM_ENCODER_CAPTE;

    writel(val, pwm_base_address + PWM_ENCODER_CTRL);
    printk("open encoder\r\n");

    return 0;
}

static int encoder_release (struct inode *inode, struct file *file)
{
    u32 val = 0;
    writel(val, pwm_base_address + PWM_ENCODER_CTRL);
    printk("release encoder\r\n");
    return 0;
}

static ssize_t encoder_read (struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    int value = readl(pwm_base_address + PWM_ENCODER_FULL_BUFFER);
    printk("count: %d\r\n",value);
    return 0;
}

static const struct file_operations encoder_fops = {
    .owner = THIS_MODULE,
    .open = encoder_open,
    .release = encoder_release,
    .read = encoder_read,
};

/* 定时器处理函数 */
static void timer_func(struct timer_list *t) {
    mod_timer(&encoder.timer, jiffies + msecs_to_jiffies(TIMER_EXPIRES));
    int value = 100000000 / readl(pwm_base_address + PWM_ENCODER_FULL_BUFFER) / 200 * (gpio_get_value(encoder.dir_gpio) * 2 -1);
    // int value = 100000000 / readl(pwm_base_address + PWM_ENCODER_FULL_BUFFER) / 200;
    printk("count: %d\r\n",value);
    
}

static struct miscdevice misc_encoder = {
    .minor = MISCENCODER_MINOR,
    .name = MISCENCODER_NAME,
    .fops = &encoder_fops,
};

static int encoder_probe(struct platform_device *pdev)
{
    printk("matching finish\r\n");

    int ret;

    ret = misc_register(&misc_encoder);
    if(ret) {
        printk("misc_register error\r\n");
        ret = -EINVAL;
        goto fail_misc;
    }

    encoder.private_data = pdev;  

    ret = pwm_encoder_init(&encoder);
    if(ret < 0) {
        printk("encoder init error\r\n");
        goto fail_encoder;
    }

    u16 val;
    val = readl(pwm_base_address + PWM_ENCODER_CTRL);       //获取控制寄存器

    val |= PWM_ENCODER_EN;
    val |= PWM_ENCODER_CAPTE;

    writel(val, pwm_base_address + PWM_ENCODER_CTRL);

    ret = dir_gpio_init(&encoder);
    if(ret < 0) {
        printk("gpio init error\r\n");
        goto fail_dir;
    }


    timer_setup(&encoder.timer, timer_func, 0);
    mod_timer(&encoder.timer, jiffies + msecs_to_jiffies(TIMER_EXPIRES));

    printk("encoder chrdev init finish\r\n");
    return 0;



fail_dir:
fail_encoder:
fail_misc:
    return ret;
}

static int encoder_remove(struct platform_device *pdev)
{
    
    del_timer_sync(&encoder.timer);

    gpio_free(encoder.dir_gpio);

    u32 val = 0;
    writel(val, pwm_base_address + PWM_ENCODER_CTRL);
    iounmap(pwm_base_address); 

    misc_deregister(&misc_encoder);
    printk("free finish \r\n");
    return 0;
}


static const struct of_device_id of_encoder_match[] = {
    {.compatible = "icar,encoder1"},
    {},
};

static struct platform_driver encoder_driver = {
    .probe = encoder_probe,
    .remove = encoder_remove,
    .driver = {
        .name = "pwm0_encoder",
        .of_match_table = of_encoder_match,
    },
};

module_platform_driver(encoder_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("WwuSama");
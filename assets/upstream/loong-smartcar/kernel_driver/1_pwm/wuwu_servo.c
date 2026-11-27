#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/pwm.h>

#define SERVO_MISCMINOR     255
#define SERVO_MISCNAME      "wuwu_servo"
#define SERVO_MAXDUTY       10000       /* 最大占空比 */
#define SERVO_HZ            50          /* 频率 */

// 在头文件或模块顶部添加
#define SERVO_IOCTL_MAGIC 'S'
#define SERVO_SET_DUTY      _IOW(SERVO_IOCTL_MAGIC, 1, int)
#define SERVO_SET_FREQ      _IOW(SERVO_IOCTL_MAGIC, 2, int)

struct Servo_dev{
    struct device *dev;
    struct pwm_device *pwm;
    int hz2ns;      /* 周期 */
    int ratio;      /* 占空比转换 */
};

struct Servo_dev servo;

static DEFINE_MUTEX(servo_mutex);


/*  ATIM驱动有点奇怪极性一直是反着的
 *  
 *  void set_servo_duty(int duty)
 *  设置舵机占空比(0~10000)
 *  反极性 适应驱动
 */
static void set_servo_duty(int duty)
{
    duty = (duty > SERVO_MAXDUTY) ? SERVO_MAXDUTY : (duty < 0) ? 0 : duty;
    duty = (SERVO_MAXDUTY - duty);

    pwm_config(servo.pwm, duty*servo.ratio, servo.hz2ns);
}

/*  设置舵机频率
 *
 *  void set_servo_freq(int freq)
 *  设置频率
 *  频率设置后占空比自动清0
 */
static void set_servo_freq(int freq)
{
    if(freq <= 0 || freq >= 1000000000) return;

    /* 舵机hz(频率)转ns(周期) */
    servo.hz2ns = 1000000000 / freq;
    /* 舵机占空比转换值 = 周期/占空比 */
    servo.ratio = servo.hz2ns / SERVO_MAXDUTY;

    pwm_config(servo.pwm, 0*servo.ratio, servo.hz2ns);
}

static int pwm_init(struct Servo_dev *servo)
{
    servo->pwm = pwm_get(servo->dev, 0);
    if(IS_ERR(servo->pwm)) {
        printk("get pwm error\r\n");
        goto fail_pwm;
    }
    return 0;

fail_pwm:
    return -ENAVAIL;
}

static int servo_open (struct inode *inode, struct file *file)
{
    set_servo_duty(0);
    return 0;
}

static int servo_release (struct inode *inode, struct file *file)
{
    set_servo_duty(0);
    return 0;
}

static long servo_unlocked_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
    mutex_lock(&servo_mutex);
    void __user *argp = (void __user *)arg;

    switch(cmd)
    {
        case SERVO_SET_DUTY: {
            int duty;
            if(copy_from_user(&duty, argp, sizeof(duty)))
                return -EFAULT;
            set_servo_duty(duty);
        }
            break;
        case SERVO_SET_FREQ: {
            int freq;
            if(copy_from_user(&freq, argp, sizeof(freq)))
                return -EFAULT;
            set_servo_freq(freq);
        }
            break;
        default:
            return -ENOTTY;
    }
    mutex_unlock(&servo_mutex);
    return 0;
}

static const struct file_operations servo_fops = {
    .owner = THIS_MODULE,
    .open = servo_open,
    .release = servo_release,
    .unlocked_ioctl = servo_unlocked_ioctl,
};

static struct miscdevice servo_miscdev = {
    .minor = SERVO_MISCMINOR,
    .name = SERVO_MISCNAME,
    .fops = &servo_fops,
};


static int servo_probe(struct platform_device *pdev)
{   
    int ret;
    printk("find servo\r\n");

    ret = misc_register(&servo_miscdev);
    if(ret) {
        ret = -EINVAL;
        printk("misc register error\r\n");
        goto fail_misc;
    }

    servo.dev = &pdev->dev;
    /* 舵机hz(频率)转ns(周期) */
    servo.hz2ns = 1000000000 / SERVO_HZ;
    /* 舵机占空比转换值 = 周期/占空比 */
    servo.ratio = servo.hz2ns / SERVO_MAXDUTY;
    
    ret = pwm_init(&servo);
    if(ret < 0) {
        printk("pwm init error\t\n");
        goto fail_pwm;
    }

    set_servo_duty(0);
    pwm_enable(servo.pwm);

    return 0;


fail_pwm:
fail_misc:
    return ret;
}

static int servo_remove(struct platform_device *pdev)
{
    printk("remove servo\r\n");
    pwm_disable(servo.pwm);
    pwm_put(servo.pwm);
    misc_deregister(&servo_miscdev);

    return 0;
}

static const struct of_device_id servo_of_match[] ={
    {.compatible = "icar,servo"},
    {},
};

static struct platform_driver servo_driver = {
    .probe = servo_probe,
    .remove = servo_remove,
    .driver = {
        .name = "servo_pwm",
        .of_match_table = servo_of_match,
    },
};



module_platform_driver(servo_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("WwuSama");

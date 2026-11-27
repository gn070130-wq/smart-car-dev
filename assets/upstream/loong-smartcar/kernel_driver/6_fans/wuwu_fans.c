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

#define FANS_IOCTL_MAGIC    'F'
#define FANS_SET_DUTY    _IOW(FANS_IOCTL_MAGIC, 1, struct Fans_duty)

struct Fans_duty {
    int dutyl;
    int dutyr;
};

static DEFINE_MUTEX(fans_mutex);

#define FANS_MISCMINOR      255
#define FANS_MISCNAME       "wuwu_fans"
#define FANS_MAXDUTY        10000       /* 最大占空比 */
#define FANS_HZ             100         /* 频率 */

struct Fans_dev {
    struct device *dev;
    struct pwm_device *pwmL;
    struct pwm_device *pwmR;

    int hz2ns;      /* HZ转ns */
    int Lduty;      /* 周期 */
    int Rduty;      /* 周期 */
    
};

struct Fans_dev fans;

static void fans_set(int dutyL, int dutyR)
{
    int Lvalue = 0, Rvalue = 0;
    fans.Lduty = (dutyL > 100) ? 100 : (dutyL < 0) ? 0 : dutyL;
    fans.Rduty = (dutyR > 100) ? 100 : (dutyR < 0) ? 0 : dutyR;
    Lvalue = 1000000 * fans.Lduty / 100;
    Rvalue = 1000000 * fans.Rduty / 100;
    pwm_config(fans.pwmL, fans.hz2ns-(1000000+Lvalue), fans.hz2ns);
    pwm_config(fans.pwmR, fans.hz2ns-(1000000+Rvalue), fans.hz2ns);
}

static void fans_enable(void)
{
    pwm_enable(fans.pwmL);
    pwm_enable(fans.pwmR);
}

static void fans_disenable(void)
{
    pwm_disable(fans.pwmL);
    pwm_disable(fans.pwmR);
}

static int fans_open (struct inode *inode, struct file *file)
{
    fans_enable();
    return 0;
}

static int fans_release (struct inode *inode, struct file *file)
{
    fans_disenable();
    return 0;
}

static long fans_unlocked_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
    mutex_lock(&fans_mutex);
    void __user *argp = (void __user *)arg;

    switch(cmd)
    {
        case FANS_SET_DUTY: {
            struct Fans_duty dutys;
            if(copy_from_user(&dutys, argp, sizeof(dutys)))
                return -EFAULT;
            fans_set(dutys.dutyl, dutys.dutyr);
        }
            break;
        default:
            break;
    }
    mutex_unlock(&fans_mutex);
    return 0;
}

static const struct file_operations fans_fops = {
    .owner = THIS_MODULE,
    .open = fans_open,
    .release = fans_release,
    .unlocked_ioctl = fans_unlocked_ioctl,
};

static struct miscdevice fans_miscdev = {
    .minor = FANS_MISCMINOR,
    .name = FANS_MISCNAME,
    .fops = &fans_fops,
};

static int pwm_init(void)
{
    fans.pwmL = pwm_get(fans.dev, "Lfans-pwm");
    if(IS_ERR(fans.pwmL)) {
        printk("get Lpwm error\r\n");
        goto fail_Lpwm;
    }
    fans.pwmR = pwm_get(fans.dev, "Rfans-pwm");
    if(IS_ERR(fans.pwmR)) {
        printk("get Rpwm error\r\n");
        goto fail_Rpwm;
    }
    return 0;
fail_Rpwm:
    pwm_put(fans.pwmL);
fail_Lpwm:
    return -ENAVAIL;
}

static void pwm_free(void)
{
    pwm_put(fans.pwmL);
    pwm_put(fans.pwmR);
}

static int fans_probe(struct platform_device *pdev)
{
    int ret;
    printk("find fans successful\r\n");
    ret = misc_register(&fans_miscdev);
    if(ret) {
        ret = -EINVAL;
        printk("misc_register error\r\n");
        goto fail_misc;
    }

    fans.dev = &pdev->dev;
    ret = pwm_init();
    if(ret < 0) {
        printk("pwm init error\r\n");
        goto fail_pwm;
    }

    fans.hz2ns = 1000000000 / FANS_HZ;
    fans.Lduty = 0;
    fans.Rduty = 0;

    return 0;


fail_pwm:
    misc_deregister(&fans_miscdev);
fail_misc:
    return ret;
}

static int fans_remove(struct platform_device *pdev)
{
    pwm_free();
    misc_deregister(&fans_miscdev);
    printk("fans remove successful\r\n");
    return 0;
}

static const struct of_device_id fans_of_match[] = {
    {.compatible = "icar,fans"},
    {},
};

static struct platform_driver fans_driver = {
    .probe = fans_probe,
    .remove = fans_remove,
    .driver = {
        .name = "fans_pwm",
        .of_match_table = fans_of_match,
    },
};

module_platform_driver(fans_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("WwuSama");
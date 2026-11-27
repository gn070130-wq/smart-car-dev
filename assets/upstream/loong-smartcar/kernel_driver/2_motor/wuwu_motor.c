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
#include <linux/miscdevice.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/delay.h>

/*  ioctl调用的宏定义
 *
 *  _IO(type,nr) //没有参数的命令
 *  _IOR(type,nr,size) //该命令是从驱动读取数据
 *  _IOW(type,nr,size) //该命令是从驱动写入数据
 *  _IOWR(type,nr,size) //双向数据传输
 */

// 在头文件或模块顶部添加
#define MOTOR_IOCTL_MAGIC 'O'
#define MOTOR_SET_SPEED    _IOW(MOTOR_IOCTL_MAGIC, 1, struct encoder_target)
#define MOTOR_GET_SPEED    _IOR(MOTOR_IOCTL_MAGIC, 2, struct encoder_counts)
#define MOTOR_SET_LPID    _IOW(MOTOR_IOCTL_MAGIC, 3, struct set_pid)
#define MOTOR_SET_RPID    _IOW(MOTOR_IOCTL_MAGIC, 4, struct set_pid)

static DEFINE_MUTEX(motor_mutex);

struct encoder_target {
    int L_encoder_target;
    int R_encoder_target;
};

struct encoder_counts {
    int L_encoder_counts;
    int R_encoder_counts;
};

struct set_pid {
    int kp, ki, kd;
};

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

#define MISCMOTOR_MINOR         255
#define MISCMOTOR_NAME          "wuwu_motor"
#define TIMER_EXPIRES           10
#define NEED_PID                (1)             /* 是否需要PID闭环 */

#define PWM_HZ                  (20000)
#define LIMITER                 (6000)          /* 占空比限幅 */
#define PID_AMPLIFY             (100)           /* PID放大器 */
#define NEED_MOTOR              (1)             /* 是否驱动电机 */

typedef struct
{
    int kp,ki,kd;                         //三个系数
    int error,lastError,lastlastError;    //误差、上次误差、上上次误差
    int output,maxOutput;                 //输出、输出限幅
} PID;


PID pidL, pidR;

/**
 * ************************************************************************
 * @brief 增量式PID参数的初始化
 * 
 * @param[in] pid  pid指针
 * @param[in] p  初始化设定的p
 * @param[in] i  初始化设定的i
 * @param[in] d  初始化设定的d
 * @param[in] maxOutput  输出限幅值
 * 
 * ************************************************************************
 */
static void Incremental_PID_Init(PID *pid, int p, int i, int d, int maxOutput)
{
	pid->kp = p;
	pid->ki = i;
	pid->kd = d;
	pid->maxOutput = maxOutput;
}

/**
 * ************************************************************************
 * @brief 增量式PID控制器
 *          
 * @param[in] pid  pid指针
 * @param[in] set_value  目标值
 * @param[in] get_value  反馈值
 * 
 * ************************************************************************
 */
static void Incremental_PID_Calc(PID *pid, int set_value,int get_value)
{
	pid->error = set_value - get_value;      									//计算偏差
	pid->output += pid->kp*(pid->error - pid->lastError) + pid->ki*pid->error + 
				pid->kd*(pid->error - 2*pid->lastError + pid->lastlastError);			//增量式PI控制器
	pid->lastlastError = pid->lastError;    											//保存上上次误差
	pid->lastError = pid->error;	           											//保存上一次偏差

	if (pid->output > pid->maxOutput)
        pid->output = pid->maxOutput;
    else if (pid->output < -pid->maxOutput)
        pid->output = -pid->maxOutput;													//输出限幅
}

static void PID_clean(void)
{
    pidL.error = 0;
    pidL.lastError = 0;
    pidL.lastlastError = 0;
    pidL.output = 0;

    pidR.error = 0;
    pidR.lastError = 0;
    pidR.lastlastError = 0;
    pidR.output = 0;
}


static void __iomem *pwmL_base_address;
static void __iomem *pwmR_base_address;

struct Motor_dev{
    /* 是否需要闭环 */
    char isPID;
    /* 左电机控制 */
    int motorL_dir;
    struct pwm_device *motorL_pwm;
    int encoderL_dir;
    int encoderL_tar;
    int encoderL_counts;
    /* 右电机控制*/
    int motorR_dir;
    struct pwm_device *motorR_pwm;
    int encoderR_dir;
    int encoderR_tar;
    int encoderR_counts;
    /* 编码器寄存器获取 */
    struct resource *EncoderL_res;
    struct resource *EncoderR_res;
    /* 定时器 */
    struct timer_list timer;
    /* pwm高电平时间和周期 */
    int Lpwm_value;
    int Rpwm_value;
    int pwm_ns;
    /* 私有数据 */
    void *private_data;
};

struct Motor_dev motor;
/* 创建并初始化自旋锁 */
static DEFINE_SPINLOCK(motor_spinlock);     /* 自旋锁 */

#if NEED_MOTOR
static int motorL_init(struct Motor_dev *dev)
{
    int ret;
    struct platform_device *pdev = (struct platform_device*)dev->private_data;

    dev->motorL_dir = of_get_named_gpio(pdev->dev.of_node, "motorL-dir", 0);
    if(dev->motorL_dir < 0) {
        printk("no find Lmotor gpio\r\n");
        ret = -ENOENT;
        goto fail_getgpio;
    }

    ret = gpio_request(dev->motorL_dir, "motorL-dir");
    if(ret) {
        printk("motorL-dir gpio request error\r\n");
        ret = -EBUSY;
        goto fail_gpiorequest;
    }

    ret = gpio_direction_output(dev->motorL_dir, 1);
    if(ret) {
        printk("gpio direction input error\r\n");
        ret = -EINVAL;
        goto fail_output;
    }

    dev->motorL_pwm = pwm_get(&pdev->dev, "Lmotor-pwm");
    if(IS_ERR(dev->motorL_pwm)) {
        printk("get pwm error\r\n");
        ret = -EINVAL;
        goto fail_pwm;
    }

    return 0;

fail_pwm:
fail_output:
    gpio_free(dev->motorL_dir);
fail_gpiorequest:
fail_getgpio:
    return ret;
}

static int motorR_init(struct Motor_dev *dev)
{
    int ret;
    struct platform_device *pdev = (struct platform_device*)dev->private_data;

    dev->motorR_dir = of_get_named_gpio(pdev->dev.of_node, "motorR-dir", 0);
    if(dev->motorR_dir < 0) {
        printk("no find Rmotor gpio\r\n");
        ret = -ENOENT;
        goto fail_getgpio;
    }

    ret = gpio_request(dev->motorR_dir, "motorR-dir");
    if(ret) {
        printk("motorR-dir gpio request error\r\n");
        ret = -EBUSY;
        goto fail_gpiorequest;
    }

    ret = gpio_direction_output(dev->motorR_dir, 1);
    if(ret) {
        printk("gpio direction input error\r\n");
        ret = -EINVAL;
        goto fail_output;
    }

    dev->motorR_pwm = pwm_get(&pdev->dev, "Rmotor-pwm");
    if(IS_ERR(dev->motorR_pwm)) {
        printk("get pwm error\r\n");
        ret = -EINVAL;
        goto fail_pwm;
    }

    return 0;

fail_pwm:
fail_output:
    gpio_free(dev->motorR_dir);
fail_gpiorequest:
fail_getgpio:
    return ret;
}

static void motorL_free(void)
{
    pwm_put(motor.motorL_pwm);
    gpio_free(motor.motorL_dir);
}

static void motorR_free(void)
{
    pwm_put(motor.motorR_pwm);
    gpio_free(motor.motorR_dir);
}

/* motorL_set_speed(int duty)
 *
 * duty -> (-10000 ~ 10000)
 * 通过value的正负判断电机速度
 * 
 */

static void motorL_set_speed(int duty)
{
    /* 限制输入大小 */
    duty = (duty > LIMITER) ? LIMITER : (duty < -LIMITER) ? -LIMITER : duty;
    /* 占空比转换 */
    motor.Lpwm_value = abs(duty) * motor.pwm_ns / 10000;
    /* 防止value < 10ns */
    motor.Lpwm_value = (motor.Lpwm_value < 10) ? 10 : motor.Lpwm_value;
    if(duty >= 0) {
        gpio_set_value(motor.motorL_dir, 1);
        pwm_config(motor.motorL_pwm, motor.Lpwm_value, motor.pwm_ns);
    } else {
        gpio_set_value(motor.motorL_dir, 0);
        pwm_config(motor.motorL_pwm, motor.Lpwm_value, motor.pwm_ns);
    }
}

/* motorR_set_speed(int duty)
 *
 * duty -> (-10000 ~ 10000)
 * 通过value的正负判断电机速度
 * 
 */

static void motorR_set_speed(int duty)
{
    /* 限制输入大小 */
    duty = (duty > LIMITER) ? LIMITER : (duty < -LIMITER) ? -LIMITER : duty;
    /* 占空比转换 */
    motor.Rpwm_value = abs(duty) * motor.pwm_ns / 10000;
    /* 防止value < 10ns */
    motor.Rpwm_value = (motor.Rpwm_value < 10) ? 10 : motor.Rpwm_value;
    if(duty >= 0) {
        gpio_set_value(motor.motorR_dir, 1);
        pwm_config(motor.motorR_pwm, motor.Rpwm_value, motor.pwm_ns);
    } else {
        gpio_set_value(motor.motorR_dir, 0);
        pwm_config(motor.motorR_pwm, motor.Rpwm_value, motor.pwm_ns);
    }
}
#endif

/* encoder_dir_init
 *
 * 初始化方向编码器 gpio引脚
 */
static int encoder_dir_init(struct Motor_dev *dev)
{
    int ret;
    struct platform_device *pdev = (struct platform_device*)dev->private_data;

    dev->encoderL_dir = of_get_named_gpio(pdev->dev.of_node, "encoderL-dir", 0);
    if(dev->encoderL_dir < 0) {
        printk("no find Lgpio\r\n");
        ret = -ENOENT;
        goto fail_getgpio;
    }

    dev->encoderR_dir = of_get_named_gpio(pdev->dev.of_node, "encoderR-dir", 0);
    if(dev->encoderR_dir < 0) {
        printk("no find Rgpio\r\n");
        ret = -ENOENT;
        goto fail_getgpio;
    }

    ret = gpio_request(dev->encoderL_dir, "encoderL-dir");
    if(ret) {
        printk("L gpio request error\r\n");
        ret = -EBUSY;
        goto fail_gpiorequest;
    }

    ret = gpio_request(dev->encoderR_dir, "encoderR-dir");
    if(ret) {
        printk("R gpio request error\r\n");
        ret = -EBUSY;
        goto fail_gpiorequest;
    }

    ret = gpio_direction_input(dev->encoderL_dir);
    if(ret) {
        printk("L gpio direction input error\r\n");
        ret = -EINVAL;
        goto fail_input;
    }

    ret = gpio_direction_input(dev->encoderR_dir);
    if(ret) {
        printk("R gpio direction input error\r\n");
        ret = -EINVAL;
        goto fail_input;
    }

    return 0;

fail_input:
    gpio_free(dev->encoderL_dir);
    gpio_free(dev->encoderR_dir);
fail_gpiorequest:
fail_getgpio:
    return ret;
}

/*  encoder_pwm_init
 *
 *  初始化编码器pwm通道
 * 
 */
static int encoder_pwm_init(struct Motor_dev *dev)
{
    int ret;
    
    struct platform_device *pdev = (struct platform_device*)dev->private_data;
    dev->EncoderL_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if(dev->EncoderL_res == NULL) {
        printk("platform_get_resource 0 error\r\n");
        ret = -ENOENT;
        goto fail_get;
    }

    dev->EncoderR_res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if(dev->EncoderR_res == NULL) {
        printk("platform_get_resource 1 error\r\n");
        ret = -ENOENT;
        goto fail_get;
    }
    pwmL_base_address = ioremap(dev->EncoderL_res->start, resource_size(dev->EncoderL_res));
    pwmR_base_address = ioremap(dev->EncoderR_res->start, resource_size(dev->EncoderR_res));

    u16 val = 0;
    val = readl(pwmL_base_address + PWM_ENCODER_CTRL);       //获取控制寄存器
    val |= PWM_ENCODER_EN;
    val |= PWM_ENCODER_CAPTE;
    writel(val, pwmL_base_address + PWM_ENCODER_CTRL);

    val = readl(pwmR_base_address + PWM_ENCODER_CTRL);       //获取控制寄存器
    val |= PWM_ENCODER_EN;
    val |= PWM_ENCODER_CAPTE;
    writel(val, pwmR_base_address + PWM_ENCODER_CTRL);

    return 0;

fail_get:
    return ret;
}

static void encoder_free(void)
{
    u32 val = 0;
    writel(val, pwmL_base_address + PWM_ENCODER_CTRL);
    writel(val, pwmR_base_address + PWM_ENCODER_CTRL);
    iounmap(pwmL_base_address); 
    iounmap(pwmR_base_address); 
    gpio_free(motor.encoderL_dir);
    gpio_free(motor.encoderR_dir);
}

/* ***********************文件操作函数集合************************** */
/* *************************************************************** */

static int motor_open(struct inode *inode, struct file *filp)
{  
    pwm_enable(motor.motorL_pwm);
    pwm_enable(motor.motorR_pwm);
    motor.isPID = 'T';
    mod_timer(&motor.timer, jiffies + msecs_to_jiffies(TIMER_EXPIRES));
    return 0;
}

static int motor_release(struct inode *inode, struct file *filp)
{
    motor.isPID = 'F';
    msleep(15);
    PID_clean();
#if NEED_MOTOR
    pwm_config(motor.motorL_pwm, 10, motor.pwm_ns);
    pwm_config(motor.motorR_pwm, 10, motor.pwm_ns);
    pwm_disable(motor.motorL_pwm);
    pwm_disable(motor.motorR_pwm);
#endif
    return 0;
}

static long motor_unlocked_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
    mutex_lock(&motor_mutex);
    void __user *argp = (void __user *)arg;

    switch(cmd)
    {
    case MOTOR_SET_SPEED: {
        struct encoder_target encoTarget;
        if(copy_from_user(&encoTarget, argp, sizeof(encoTarget)))
            return -EFAULT;
        motor.encoderL_tar = encoTarget.L_encoder_target;
        motor.encoderR_tar = encoTarget.R_encoder_target;
    }
        break;
    case MOTOR_GET_SPEED: {
        struct encoder_counts encoCounts;
        unsigned long flags;
        /* 上锁保护 */
        spin_lock_irqsave(&motor_spinlock, flags);
        encoCounts.L_encoder_counts = motor.encoderL_counts;
        encoCounts.R_encoder_counts = motor.encoderR_counts;
        spin_unlock_irqrestore(&motor_spinlock, flags);
        /* 解锁释放 */
        if(copy_to_user(argp, &encoCounts, sizeof(encoCounts)))
            return -EFAULT;
    }
        break;
    case MOTOR_SET_LPID: {
        struct set_pid pid;
        if(copy_from_user(&pid, argp, sizeof(pid)))
            return -EFAULT;
        Incremental_PID_Init(&pidL, pid.kp, pid.ki, pid.kd, LIMITER * PID_AMPLIFY);
    }
        break;
    case MOTOR_SET_RPID: {
        struct set_pid pid;
        if(copy_from_user(&pid, argp, sizeof(pid)))
            return -EFAULT;
        Incremental_PID_Init(&pidR, pid.kp, pid.ki, pid.kd, LIMITER * PID_AMPLIFY);
    }
        break;
    default:
        return -ENOTTY;
    }

    mutex_unlock(&motor_mutex);
    return 0;
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */

const struct file_operations motorL_fops = {
    .owner   = THIS_MODULE,
    .open    = motor_open,
    .release = motor_release,
    .unlocked_ioctl = motor_unlocked_ioctl,
};

/* WWWWWWWWWWWWWWWWWWWWWWWWWWWWW定时器处理函数WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW */
static void timer_func(struct timer_list *t) {
    unsigned long flags;
    if(motor.isPID == 'T')
    mod_timer(&motor.timer, jiffies + msecs_to_jiffies(TIMER_EXPIRES));

    /* 编码器速度方向融合 */
    s8 Le_dir = (gpio_get_value(motor.encoderL_dir)) ? -1 : 1;
    s8 Re_dir = (!gpio_get_value(motor.encoderR_dir)) ? -1 : 1;
    int L_count = 100000000 / readl(pwmL_base_address + PWM_ENCODER_FULL_BUFFER) / 300 * Le_dir;
    int R_count = 100000000 / readl(pwmR_base_address + PWM_ENCODER_FULL_BUFFER) / 300 * Re_dir;
    /* 上锁保护 */
    spin_lock_irqsave(&motor_spinlock, flags);
    motor.encoderL_counts = L_count;
    motor.encoderR_counts = R_count;
    spin_unlock_irqrestore(&motor_spinlock, flags);
    /* 解锁释放 */
#if NEED_PID
    /*电机闭环*/
    Incremental_PID_Calc(&pidL, motor.encoderL_tar, L_count);
    Incremental_PID_Calc(&pidR, motor.encoderR_tar, R_count);
    motorL_set_speed(pidL.output / PID_AMPLIFY);
    motorR_set_speed(pidR.output / PID_AMPLIFY);
#endif
}
/* WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW */

static struct miscdevice miscmotorL_dev = {
    .minor = MISCMOTOR_MINOR,
    .name = MISCMOTOR_NAME,
    .fops = &motorL_fops,
};

static int motor_probe(struct platform_device *pdev)
{
    int ret;
    ret = misc_register(&miscmotorL_dev);
    if(ret) {
        ret = -EINVAL;
        printk("misc_register error\r\n");
        goto fail_misc;
    }

    motor.private_data = pdev;

    ret = encoder_pwm_init(&motor);
    if(ret < 0) {
        printk("encoder_pwm_init error\r\n");
        goto fail_encoderpwm;
    }

    ret = encoder_dir_init(&motor);
    if(ret < 0) {
        printk("encoder init error\r\n");
        goto fail_encoderdir;
    }

    motor.pwm_ns = 1000000000 / PWM_HZ;

#if NEED_MOTOR
    ret = motorL_init(&motor);
    if(ret < 0) {
        printk("Lmotor init error\r\n");
        goto fail_motor;
    }

    ret = motorR_init(&motor);
    if(ret < 0) {
        printk("Rmotor init error\r\n");
        goto fail_motor;
    } 
#endif

#if NEED_PID
    Incremental_PID_Init(&pidL, 8000, 2600, 0, LIMITER * PID_AMPLIFY);
    Incremental_PID_Init(&pidR, 8000, 2600, 0, LIMITER * PID_AMPLIFY);
    motor.encoderL_tar = 0;
    motor.encoderR_tar = 0;
#endif

    motor.isPID = 'F';
    timer_setup(&motor.timer, timer_func, 0);  

    printk("probe finish\r\n");
    return 0;



#if NEED_MOTOR
fail_motor:
#endif
fail_encoderdir:
fail_encoderpwm:
fail_misc:
    return ret;
}

static int motor_remove(struct platform_device *pdev)
{
    del_timer_sync(&motor.timer);
    encoder_free();
#if NEED_MOTOR
    motorL_free();
    motorR_free();
#endif
    misc_deregister(&miscmotorL_dev);
    printk("motor remove finish\r\n");
    return 0;
}

static struct of_device_id of_pwm_motor_match[] = {
    {.compatible = "icar,motor"},
    {},
};

static struct platform_driver motorL_driver = {
    .probe = motor_probe,
    .remove = motor_remove,
    .driver = {
        .name = "motor",
        .of_match_table = of_pwm_motor_match,
    }

};

module_platform_driver(motorL_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("WwuSama");
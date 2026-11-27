#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/ioctl.h>
#include "imc20602.h"

#define IMC20602_COUNT   1
#define IMC20602_NAME    "wuwu_imc20602"

// 定义IOCTL命令
#define IMU_IOCTL_MAGIC   'i'
#define IMU_GET_ACCEL     _IOR(IMU_IOCTL_MAGIC, 1, struct imu_accel_data)
#define IMU_GET_GYRO      _IOR(IMU_IOCTL_MAGIC, 2, struct imu_gyro_data)

// 用户空间使用的数据结构（需在用户层头文件中同步）
struct imu_accel_data {
    short x, y, z;
};
struct imu_gyro_data {
    short x, y, z;
};

struct imc20602_dev{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;

    void *private_data;
    struct mutex lock;
    short icm20602_gyro_x, icm20602_gyro_y, icm20602_gyro_z;
    short icm20602_acc_x, icm20602_acc_y, icm20602_acc_z;
};

struct imc20602_dev imc20602;


static int imc20602_read_regs(struct imc20602_dev *dev, u8 reg, void *val, int len)
{
    struct i2c_client *client = (struct i2c_client *)dev->private_data;

    struct i2c_msg msg[] = {
        {
            .addr = client->addr,
            .flags = 0,
            .len = 1,
            .buf = &reg,
        },
        {
            .addr = client->addr,
            .flags = I2C_M_RD,
            .len = len,
            .buf = val,
        },
    };

    return i2c_transfer(client->adapter, msg, 2);
}

static unsigned char imc20602_read_onereg(struct imc20602_dev *dev, u8 reg)
{
    u8 data = 0;
    imc20602_read_regs(dev, reg, &data, 1);
    return data;
}

static int imc20602_write_regs(struct imc20602_dev *dev, u8 reg, u8 *buf, u8 len)
{
    u8 b [256];
    struct i2c_msg msg;
    struct i2c_client *client = (struct i2c_client *)dev->private_data;

    b[0] = reg;
    memcpy(&b[1], buf, len);

    msg.addr = client->addr;
    msg.flags = 0;
    msg.len = len + 1;
    msg.buf = b;

    return i2c_transfer(client->adapter, &msg, 1);
}

static void imc20602_write_onereg(struct imc20602_dev *dev, u8 reg, u8 data)
{
    u8 buf = 0;
    buf = data;
    imc20602_write_regs(dev, reg, &buf, 1);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     获取 ICM20602 加速度计数据
// 参数说明     void
// 返回参数     void
// 使用示例     icm20602_get_acc();                                             // 执行该函数后，直接查看对应的变量即可
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
static void icm20602_get_acc (struct imc20602_dev *dev)
{
    u8 dat[6];
    imc20602_read_regs(dev, ICM20602_ACCEL_XOUT_H, dat, 6);
    dev->icm20602_acc_x = (s16)(((u16)dat[0] << 8 | dat[1]));
    dev->icm20602_acc_y = (s16)(((u16)dat[2] << 8 | dat[3]));
    dev->icm20602_acc_z = (s16)(((u16)dat[4] << 8 | dat[5]));
}
//-------------------------------------------------------------------------------------------------------------------
// 函数简介     获取 ICM20602 陀螺仪数据
// 参数说明     void
// 返回参数     void
// 使用示例     icm20602_get_gyro();                                            // 执行该函数后，直接查看对应的变量即可
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
static void imc20602_read_gyro (struct imc20602_dev *dev)
{
    u8 dat[6];
    imc20602_read_regs(dev, ICM20602_GYRO_XOUT_H, dat, 6);
    dev->icm20602_gyro_x = (s16)(((u16)dat[0] << 8 | dat[1]));
    dev->icm20602_gyro_y = (s16)(((u16)dat[2] << 8 | dat[3]));
    dev->icm20602_gyro_z = (s16)(((u16)dat[4] << 8 | dat[5]));
}

static int imc20602_open (struct inode *inode, struct file *file)
{
    file->private_data = &imc20602;
    unsigned char value = 0;
    value = imc20602_read_onereg(file->private_data, ICM20602_WHO_AM_I);
    if(value != 0x12) {
        printk("Error: ICM20602 match failed\r\n");
    }
    

    imc20602_write_onereg(file->private_data, ICM20602_PWR_MGMT_1, 0x80);                     // 复位设备
    msleep(2);
    value = imc20602_read_onereg(file->private_data, ICM20602_PWR_MGMT_1);
    if(value != 0x41) {
        printk("Error: ICM20602 match failed\r\n");
    }

    imc20602_write_onereg(file->private_data, ICM20602_PWR_MGMT_1,     0x01);                 // 时钟设置
    imc20602_write_onereg(file->private_data, ICM20602_PWR_MGMT_2,     0x00);                 // 开启陀螺仪和加速度计
    imc20602_write_onereg(file->private_data, ICM20602_CONFIG,         0x01);                 // 176HZ 1KHZ
    imc20602_write_onereg(file->private_data, ICM20602_SMPLRT_DIV,     0x07);                 // 采样速率 SAMPLE_RATE = INTERNAL_SAMPLE_RATE / (1 + SMPLRT_DIV)

    // ICM20602_ACCEL_CONFIG 寄存器
    // 设置为 0x00 加速度计量程为 ±2  g   获取到的加速度计数据除以 16384  可以转化为带物理单位的数据 单位 g(m/s^2)
    // 设置为 0x08 加速度计量程为 ±4  g   获取到的加速度计数据除以 8192   可以转化为带物理单位的数据 单位 g(m/s^2)
    // 设置为 0x10 加速度计量程为 ±8  g   获取到的加速度计数据除以 4096   可以转化为带物理单位的数据 单位 g(m/s^2)
    // 设置为 0x18 加速度计量程为 ±16 g   获取到的加速度计数据除以 2048   可以转化为带物理单位的数据 单位 g(m/s^2)

    switch(ICM20602_ACC_SAMPLE_DEFAULT)
    {
        case ICM20602_ACC_SAMPLE_SGN_2G:
        {
            imc20602_write_onereg(file->private_data, ICM20602_ACCEL_CONFIG, 0x00);
            // icm20602_transition_factor[0] = 16384;
        }break;
        case ICM20602_ACC_SAMPLE_SGN_4G:
        {
            imc20602_write_onereg(file->private_data, ICM20602_ACCEL_CONFIG, 0x08);
            // icm20602_transition_factor[0] = 8192;
        }break;
        case ICM20602_ACC_SAMPLE_SGN_8G:
        {
            imc20602_write_onereg(file->private_data, ICM20602_ACCEL_CONFIG, 0x10);
            // icm20602_transition_factor[0] = 4096;
        }break;
        case ICM20602_ACC_SAMPLE_SGN_16G:
        {
            imc20602_write_onereg(file->private_data, ICM20602_ACCEL_CONFIG, 0x18);
            // icm20602_transition_factor[0] = 2048;
        }break;
        default:
        break;
    }

    // ICM20602_GYRO_CONFIG 寄存器
    // 设置为 0x00 陀螺仪量程为 ±250  dps    获取到的陀螺仪数据除以 131     可以转化为带物理单位的数据 单位为 °/s
    // 设置为 0x08 陀螺仪量程为 ±500  dps    获取到的陀螺仪数据除以 65.5    可以转化为带物理单位的数据 单位为 °/s
    // 设置为 0x10 陀螺仪量程为 ±1000 dps    获取到的陀螺仪数据除以 32.8    可以转化为带物理单位的数据 单位为 °/s
    // 设置为 0x18 陀螺仪量程为 ±2000 dps    获取到的陀螺仪数据除以 16.4    可以转化为带物理单位的数据 单位为 °/s
    switch(ICM20602_GYRO_SAMPLE_DEFAULT)
    {
        case ICM20602_GYRO_SAMPLE_SGN_250DPS:
        {
            imc20602_write_onereg(file->private_data, ICM20602_GYRO_CONFIG, 0x00);
            // icm20602_transition_factor[1] = 131.0;
        }break;
        case ICM20602_GYRO_SAMPLE_SGN_500DPS:
        {
            imc20602_write_onereg(file->private_data, ICM20602_GYRO_CONFIG, 0x08);
            // icm20602_transition_factor[1] = 65.5;
        }break;
        case ICM20602_GYRO_SAMPLE_SGN_1000DPS:
        {
            imc20602_write_onereg(file->private_data, ICM20602_GYRO_CONFIG, 0x10);
            // icm20602_transition_factor[1] = 32.8;
        }break;
        case ICM20602_GYRO_SAMPLE_SGN_2000DPS:
        {
            imc20602_write_onereg(file->private_data, ICM20602_GYRO_CONFIG, 0x18);
            // icm20602_transition_factor[1] = 16.4;
        }break;
        default:
        break;
    }

    imc20602_write_onereg(file->private_data, ICM20602_ACCEL_CONFIG_2, 0x03); // Average 4 samples   44.8HZ   //0x23 Average 16 samples
    return 0;
}

static int imc20602_release (struct inode *inode, struct file *file)
{ 
    return 0;
}

static long imc20602_unlocked_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct imc20602_dev *dev = file->private_data;
    void __user *argp = (void __user *)arg;
    mutex_lock(&imc20602.lock);
    switch (cmd)
    {
    case IMU_GET_ACCEL:
        struct imu_accel_data accel;
        icm20602_get_acc(dev);
        accel.x = dev->icm20602_acc_x;
        accel.y = dev->icm20602_acc_y;
        accel.z = dev->icm20602_acc_z;
        if (copy_to_user(argp, &accel, sizeof(accel)))
            ret = -EFAULT;
        break;
    
    case IMU_GET_GYRO: {
        struct imu_gyro_data gyro;
        imc20602_read_gyro(dev);
        gyro.x = dev->icm20602_gyro_x;
        gyro.y = dev->icm20602_gyro_y;
        gyro.z = dev->icm20602_gyro_z;
        if (copy_to_user(argp, &gyro, sizeof(gyro)))
            ret = -EFAULT;
        break;
    }
    
    default:
        ret = -ENOTTY;
        break;
    }
    mutex_unlock(&dev->lock);
    return ret;
}

static const struct file_operations imc20602_fops = {
    .owner = THIS_MODULE,
    .open = imc20602_open,
    .release = imc20602_release,
    .unlocked_ioctl = imc20602_unlocked_ioctl,
};


static int imc20602_probe(struct i2c_client *client)
{
    int ret;

    ret = alloc_chrdev_region(&imc20602.devid, 0, IMC20602_COUNT, IMC20602_NAME);
    if(ret < 0) {
        printk("alloc_chrdev_region error \r\n");
        goto fail_region;
    }

    imc20602.cdev.owner = THIS_MODULE;
    cdev_init(&imc20602.cdev, &imc20602_fops);
    ret = cdev_add(&imc20602.cdev, imc20602.devid, IMC20602_COUNT);
    if(ret < 0) {
        printk("cdev_add error\r\n");
        goto fail_cdev;
    }

    imc20602.class = class_create(IMC20602_NAME);
    if(IS_ERR(imc20602.class)) {
        printk("class_create error\r\n");
        ret = PTR_ERR(imc20602.class);
        goto fail_class;
    }

    imc20602.device = device_create(imc20602.class, NULL, imc20602.devid, NULL, IMC20602_NAME);
    if(IS_ERR(imc20602.device)) {
        printk("device_create error\r\n");
        ret = PTR_ERR(imc20602.device);
        goto fail_device;
    }

    mutex_init(&imc20602.lock);     // 初始化锁
    imc20602.private_data = client;
    printk("imc20608 probe successfully\r\n");

    return 0;

fail_device:
    class_destroy(imc20602.class);
fail_class:
    cdev_del(&imc20602.cdev);
fail_cdev:
    unregister_chrdev_region(imc20602.devid, IMC20602_COUNT);
fail_region:
    return ret;
}

static void imc20602_remove(struct i2c_client *client)
{
    device_destroy(imc20602.class ,imc20602.devid);
    class_destroy(imc20602.class);
    cdev_del(&imc20602.cdev);
    unregister_chrdev_region(imc20602.devid, IMC20602_COUNT);
    printk("remove successfully\r\n");
}

static const struct of_device_id imc20602_of_match[] = {
    {.compatible = "imc20602-i2c"},
    {},
};

static const struct i2c_device_id imc20602_id[] = {
    {"imc20602-i2c", 0},
    {},
};

static struct i2c_driver imc20602_driver = {
    .probe = imc20602_probe,
    .remove = imc20602_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "imc20602",
        .of_match_table = of_match_ptr(imc20602_of_match),
    },
    .id_table = imc20602_id,

};

module_i2c_driver(imc20602_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("WwuSama");
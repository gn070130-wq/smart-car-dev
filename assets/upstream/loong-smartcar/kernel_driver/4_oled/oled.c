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
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include "oledfont.h"

#define SSD1306_COUNT   1
#define SSD1306_NAME    "oled"

#define OLED_CMD    1
#define OLED_DATA   0


/*  ioctl调用的宏定义
 *
 *  _IO(type,nr) //没有参数的命令
 *  _IOR(type,nr,size) //该命令是从驱动读取数据
 *  _IOW(type,nr,size) //该命令是从驱动写入数据
 *  _IOWR(type,nr,size) //双向数据传输
 */

// 在头文件或模块顶部添加
#define OLED_IOCTL_MAGIC 'O'

#define OLED_SHOW_CHAR    _IOW(OLED_IOCTL_MAGIC, 1, struct oled_char_param)
#define OLED_SHOW_NUM     _IOW(OLED_IOCTL_MAGIC, 2, struct oled_num_param)
#define OLED_CLEAR        _IO(OLED_IOCTL_MAGIC, 3)
// 其他命令...
static DEFINE_MUTEX(oled_mutex);

struct oled_char_param {
    u8 x;
    u8 y;
    u8 *chr;
    u8 size;
    u8 mode;
};

struct oled_num_param {
    u8 x;
    u8 y;
    u32 num;
    u8 len;
    u8 size;
    u8 mode;
};

/*-------------------------------------------------------------------------------------------------------*/

u8 OLED_GRAM[128][8];

struct ssd1306_dev{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;

    void *private_data;
};

struct ssd1306_dev ssd1306;

#if 0
static int ssd1306_read_regs(struct ssd1306_dev *dev, u8 reg, void *val, int len)
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

static unsigned char ssd1306_read_onereg(struct ssd1306_dev *dev, u8 reg)
{
    u8 data = 0;
    ssd1306_read_regs(dev, reg, &data, 1);
    return data;
}
#endif

static int ssd1306_write_regs(struct ssd1306_dev *dev, u8 reg, u8 *buf, u8 len)
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

static void ssd1306_write_onereg(struct ssd1306_dev *dev, u8 reg, u8 data)
{
    u8 buf = 0;
    buf = data;
    ssd1306_write_regs(dev, reg, &buf, 1);
}

static void SSD1306_Write(u8 data, u8 is_cmd)
{
    u8 control_byte = is_cmd ? 0x00 : 0x40;
    ssd1306_write_onereg(&ssd1306, control_byte, data);
}


static void OLED_Refresh(void)
{
	u8 i, x;
	for(i=0;i<8;i++)
	{
		SSD1306_Write(0xb0+i,OLED_CMD); //设置行起始地址
		SSD1306_Write(0x00,OLED_CMD);   //设置低列起始地址
		SSD1306_Write(0x10,OLED_CMD);   //设置高列起始地址
		u8 data[128];
        for(x = 0; x < 128; x++) {
            data[x] = OLED_GRAM[x][i];
        }
        ssd1306_write_regs(&ssd1306, 0x40, data, 128); // 0x40 表示数据模式
    }
}

static void OLED_Clear(void)
{
	u8 i,n;
	for(i=0;i<8;i++) {
	   for(n=0;n<128;n++) {
			 OLED_GRAM[n][i]=0;//清除所有数据
        }
    }
	OLED_Refresh();//更新显示
}


static void oled_init(void) 
{
    SSD1306_Write(0xAE, OLED_CMD);
	SSD1306_Write(0xD5, OLED_CMD);
	SSD1306_Write(0x80, OLED_CMD);
	SSD1306_Write(0x81, OLED_CMD);
	SSD1306_Write(0xFF, OLED_CMD);
	SSD1306_Write(0xD3, OLED_CMD);
	SSD1306_Write(0x00, OLED_CMD);
	SSD1306_Write(0x40, OLED_CMD);
	SSD1306_Write(0x8D, OLED_CMD);
	SSD1306_Write(0x14, OLED_CMD);
	SSD1306_Write(0x20, OLED_CMD);
	SSD1306_Write(0x00, OLED_CMD);
	SSD1306_Write(0x21, OLED_CMD);
	SSD1306_Write(0x00, OLED_CMD);
	SSD1306_Write(0x7F, OLED_CMD);
	SSD1306_Write(0x22, OLED_CMD);
	SSD1306_Write(0x00, OLED_CMD);
	SSD1306_Write(0x07, OLED_CMD);
	SSD1306_Write(0xAF, OLED_CMD);
    OLED_Clear();
}

static u32 OLED_Pow(u8 m,u8 n)
{
	u32 result=1;
	while(n--)
	{
	  result*=m;
	}
	return result;
}

static void OLED_DrawPoint(u8 x,u8 y,u8 t)
{
	u8 i,m,n;
	i=y/8;
	m=y%8;
	n=1<<m;
	if(t){OLED_GRAM[x][i]|=n;}
	else
	{
		OLED_GRAM[x][i]=~OLED_GRAM[x][i];
		OLED_GRAM[x][i]|=n;
		OLED_GRAM[x][i]=~OLED_GRAM[x][i];
	}
}

static void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 size1,u8 mode)
{
	u8 i,m,temp,size2,chr1;
	u8 x0=x,y0=y;
	if(size1==8)size2=6;
	else size2=(size1/8+((size1%8)?1:0))*(size1/2);  //得到字体一个字符对应点阵集所占的字节数
	chr1=chr-' ';  //计算偏移后的值
	for(i=0;i<size2;i++)
	{
		if(size1==8)
			  {temp=asc2_0806[chr1][i];} //调用0806字体
		else if(size1==12)
        {temp=asc2_1206[chr1][i];} //调用1206字体
		else if(size1==16)
        {temp=asc2_1608[chr1][i];} //调用1608字体
		else if(size1==24)
        {temp=asc2_2412[chr1][i];} //调用2412字体
		else return;
		for(m=0;m<8;m++)
		{
			if(temp&0x01)OLED_DrawPoint(x,y,mode);
			else OLED_DrawPoint(x,y,!mode);
			temp>>=1;
			y++;
		}
		x++;
		if((size1!=8)&&((x-x0)==size1/2))
		{x=x0;y0=y0+8;}
		y=y0;
  }
}


static void OLED_ShowString(u8 x,u8 y,u8 *chr,u8 size1,u8 mode)
{
	while((*chr>=' ')&&(*chr<='~'))//判断是不是非法字符!
	{
		OLED_ShowChar(x,y,*chr,size1,mode);
		if(size1==8)x+=6;
		else x+=size1/2;
		chr++;
  }
	OLED_Refresh();
}

static void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size1,u8 mode)
{
	u8 t,temp,m=0;
	if(size1==8)m=2;
	for(t=0;t<len;t++)
	{
		temp=(num/OLED_Pow(10,len-t-1))%10;
			if(temp==0)
			{
				OLED_ShowChar(x+(size1/2+m)*t,y,'0',size1,mode);
      }
			else 
			{
			  OLED_ShowChar(x+(size1/2+m)*t,y,temp+'0',size1,mode);
			}
  }
	OLED_Refresh();
}

static int oled_open (struct inode *inode, struct file *file)
{
    file->private_data = &ssd1306;
    oled_init();
    return 0;
}

static int oled_release (struct inode *inode, struct file *file)
{
    // struct ssd1306_dev *dev = file->private_data;
    OLED_Clear();
    return 0;
}

static long oled_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    mutex_lock(&oled_mutex);
    void __user *argp = (void __user *)arg;
    
    switch (cmd) {
    case OLED_SHOW_CHAR: {
        struct oled_char_param param;
        if (copy_from_user(&param, argp, sizeof(param)))
            return -EFAULT;
        OLED_ShowString(param.x, param.y, param.chr, param.size, param.mode);
        break;
    }
    case OLED_SHOW_NUM: {
        struct oled_num_param param;
        if (copy_from_user(&param, argp, sizeof(param)))
            return -EFAULT;
        OLED_ShowNum(param.x, param.y, param.num, param.len, param.size, param.mode);
        break;
    }
    case OLED_CLEAR:
        OLED_Clear();
        break;
    default:
        return -ENOTTY;
    }
    mutex_unlock(&oled_mutex);
    return 0;
}

static const struct file_operations ssd1306_fops = {
    .owner = THIS_MODULE,
    .open = oled_open,
    .release = oled_release,
    .unlocked_ioctl = oled_ioctl,
};


static int ssd1306_probe(struct i2c_client *client)
{
    int ret;

    ret = alloc_chrdev_region(&ssd1306.devid, 0, SSD1306_COUNT, SSD1306_NAME);
    if(ret < 0) {
        printk("alloc_chrdev_region error \r\n");
        goto fail_region;
    }

    ssd1306.cdev.owner = THIS_MODULE;
    cdev_init(&ssd1306.cdev, &ssd1306_fops);
    ret = cdev_add(&ssd1306.cdev, ssd1306.devid, SSD1306_COUNT);
    if(ret < 0) {
        printk("cdev_add error\r\n");
        goto fail_cdev;
    }

    ssd1306.class = class_create(SSD1306_NAME);
    if(IS_ERR(ssd1306.class)) {
        printk("class_create error\r\n");
        ret = PTR_ERR(ssd1306.class);
        goto fail_class;
    }

    ssd1306.device = device_create(ssd1306.class, NULL, ssd1306.devid, NULL, SSD1306_NAME);
    if(IS_ERR(ssd1306.device)) {
        printk("device_create error\r\n");
        ret = PTR_ERR(ssd1306.device);
        goto fail_device;
    }

    ssd1306.private_data = client;


    printk("probe successfully\r\n");
    return 0;

fail_device:
    class_destroy(ssd1306.class);
fail_class:
    cdev_del(&ssd1306.cdev);
fail_cdev:
    unregister_chrdev_region(ssd1306.devid, SSD1306_COUNT);
fail_region:
    return ret;
}

static void ssd1306_remove(struct i2c_client *client)
{
    SSD1306_Write(0xAE, OLED_CMD);  //关闭屏幕

    device_destroy(ssd1306.class ,ssd1306.devid);
    class_destroy(ssd1306.class);
    cdev_del(&ssd1306.cdev);
    unregister_chrdev_region(ssd1306.devid, SSD1306_COUNT);
    printk("remove successfully\r\n");
}

static const struct of_device_id ssd1306_of_match[] = {
    {.compatible = "solomon,ssd1306fb-i2c"},
    {},
};

static const struct i2c_device_id ssd1306_id[] = {
    {"ssd1306fb-i2c", 0},
    {},
};

static struct i2c_driver ssd1306_driver = {
    .probe = ssd1306_probe,
    .remove = ssd1306_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "ssd1306",
        .of_match_table = of_match_ptr(ssd1306_of_match),
    },
    .id_table = ssd1306_id,

};

module_i2c_driver(ssd1306_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("WwuSama");
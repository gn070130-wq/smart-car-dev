#ifndef _OLED_H_
#define _OLED_H_

/*  ioctl调用的宏定义
 *
 *  _IO(type,nr) //没有参数的命令
 *  _IOR(type,nr,size) //该命令是从驱动读取数据
 *  _IOW(type,nr,size) //该命令是从驱动写入数据
 *  _IOWR(type,nr,size) //双向数据传输
 */
#define OLED_IOCTL_MAGIC 'O'
#define OLED_SHOW_CHAR _IOW(OLED_IOCTL_MAGIC, 1, struct oled_char_param)
#define OLED_SHOW_NUM  _IOW(OLED_IOCTL_MAGIC, 2, struct oled_num_param)
#define OLED_CLEAR     _IO(OLED_IOCTL_MAGIC, 3)



struct oled_char_param {
    unsigned char x;
    unsigned char y;
    unsigned char* chr;
    unsigned char size;
    unsigned char mode;
};

struct oled_num_param {
    unsigned char x;
    unsigned char y;
    unsigned int num;
    unsigned char len;
    unsigned char size;
    unsigned char mode;
};

#endif

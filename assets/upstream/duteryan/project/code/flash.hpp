#ifndef __FLASH_hpp__
#define __FLASH_hpp__

#include  "zf_common_headfile.hpp"

extern zf_driver_file_string file_u32;

#define FILENAME    "parameter_new.txt"

extern char buffer[100];

// 原来 4页 → 现在改成 6页（足够存 Para1~Para5 + CAMA 相机参数）
extern uint32 parameter_data[6][14];    // 6页参数，每页14个

void Parameter_Init(void);
void Parameter_Write(void);
void Parameter_Read(void);

#endif

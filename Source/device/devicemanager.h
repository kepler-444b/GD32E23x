#ifndef _DEVICEMANAGER_H_
#define _DEVICEMANAGER_H_
#include <stdio.h>

// 设备名称
typedef enum {
    triac_driver = 0x01, // 4路可控硅调光
} dev_type;

// 命令类型
typedef enum {
    get_dev_info = 0x05, // 获取设备信息
    set_dev_info = 0x06, // 设置设备信息
} cmd_type;

// 条件编译
#define TRIAC_DRIVER       // 4路可控硅调光
#define TRIAC_DRIVER_VER 1 // 4路可控硅调光固件版本

#endif //_DEVICEMANAGER_H_
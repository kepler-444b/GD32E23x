#include "device.h"
#include "devicemanager.h"
#include "triac_driver/triac_driver.h"

// 根据宏定义,执行条件编译
void jump_to_device_init(void)
{
#if defined TRIAC_DRIVER // 4路可控硅调光
    triac_driver_init();
#endif
}
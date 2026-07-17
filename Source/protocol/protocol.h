#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

void app_portocol_init(void);

// 上位机软件发送数据最大长度
#define SOFT_DATA_MAX_LEN 64

// 控制协议帧头
#define EXTEND_FH_1 0xFB
#define EXTEND_FH_2 0x01

// 上位机软件帧头
#define SOFT_R_FH_1 0xFF
#define SOFT_R_FH_2 0xAA

// 回复上位机软件帧头
#define SOFT_T_FH_1 0xFE
#define SOFT_T_FH_2 0xBB

#define SOFT_T_FT_1 0x0D
#define SOFT_T_FT_2 0x0A

#endif // _PROTOCOL_H_
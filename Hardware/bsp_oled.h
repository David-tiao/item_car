#ifndef __BSP_OLED_H
#define __BSP_OLED_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* 标准库 typedef 兼容（HAL 工程需要） */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

extern u8 OLED_GRAM[128][8];

/* ==================== OLED 端口宏定义（HAL 版） ==================== */
/* 引脚分配：
 *   PD11 -> DC  (数据/命令选择)
 *   PD12 -> RST (复位)
 *   PD13 -> SDA (SDIN, I2C 数据)
 *   PD14 -> SCL (SCLK, I2C 时钟)
 */
#define OLED_RST_Clr()   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET)
#define OLED_RST_Set()   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET)

#define OLED_RS_Clr()    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, GPIO_PIN_RESET)
#define OLED_RS_Set()    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, GPIO_PIN_SET)

#define OLED_SCLK_Clr()  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET)
#define OLED_SCLK_Set()  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET)

#define OLED_SDIN_Clr()  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET)
#define OLED_SDIN_Set()  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET)

#define OLED_CMD   0   /* 写命令 */
#define OLED_DATA  1   /* 写数据 */

#define OLED_Height  64
#define OLED_Width   128

/* ==================== 函数声明 ==================== */
void OLED_WR_Byte(u8 dat, u8 cmd);
void OLED_Display_On(void);
void OLED_Display_Off(void);
void OLED_Refresh_Gram(void);
void OLED_Refresh_Line(void);
void OLED_Init(void);
void OLED_Clear(void);
void OLED_DrawPoint(u8 x, u8 y, u8 t);
void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 size, u8 mode);
void OLED_ShowNumber(u8 x, u8 y, u32 num, u8 len, u8 size);
void OLED_ShowString(u8 x, u8 y, const u8 *p);
void OLED_ShowString16(u8 x, u8 y, const u8 *p);

#define CNSizeWidth   16
#define CNSizeHeight  16

void OLED_ShowCHinese(u8 x, u8 y, u8 *str);
void OLED_ShowFont16(u8 x, u8 y, u8 *s);
void OLED_ShowFont12(u8 x, u8 y, u8 *s);
void OLED_ShowCHinese12(u8 x, u8 y, u8 *str);
void OLED_DrawPoint_Shu(u8 x, u8 y, u8 t);
void OLED_Set_Pos(unsigned char x, unsigned char y);
void OLED_ShowFloat(u8 show_x, u8 show_y, const float needtoshow, u8 zs_num, u8 xs_num);

#endif

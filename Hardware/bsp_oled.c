#include "bsp_oled.h"
#include "oledfont.h"
#include <stdlib.h>

u8 OLED_GRAM[128][8];

/**************************************************************************
函数功能：刷新 OLED 屏幕（整屏）
**************************************************************************/
void OLED_Refresh_Gram(void)
{
    u8 i, n;
    for (i = 0; i < 8; i++)
    {
        OLED_WR_Byte(0xb0 + i, OLED_CMD);   /* 设置页地址（0~7） */
        OLED_WR_Byte(0x00, OLED_CMD);       /* 列低地址 */
        OLED_WR_Byte(0x10, OLED_CMD);       /* 列高地址 */
        for (n = 0; n < 128; n++)
            OLED_WR_Byte(OLED_GRAM[n][i], OLED_DATA);
    }
}

/**************************************************************************
函数功能：刷新前两页（部分刷新）
**************************************************************************/
void OLED_Refresh_Line(void)
{
    u8 i, n;
    for (i = 0; i < 2; i++)
    {
        OLED_WR_Byte(0xb0 + i, OLED_CMD);
        OLED_WR_Byte(0x00, OLED_CMD);
        OLED_WR_Byte(0x10, OLED_CMD);
        for (n = 0; n < 128; n++)
            OLED_WR_Byte(OLED_GRAM[n][i], OLED_DATA);
    }
}

/**************************************************************************
函数功能：向 OLED 写入一个字节
入口参数：dat: 要写入的数据/命令，cmd: 0-命令 1-数据
**************************************************************************/
void OLED_WR_Byte(u8 dat, u8 cmd)
{
    u8 i;
    if (cmd)
        OLED_RS_Set();
    else
        OLED_RS_Clr();

    for (i = 0; i < 8; i++)
    {
        OLED_SCLK_Clr();
        if (dat & 0x80)
            OLED_SDIN_Set();
        else
            OLED_SDIN_Clr();
        OLED_SCLK_Set();
        dat <<= 1;
    }
    OLED_RS_Set();
}

/**************************************************************************
函数功能：开启 OLED 显示
**************************************************************************/
void OLED_Display_On(void)
{
    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x14, OLED_CMD);
    OLED_WR_Byte(0xAF, OLED_CMD);
}

/**************************************************************************
函数功能：关闭 OLED 显示
**************************************************************************/
void OLED_Display_Off(void)
{
    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x10, OLED_CMD);
    OLED_WR_Byte(0xAE, OLED_CMD);
}

/**************************************************************************
函数功能：清屏
**************************************************************************/
void OLED_Clear(void)
{
    u8 i, n;
    for (i = 0; i < 8; i++)
        for (n = 0; n < 128; n++)
            OLED_GRAM[n][i] = 0x00;
    OLED_Refresh_Gram();
}

/**************************************************************************
函数功能：画点
入口参数：x,y: 起点坐标; t: 1-填充 0-清空
**************************************************************************/
void OLED_DrawPoint(u8 x, u8 y, u8 t)
{
    u8 pos, bx, temp = 0;
    if (x > 127 || y > 63) return;
    pos = 7 - y / 8;
    bx = y % 8;
    temp = 1 << (7 - bx);
    if (t)
        OLED_GRAM[x][pos] |= temp;
    else
        OLED_GRAM[x][pos] &= ~temp;
}

/**************************************************************************
函数功能：在指定位置显示一个字符（包括部分字符）
入口参数：x,y: 起点坐标; chr: 字符; size: 12/16; mode: 0-反白 1-正常
**************************************************************************/
void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 size, u8 mode)
{
    u8 temp, t, t1;
    u8 y0 = y;
    chr = chr - ' ';
    for (t = 0; t < size; t++)
    {
        if (size == 12) temp = oled_asc2_1206[chr][t];
        else            temp = oled_asc2_1608[chr][t];
        for (t1 = 0; t1 < 8; t1++)
        {
            if (temp & 0x80) OLED_DrawPoint(x, y, mode);
            else             OLED_DrawPoint(x, y, !mode);
            temp <<= 1;
            y++;
            if ((y - y0) == size)
            {
                y = y0;
                x++;
                break;
            }
        }
    }
}

/**************************************************************************
函数功能：求 m 的 n 次方
**************************************************************************/
u32 oled_pow(u8 m, u8 n)
{
    u32 result = 1;
    while (n--) result *= m;
    return result;
}

/**************************************************************************
函数功能：显示数字
入口参数：x,y: 起点; num: 数值; len: 位数; size: 字体大小
**************************************************************************/
void OLED_ShowNumber(u8 x, u8 y, u32 num, u8 len, u8 size)
{
    u8 t, temp;
    u8 enshow = 0;
    for (t = 0; t < len; t++)
    {
        temp = (num / oled_pow(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1))
        {
            if (temp == 0)
            {
                OLED_ShowChar(x + (size / 2) * t, y, ' ', size, 1);
                continue;
            }
            else enshow = 1;
        }
        OLED_ShowChar(x + (size / 2) * t, y, temp + '0', size, 1);
    }
}

/**************************************************************************
函数功能：显示字符串（12 号字体）
**************************************************************************/
void OLED_ShowString(u8 x, u8 y, const u8 *p)
{
#define MAX_CHAR_POSX 122
#define MAX_CHAR_POSY 58
    while (*p != '\0')
    {
        if (x > MAX_CHAR_POSX) { x = 0; y += 16; }
        if (y > MAX_CHAR_POSY) { y = x = 0; OLED_Clear(); }
        OLED_ShowChar(x, y, *p, 12, 1);
        x += 8;
        p++;
    }
}

/**************************************************************************
函数功能：显示字符串（16 号字体）
**************************************************************************/
void OLED_ShowString16(u8 x, u8 y, const u8 *p)
{
#define MAX_CHAR_POSX 122
#define MAX_CHAR_POSY 58
    while (*p != '\0')
    {
        if (x > MAX_CHAR_POSX) { x = 0; y += 16; }
        if (y > MAX_CHAR_POSY) { y = x = 0; OLED_Clear(); }
        OLED_ShowChar(x, y, *p, 16, 1);
        x += 8;
        p++;
    }
}

/**************************************************************************
函数功能：初始化 OLED
说明：GPIO（PD11~PD14）已由 CubeMX 在 MX_GPIO_Init() 中初始化
      本函数只负责 OLED 上电时序和寄存器配置
**************************************************************************/
void OLED_Init(void)
{
    OLED_RST_Clr();
    HAL_Delay(100);
    OLED_RST_Set();

    OLED_WR_Byte(0xAE, OLED_CMD);  /* 关闭显示 */
    OLED_WR_Byte(0xD5, OLED_CMD);  /* 设置时钟分频因子 */
    OLED_WR_Byte(80,   OLED_CMD);
    OLED_WR_Byte(0xA8, OLED_CMD);  /* 设置驱动路数 */
    OLED_WR_Byte(0x3F, OLED_CMD);  /* 默认 1/64 */
    OLED_WR_Byte(0xD3, OLED_CMD);  /* 设置显示偏移 */
    OLED_WR_Byte(0x00, OLED_CMD);
    OLED_WR_Byte(0x40, OLED_CMD);  /* 设置显示开始行 */
    OLED_WR_Byte(0x8D, OLED_CMD);  /* 电荷泵设置 */
    OLED_WR_Byte(0x14, OLED_CMD);
    OLED_WR_Byte(0x20, OLED_CMD);  /* 设置内存地址模式 */
    OLED_WR_Byte(0x02, OLED_CMD);  /* 页地址模式 */
    OLED_WR_Byte(0xA1, OLED_CMD);  /* 段重定义 */
    OLED_WR_Byte(0xC0, OLED_CMD);  /* COM 扫描方向 */
    OLED_WR_Byte(0xDA, OLED_CMD);  /* COM 硬件引脚配置 */
    OLED_WR_Byte(0x12, OLED_CMD);
    OLED_WR_Byte(0x81, OLED_CMD);  /* 对比度设置 */
    OLED_WR_Byte(0xEF, OLED_CMD);
    OLED_WR_Byte(0xD9, OLED_CMD);  /* 预充电周期 */
    OLED_WR_Byte(0xF1, OLED_CMD);
    OLED_WR_Byte(0xDB, OLED_CMD);  /* VCOMH 电压倍率 */
    OLED_WR_Byte(0x30, OLED_CMD);
    OLED_WR_Byte(0xA4, OLED_CMD);  /* 全局显示 */
    OLED_WR_Byte(0xA6, OLED_CMD);  /* 正常显示 */
    OLED_WR_Byte(0xAF, OLED_CMD);  /* 开启显示 */
    OLED_Clear();
}

/**************************************************************************
函数功能：显示 16x16 汉字
入口参数：x,y: 起点坐标; s: 指向汉字 GBK 编码的指针
**************************************************************************/
void OLED_ShowFont16(u8 x, u8 y, u8 *s)
{
    u8 i, j, k, tmp;
    u16 num;
    num = sizeof(cfont16) / sizeof(Chinese_GB16);
    for (i = 0; i < num; i++)
    {
        if ((cfont16[i].Index[0] == *s) && (cfont16[i].Index[1] == *(s + 1)))
        {
            for (j = 0; j < 32; j++)
            {
                tmp = cfont16[i].Msk[j];
                for (k = 0; k < 8; k++)
                {
                    if (tmp & (0x80 >> k))
                        OLED_DrawPoint(x + (j % 2) * 8 + k, y + j / 2, 1);
                    else
                        OLED_DrawPoint(x + (j % 2) * 8 + k, y + j / 2, 0);
                }
            }
            break;
        }
    }
}

/**************************************************************************
函数功能：显示一行 16x16 汉字
**************************************************************************/
void OLED_ShowCHinese(u8 x, u8 y, u8 *str)
{
    while (*str != '\0')
    {
        OLED_ShowFont16(x, y, str);
        x += 16;
        if (x > OLED_Width - 16)
        {
            x = 0;
            y += 16;
        }
        str += 2;
    }
}

/**************************************************************************
函数功能：显示 12x12 汉字
**************************************************************************/
void OLED_ShowFont12(u8 x, u8 y, u8 *s)
{
    u8 i, j, k, tmp;
    u16 num;
    num = sizeof(cfont12) / sizeof(Chinese_GB12);
    for (i = 0; i < num; i++)
    {
        if ((cfont12[i].Index_12[0] == *s) && (cfont12[i].Index_12[1] == *(s + 1)))
        {
            for (j = 0; j < 24; j++)
            {
                tmp = cfont12[i].Msk_12[j];
                for (k = 0; k < 8; k++)
                {
                    if (tmp & (0x80 >> k))
                        OLED_DrawPoint(x + (j % 2) * 8 + k, y + j / 2, 1);
                    else
                        OLED_DrawPoint(x + (j % 2) * 8 + k, y + j / 2, 0);
                }
            }
            break;
        }
    }
}

/**************************************************************************
函数功能：显示一行 12x12 汉字
**************************************************************************/
void OLED_ShowCHinese12(u8 x, u8 y, u8 *str)
{
    while (*str != '\0')
    {
        OLED_ShowFont12(x, y, str);
        x += 12;
        if (x > OLED_Width - 12)
        {
            x = 0;
            y += 12;
        }
        str += 2;
    }
}

/**************************************************************************
函数功能：竖着画点（用于 CCD 条形显示）
**************************************************************************/
void OLED_DrawPoint_Shu(u8 x, u8 y, u8 t)
{
    u8 i = 0;
    OLED_DrawPoint(x, y, t);
    OLED_DrawPoint(x, y, t);
    for (i = 0; i < 8; i++)
    {
        OLED_DrawPoint(x, y + i, t);
    }
}

/**************************************************************************
函数功能：设置显示坐标（用于 BMP 等按页写入）
**************************************************************************/
void OLED_Set_Pos(unsigned char x, unsigned char y)
{
    OLED_WR_Byte(0xb0 + y, OLED_CMD);
    OLED_WR_Byte(((x & 0xf0) >> 4) | 0x10, OLED_CMD);
    OLED_WR_Byte((x & 0x0f), OLED_CMD);
}

/**************************************************************************
函数功能：浮点数分解
**************************************************************************/
int* FenJie_float(const float fudian)
{
    static int tmp[3];
    float temp;
    temp = fudian;
    if (temp < 0) temp = -temp, tmp[2] = -1;
    else tmp[2] = 1;

    tmp[0] = (int)temp;              /* 整数部分 */
    tmp[1] = (temp - tmp[0]) * 1000; /* 小数部分（3 位） */
    return tmp;
}

/**************************************************************************
函数功能：OLED 显示浮点数
入口参数：show_x, show_y: 起点坐标; needtoshow: 待显示浮点数
          zs_num: 整数位数; xs_num: 小数位数
**************************************************************************/
void OLED_ShowFloat(u8 show_x, u8 show_y, const float needtoshow, u8 zs_num, u8 xs_num)
{
    static int* p;
    p = FenJie_float(needtoshow);

    /* 符号位 */
    if (p[2] > 0) OLED_ShowChar(show_x, show_y, '+', 12, 1);
    else          OLED_ShowChar(show_x, show_y, '-', 12, 1);

    /* 整数部分 */
    OLED_ShowNumber(show_x + 8, show_y, p[0], zs_num, 12);

    /* 小数点 */
    OLED_ShowChar(show_x + 6 + 8 * zs_num, show_y, '.', 12, 1);

    /* 小数部分 */
    if (p[1] < 100)
    {
        if (xs_num == 3)
        {
            OLED_ShowNumber(show_x + 12 + 8 * zs_num, show_y, 0, 1, 12);
            if (p[1] >= 10)
                OLED_ShowNumber(show_x + 18 + 8 * zs_num, show_y, p[1], 2, 12);
            else
            {
                OLED_ShowNumber(show_x + 18 + 8 * zs_num, show_y, 0, 1, 12);
                OLED_ShowNumber(show_x + 24 + 8 * zs_num, show_y, p[1], 1, 12);
            }
        }
        else
        {
            if (p[1] >= 0 && p[1] < 100)
            {
                OLED_ShowNumber(show_x + 12 + 8 * zs_num, show_y, 0, 1, 12);
                OLED_ShowNumber(show_x + 18 + 8 * zs_num, show_y, p[1] / 10, 1, 12);
            }
        }
    }
    else
    {
        if (xs_num == 3)
            OLED_ShowNumber(show_x + 12 + 8 * zs_num, show_y, p[1], 3, 12);
        else
            OLED_ShowNumber(show_x + 12 + 8 * zs_num, show_y, p[1] / 10, 2, 12);
    }
}

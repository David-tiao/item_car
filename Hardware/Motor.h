#ifndef __MOTOR_H
#define __MOTOR_H

#include "stm32f4xx_hal.h"


/* PID 输出统一量程（与具体定时器 ARR 无关） */
#define PID_OUTPUT_MAX   1000.0f

/**
 * @brief  单电机控制结构体
 * @note   适配AT8236双PWM输入驱动芯片（IN1/IN2）
 */
 

typedef struct {
    /* 硬件资源 */
    TIM_HandleTypeDef *pwm_tim1;   // PWM定时器1 → AT8236.IN1
    uint32_t         pwm_ch1;      // PWM通道1 
    uint32_t         pwm_arr1;     // PWM定时器1的 ARR（用于计算归一化PID）
    TIM_HandleTypeDef *pwm_tim2;   // PWM定时器2 → AT8236.IN2
    uint32_t         pwm_ch2;      // PWM通道2
    uint32_t         pwm_arr2;     // PWM定时器2的 ARR（用于计算归一化PID）
    TIM_HandleTypeDef *enc_tim;    // 编码器定时器
    int8_t motor_dir;   			 // 电机输出方向：+1 或 -1(正确为+1，反向为-1)
    int8_t enc_dir;     			 // 编码器方向：+1 或 -1(正确为+1，反向为-1)
    /* 运行状态 */
    int32_t  enc_last_cnt;         // 上一次编码器计数值
    float    actual_speed;         // 实际速度（脉冲/秒）
    float    target_speed;         // 目标速度（脉冲/秒）
  
    /* PID控制器 */
    float kp;
    float ki;
    float kd;
    float err;
    float err_last;
    float integral;
    float integral_max;
    float output;
    float output_max;
} Motor_t;

/* 函数声明 */
//电机初始化函数
void Motor_Init(Motor_t *motor,
                TIM_HandleTypeDef *pwm_tim1, uint32_t pwm_ch1,uint32_t pwm_arr1,
                TIM_HandleTypeDef *pwm_tim2, uint32_t pwm_ch2,uint32_t pwm_arr2,
                TIM_HandleTypeDef *enc_tim,
                float kp, float ki, float kd,
			   int8_t motor_dir, int8_t enc_dir
                );

void Motor_SetTargetSpeed(Motor_t *motor, float target_speed);
void Motor_Update(Motor_t *motor, float dt);
void Motor_Stop(Motor_t *motor);
float Motor_GetSpeed(Motor_t *motor);

#endif


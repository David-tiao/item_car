#include "motor.h"

/**
 * @brief  读取编码器增量
 */
static int32_t Encoder_ReadDelta(Motor_t *motor)
{
    int32_t now_cnt = (int32_t)__HAL_TIM_GET_COUNTER(motor->enc_tim);
    int32_t delta = now_cnt - motor->enc_last_cnt;
    // 假设编码器为 16 位计数器，处理溢出
    if (delta > 32767) delta -= 65536;
    else if (delta < -32768) delta += 65536;
    motor->enc_last_cnt = now_cnt;
    return delta;
}

/**
 * @brief  电机初始化：绑定硬件+启动外设+PID初始化
 */
void Motor_Init(Motor_t *motor,
                
                TIM_HandleTypeDef *pwm_tim1,  uint32_t pwm_ch1,  uint32_t pwm_arr1,
                
                TIM_HandleTypeDef *pwm_tim2,  uint32_t pwm_ch2,  uint32_t pwm_arr2,
                
                TIM_HandleTypeDef *enc_tim,
                
                float kp, float ki, float kd,
                
				int8_t motor_dir, int8_t enc_dir
                )
{
    /* 绑定硬件资源 */
    motor->pwm_tim1 = pwm_tim1;
    motor->pwm_ch1  = pwm_ch1;
    motor->pwm_arr1 = pwm_arr1;
    motor->pwm_tim2 = pwm_tim2;
    motor->pwm_ch2  = pwm_ch2;
    motor->pwm_arr2 = pwm_arr2;
    motor->enc_tim  = enc_tim;
    motor->motor_dir = motor_dir;
    motor->enc_dir   = enc_dir;
    /* 状态初始化 */
    motor->enc_last_cnt = 0;
    motor->actual_speed = 0.0f;
    motor->target_speed = 0.0f;
    /* PID参数初始化 */
    motor->kp = kp;
    motor->ki = ki;
    motor->kd = kd;
    motor->err = 0.0f;
    motor->err_last = 0.0f;
    motor->integral = 0.0f;
    motor->output = 0.0f;
    motor->output_max = PID_OUTPUT_MAX;           // 统一 1000
    motor->integral_max = PID_OUTPUT_MAX * 0.5f;
    /************************* 【修正：启动外设，避免重复启动同一定时器】 *************************/
    /* 启动第一路PWM通道 */
    if (HAL_TIM_PWM_GetState(pwm_tim1) == HAL_TIM_STATE_RESET)
    {
        HAL_TIM_PWM_Start(pwm_tim1, pwm_ch1);
    }
    else
    {
        HAL_TIM_PWM_Start(pwm_tim1, pwm_ch1);
    }

    /* 启动第二路PWM通道：若和第一路是同一个定时器，只启动通道，不重复初始化定时器 */
    if (pwm_tim2 != pwm_tim1)
    {
        HAL_TIM_PWM_Start(pwm_tim2, pwm_ch2);
    }
    else
    {
        HAL_TIM_PWM_Start(pwm_tim2, pwm_ch2);
    }

    /* 启动编码器：标准写法为通道1 | 通道2，修正TIM_CHANNEL_ALL的不兼容问题 */
    HAL_TIM_Encoder_Start(enc_tim, TIM_CHANNEL_1 | TIM_CHANNEL_2);
    /* 关键：读取启动后的编码器当前值，避免第一次速度计算爆炸 */
    motor->enc_last_cnt = (int32_t)__HAL_TIM_GET_COUNTER(motor->enc_tim);
    /* 初始状态：停止，两路PWM均置0 */
    __HAL_TIM_SET_COMPARE(motor->pwm_tim1, motor->pwm_ch1, 0);
    __HAL_TIM_SET_COMPARE(motor->pwm_tim2, motor->pwm_ch2, 0);
}

/**
 * @brief  设置目标速度（正值正转，负值反转）
 */
void Motor_SetTargetSpeed(Motor_t *motor, float target_speed)
{
    motor->target_speed = target_speed;
}

/**
 * @brief  电机速度环更新（必须周期调用，建议1ms）
 * @param  dt: 控制周期，单位秒
 * @note   AT8236逻辑：正转=IN1发PWM、IN2置0；反转=IN1置0、IN2发PWM
 */
void Motor_Update(Motor_t *motor, float dt)
{
    if (dt <= 0.0f) return;

    /* 1. 计算实际速度 */
    int32_t delta = Encoder_ReadDelta(motor);
    motor->actual_speed = (float)delta / dt * motor->enc_dir;  //引入enc_dir参数修正电机接线错误

    /* 2. PID计算 */
    motor->err = motor->target_speed - motor->actual_speed;

    float p_out = motor->kp * motor->err;

    motor->integral += motor->err * dt;
    if (motor->integral >  motor->integral_max) motor->integral =  motor->integral_max;
    if (motor->integral < -motor->integral_max) motor->integral = -motor->integral_max;
    float i_out = motor->ki * motor->integral;

    float d_out = motor->kd * (motor->err - motor->err_last) / dt;
    motor->err_last = motor->err;

    motor->output = p_out + i_out + d_out;
    
    /* 输出限幅 */
    if (motor->output >  motor->output_max) motor->output =  motor->output_max;
    if (motor->output < -motor->output_max) motor->output = -motor->output_max;
    
    /* 3. 归一化 → 按各自 ARR 映射到 CCR */
    float duty = motor->output / motor->output_max * motor->motor_dir;   // -1.0 ~ +1.0（输出方向修正 ）

    if (duty > 0.001f)          /* 正转：IN1 发 PWM，IN2 置 0 */
    {
        uint32_t ccr = (uint32_t)(duty * (float)motor->pwm_arr1 + 0.5f);
        if (ccr > motor->pwm_arr1) ccr = motor->pwm_arr1;
        __HAL_TIM_SET_COMPARE(motor->pwm_tim1, motor->pwm_ch1, ccr);
        __HAL_TIM_SET_COMPARE(motor->pwm_tim2, motor->pwm_ch2, 0);
    }
    else if (duty < -0.001f)    /* 反转：IN1 置 0，IN2 发 PWM */
    {
        uint32_t ccr = (uint32_t)(-duty * (float)motor->pwm_arr2 + 0.5f);
        if (ccr > motor->pwm_arr2) ccr = motor->pwm_arr2;
        __HAL_TIM_SET_COMPARE(motor->pwm_tim1, motor->pwm_ch1, 0);
        __HAL_TIM_SET_COMPARE(motor->pwm_tim2, motor->pwm_ch2, ccr);
    }
    else                        /* 死区：两路都置 0 */
    {
        __HAL_TIM_SET_COMPARE(motor->pwm_tim1, motor->pwm_ch1, 0);
        __HAL_TIM_SET_COMPARE(motor->pwm_tim2, motor->pwm_ch2, 0);
    }
    // /* 3. 输出到AT8236 */（原始版本）
    // uint32_t pwm_val;

    // if (motor->output > 0.1f)
    // {
    //     pwm_val = (uint32_t)(motor->output + 0.5f);
    //     __HAL_TIM_SET_COMPARE(motor->pwm_tim1, motor->pwm_ch1, pwm_val);
    //     __HAL_TIM_SET_COMPARE(motor->pwm_tim2, motor->pwm_ch2, 0);
    // }
    // else if (motor->output < -0.1f)
    // {
    //     pwm_val = (uint32_t)((-motor->output) + 0.5f);
    //     __HAL_TIM_SET_COMPARE(motor->pwm_tim1, motor->pwm_ch1, 0);
    //     __HAL_TIM_SET_COMPARE(motor->pwm_tim2, motor->pwm_ch2, pwm_val);
    // }
    // else
    // {
    //     __HAL_TIM_SET_COMPARE(motor->pwm_tim1, motor->pwm_ch1, 0);
    //     __HAL_TIM_SET_COMPARE(motor->pwm_tim2, motor->pwm_ch2, 0);
    // }
}




/**
 * @brief  电机停止（双路低电平，滑行模式）
 */
void Motor_Stop(Motor_t *motor)
{
    motor->integral = 0.0f;
    motor->err = 0.0f;
    motor->err_last = 0.0f;//清零积分和误差
    motor->target_speed = 0.0f;
    motor->output = 0.0f;
    __HAL_TIM_SET_COMPARE(motor->pwm_tim1, motor->pwm_ch1, 0);
    __HAL_TIM_SET_COMPARE(motor->pwm_tim2, motor->pwm_ch2, 0);
}

/**
 * @brief  获取当前实际速度
 */
float Motor_GetSpeed(Motor_t *motor)
{
    return motor->actual_speed;
}


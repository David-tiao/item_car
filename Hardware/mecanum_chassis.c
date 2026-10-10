#include "mecanum_chassis.h"
#include "bsp_oled.h"
#include <stdlib.h>
/************************ 内部工具函数 ************************/
/**
 * @brief  角度归一化到 [-π, π]
 */
static float Angle_Normalize(float angle)
{
    while (angle >  PI) angle -= 2.0f * PI;
    while (angle < -PI) angle += 2.0f * PI;
    return angle;
}

/**
 * @brief  位置PID计算
 */
static float PosPID_Calc(PosPID_t *pid, float target, float actual, float dt)
{
    pid->err = target - actual;

    pid->integral += pid->err * dt;
    if (pid->integral >  pid->integral_max) pid->integral =  pid->integral_max;
    if (pid->integral < -pid->integral_max) pid->integral = -pid->integral_max;

    float derivative = (pid->err - pid->err_last) / dt;
    pid->err_last = pid->err;

    pid->output = pid->kp * pid->err + pid->ki * pid->integral + pid->kd * derivative;
    if (pid->output >  pid->output_max) pid->output =  pid->output_max;
    if (pid->output < -pid->output_max) pid->output = -pid->output_max;

    return pid->output;
}

/************************ 运动学核心 ************************/
/**
 * @brief  逆运动学：底盘速度 → 四轮脉冲速度
 */
static void Mecanum_InverseKinematics(MecanumChassis_t *chassis,
                                      float vx, float vy, float omega,
                                      float *wheel_pulse)
{
    float L = chassis->L;
    float m2p = chassis->meter_to_pulse;

    float v_lf = (vx - vy - L * omega) / SQRT2;
    float v_rf = (vx + vy + L * omega) / SQRT2;
    float v_lb = (vx + vy - L * omega) / SQRT2;
    float v_rb = (vx - vy + L * omega) / SQRT2;

    wheel_pulse[0] = v_lf * m2p;
    wheel_pulse[1] = v_rf * m2p;
    wheel_pulse[2] = v_lb * m2p;
    wheel_pulse[3] = v_rb * m2p;
}

/**
 * @brief  正运动学：四轮脉冲速度 → 底盘速度
 */
static void Mecanum_ForwardKinematics(MecanumChassis_t *chassis,
                                      float *wheel_pulse,
                                      float *vx, float *vy, float *omega)
{
    float p2m = chassis->pulse_to_meter;
    float L = chassis->L;

    float v_lf = wheel_pulse[0] * p2m;
    float v_rf = wheel_pulse[1] * p2m;
    float v_lb = wheel_pulse[2] * p2m;
    float v_rb = wheel_pulse[3] * p2m;

    *vx    = (v_lf + v_rf + v_lb + v_rb) * SQRT2 / 4.0f;
    *vy    = (-v_lf + v_rf + v_lb - v_rb) * SQRT2 / 4.0f;
    *omega = (-v_lf + v_rf - v_lb + v_rb) * SQRT2 / (4.0f * L);
}

/************************ 状态更新 ************************/
/**
 * @brief  更新底盘实际状态：速度+位置（航迹推算）
 */
static void Mecanum_UpdateState(MecanumChassis_t *chassis, float dt)
{
    float wheel_pulse[4];

    wheel_pulse[0] = Motor_GetSpeed(chassis->motor_lf);
    wheel_pulse[1] = Motor_GetSpeed(chassis->motor_rf);
    wheel_pulse[2] = Motor_GetSpeed(chassis->motor_lb);
    wheel_pulse[3] = Motor_GetSpeed(chassis->motor_rb);

    Mecanum_ForwardKinematics(chassis, wheel_pulse,
                              &chassis->vx_actual, &chassis->vy_actual, &chassis->omega_actual);

    chassis->x_pos += chassis->vx_actual * dt;
    chassis->y_pos += chassis->vy_actual * dt;
    chassis->theta_pos += chassis->omega_actual * dt;
    chassis->theta_pos = Angle_Normalize(chassis->theta_pos);
}

/************************ 位置环闭环 ************************/
/**
 * @brief  位置环PID计算
 */
static void Mecanum_PositionLoop(MecanumChassis_t *chassis, float dt)
{
    if (chassis->mode != 1) return;

    float err_x = chassis->x_target - chassis->x_pos;
    float err_y = chassis->y_target - chassis->y_pos;
    float err_theta = Angle_Normalize(chassis->theta_target - chassis->theta_pos);

    chassis->vx_target    = PosPID_Calc(&chassis->pid_x, chassis->x_target, chassis->x_pos, dt);
    chassis->vy_target    = PosPID_Calc(&chassis->pid_y, chassis->y_target, chassis->y_pos, dt);
    chassis->omega_target = PosPID_Calc(&chassis->pid_theta, chassis->theta_target, chassis->theta_pos, dt);

    /* 速度限幅 */
    if (chassis->vx_target >  chassis->max_speed) chassis->vx_target =  chassis->max_speed;
    if (chassis->vx_target < -chassis->max_speed) chassis->vx_target = -chassis->max_speed;
    if (chassis->vy_target >  chassis->max_speed) chassis->vy_target =  chassis->max_speed;
    if (chassis->vy_target < -chassis->max_speed) chassis->vy_target = -chassis->max_speed;
    if (chassis->omega_target >  chassis->max_omega) chassis->omega_target =  chassis->max_omega;
    if (chassis->omega_target < -chassis->max_omega) chassis->omega_target = -chassis->max_omega;

    /* 到位判断 */
    if (fabsf(err_x) < chassis->position_tol &&
        fabsf(err_y) < chassis->position_tol &&
        fabsf(err_theta) < chassis->theta_tol)
    {
        chassis->target_reached = 1;
        chassis->vx_target = 0.0f;
        chassis->vy_target = 0.0f;
        chassis->omega_target = 0.0f;
    }
    else
    {
        chassis->target_reached = 0;
    }
}

/************************ 对外接口 ************************/
/**
 * @brief  底盘初始化
 */
void MecanumChassis_Init(MecanumChassis_t *chassis,
                         Motor_t *lf, Motor_t *rf, Motor_t *lb, Motor_t *rb,
                       float wheel_radius, 
					   float wheel_base_half, 
					   float track_width_half,
                       float gear_ratio,   
					   float enc_ppr_x4)
//wheel_radius		轮子半径						m			卡尺测量，如 50mm 轮子 = 0.05f
//wheel_base_half	半轴距（前后轮中心距的一半）	m			测量前后轮中心距离 ÷ 2
//track_width_half	半轮距（左右轮中心距的一半）	m			测量左右轮中心距离 ÷ 2
//gear_ratio			减速比							—			电机规格书，如 1:30 减速箱 = 30.0f
//enc_ppr_x4			编码器 4 倍频每转脉冲数			—			编码器线数 × 4，如 500 线 = 2000.0f
							 
{
    /* 绑定电机 */
    chassis->motor_lf = lf;
    chassis->motor_rf = rf;
    chassis->motor_lb = lb;
    chassis->motor_rb = rb;

    /* 机械参数 */
    chassis->wheel_radius = wheel_radius;
    chassis->wheel_base_half = wheel_base_half;
    chassis->track_width_half = track_width_half;
    chassis->L = wheel_base_half + track_width_half;
    chassis->gear_ratio = gear_ratio;
    chassis->enc_ppr_x4 = enc_ppr_x4;

    /* 计算转换系数 */
    float wheel_perimeter = 2.0f * PI * wheel_radius;
    float pulse_per_wheel = enc_ppr_x4 * gear_ratio;
    chassis->pulse_to_meter = wheel_perimeter / pulse_per_wheel;
    chassis->meter_to_pulse = pulse_per_wheel / wheel_perimeter;

    /* 状态清零 */
    chassis->x_pos = 0.0f;
    chassis->y_pos = 0.0f;
    chassis->theta_pos = 0.0f;
    chassis->vx_actual = 0.0f;
    chassis->vy_actual = 0.0f;
    chassis->omega_actual = 0.0f;
    chassis->vx_target = 0.0f;
    chassis->vy_target = 0.0f;
    chassis->omega_target = 0.0f;

    /* 默认参数 */
    chassis->max_speed = 1.0f;
    chassis->max_omega = PI;
    chassis->position_tol = 0.01f;
    chassis->theta_tol = 0.05f;

    chassis->enable = 1;
    chassis->mode = 0;
    chassis->target_reached = 0;
}

/**
 * @brief  底盘主控制循环（建议10ms调用一次）
 */
void MecanumChassis_Loop(MecanumChassis_t *chassis, float dt)
{
    if (!chassis->enable)
    {
        Motor_Stop(chassis->motor_lf);
        Motor_Stop(chassis->motor_rf);
        Motor_Stop(chassis->motor_lb);
        Motor_Stop(chassis->motor_rb);
        return;
    }

    /* 1. 更新实际状态 */
    Mecanum_UpdateState(chassis, dt);

    /* 2. 位置模式下运行位置环 */
    if (chassis->mode == 1)
    {
        Mecanum_PositionLoop(chassis, dt);
    }

    /* 3. 逆运动学解算 */
    float wheel_target[4];
    Mecanum_InverseKinematics(chassis,
                              chassis->vx_target, chassis->vy_target, chassis->omega_target,
                              wheel_target);

    /* 4. 下发目标速度到电机 */
    Motor_SetTargetSpeed(chassis->motor_lf, wheel_target[0]);
    Motor_SetTargetSpeed(chassis->motor_rf, wheel_target[1]);
    Motor_SetTargetSpeed(chassis->motor_lb, wheel_target[2]);
    Motor_SetTargetSpeed(chassis->motor_rb, wheel_target[3]);
}

/************************ 模式切换与运动接口 ************************/
void Mecanum_SetSpeedMode(MecanumChassis_t *chassis, float vx, float vy, float omega)
{
    chassis->mode = 0;
    chassis->target_reached = 0;

    if (vx >  chassis->max_speed) vx =  chassis->max_speed;
    if (vx < -chassis->max_speed) vx = -chassis->max_speed;
    if (vy >  chassis->max_speed) vy =  chassis->max_speed;
    if (vy < -chassis->max_speed) vy = -chassis->max_speed;
    if (omega >  chassis->max_omega) omega =  chassis->max_omega;
    if (omega < -chassis->max_omega) omega = -chassis->max_omega;

    chassis->vx_target = vx;
    chassis->vy_target = vy;
    chassis->omega_target = omega;
}

void Mecanum_SetPositionMode(MecanumChassis_t *chassis, float x, float y, float theta)
{
    chassis->mode = 1;
    chassis->target_reached = 0;
    chassis->x_target = x;
    chassis->y_target = y;
    chassis->theta_target = Angle_Normalize(theta);

    /* 切换模式清空积分，防止冲击 */
    chassis->pid_x.err_last = 0.0f;
    chassis->pid_x.integral = 0.0f;
    chassis->pid_y.integral = 0.0f;
    chassis->pid_theta.integral = 0.0f;
}

void Mecanum_MoveForward(MecanumChassis_t *chassis, float speed)
{
    Mecanum_SetSpeedMode(chassis, speed, 0.0f, 0.0f);
}

void Mecanum_MoveBackward(MecanumChassis_t *chassis, float speed)
{
    Mecanum_SetSpeedMode(chassis, -speed, 0.0f, 0.0f);
}

void Mecanum_MoveLeft(MecanumChassis_t *chassis, float speed)
{
    Mecanum_SetSpeedMode(chassis, 0.0f, speed, 0.0f);
}

void Mecanum_MoveRight(MecanumChassis_t *chassis, float speed)
{
    Mecanum_SetSpeedMode(chassis, 0.0f, -speed, 0.0f);
}

void Mecanum_RotateCCW(MecanumChassis_t *chassis, float omega)
{
    Mecanum_SetSpeedMode(chassis, 0.0f, 0.0f, omega);
}

void Mecanum_RotateCW(MecanumChassis_t *chassis, float omega)
{
    Mecanum_SetSpeedMode(chassis, 0.0f, 0.0f, -omega);
}

void Mecanum_Stop(MecanumChassis_t *chassis)
{
    Mecanum_SetSpeedMode(chassis, 0.0f, 0.0f, 0.0f);
}


/* ==================== 新增接口函数实现 ==================== */

/**
 * @brief  统一运动接口：速度模式
 * @param  vx     前后速度，m/s，正=前
 * @param  vy     左右速度，m/s，正=左
 * @param  omega  自转角速度，rad/s，正=逆时针
 */
void Mecanum_Move(MecanumChassis_t *chassis, float vx, float vy, float omega)
{
    Mecanum_SetSpeedMode(chassis, vx, vy, omega);
}

/**
 * @brief  获取当前位置
 */
void Mecanum_GetPosition(MecanumChassis_t *chassis, float *x, float *y, float *theta)
{
    if (x)     *x     = chassis->x_pos;
    if (y)     *y     = chassis->y_pos;
    if (theta) *theta = chassis->theta_pos;
}

/**
 * @brief  获取当前速度
 */
void Mecanum_GetVelocity(MecanumChassis_t *chassis, float *vx, float *vy, float *omega)
{
    if (vx)    *vx    = chassis->vx_actual;
    if (vy)    *vy    = chassis->vy_actual;
    if (omega) *omega = chassis->omega_actual;
}

/**
 * @brief  是否到达目标位置（位置模式下有效）
 */
uint8_t Mecanum_IsTargetReached(MecanumChassis_t *chassis)
{
    return chassis->target_reached;
}

/**
 * @brief  使能 / 失能底盘
 * @param  enable: 1=使能，0=失能（失能后电机停止）
 */
void Mecanum_Enable(MecanumChassis_t *chassis, uint8_t enable)
{
    chassis->enable = enable;
    if (!enable)
    {
        Mecanum_Stop(chassis);
    }
}

/**
 * @brief  将当前位置清零（作为新的原点）
 */
void Mecanum_ResetPosition(MecanumChassis_t *chassis)
{
    chassis->x_pos = 0.0f;
    chassis->y_pos = 0.0f;
    chassis->theta_pos = 0.0f;
}

/**
 * @brief  设置最大线速度和角速度
 */
void Mecanum_SetSpeedLimit(MecanumChassis_t *chassis, float max_speed, float max_omega)
{
    chassis->max_speed = max_speed;
    chassis->max_omega = max_omega;
}

/**
 * @brief  设置到位容差
 */
void Mecanum_SetTolerance(MecanumChassis_t *chassis, float position_tol, float theta_tol)
{
    chassis->position_tol = position_tol;
    chassis->theta_tol = theta_tol;
}

/**
 * @brief  一次性设置位置环三轴 PID 参数
 */
void Mecanum_SetPositionPID(MecanumChassis_t *chassis,
                            float kp, float ki, float kd,
                            float integral_max, float output_max)
{
    chassis->pid_x.kp = kp;
    chassis->pid_x.ki = ki;
    chassis->pid_x.kd = kd;
    chassis->pid_x.integral_max = integral_max;
    chassis->pid_x.output_max = output_max;

    chassis->pid_y.kp = kp;
    chassis->pid_y.ki = ki;
    chassis->pid_y.kd = kd;
    chassis->pid_y.integral_max = integral_max;
    chassis->pid_y.output_max = output_max;

    chassis->pid_theta.kp = kp;
    chassis->pid_theta.ki = ki;
    chassis->pid_theta.kd = kd;
    chassis->pid_theta.integral_max = integral_max;
    chassis->pid_theta.output_max = output_max;
}
/* 内部辅助：编码器计数需要用大范围显示（可能负数、可能超 5 位） */
static void Disp_EncoderVal(u8 x, u8 y, int32_t v)
{
    if (v < 0) { OLED_ShowChar(x, y, '-', 12, 1); v = -v; }
    else       { OLED_ShowChar(x, y, '+', 12, 1); }

    /* 显示 6 位数字，足够覆盖 16 位/32 位计数 */
    OLED_ShowNumber(x + 6, y, (uint32_t)v, 6, 12);
}

/**
 * @brief  显示单个电机的编码器原始计数 + 增量
 * @param  motor: 电机指针
 * @param  y:     显示行 y 坐标
 * @param  name:  电机名，如 "A"
 * @note   一行显示：A C:+000123 D:+000005
 *         C = 原始计数 (Counter)
 *         D = 增量 (Delta，本次采样周期内的变化)
 */
void Chassis_Display_Encoder(Motor_t *motor, u8 y, const char *name)
{
    /* 读取当前原始计数 */
    int32_t now = (int32_t)__HAL_TIM_GET_COUNTER(motor->enc_tim);
    
    /* 计算相对上次采样值的增量 */
    int32_t delta = now - motor->enc_last_cnt;

    OLED_ShowString(0, y, (const u8*)name);       /* A */
    OLED_ShowString(6, y, (const u8*)"C");
    Disp_EncoderVal(12, y, now);                  /* 原始计数 */

    OLED_ShowString(66, y, (const u8*)"D");
    Disp_EncoderVal(72, y, delta);                /* 增量 */
}

/**
 * @brief  显示四个电机的编码器（全屏）
 * @note   4 行电机数据 + 1 行表头
 *         表头：C=Counter D=Delta
 */
void Chassis_Display_AllEncoder(MecanumChassis_t *chassis)
{
    /* 清 GRAM */
    for (u8 i = 0; i < 8; i++)
        for (u8 n = 0; n < 128; n++)
            OLED_GRAM[n][i] = 0x00;

    /* 表头 */
    OLED_ShowString(0, 0, (const u8*)"C:Counter D:Delta");

    /* 四个电机 */
    Chassis_Display_Encoder(chassis->motor_lf, 12, "A");
    Chassis_Display_Encoder(chassis->motor_rf, 24, "B");
    Chassis_Display_Encoder(chassis->motor_lb, 36, "C");
    Chassis_Display_Encoder(chassis->motor_rb, 48, "D");

    /* 刷新到屏幕 */
    OLED_Refresh_Gram();
}

/* 内部辅助：带符号整数显示 */
static void Disp_SpeedVal(u8 x, u8 y, float v)
{
    if (v < 0.0f) { OLED_ShowChar(x, y, '-', 12, 1); v = -v; }
    else          { OLED_ShowChar(x, y, '+', 12, 1); }

    int32_t iv = (int32_t)(v + 0.5f);
    if (iv > 99999) iv = 99999;
    OLED_ShowNumber(x + 6, y, (uint32_t)iv, 5, 12);
}

///**
// * @brief  显示四个电机的实际速度和目标速度（单页）
// * @note   每行一个电机：名字 + A:实际速度 + T:目标速度
// *         布局：A A:+01234 T:+01234
// *               A = 实际速度（脉冲/秒）
// *               T = 目标速度（脉冲/秒）
// */
//void Chassis_Display_MotorSpeed(MecanumChassis_t *chassis)
//{
//    /* 清 GRAM */
//    for (u8 i = 0; i < 8; i++)
//        for (u8 n = 0; n < 128; n++)
//            OLED_GRAM[n][i] = 0x00;

//    /* 表头 */
//    OLED_ShowString(0, 0, (const u8*)"A:Act T:Tgt");

//    Motor_t *m[4] = {
//        chassis->motor_lf, chassis->motor_rf,
//        chassis->motor_lb, chassis->motor_rb
//    };
//    const char *name[4] = { "A", "B", "C", "D" };

//    for (u8 i = 0; i < 4; i++)
//    {
//        u8 y = 12 + i * 12;
//        OLED_ShowString(0, y, (const u8*)name[i]);   /* 电机名 */
//        OLED_ShowString(6, y, (const u8*)"A");       /* A: 实际 */
//        Disp_SpeedVal(12, y, m[i]->actual_speed);

//        OLED_ShowString(60, y, (const u8*)"T");      /* T: 目标 */
//        Disp_SpeedVal(66, y, m[i]->target_speed);
//    }

//    OLED_Refresh_Gram();
//}
/**
 * @brief  手动逐位显示 6 位整数，不抑制前导零
 * @param  x, y: 起点坐标
 * @param  v:    要显示的整数
 * @note   固定占 7 个字符宽度：符号(1) + 6 位数字
 *         每个字符 6 像素，共 42 像素
 */
static void Disp_Num6(u8 x, u8 y, int32_t v)
{
    /* 符号 */
    if (v < 0)
    {
        OLED_ShowChar(x, y, '-', 12, 1);
        v = -v;
    }
    else
    {
        OLED_ShowChar(x, y, '+', 12, 1);
    }

    if (v > 999999) v = 999999;

    /* 逐位拆数字，高位在前，前导零也显示为 '0' */
    u8 digits[6];
    digits[0] = (v / 100000) % 10;
    digits[1] = (v /  10000) % 10;
    digits[2] = (v /   1000) % 10;
    digits[3] = (v /    100) % 10;
    digits[4] = (v /     10) % 10;
    digits[5] =  v             % 10;

    for (u8 i = 0; i < 6; i++)
    {
        OLED_ShowChar(x + 6 + i * 6, y, '0' + digits[i], 12, 1);
    }
}

/**
 * @brief  显示四个电机的目标速度和实际速度
 * @note   每行格式：电机名 T目标值 A实际值
 *         例如：A T+000100 A+000099
 *         布局宽度：6 + 6 + 42 + 6 + 42 = 102 像素 < 128
 */
void Chassis_Display_Speed(MecanumChassis_t *chassis)
{
    /* 清 GRAM */
    for (u8 i = 0; i < 8; i++)
        for (u8 n = 0; n < 128; n++)
            OLED_GRAM[n][i] = 0x00;

    Motor_t *m[4] = { chassis->motor_lf, chassis->motor_rf,
                      chassis->motor_lb, chassis->motor_rb };

    for (u8 i = 0; i < 4; i++)
    {
        u8 y = i * 12;   /* 4 行，每行 12 像素 */

        /* 电机名：x=0 */
        OLED_ShowChar(0, y, 'A' + i, 12, 1);

        /* T 标签：x=6 */
        OLED_ShowChar(6, y, 'T', 12, 1);

        /* 目标速度：x=12 起，占 42 像素（12~53） */
        Disp_Num6(12, y, (int32_t)m[i]->target_speed);

        /* A 标签：x=54 */
        OLED_ShowChar(54, y, 'A', 12, 1);

        /* 实际速度：x=60 起，占 42 像素（60~101） */
        Disp_Num6(60, y, (int32_t)m[i]->actual_speed);
    }

    OLED_Refresh_Gram();
}

#ifndef __MECANUM_CHASSIS_H
#define __MECANUM_CHASSIS_H

#include "stm32f4xx_hal.h"
#include "motor.h"
#include "math.h"

#define PI     3.141592653589793f
#define SQRT2  1.41421356237f
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
/**
 * @brief  位置环PID结构体
 */
typedef struct {
    float kp;
    float ki;
    float kd;
    float err;
    float err_last;
    float integral;
    float integral_max;
    float output_max;
    float output;
} PosPID_t;

/**
 * @brief  四轮麦克纳姆轮底盘控制结构体
 * @note   X型轮布局；坐标系：X向前、Y向左、逆时针为正
 */
typedef struct {
    /* 电机实例指针 */
    Motor_t *motor_lf;   // 左前轮
    Motor_t *motor_rf;   // 右前轮
    Motor_t *motor_lb;   // 左后轮
    Motor_t *motor_rb;   // 右后轮

    /* 机械参数 */
    float wheel_radius;       // 轮子半径，m
    float wheel_base_half;    // 半轴距，m
    float track_width_half;   // 半轮距，m
    float L;                  // 旋转系数 = 半轴距 + 半轮距
    float gear_ratio;         // 减速比
    float enc_ppr_x4;         // 编码器4倍频每转脉冲数
    float pulse_to_meter;     // 脉冲 → 米 转换系数
    float meter_to_pulse;     // 米 → 脉冲 转换系数

    /* 位置环PID */
    PosPID_t pid_x;
    PosPID_t pid_y;
    PosPID_t pid_theta;

    /* 当前状态 */
    float x_pos;
    float y_pos;
    float theta_pos;
    float vx_actual;
    float vy_actual;
    float omega_actual;

    /* 目标状态 */
    float x_target;
    float y_target;
    float theta_target;
    float vx_target;
    float vy_target;
    float omega_target;

    /* 限制参数 */
    float max_speed;
    float max_omega;
    float position_tol;
    float theta_tol;

    /* 状态标志 */
    uint8_t enable;
    uint8_t mode;             // 0-速度模式，1-位置模式
    uint8_t target_reached;
} MecanumChassis_t;

/* 函数声明 */
void MecanumChassis_Init(MecanumChassis_t *chassis,
                         Motor_t *lf, Motor_t *rf, Motor_t *lb, Motor_t *rb,
                         float wheel_radius, float wheel_base_half, float track_width_half,
                         float gear_ratio, float enc_ppr_x4);

void MecanumChassis_Loop(MecanumChassis_t *chassis, float dt);

void Mecanum_SetSpeedMode(MecanumChassis_t *chassis, float vx, float vy, float omega);
void Mecanum_SetPositionMode(MecanumChassis_t *chassis, float x, float y, float theta);

void Mecanum_MoveForward(MecanumChassis_t *chassis, float speed);
void Mecanum_MoveBackward(MecanumChassis_t *chassis, float speed);
void Mecanum_MoveLeft(MecanumChassis_t *chassis, float speed);
void Mecanum_MoveRight(MecanumChassis_t *chassis, float speed);
void Mecanum_RotateCCW(MecanumChassis_t *chassis, float omega);
void Mecanum_RotateCW(MecanumChassis_t *chassis, float omega);
void Mecanum_Stop(MecanumChassis_t *chassis);
/* ==================== 新增接口函数 ==================== */

/* 统一运动入口（推荐主函数优先用这个） */
void Mecanum_Move(MecanumChassis_t *chassis, float vx, float vy, float omega);

/* 状态查询 */
void Mecanum_GetPosition(MecanumChassis_t *chassis, float *x, float *y, float *theta);
void Mecanum_GetVelocity(MecanumChassis_t *chassis, float *vx, float *vy, float *omega);
uint8_t Mecanum_IsTargetReached(MecanumChassis_t *chassis);

/* 使能 / 失能 / 复位 */
void Mecanum_Enable(MecanumChassis_t *chassis, uint8_t enable);
void Mecanum_ResetPosition(MecanumChassis_t *chassis);

/* 参数配置 */
void Mecanum_SetSpeedLimit(MecanumChassis_t *chassis, float max_speed, float max_omega);
void Mecanum_SetTolerance(MecanumChassis_t *chassis, float position_tol, float theta_tol);
void Mecanum_SetPositionPID(MecanumChassis_t *chassis,
                            float kp, float ki, float kd,
                            float integral_max, float output_max);
							
/* ==================== 编码器调试显示 ==================== */
/* 显示单个电机的编码器原始计数 + 增量 */
void Chassis_Display_Encoder(Motor_t *motor, u8 y, const char *name);

/* 显示四个电机的编码器（全屏） */
void Chassis_Display_AllEncoder(MecanumChassis_t *chassis);
/* 显示四个电机的 actual/target speed（单页） */
//void Chassis_Display_MotorSpeed(MecanumChassis_t *chassis);
void Chassis_Display_Speed(MecanumChassis_t *chassis);
#endif


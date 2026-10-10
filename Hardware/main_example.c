//#include "stm32f4xx_hal.h"
//#include "motor.h"
//#include "mecanum_chassis.h"

///************************ 全局实例 ************************/
//Motor_t motor_left_front;   // 电机A = 左前轮
//Motor_t motor_right_front;  // 电机B = 右前轮
//Motor_t motor_left_back;    // 电机C = 左后轮
//Motor_t motor_right_back;   // 电机D = 右后轮

//MecanumChassis_t chassis;

///************************ 外部定时器句柄（CubeMX生成） ************************/
//extern TIM_HandleTypeDef htim1;
//extern TIM_HandleTypeDef htim2;
//extern TIM_HandleTypeDef htim3;
//extern TIM_HandleTypeDef htim4;
//extern TIM_HandleTypeDef htim5;
//extern TIM_HandleTypeDef htim6;
//extern TIM_HandleTypeDef htim9;
//extern TIM_HandleTypeDef htim10;
//extern TIM_HandleTypeDef htim11;

///************************ 函数声明 ************************/
//void SystemClock_Config(void);
//static void MX_GPIO_Init(void);
//static void MX_TIM1_Init(void);
//static void MX_TIM2_Init(void);
//static void MX_TIM3_Init(void);
//static void MX_TIM4_Init(void);
//static void MX_TIM5_Init(void);
//static void MX_TIM6_Init(void);
//static void MX_TIM9_Init(void);
//static void MX_TIM10_Init(void);
//static void MX_TIM11_Init(void);

///************************ 主函数 ************************/
//int main(void)
//{
//    /* 1. 基础初始化（顺序不可变） */
//    HAL_Init();
//    SystemClock_Config();
//    MX_GPIO_Init();
//	
//    /* 2. 所有定时器外设初始化 */
//    MX_TIM1_Init();
//    MX_TIM2_Init();
//    MX_TIM3_Init();
//    MX_TIM4_Init();
//    MX_TIM5_Init();
//    MX_TIM6_Init();
//    MX_TIM9_Init();
//    MX_TIM10_Init();
//    MX_TIM11_Init();
//   
//    /* 3. 高级定时器TIM1开启主输出（必须！否则无PWM） */
//    __HAL_TIM_MOE_ENABLE(&htim1);
//    
//    /* 4. 四个电机初始化（完全对应引脚表） */
//    // 电机A（左前）：PWM=TIM10_CH1+TIM11_CH1；编码器=TIM2
//    Motor_Init(&motor_left_front,
//               &htim10, TIM_CHANNEL_1,  8399,
//               &htim11, TIM_CHANNEL_1,  8399,
//               &htim2,
//               2.0f, 0.1f, 0.05f,1,1
//               );

//    // 电机B（右前）：PWM=TIM9_CH1+TIM9_CH2；编码器=TIM3
//    Motor_Init(&motor_right_front,
//               &htim9, TIM_CHANNEL_1,  8399,
//               &htim9, TIM_CHANNEL_2,  8399,
//               &htim3,
//               2.0f, 0.1f, 0.05f,1,1
//               );

//    // 电机C（左后）：PWM=TIM1_CH1+TIM1_CH2；编码器=TIM4
//    Motor_Init(&motor_left_back,
//               &htim1, TIM_CHANNEL_1,  4199,
//               &htim1, TIM_CHANNEL_2,  4199,
//               &htim4,
//               2.0f, 0.1f, 0.05f,1,1
//               );

//    // 电机D（右后）：PWM=TIM1_CH3+TIM1_CH4；编码器=TIM5
//    Motor_Init(&motor_right_back,
//               &htim1, TIM_CHANNEL_3,  4199,
//               &htim1, TIM_CHANNEL_4,  4199,
//               &htim5,
//               2.0f, 0.1f, 0.05f,1,1
//               );

////kp	比例增益	越大响应越快，但过大会振荡
////ki	积分增益	消除稳态误差，过大会超调/积分饱和
////kd	微分增益	抑制超调，但对噪声敏感    

//    /* 5. 底盘初始化 */
//    MecanumChassis_Init(&chassis,
//                        &motor_left_front, &motor_right_front,
//                        &motor_left_back, &motor_right_back,
//                        0.05f, 0.15f, 0.15f,
//                        30.0f, 4096.0f);
////wheel_radius		轮子半径						m			卡尺测量，如 50mm 轮子 = 0.05f
////wheel_base_half	半轴距（前后轮中心距的一半）	m			测量前后轮中心距离 ÷ 2
////track_width_half	半轮距（左右轮中心距的一半）	m			测量左右轮中心距离 ÷ 2
////gear_ratio			减速比							—			电机规格书，如 1:30 减速箱 = 30.0f
////enc_ppr_x4			编码器 4 倍频每转脉冲数			—			编码器线数 × 4，如 500 线 = 2000.0f

//    /* 6. 配置位置环PID参数（示例值，需现场整定） */
//    chassis.pid_x.kp = 3.0f;
//    chassis.pid_x.ki = 0.2f;
//    chassis.pid_x.kd = 0.1f;
//    chassis.pid_x.integral_max = 0.5f;
//    chassis.pid_x.output_max = 1.0f;

//    chassis.pid_y.kp = 3.0f;
//    chassis.pid_y.ki = 0.2f;
//    chassis.pid_y.kd = 0.1f;
//    chassis.pid_y.integral_max = 0.5f;
//    chassis.pid_y.output_max = 1.0f;

//    chassis.pid_theta.kp = 2.0f;
//    chassis.pid_theta.ki = 0.1f;
//    chassis.pid_theta.kd = 0.05f;
//    chassis.pid_theta.integral_max = 1.0f;
//    chassis.pid_theta.output_max = PI;

//    /* 7. 开启1ms定时中断（最后开，防止初始化未完成触发中断） */
//    HAL_TIM_Base_Start_IT(&htim6);

//	uint32_t last_tick = 0;

//    while (1)
//    {
//        /* 每 150ms 刷新一次编码器显示 */
//        if (HAL_GetTick() - last_tick >= 150)
//        {
//            last_tick = HAL_GetTick();
//            Chassis_Display_AllEncoder(&chassis);
//        }

//        /* 调编码器方向：不要给电机上电，用手慢慢转轮子 */
//    }
//}

///************************ 1ms定时器中断回调 ************************/
//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
//{
//    static uint8_t chassis_cnt = 0;

//    if (htim->Instance == TIM6)
//    {
//        /* 1ms：4路电机速度环闭环 */
//        Motor_Update(&motor_left_front,  0.001f);
//        Motor_Update(&motor_right_front, 0.001f);
//        Motor_Update(&motor_left_back,   0.001f);
//        Motor_Update(&motor_right_back,  0.001f);

//        /* 10ms：底盘运动学+位置环 */
//        chassis_cnt++;
//        if (chassis_cnt >= 10)
//        {
//            chassis_cnt = 0;
//            MecanumChassis_Loop(&chassis, 0.01f);
//        }
//    }
////}

///************************ 以下为CubeMX生成的虚函数占位 ************************/
//void SystemClock_Config(void)
//{
//  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
//  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

//  /** Configure the main internal regulator output voltage
//  */
//  __HAL_RCC_PWR_CLK_ENABLE();
//  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

//  /** Initializes the RCC Oscillators according to the specified parameters
//  * in the RCC_OscInitTypeDef structure.
//  */
//  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
//  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
//  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
//  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
//  RCC_OscInitStruct.PLL.PLLM = 4;
//  RCC_OscInitStruct.PLL.PLLN = 168;
//  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
//  RCC_OscInitStruct.PLL.PLLQ = 4;
//  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
//  {
//    Error_Handler();
//  }

//  /** Initializes the CPU, AHB and APB buses clocks
//  */
//  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
//                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
//  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
//  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
//  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
//  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

//  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
//  {
//    Error_Handler();
//  }
//}

//static void MX_GPIO_Init(void)
//{
//    // 由CubeMX生成，此处省略
//}

//static void MX_TIM1_Init(void)  { /* CubeMX生成 */ }
//static void MX_TIM2_Init(void)  { /* CubeMX生成 */ }
//static void MX_TIM3_Init(void)  { /* CubeMX生成 */ }
//static void MX_TIM4_Init(void)  { /* CubeMX生成 */ }
//static void MX_TIM5_Init(void)  { /* CubeMX生成 */ }
//static void MX_TIM6_Init(void)  { /* CubeMX生成 */ }
//static void MX_TIM9_Init(void)  { /* CubeMX生成 */ }
//static void MX_TIM10_Init(void) { /* CubeMX生成 */ }
//static void MX_TIM11_Init(void) { /* CubeMX生成 */ }

//void Error_Handler(void)
//{
//  /* USER CODE BEGIN Error_Handler_Debug */
//  /* User can add his own implementation to report the HAL error return state */
//  __disable_irq();
//  while (1)
//  {
//  }
//  /* USER CODE END Error_Handler_Debug */
//}


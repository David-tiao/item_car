/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "gpio.h"
#include "stm32f4xx_hal.h"
#include "bsp_oled.h"
#include <stdlib.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "motor.h"
#include "mecanum_chassis.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
Motor_t motor_left_front;   // 电机A = 左前轮
Motor_t motor_right_front;  // 电机B = 右前轮
Motor_t motor_left_back;    // 电机C = 左后轮
Motor_t motor_right_back;   // 电机D = 右后轮

MecanumChassis_t chassis;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define PULSE_PER_METER  6500    /* 实测：走1米约6500脉冲 */
#define SPEED            1000    /* 前进速度，脉冲/秒 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
   int32_t enc_start = (int32_t)__HAL_TIM_GET_COUNTER(&htim2);   /* A轮编码器 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
 
	
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_TIM5_Init();
  MX_TIM6_Init();
  MX_TIM9_Init();
  MX_TIM10_Init();
  MX_TIM11_Init();
  MX_TIM12_Init();
  MX_TIM8_Init();
  /* USER CODE BEGIN 2 */
	OLED_Init();
	/* 3. 高级定时器TIM1开启主输出（必须！否则无PWM） */
	__HAL_TIM_MOE_ENABLE(&htim1);
    
    /* 4. 四个电机初始化（完全对应引脚表） */
    // 电机A（左前）：PWM=TIM10_CH1+TIM11_CH1；编码器=TIM2
    Motor_Init(&motor_left_front,
               &htim10, TIM_CHANNEL_1,  4199,
               &htim11, TIM_CHANNEL_1,  4199,
               &htim2,
               4.0f, 0.1f, 0.0f,1,1
               );

    // 电机B（右前）：PWM=TIM9_CH1+TIM9_CH2；编码器=TIM3
    Motor_Init(&motor_right_front,
               &htim9, TIM_CHANNEL_1,  4199,
               &htim9, TIM_CHANNEL_2,  4199,
               &htim3,
               4.0f, 0.1f, 0.0f,1,-1
               );

    // 电机C（左后）：PWM=TIM1_CH1+TIM1_CH2；编码器=TIM4
    Motor_Init(&motor_left_back,
               &htim1, TIM_CHANNEL_1,  8399,
               &htim1, TIM_CHANNEL_2,  8399,
               &htim4,
               4.0f, 0.1f, 0.0f,-1,1
               );

    // 电机D（右后）：PWM=TIM1_CH3+TIM1_CH4；编码器=TIM5
    Motor_Init(&motor_right_back,
               &htim1, TIM_CHANNEL_3,  8399,
               &htim1, TIM_CHANNEL_4,  8399,
               &htim5,
               4.0f, 0.1f, 0.0f,-1,-1
               );

//kp	比例增益	越大响应越快，但过大会振荡
//ki	积分增益	消除稳态误差，过大会超调/积分饱和
//kd	微分增益	抑制超调，但对噪声敏感    

    /* 5. 底盘初始化 */
    MecanumChassis_Init(&chassis,
                        &motor_left_front, &motor_right_front,
                        &motor_left_back, &motor_right_back,
                        0.04f, 0.093f, 0.11f,
                        1.0f, 440.0f);
//wheel_radius		    轮子半径						m			卡尺测量，如 50mm 轮子 = 0.05f
//wheel_base_half	    半轴距（前后轮中心距的一半）	m			测量前后轮中心距离 ÷ 2
//track_width_half	    半轮距（左右轮中心距的一半）	m			测量左右轮中心距离 ÷ 2
//gear_ratio			减速比							—			这里直接填1
//enc_ppr_x4			编码器 4 倍频每转脉冲数			—			编码器线数 × 4，如 500 线 = 2000.0f

	/* 6. 配置位置环PID参数（示例值，需现场整定） */
	chassis.pid_x.kp = 3.0f;
    chassis.pid_x.ki = 0.2f;
    chassis.pid_x.kd = 0.1f;
    chassis.pid_x.integral_max = 0.5f;
    chassis.pid_x.output_max = 1.0f;

    chassis.pid_y.kp = 3.0f;
    chassis.pid_y.ki = 0.2f;
    chassis.pid_y.kd = 0.1f;
    chassis.pid_y.integral_max = 0.5f;
    chassis.pid_y.output_max = 1.0f;

    chassis.pid_theta.kp = 2.0f;
    chassis.pid_theta.ki = 0.1f;
    chassis.pid_theta.kd = 0.05f;
    chassis.pid_theta.integral_max = 1.0f;
    chassis.pid_theta.output_max = PI;
	
	 /* 7. 开启1ms定时中断（最后开，防止初始化未完成触发中断） */
    HAL_TIM_Base_Start_IT(&htim6);

	uint32_t last_tick = 0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {        
//	  int32_t now = (int32_t)__HAL_TIM_GET_COUNTER(&htim2);

//	  if (now - enc_start <= PULSE_PER_METER) 
//	{
	     Mecanum_Move(&chassis, 0.0f, 0.0f, 3.0f);
//	}
    /* USER CODE END WHILE */
    /* 每 150ms 刷新一次编码器显示 */
       
//    if (now - enc_start >= PULSE_PER_METER)   /* 只判断A轮 */
//    {
//        Motor_SetTargetSpeed(&motor_left_front, 0);
//        Motor_SetTargetSpeed(&motor_right_front, 0);
//        Motor_SetTargetSpeed(&motor_left_back, 0);
//        Motor_SetTargetSpeed(&motor_right_back, 0);
//    }
	  
	     if (HAL_GetTick() - last_tick >= 150)
        {
            last_tick = HAL_GetTick();
            //Chassis_Display_AllEncoder(&chassis);
			//Chassis_Display_MotorSpeed(&chassis);   /* ★ 换成这个 */
			Chassis_Display_Speed(&chassis);   /* ★ 只调这一个 */
        }
    /* USER CODE BEGIN 3 */
  }
  
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    static uint8_t chassis_cnt = 0;

    if (htim->Instance == TIM6)
    {
		static uint8_t speed_cnt = 0;

        speed_cnt++;
        if (speed_cnt >= 1)     /* 每 1ms 测一次 */
        /* 1ms：4路电机速度环闭环 */
        {
		speed_cnt = 0;
		Motor_Update(&motor_left_front,  0.001f);
        Motor_Update(&motor_right_front, 0.001f);
        Motor_Update(&motor_left_back,   0.001f);
        Motor_Update(&motor_right_back,  0.001f);
		}
        /* 10ms：底盘运动学+位置环 */
        chassis_cnt++;
        if (chassis_cnt >= 10)
        {
            chassis_cnt = 0;
            MecanumChassis_Loop(&chassis, 0.01f);
        }
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

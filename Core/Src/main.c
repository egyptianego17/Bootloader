/*
 *
 *		App: Bootloader Entry Point
 *  	Created on: Jan 27, 2024
 *      Author: Abdulrahman Omar
 *      Version: v 1.0
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crc.h"
#include "usart.h"
#include "gpio.h"
#include "BTL_Interface.h"

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  uint8_t* dataBuffer;
  BTL_CMDTypeDef receivedCMD;
  uint16_t dataLength = 0;
  dataBuffer = (uint8_t*)calloc(DATA_BUFFER_SIZE, sizeof(uint8_t));   /* 2 Kb Buffer to hold the messages */

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_CRC_Init();

  while (1)
  {
	  /* Clear the buffer before reception  */
	  memset(dataBuffer, 0, DATA_BUFFER_SIZE);
	  __HAL_UART_FLUSH_DRREGISTER(&huart1);  /* Flush the data register      */
	  __HAL_UART_CLEAR_OREFLAG(&huart1);     /* Clear the overrun flag       */
	  HAL_UART_AbortReceive(&huart1);        /* Abort the ongoing reception  */

	  /* Getting command type from the host */
	  receivedCMD = BTL_GetMessage(dataBuffer);
	  if (receivedCMD != BTL_ERROR_CMD)
	  {
		  /* Calculate the dataLength to be received if it exists */
		  dataLength = dataBuffer[BTL_DATA_SIZE0] << 4 | dataBuffer[BTL_DATA_SIZE1];
		  switch(receivedCMD)
		  {
		  case BTL_GET_VERSION: BTL_GetVersion(); break;
		  case BTL_GET_HELP: BTL_SendMessage("Sent command is: %d, Size %c%c\n", dataBuffer[2],dataBuffer[0], dataBuffer[1]); break;
		  case BTL_GET_ID: BTL_SendMessage("Sent command is: %d, Size %c%c\n", dataBuffer[2],dataBuffer[0], dataBuffer[1]); break;
		  case BTL_FLASH_ERASE: BTL_SendMessage("Sent command is: %d, Size %c%c\n", dataBuffer[2],dataBuffer[0], dataBuffer[1]); break;
		  case BTL_MEM_READ: BTL_SendMessage("Sent command is: %d, Size %c%c\n", dataBuffer[2],dataBuffer[0], dataBuffer[1]); break;
		  case BTL_OTP_READ: BTL_SendMessage("Sent command is: %d, Size %c%c\n", dataBuffer[2],dataBuffer[0], dataBuffer[1]); break;
		  case BTL_APP_FLASH: BTL_UpdateFirmware(dataBuffer, dataLength);break;
		  default: BTL_SendMessage("%d %c%c",dataBuffer[2],dataBuffer[0], dataBuffer[1]) ;break;
		  }

	  }
  }

  return 0;
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
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

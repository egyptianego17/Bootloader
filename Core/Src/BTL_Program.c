/*
 * BTL_Program.c
 *
 *  Created on: Jan 27, 2024
 *      Author: Xavi Omar
 */


#include "usart.h"
#include "stm32f4xx_hal_flash.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "BTL_Private.h"
#include "BTL_Config.h"
#include "BTL_Interface.h"

BTL_StatusTypeDef BTL_SendMessage(char* messageFormat, ...);
BTL_CMDTypeDef BTL_GetMessage(uint8_t* messageBuffer);
BTL_StatusTypeDef BTL_SendAck(BTL_CMDTypeDef cmdID);
BTL_StatusTypeDef BTL_GetVersion(void);
BTL_StatusTypeDef BTL_UpdateFirmware(uint8_t* messageBuffer, uint16_t dataLength);
uint8_t BTL_ASCHIIToHex(uint8_t ASCHIIValue);
BTL_StatusTypeDef BTL_HexFlasher(uint8_t* dataBuffer, BTL_RecordTypeDef* currentRecord);
BTL_StatusTypeDef BTL_FlashWrite(uint8_t* dataBuffer, uint16_t datalength, BTL_RecordTypeDef* currentRecord);

BTL_StatusTypeDef BTL_SendMessage(char* messageFormat, ...)
{
	BTL_StatusTypeDef BTL_STATUS = BTL_ERROR;

    char message[512] = {0};
	/* Init va_list to handle the message */
    va_list args;
    va_start(args, messageFormat);

    /* Using vsnprintf to format the message */
    vsnprintf(message, sizeof(message), messageFormat, args);

    /* Transmitting the formatted data to the Host */
	if (HAL_UART_Transmit(&huart1, (uint8_t*) message, sizeof(message), HAL_MAX_DELAY) == HAL_OK)
	{
		BTL_STATUS = BTL_OK;
	}

    va_end(args);
    return BTL_STATUS;
}


BTL_CMDTypeDef BTL_GetMessage(uint8_t* messageBuffer)
{
	BTL_CMDTypeDef BTL_CMD = BTL_ERROR_CMD;

	/* Getting the size of the data & command type */
	if (HAL_UART_Receive(&huart1, (uint8_t*) &messageBuffer[0], 3, HAL_MAX_DELAY) == HAL_OK)
	{
		BTL_CMD = messageBuffer[BTL_CMD_TYPE];
	}

    return BTL_CMD;
}

BTL_StatusTypeDef BTL_SendAck(BTL_CMDTypeDef cmdID)
{
	BTL_StatusTypeDef BTL_STATUS = BTL_ERROR;
	/* Send ID as A*/
	if (BTL_SendMessage("%c", cmdID) == BTL_OK)
	{
		BTL_STATUS = BTL_OK;
	}
	return BTL_STATUS;
}

BTL_StatusTypeDef BTL_SendNAck()
{
	BTL_StatusTypeDef BTL_STATUS = BTL_ERROR;
	/* Send ID as A*/
	if (BTL_SendMessage("%c", 0) == BTL_OK)
	{
		BTL_STATUS = BTL_OK;
	}
	return BTL_STATUS;
}


BTL_StatusTypeDef BTL_GetVersion()
{
	BTL_StatusTypeDef BTL_STATUS = BTL_ERROR;
	if (BTL_SendMessage("Bootloader Version: %c.%c.%c\r\n", BTL_V_MAJOR, BTL_V_MINOR, BTL_V_PATCH) == BTL_OK)
	{
		BTL_STATUS = BTL_OK;
	}
	return BTL_STATUS;
}

BTL_StatusTypeDef BTL_UpdateFirmware(uint8_t* messageBuffer, uint16_t dataLength)
{
	BTL_StatusTypeDef BTL_STATUS = BTL_ERROR;
	BTL_StatusTypeDef BTL_FIRMWARE_DONE = BTL_ERROR;

	/* Send ack */
	if (BTL_SendAck(messageBuffer[BTL_CMD_TYPE]) == BTL_OK)
	{
		/* While file is not done receive remaining records */
		while (BTL_FIRMWARE_DONE != BTL_OK)
		{
			if (HAL_UART_Receive(&huart1, (uint8_t*) &messageBuffer[BTL_BUFFER_RECORDS0], dataLength, HAL_MAX_DELAY) == HAL_OK)
			{
				/* Struct the hold all records data used in flashing and logic */
				BTL_RecordTypeDef* RecordsData;

				/* Parse the number of the records in the buffer */
				RecordsData->BTL_NO_OF_BUFFER_RECORDS  = messageBuffer[BTL_BUFFER_RECORDS0];

				/* Init first byte position in the records */
				RecordsData->BTL_RECORD_INDEX = 0;

				/* Check if it's the end of the file or not */
				BTL_FIRMWARE_DONE = messageBuffer[BTL_DONE_FLAG];

				/* Start flashing the packet */
				if (BTL_FlashWrite(&messageBuffer[BTL_DATA_START], dataLength, &RecordsData) == BTL_OK)
				{
					/* Send packet flashing done */
					BTL_SendAck(messageBuffer[BTL_CMD_TYPE]);
					BTL_STATUS = BTL_OK;
				}

				else
				{
					/* Send packet flashing error */
					BTL_SendNAck();
					BTL_STATUS = BTL_ERROR;
					break;
				}

			}
			else
			{
				BTL_STATUS = BTL_ERROR;
				break;
			}

			/* Update the next packet size length */
			dataLength  = (messageBuffer[BTL_BUFFER_NEXT_SIZE0] << 4) \
					    |  messageBuffer[BTL_BUFFER_NEXT_SIZE1];
		}

	}
	return BTL_STATUS && BTL_FIRMWARE_DONE;
}


uint8_t BTL_ASCHIIToHex(uint8_t ASCHIIValue)
{
	uint8_t hexValue;
	if ((ASCHIIValue >= 48) && (ASCHIIValue <= 57))
	{
		hexValue = ASCHIIValue - 48;
	}
	else
	{
		hexValue = ASCHIIValue - 55;
	}
	return hexValue;
}


BTL_StatusTypeDef BTL_HexFlasher(uint8_t* dataBuffer, BTL_RecordTypeDef* currentRecord)
{
	BTL_StatusTypeDef BTL_STATUS = BTL_ERROR;

	currentRecord->BTL_ADDRESS_HIGH = 0x0800;
	currentRecord->BTL_BUFFER_POINTER = 0;
	/* Check RecordType */
	currentRecord->BTL_RECORD_TYPE = BTL_ASCHIIToHex(dataBuffer[(currentRecord->BTL_BUFFER_POINTER)+ BTL_RT_0]) \
				                   | BTL_ASCHIIToHex(dataBuffer[(currentRecord->BTL_BUFFER_POINTER)+ BTL_RT_1]);

	if (currentRecord->BTL_RECORD_TYPE == BTL_DATA_RECORD_TYPE)
	{
		currentRecord->BTL_CC  = (BTL_ASCHIIToHex(dataBuffer[(currentRecord->BTL_BUFFER_POINTER) + BTL_CC_0]) << 4)\
							   |  BTL_ASCHIIToHex(dataBuffer[(currentRecord->BTL_BUFFER_POINTER) + BTL_CC_1]);

		currentRecord->BTL_ADD = ((currentRecord->BTL_ADDRESS_HIGH) << 16) \
							   | ((BTL_ASCHIIToHex(dataBuffer[(currentRecord->BTL_BUFFER_POINTER) + BTL_ADD_0])) << 12) \
							   | ((BTL_ASCHIIToHex(dataBuffer[(currentRecord->BTL_BUFFER_POINTER) + BTL_ADD_1])) << 8)\
							   | ((BTL_ASCHIIToHex(dataBuffer[(currentRecord->BTL_BUFFER_POINTER) + BTL_ADD_2])) << 4)\
							   |   BTL_ASCHIIToHex(dataBuffer[(currentRecord->BTL_BUFFER_POINTER) + BTL_ADD_3]);


		/* Send the next by  */
		for (uint8_t bytesCounter = 0; bytesCounter < currentRecord->BTL_CC; bytesCounter++)
		{
			currentRecord->BTL_DATA = ((BTL_ASCHIIToHex(dataBuffer[(currentRecord->BTL_BUFFER_POINTER) + BTL_DATA_0 + bytesCounter*2])) << 4) \
									|  BTL_ASCHIIToHex(dataBuffer[(currentRecord->BTL_BUFFER_POINTER) + BTL_DATA_1 + bytesCounter*2]);
			HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, currentRecord->BTL_ADD, currentRecord->BTL_DATA);
		}

		/* 18 = 2 CC + 4 ADD + 2 RT + CC*2 + 2 CS + 1 \n */
		currentRecord->BTL_BUFFER_POINTER += (currentRecord->BTL_CC)*2 + 11;
	}
	else if (currentRecord->BTL_RECORD_TYPE == BTL_EXTEND_RECORD_TYPE)
	{
		currentRecord->BTL_ADDRESS_HIGH = ((BTL_ASCHIIToHex(dataBuffer[(currentRecord->BTL_BUFFER_POINTER) + BTL_DATA_0])) << 12) \
				                        | ((BTL_ASCHIIToHex(dataBuffer[(currentRecord->BTL_BUFFER_POINTER) + BTL_DATA_1])) << 8) \
										| ((BTL_ASCHIIToHex(dataBuffer[(currentRecord->BTL_BUFFER_POINTER) + BTL_DATA_2])) << 4) \
										|   BTL_ASCHIIToHex(dataBuffer[(currentRecord->BTL_BUFFER_POINTER) + BTL_DATA_3]);

		/* 18 = 2 CC + 4 ADD + 2 RT + CC*2 + 2 CS + 1 \n */
		currentRecord->BTL_BUFFER_POINTER += (currentRecord->BTL_CC)*2 + 11;
	}
	return BTL_STATUS;
}


BTL_StatusTypeDef BTL_FlashWrite(uint8_t* dataBuffer, uint16_t datalength, BTL_RecordTypeDef* currentRecord)
{
	BTL_StatusTypeDef BTL_STATUS = BTL_ERROR;

	static FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t SECTOR_ERROR = 0;

	/* Unlock the flash */
	HAL_FLASH_Unlock();

	/* Init erasing parameters */
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
	EraseInitStruct.Banks = FLASH_BANK_1;
	EraseInitStruct.Sector = FLASH_SECTOR_2;
	EraseInitStruct.NbSectors = 4;

	/* Start erasing */
	HAL_FLASHEx_Erase(&EraseInitStruct, &SECTOR_ERROR);

	/* If erasing succeed then start parsing */
	if(SECTOR_ERROR == 0xFFFFFFFFU)
	{
		//while ((currentRecord->BTL_RECORD_INDEX) < (currentRecord->BTL_NO_OF_BUFFER_RECORDS))
		//{
			/* Start Flashing */
			if (BTL_HexFlasher(dataBuffer, &currentRecord) == BTL_OK)
			{
				BTL_STATUS = BTL_OK;
			}
		//}
	}
	return BTL_STATUS;
}


//static BTL_StatusTypeDef BTL_CheckSum(uint8_t dataBuffer, uint16_t datalength);



//BTL_SendMessage("Chip ID: %c%c", ((uint8_t)DBGMCU->IDCODE >> 8), (uint8_t)DBGMCU->IDCODE)

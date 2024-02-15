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
BTL_StatusTypeDef BTL_SendNAck(void);
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
    BTL_StatusTypeDef BTL_DONE = BTL_ERROR;

    if (BTL_SendAck(BTL_MEM_WRITE_CMD) != BTL_OK) {
        return BTL_ERROR;
    }

    if (HAL_UART_Receive(&huart1, &messageBuffer[BTL_DONE_FLAG], dataLength + 4, HAL_MAX_DELAY) != HAL_OK)
    {
        return BTL_ERROR;
    }

    uint8_t FlashFailure = 0;

    static FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SECTOR_ERROR = 0;

    HAL_FLASH_Unlock();

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Sector = FLASH_SECTOR_2;
    EraseInitStruct.NbSectors = 4;

    HAL_FLASHEx_Erase(&EraseInitStruct, &SECTOR_ERROR);

    if (SECTOR_ERROR == 0xFFFFFFFFU)
    {
    	do
		{
    		BTL_DONE = messageBuffer[BTL_DONE_FLAG];
			BTL_RecordTypeDef* RecordsData = malloc(sizeof(BTL_RecordTypeDef));

			if (RecordsData == NULL)
			{
				return BTL_ERROR;
			}

			RecordsData->BTL_NO_OF_BUFFER_RECORDS = messageBuffer[BTL_BUFFER_RECORDS0];
			RecordsData->BTL_RECORD_INDEX = 0;


			BTL_StatusTypeDef BTL_FLASH_STATUS = BTL_FlashWrite(&messageBuffer[BTL_DATA_START], dataLength, RecordsData);

			if (BTL_FLASH_STATUS == BTL_OK)
			{
				BTL_SendAck(BTL_MEM_WRITE_CMD);
				BTL_STATUS = BTL_OK;
			}
			else
			{
				BTL_SendNAck();
				FlashFailure++;
				BTL_STATUS = BTL_ERROR;
				break;
			}

			dataLength = (messageBuffer[BTL_BUFFER_NEXT_SIZE0] << 4) | messageBuffer[BTL_BUFFER_NEXT_SIZE1];

			memset(messageBuffer, 0, DATA_BUFFER_SIZE);

			free(RecordsData);

			if ((FlashFailure >= MAX_TIMEOUT) || (BTL_DONE == BTL_OK)) {
				break;
			}

			else if (HAL_UART_Receive(&huart1, &messageBuffer[BTL_CMD_TYPE], dataLength + 4, HAL_MAX_DELAY) != HAL_OK)
	        {
	            return BTL_ERROR;
	        }

		} while (BTL_DONE != BTL_OK);

    }

    else
    {
		BTL_STATUS = BTL_ERROR;
    }

    HAL_FLASH_Lock();

    return BTL_STATUS;
}

uint8_t BTL_ASCHIIToHex(uint8_t ASCHIIValue)
{
    if ((ASCHIIValue >= '0') && (ASCHIIValue <= '9'))
    {
        return ASCHIIValue - '0';
    }
    else if ((ASCHIIValue >= 'A') && (ASCHIIValue <= 'F'))
    {
        return ASCHIIValue - 'A' + 10;
    }
    else if ((ASCHIIValue >= 'a') && (ASCHIIValue <= 'f'))
    {
        return ASCHIIValue - 'a' + 10;
    }


    return 0;
}

BTL_StatusTypeDef BTL_HexFlasher(uint8_t* dataBuffer, BTL_RecordTypeDef* currentRecord)
{
    BTL_StatusTypeDef BTL_STATUS = BTL_OK;

    currentRecord->BTL_ADDRESS_HIGH = 0x0800;

    currentRecord->BTL_RECORD_TYPE = BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_RT_0]) << 4 |
                                     BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_RT_1]);


    currentRecord->BTL_CC = (BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_CC_0]) << 4) |
                             BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_CC_1]);

    if (currentRecord->BTL_RECORD_TYPE == BTL_EOF_RECORD_TYPE)
    {
    	return BTL_OK;
    }

    else if (currentRecord->BTL_RECORD_TYPE == BTL_DATA_RECORD_TYPE)
    {


        currentRecord->BTL_ADD = ((currentRecord->BTL_ADDRESS_HIGH) << 16) |
                                 ((BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_ADD_0])) << 12) |
                                 ((BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_ADD_1])) << 8) |
                                 ((BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_ADD_2])) << 4) |
                                   BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_ADD_3]);

        for (uint8_t bytesCounter = 0; bytesCounter < currentRecord->BTL_CC; bytesCounter++)
        {
            currentRecord->BTL_DATA = (BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_DATA_0 + bytesCounter * 2]) << 4) |
                                      BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_DATA_1 + bytesCounter * 2]);

            BTL_STATUS = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, currentRecord->BTL_ADD + BTL_BOOTLOADER_SIZE + bytesCounter, currentRecord->BTL_DATA);
        }
        currentRecord->BTL_BUFFER_POINTER += (currentRecord->BTL_CC) * 2 + 11;
    }
    else if (currentRecord->BTL_RECORD_TYPE == BTL_EXT_LINEAR_ADDR_RECORD)
    {

        currentRecord->BTL_ADDRESS_HIGH = ((BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_DATA_0])) << 12) |
                                          ((BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_DATA_1])) << 8) |
                                          ((BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_DATA_2])) << 4) |
                                            BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_DATA_3]);

        currentRecord->BTL_BUFFER_POINTER += (currentRecord->BTL_CC) * 2 + 11;

    }
    else if (currentRecord->BTL_RECORD_TYPE == BTL_START_LINEAR_ADDR_RECORD)
    {

        currentRecord->BTL_ADD = ((BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_FULL_ADD0])) << 28) |
                                 ((BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_FULL_ADD1])) << 24) |
                                 ((BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_FULL_ADD2])) << 20) |
        						 ((BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_FULL_ADD3])) << 16) |
        						 ((BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_FULL_ADD4])) << 12) |
                                 ((BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_FULL_ADD5])) << 8)  |
                                 ((BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_FULL_ADD6])) << 4)  |
                                   BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_FULL_ADD7]);

        currentRecord->BTL_BUFFER_POINTER += (currentRecord->BTL_CC) * 2 + 11;

    }
    else
    {
    	BTL_STATUS = BTL_ERROR;
    }

    return BTL_STATUS;
}

BTL_StatusTypeDef BTL_FlashWrite(uint8_t* dataBuffer, uint16_t dataLength, BTL_RecordTypeDef* currentRecord)
{
    BTL_StatusTypeDef BTL_STATUS = BTL_ERROR;

	currentRecord->BTL_BUFFER_POINTER = 0;

	while (currentRecord->BTL_RECORD_INDEX < currentRecord->BTL_NO_OF_BUFFER_RECORDS)
	{
		if (BTL_HexFlasher(dataBuffer, currentRecord) == BTL_OK)
		{
			currentRecord->BTL_RECORD_INDEX += 1;
			BTL_STATUS = BTL_OK;
		}
	}

    return BTL_STATUS;
}


//static BTL_StatusTypeDef BTL_CheckSum(uint8_t dataBuffer, uint16_t datalength);



//BTL_SendMessage("Chip ID: %c%c", ((uint8_t)DBGMCU->IDCODE >> 8), (uint8_t)DBGMCU->IDCODE)

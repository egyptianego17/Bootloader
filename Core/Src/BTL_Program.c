/*****************************************************/
/*                 SWC: Bootloader                   */
/*            Author: Abdulrahman Omar               */
/*                 Version: v 1.0                    */
/*              Date: 27 Jan - 2024                  */
/*****************************************************/

#include "usart.h"
#include "stm32f4xx_hal_flash.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "BTL_Private.h"
#include "BTL_Config.h"
#include "BTL_Interface.h"
#include "crc.h"

static BTL_StatusTypeDef BTL_SendAck(BTL_CMDTypeDef cmdID);
static BTL_StatusTypeDef BTL_SendNAck();
static uint8_t BTL_ASCHIIToHex(uint8_t ASCHIIValue);
static BTL_StatusTypeDef BTL_HexFlasher(uint8_t* dataBuffer, BTL_RecordTypeDef* currentRecord);
static BTL_StatusTypeDef BTL_FlashWrite(uint8_t* dataBuffer, uint16_t dataLength, BTL_RecordTypeDef* currentRecord);
static BTL_StatusTypeDef BTL_CheckRecord(uint8_t* dataBuffer, BTL_RecordTypeDef* currentRecord);
static uint8_t CalculateChecksum(const uint8_t *data, size_t length);


/**
 * @brief Send a formatted message over UART.
 * @param messageFormat Format string for the message.
 * @param ... Variable number of arguments for the formatted message.
 * @return BTL_StatusTypeDef Status of the message transmission.
 */
BTL_StatusTypeDef BTL_SendMessage(char* messageFormat, ...)
{
    BTL_StatusTypeDef BTL_STATUS = BTL_ERROR;

    char message[512] = {0};
    /* Initialize va_list to handle the message */
    va_list args;
    va_start(args, messageFormat);

    /* Use vsnprintf to format the message */
    vsnprintf(message, sizeof(message), messageFormat, args);

    /* Transmit the formatted data to the Host */
    if (HAL_UART_Transmit(&huart1, (uint8_t*) message, sizeof(message), HAL_MAX_DELAY) == HAL_OK)
    {
        BTL_STATUS = BTL_OK;
    }

    va_end(args);
    return BTL_STATUS;
}

/**
 * @brief Receive a message from UART.
 * @param messageBuffer Buffer to store the received message.
 * @return BTL_CMDTypeDef Type of command received.
 */
BTL_CMDTypeDef BTL_GetMessage(uint8_t* messageBuffer)
{
    BTL_CMDTypeDef BTL_CMD = BTL_ERROR_CMD;

    /* Get the size of the data & command type */
    if (HAL_UART_Receive(&huart1, (uint8_t*) &messageBuffer[0], 3, HAL_MAX_DELAY) == HAL_OK)
    {
        BTL_CMD = messageBuffer[BTL_CMD_TYPE];
    }

    return BTL_CMD;
}

/**
 * @brief Send acknowledgment for a command.
 * @param cmdID ID of the command to acknowledge.
 * @return BTL_StatusTypeDef Status of the acknowledgment transmission.
 */
static BTL_StatusTypeDef BTL_SendAck(BTL_CMDTypeDef cmdID)
{
    BTL_StatusTypeDef BTL_STATUS = BTL_ERROR;
    /* Send cmd ID as Acknowledgment */
    if (BTL_SendMessage("%c", cmdID) == BTL_OK)
    {
        BTL_STATUS = BTL_OK;
    }
    return BTL_STATUS;
}

/**
 * @brief Send a negative acknowledgment.
 * @return BTL_StatusTypeDef Status of the negative acknowledgment transmission.
 */
static BTL_StatusTypeDef BTL_SendNAck()
{
    BTL_StatusTypeDef BTL_STATUS = BTL_ERROR;
    /* Send ID as A */
    if (BTL_SendMessage("%c", 0) == BTL_OK)
    {
        BTL_STATUS = BTL_OK;
    }
    return BTL_STATUS;
}

/**
 * @brief Get the bootloader version and send it over UART.
 * @return BTL_StatusTypeDef Status of the version information transmission.
 */
BTL_StatusTypeDef BTL_GetVersion()
{
    BTL_StatusTypeDef BTL_STATUS = BTL_ERROR;
    if (BTL_SendMessage("Bootloader Version: %c.%c.%c\r\n", BTL_V_MAJOR, BTL_V_MINOR, BTL_V_PATCH) == BTL_OK)
    {
        BTL_STATUS = BTL_OK;
    }
    return BTL_STATUS;
}

/**
 * @brief Update firmware based on the provided message buffer.
 *
 * This function manages the firmware update process using the received
 * message buffer. It involves sending acknowledgment, erasing flash sectors,
 * and flashing the received packets to memory.
 *
 * @param messageBuffer Buffer containing the firmware update data.
 * @param dataLength Length of the data in the buffer.
 * @return BTL_StatusTypeDef Status of the firmware update operation.
 */
BTL_StatusTypeDef BTL_UpdateFirmware(uint8_t* messageBuffer, uint16_t dataLength)
{
    BTL_StatusTypeDef BTL_STATUS = BTL_ERROR;
    BTL_StatusTypeDef BTL_DONE = BTL_ERROR;

    /* Transmit an acknowledgment to signal MCU readiness for flashing */
    if (BTL_SendAck(BTL_APP_FLASH) != BTL_OK) {
        return BTL_ERROR;
    }

    /* Begin receiving the first packet of the program, prefixed with metadata:
     * BTL_DONE_FLAG            0 - Indicates if this is the last packet or not.
     * BTL_BUFFER_RECORDS0      1 - Indicates how many records are in the received buffer for iteration.
     * BTL_BUFFER_NEXT_SIZE0    3
     * BTL_BUFFER_NEXT_SIZE1    4 - Indicates how much data will be sent in the next packet.
     */
    if (HAL_UART_Receive(&huart1, &messageBuffer[BTL_DONE_FLAG], dataLength + 4, HAL_MAX_DELAY) != HAL_OK)
    {
        return BTL_ERROR; /* Return error if packet reception fails */
    }

    /* Counter to exit the loop in case of too many failures */
    uint8_t FlashFailure = 0;

    /* Start erasing the flash to prepare for writing */
    static FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SECTOR_ERROR = 0;

    HAL_FLASH_Unlock();

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Sector = FLASH_SECTOR_2;
    EraseInitStruct.NbSectors = 4;

    HAL_FLASHEx_Erase(&EraseInitStruct, &SECTOR_ERROR);

    /* If SECTOR_ERROR == 0xFFFFFFFFU, erasing is complete */
    if (SECTOR_ERROR == 0xFFFFFFFFU)
    {
        do
        {
        	/* Update the status indicating whether the process is complete or ongoing */
            BTL_DONE = messageBuffer[BTL_DONE_FLAG];

            /* RecordsData holds the current record for writing to memory */
            BTL_RecordTypeDef* RecordsData = malloc(sizeof(BTL_RecordTypeDef));

            /* Check allocation error */
            if (RecordsData == NULL)
            {
                return BTL_ERROR;
            }

            /* Update information about the current records in the packet */
            RecordsData->BTL_NO_OF_BUFFER_RECORDS = messageBuffer[BTL_BUFFER_RECORDS0];
            RecordsData->BTL_RECORD_INDEX = 0;

            /* Initiate flashing for the received packet. */
            BTL_StatusTypeDef BTL_FLASH_STATUS = BTL_FlashWrite(&messageBuffer[BTL_DATA_START], dataLength, RecordsData);

            /* Check for any allocation errors */
            if (BTL_FLASH_STATUS == BTL_OK)
            {
                BTL_SendAck(BTL_APP_FLASH);
                BTL_STATUS = BTL_OK;
            }
            else
            {
                BTL_SendNAck();
                FlashFailure++;
                BTL_STATUS = BTL_ERROR;
                break;
            }

            /** Update the dataLength to begin receiving the next packet. */
            dataLength = (messageBuffer[BTL_BUFFER_NEXT_SIZE0] << 4) | messageBuffer[BTL_BUFFER_NEXT_SIZE1];

            /* Clear the messageBuffer to prepare for receiving the next packet. */
            memset(messageBuffer, 0, DATA_BUFFER_SIZE);

            free(RecordsData);

            /* Check for timeout or the last packet; if true, end the process. */
            if ((FlashFailure >= MAX_TIMEOUT) || (BTL_DONE == BTL_OK)) {
                break;
            }

            /** Start the reception of the next packet. */
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

/**
 * @brief Convert ASCII representation of a hex value to its equivalent integer.
 * @param ASCHIIValue ASCII representation of the hex value.
 * @return uint8_t Equivalent integer value.
 */
static uint8_t BTL_ASCHIIToHex(uint8_t ASCHIIValue)
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

/**
 * @brief Flash hex data to the memory.
 * @param dataBuffer Buffer containing the hex data.
 * @param currentRecord Structure containing record information.
 * @return BTL_StatusTypeDef Status of the flashing operation.
 */
static BTL_StatusTypeDef BTL_HexFlasher(uint8_t* dataBuffer, BTL_RecordTypeDef* currentRecord)
{
    BTL_StatusTypeDef BTL_STATUS = BTL_OK;

    currentRecord->BTL_ADDRESS_HIGH = 0x0800;

    /* Parse the Record Type of the current record from the buffer */
    currentRecord->BTL_RECORD_TYPE = BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_RT_0]) << 4 |
                                     BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_RT_1]);

    /* Parse the Character Count of the current record from the buffer */
    currentRecord->BTL_CC = (BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_CC_0]) << 4) |
                             BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_CC_1]);

    /* Parse the CheckSum of the current record from the buffer */
    currentRecord->BTL_CHECKSUM = (BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + currentRecord->BTL_CC*2 + 8]) << 4) |
                                   BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + currentRecord->BTL_CC*2 + 9]);

    /* If End Of The file, then return BTL_OK */
    if (currentRecord->BTL_RECORD_TYPE == BTL_EOF_RECORD_TYPE)
    {
        return BTL_OK;
    }
    else if (currentRecord->BTL_RECORD_TYPE == BTL_DATA_RECORD_TYPE)
    {
        /* Begin parsing the address of the current record from the buffer */
        currentRecord->BTL_ADD = ((currentRecord->BTL_ADDRESS_HIGH) << 16) |
                                 ((BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_ADD_0])) << 12) |
                                 ((BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_ADD_1])) << 8) |
                                 ((BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_ADD_2])) << 4) |
                                 BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_ADD_3]);

        /* Validate the record */
        if (BTL_CheckRecord(dataBuffer ,currentRecord) == BTL_OK)
        {
            /* If the record is valid, then start flashing Record Byte by byte */
            for (uint8_t bytesCounter = 0; bytesCounter < currentRecord->BTL_CC; bytesCounter++)
            {
                currentRecord->BTL_DATA = (BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_DATA_0 + bytesCounter * 2]) << 4) |
                                          BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_DATA_1 + bytesCounter * 2]);

                BTL_STATUS = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, currentRecord->BTL_ADD + BTL_BOOTLOADER_SIZE + bytesCounter, currentRecord->BTL_DATA);
            }
            /* Shift the buffer pointer to point to the start of the next record
             * 11 = 2 (CC) + 4 (ADD) + 2 (RT) + 2 (CHSUM) + 1 (\n) */
            currentRecord->BTL_BUFFER_POINTER += (currentRecord->BTL_CC) * 2 + 11;
        }
        else
        {
            return BTL_ERROR;
        }
    }
    /* Set the high address */
    else if (currentRecord->BTL_RECORD_TYPE == BTL_EXT_LINEAR_ADDR_RECORD)
    {
        currentRecord->BTL_ADDRESS_HIGH = ((BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_DATA_0])) << 12) |
                                          ((BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_DATA_1])) << 8) |
                                          ((BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_DATA_2])) << 4) |
                                          BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + BTL_DATA_3]);

        /* Shift the buffer pointer to point to the start of the next record
         * 11 = 2 (CC) + 4 (ADD) + 2 (RT) + 2 (CHSUM) + 1 (\n) */
        currentRecord->BTL_BUFFER_POINTER += (currentRecord->BTL_CC) * 2 + 11;
    }
    /* Set the full address */
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

        /* Shift the buffer pointer to point to the start of the next record
         * 11 = 2 (CC) + 4 (ADD) + 2 (RT) + 2 (CHSUM) + 1 (\n) */
        currentRecord->BTL_BUFFER_POINTER += (currentRecord->BTL_CC) * 2 + 11;
    }
    else
    {
        BTL_STATUS = BTL_ERROR;
    }

    return BTL_STATUS;
}

/**
 * @brief Write hex data to the Flash memory.
 * @param dataBuffer Buffer containing the hex data.
 * @param dataLength Length of the data in the buffer.
 * @param currentRecord Structure containing record information.
 * @return BTL_StatusTypeDef Status of the Flash write operation.
 */
static BTL_StatusTypeDef BTL_FlashWrite(uint8_t* dataBuffer, uint16_t dataLength, BTL_RecordTypeDef* currentRecord)
{
    BTL_StatusTypeDef BTL_STATUS = BTL_ERROR;

    /* Counter to exit the loop in case of too many failures */
    uint8_t FlashFailure = 0;

    currentRecord->BTL_BUFFER_POINTER = 0;

    /* Iterate until all records received from the packet are flashed */
    while (currentRecord->BTL_RECORD_INDEX < currentRecord->BTL_NO_OF_BUFFER_RECORDS)
    {
        if (BTL_HexFlasher(dataBuffer, currentRecord) == BTL_OK)
        {
            currentRecord->BTL_RECORD_INDEX += 1;
            BTL_STATUS = BTL_OK;
        }
        else
        {
            FlashFailure++;
        }

        if ((FlashFailure >= MAX_TIMEOUT)) {
            return BTL_ERROR;
        }
    }

    return BTL_STATUS;
}

/**
 * @brief Check the integrity of a record by verifying its checksum.
 * @param dataBuffer Buffer containing the record data.
 * @param currentRecord Structure containing record information.
 * @return BTL_StatusTypeDef Status of the record integrity check.
 */
static BTL_StatusTypeDef BTL_CheckRecord(uint8_t* dataBuffer, BTL_RecordTypeDef* currentRecord) {
    BTL_StatusTypeDef BTL_STATUS = BTL_ERROR;

    /* Validate the address and the character count */
    if ((currentRecord->BTL_ADD >= BTL_MIN_ADDRESS) && (currentRecord->BTL_ADD <= BTL_MAX_ADDRESS) &&
        (currentRecord->BTL_CC >= BTL_MIN_CC) && (currentRecord->BTL_CC <= BTL_MAX_CC))
    {
        /* Calculate and check the checksum received from the PC */
        uint8_t* CRC_Buffer = NULL;
        CRC_Buffer = (uint8_t*)calloc(currentRecord->BTL_CC + 4, sizeof(uint8_t));

        /* Calculate the checksum using some hard-code lol */
        for (uint8_t bytesCounter = 0; bytesCounter < currentRecord->BTL_CC + 4; bytesCounter++)
        {
            CRC_Buffer[bytesCounter] = (BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + bytesCounter * 2]) << 4) |
                                        BTL_ASCHIIToHex(dataBuffer[currentRecord->BTL_BUFFER_POINTER + bytesCounter * 2 + 1]);
        }

        uint8_t mcuCHECKSUM = CalculateChecksum(CRC_Buffer, currentRecord->BTL_CC + 4);

        /* If the calculated checksum matches the received checksum, the validation is successful */
        if (mcuCHECKSUM == currentRecord->BTL_CHECKSUM) {
            BTL_STATUS = BTL_OK;
        }

        free(CRC_Buffer);
    }

    return BTL_STATUS;
}

/**
 * @brief Calculate checksum for a given data buffer.
 * @param data Pointer to the data buffer.
 * @param length Length of the data buffer.
 * @return uint8_t Calculated checksum value.
 */
static uint8_t CalculateChecksum(const uint8_t *data, size_t length)
{
    uint16_t sum = 0;

    for (size_t i = 0; i < length; i++)
    {
        sum += data[i];
    }

    /* Take 2's complement of the least significant 8 bits */
    return (uint8_t)(~sum + 1);
}

//static BTL_StatusTypeDef BTL_CheckSum(uint8_t dataBuffer, uint16_t datalength);

//BTL_SendMessage("Chip ID: %c%c", ((uint8_t)DBGMCU->IDCODE >> 8), (uint8_t)DBGMCU->IDCODE)

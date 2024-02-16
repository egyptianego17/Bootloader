/*
 * BTL_Interface.h
 *
 *  Created on: Jan 27, 2024
 *      Author: Xavi Omar
 */

#include "BTL_Private.h"


#ifndef INC_BTL_INTERFACE_H_
#define INC_BTL_INTERFACE_H_

#define DATA_BUFFER_SIZE 2048

#define BTL_DATA_SIZE0        0
#define BTL_DATA_SIZE1        1

BTL_StatusTypeDef BTL_SendMessage(char* messageFormat, ...);
BTL_CMDTypeDef BTL_GetMessage(uint8_t* messageBuffer);
BTL_StatusTypeDef BTL_SendAck(BTL_CMDTypeDef cmdID);
BTL_StatusTypeDef BTL_SendNAck(void);
BTL_StatusTypeDef BTL_GetVersion(void);
BTL_StatusTypeDef BTL_UpdateFirmware(uint8_t* messageBuffer, uint16_t dataLength);
uint8_t BTL_ASCHIIToHex(uint8_t ASCHIIValue);
BTL_StatusTypeDef BTL_HexFlasher(uint8_t* dataBuffer, BTL_RecordTypeDef* currentRecord);
BTL_StatusTypeDef BTL_FlashWrite(uint8_t* dataBuffer, uint16_t datalength, BTL_RecordTypeDef* currentRecord);
BTL_StatusTypeDef BTL_CheckRecord(uint8_t* dataBuffer ,BTL_RecordTypeDef* currentRecord);
uint8_t CalculateChecksum(const uint8_t *data, size_t length);

#endif /* INC_BTL_INTERFACE_H_ */

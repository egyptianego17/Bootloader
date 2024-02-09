/*
 * BTL_Interface.h
 *
 *  Created on: Jan 27, 2024
 *      Author: Xavi Omar
 */

#include "BTL_Private.h"


#ifndef INC_BTL_INTERFACE_H_
#define INC_BTL_INTERFACE_H_

#define DATA_BUFFER_SIZE 1024

#define BTL_DATA_SIZE0        0
#define BTL_DATA_SIZE1        1

BTL_StatusTypeDef BTL_SendMessage(char* messageFormat, ...);
BTL_CMDTypeDef BTL_GetMessage(uint8_t* messageBuffer);
BTL_StatusTypeDef BTL_GetVersion();
BTL_StatusTypeDef BTL_UpdateFirmware(uint8_t* messageBuffer, uint16_t dataLength);

#endif /* INC_BTL_INTERFACE_H_ */

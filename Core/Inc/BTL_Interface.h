/*****************************************************/
/*                 SWC: Bootloader                   */
/*            Author: Abdulrahman Omar               */
/*                 Version: v 1.0                    */
/*              Date: 27 Jan - 2024                  */
/*****************************************************/

#include "BTL_Private.h"


#ifndef INC_BTL_INTERFACE_H_
#define INC_BTL_INTERFACE_H_

#define DATA_BUFFER_SIZE 2048

#define BTL_DATA_SIZE0        0
#define BTL_DATA_SIZE1        1

BTL_StatusTypeDef BTL_SendMessage(char* messageFormat, ...);
BTL_CMDTypeDef BTL_GetMessage(uint8_t* messageBuffer);
BTL_StatusTypeDef BTL_GetVersion(void);
BTL_StatusTypeDef BTL_UpdateFirmware(uint8_t* messageBuffer, uint16_t dataLength);
static uint8_t CalculateChecksum(const uint8_t *data, size_t length);

#endif /* INC_BTL_INTERFACE_H_ */

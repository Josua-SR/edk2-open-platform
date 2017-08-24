/**
*
*  Copyright (c) 2018, Marvell International Ltd. All rights reserved.
*
*  This program and the accompanying materials are licensed and made available
*  under the terms and conditions of the BSD License which accompanies this
*  distribution. The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/
#ifndef __MV_PCA953X_H__
#define __MV_PCA953X_H__

#include <Library/ArmadaBoardDescLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/BoardDesc.h>
#include <Protocol/MvGpio.h>
#include <Protocol/MvI2c.h>

#include <Uefi/UefiBaseType.h>

#define PCA95XX_GPIO_SIGNATURE   SIGNATURE_32 ('I', 'O', 'E', 'X')

#ifndef BIT
#define BIT(nr)                  (1 << (nr))
#endif

#define PCA95XX_INPUT_REG        0x0
#define PCA95XX_OUTPUT_REG       0x2
#define PCA95XX_DIRECTION_REG    0x6

#define PCA95XX_BANK_SIZE        8
#define PCA95XX_OPERATION_COUNT  2
#define PCA95XX_OPERATION_LENGTH 1

typedef enum {
  PCA9505_PIN_COUNT = 40,
  PCA9534_PIN_COUNT = 8,
  PCA9535_PIN_COUNT = 16,
  PCA9536_PIN_COUNT = 4,
  PCA9537_PIN_COUNT = 4,
  PCA9538_PIN_COUNT = 8,
  PCA9539_PIN_COUNT = 16,
  PCA9554_PIN_COUNT = 8,
  PCA9555_PIN_COUNT = 16,
  PCA9556_PIN_COUNT = 16,
  PCA9557_PIN_COUNT = 16,
} PCA95XX_PIN_COUNT;

typedef enum {
  PCA95XX_READ,
  PCA95XX_WRITE,
} PCA95XX_OPERATION;

typedef struct {
  MARVELL_GPIO_PROTOCOL   GpioProtocol;
  MV_I2C_IO_EXPANDER_DESC *ControllerDesc;
  UINTN                   ControllerCount;
  UINTN                   Signature;
  EFI_HANDLE              ControllerHandle;
} PCA95XX;

#endif

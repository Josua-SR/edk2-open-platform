/**
*
*  Copyright (C) 2018, Marvell International Ltd. and its affiliates.
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
#ifndef __MV_GPIO_PROTOCOL_H__
#define __MV_GPIO_PROTOCOL_H__

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/EmbeddedGpio.h>

typedef enum {
  MV_GPIO_DRIVER_TYPE_SOC_CONTROLLER,
} MV_GPIO_DRIVER_TYPE;

typedef enum {
  MV_GPIO_AP806_CONTROLLER0,
  MV_GPIO_CP0_CONTROLLER0,
  MV_GPIO_CP0_CONTROLLER1,
  MV_GPIO_CP1_CONTROLLER0,
  MV_GPIO_CP1_CONTROLLER1,
  MV_GPIO_CP2_CONTROLLER0,
  MV_GPIO_CP2_CONTROLLER1,
} MV_GPIO_SOC_CONTROLLER_TYPE;

typedef struct {
  VENDOR_DEVICE_PATH        Header;
  MV_GPIO_DRIVER_TYPE       GpioDriverType;
  EFI_DEVICE_PATH_PROTOCOL  End;
} MV_GPIO_DEVICE_PATH;

typedef struct {
  UINTN    ControllerId;
  UINTN    PinNumber;
  BOOLEAN  ActiveHigh;
} MV_GPIO_PIN;

/*
 * Check if the driver type matches the requested value.
 * In case of the success open the GPIO protocol and return.
 */
STATIC
inline
EFI_STATUS
EFIAPI
MvGpioFindMatchingDriver (
  IN     MV_GPIO_DRIVER_TYPE    GpioDriverType,
  IN     EFI_HANDLE             HandleBuffer,
  IN     EFI_DEVICE_PATH       *DevicePath,
  IN OUT EMBEDDED_GPIO        **GpioProtocol
  )
{
  MV_GPIO_DEVICE_PATH *GpioDevicePath;
  EFI_STATUS           Status;

  while (!IsDevicePathEndType (DevicePath)) {
    /* Check if GpioDriverType matches one found in the device path */
    GpioDevicePath = (MV_GPIO_DEVICE_PATH *)DevicePath;
    if (GpioDevicePath->GpioDriverType != GpioDriverType) {
      DevicePath = NextDevicePathNode (DevicePath);
      continue;
    }

    /*
     * Open GpioProtocol. With EFI_OPEN_PROTOCOL_GET_PROTOCOL attribute
     * the consumer is not obliged to call CloseProtocol.
     */
    Status = gBS->OpenProtocol (HandleBuffer,
                    &gEmbeddedGpioProtocolGuid,
                    (VOID **)GpioProtocol,
                    gImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    return Status;
  }

  return EFI_NOT_FOUND;
}

/*
 * Select desired protocol producer upon MV_GPIO_DRIVER_TYPE
 * field of driver's MV_GPIO_DEVICE_PATH.
 */
STATIC
inline
EFI_STATUS
EFIAPI
MvGpioGetProtocol (
  IN     MV_GPIO_DRIVER_TYPE    GpioDriverType,
  IN OUT EMBEDDED_GPIO        **GpioProtocol
  )
{
  EFI_DEVICE_PATH     *DevicePath;
  EFI_HANDLE          *HandleBuffer;
  EFI_STATUS           Status;
  UINTN                HandleCount;
  UINTN                Index;

  /* Locate Handles of all EMBEDDED_GPIO producers */
  Status = gBS->LocateHandleBuffer (ByProtocol,
                  &gEmbeddedGpioProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Unable to locate handles\n", __FUNCTION__));
    return Status;
  }

  /* Iterate over all protocol producers */
  for (Index = 0; Index < HandleCount; Index++) {
    /* Open device path protocol installed on each handle */
    Status = gBS->OpenProtocol (HandleBuffer[Index],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&DevicePath,
                    gImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Unable to find DevicePath\n", __FUNCTION__));
      continue;
    }

    /*
     * Check if the driver type matches the requested value.
     * In case of the success open the GPIO protocol and return.
     * If there is no match, repeat procedure with the next handle.
     */
    Status = MvGpioFindMatchingDriver (GpioDriverType,
               HandleBuffer[Index],
               DevicePath,
               GpioProtocol);
    if (Status == EFI_NOT_FOUND) {
      continue;
    } else if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR,
        "%a: Unable to open GPIO protocol\n",
        __FUNCTION__));
    }

    gBS->FreePool (HandleBuffer);

    return Status;
  }

  /* No matching GPIO protocol producer was found */
  gBS->FreePool (HandleBuffer);

  return EFI_NOT_FOUND;
}

#endif // __MV_GPIO_PROTOCOL_H__

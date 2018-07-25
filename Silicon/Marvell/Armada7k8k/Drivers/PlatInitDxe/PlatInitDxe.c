/** @file
  Copyright (c) 2017, Linaro Limited. All rights reserved.
  Copyright (c) 2017, Marvell International Ltd. and its affiliates

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Guid/EventGroup.h>

#include <IndustryStandard/MvSmc.h>

#include <Library/ArmadaIcuLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/DebugLib.h>
#include <Library/MppLib.h>
#include <Library/MvComPhyLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UtmiPhyLib.h>

STATIC EFI_EVENT mArmada7k8kExitBootServicesEvent;

/**
  Disable extra EL3 handling of the PMU interrupts for DT world.

  @param[in]   Event                  Event structure
  @param[in]   Context                Notification context

**/
STATIC
VOID
Armada7k8kExitBootServicesHandler (
  IN EFI_EVENT  Event,
  IN VOID      *Context
  )
{
  ARM_SMC_ARGS SmcRegs = {0};

  SmcRegs.Arg0 = MV_SMC_ID_PMU_IRQ_DISABLE;
  ArmCallSmc (&SmcRegs);

  return;
}

/**
  Check if we boot with DT and trigger EBS event in such case.

  @param[in]   Event                  Event structure
  @param[in]   Context                Notification context

**/
STATIC
VOID
Armada7k8kOnReadyToBootHandler (
  IN EFI_EVENT  Event,
  IN VOID      *Context
  )
{
  EFI_STATUS    Status;
  VOID         *Interface;

  Status = gBS->LocateProtocol (&gEdkiiPlatformHasAcpiGuid,
                  NULL,
                  (VOID **)&Interface);
  if (EFI_ERROR (Status)) {
    Status = gBS->CreateEvent (EVT_SIGNAL_EXIT_BOOT_SERVICES,
                    TPL_NOTIFY,
                    Armada7k8kExitBootServicesHandler,
                    NULL,
                    &mArmada7k8kExitBootServicesEvent);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Fail to register EBS event\n", __FUNCTION__));
    }
  }
}

EFI_STATUS
EFIAPI
ArmadaPlatInitDxeEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  ARM_SMC_ARGS  SmcRegs = {0};
  EFI_STATUS    Status;
  EFI_EVENT     Event;

  DEBUG ((DEBUG_ERROR, "\nArmada Platform Init\n\n"));

  Status = gBS->InstallProtocolInterface (&ImageHandle,
                  &gMarvellPlatformInitCompleteProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL);
  ASSERT_EFI_ERROR (Status);

  MvComPhyInit ();
  UtmiPhyInit ();
  MppInitialize ();
  ArmadaIcuInitialize ();

  /*
   * Enable EL3 PMU interrupt handler and
   * register the ReadyToBoot event.
   */
  SmcRegs.Arg0 = MV_SMC_ID_PMU_IRQ_ENABLE;
  ArmCallSmc (&SmcRegs);

  Status = gBS->CreateEventEx (EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  Armada7k8kOnReadyToBootHandler,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &Event);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
      "%a: Fail to register OnReadyToBoot event\n",
      __FUNCTION__));
  }

  return EFI_SUCCESS;
}

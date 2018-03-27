/*******************************************************************************
Copyright (C) 2018 Marvell International Ltd.

Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

* Neither the name of Marvell nor the names of its contributors may be
  used to endorse or promote products derived from this software without
  specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/
#include "MvBoardDescDxe.h"

MV_BOARD_DESC *mBoardDescInstance;

STATIC
EFI_STATUS
MvBoardDescPp2Get (
  IN MARVELL_BOARD_DESC_PROTOCOL  *This,
  IN OUT MV_BOARD_PP2_DESC       **Pp2Desc
  )
{
  UINT8 *Pp2DeviceTable, Pp2Count;
  UINTN Pp2DeviceTableSize, Pp2Index, Index;
  MV_BOARD_PP2_DESC *BoardDesc;
  MV_SOC_PP2_DESC *SoCDesc;
  EFI_STATUS Status;

  /* Get SoC data about all available PP2 controllers */
  Status = ArmadaSoCDescPp2Get (&SoCDesc, &Pp2Count);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  /* Obtain table with enabled PP2 NICs */
  Pp2DeviceTable = (UINT8 *)PcdGetPtr (PcdPp2Controllers);
  if (Pp2DeviceTable == NULL) {
    /* No PP2 NIC on platform */
    return EFI_SUCCESS;
  }

  Pp2DeviceTableSize = PcdGetSize (PcdPp2Controllers);

  /* Check if PCD with PP2 NICs is correctly defined */
  if (Pp2DeviceTableSize > Pp2Count) {
    DEBUG ((DEBUG_ERROR, "%a: Wrong PcdPp2Controllers format\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  /* Allocate and fill board description */
  BoardDesc = AllocateZeroPool (Pp2DeviceTableSize * sizeof (MV_BOARD_PP2_DESC));
  if (BoardDesc == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot allocate memory\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  Pp2Index = 0;
  for (Index = 0; Index < Pp2DeviceTableSize; Index++) {
    if (!MVHW_DEV_ENABLED (Pp2, Index)) {
      DEBUG ((DEBUG_ERROR, "%a: Skip Pp2 controller %d\n", __FUNCTION__, Index));
      continue;
    }

    BoardDesc[Pp2Index].SoC = &SoCDesc[Index];
    Pp2Index++;
  }

  BoardDesc->Pp2DevCount = Pp2Index;

  *Pp2Desc = BoardDesc;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
MvBoardDescUtmiGet (
  IN MARVELL_BOARD_DESC_PROTOCOL  *This,
  IN OUT MV_BOARD_UTMI_DESC      **UtmiDesc
  )
{
  UINT8 *UtmiDeviceTable, *XhciDeviceTable, *UtmiPortType, UtmiCount;
  UINTN UtmiDeviceTableSize, UtmiIndex, Index;
  MV_BOARD_UTMI_DESC *BoardDesc;
  MV_SOC_UTMI_DESC *SoCDesc;
  EFI_STATUS Status;

  /* Get SoC data about all available UTMI controllers */
  Status = ArmadaSoCDescUtmiGet (&SoCDesc, &UtmiCount);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  /* Obtain table with enabled Utmi PHY's */
  UtmiDeviceTable = (UINT8 *)PcdGetPtr (PcdUtmiControllersEnabled);
  if (UtmiDeviceTable == NULL) {
    /* No UTMI PHY on platform */
    return EFI_SUCCESS;
  }

  /* Make sure XHCI controllers table is present */
  XhciDeviceTable = (UINT8 *)PcdGetPtr (PcdPciEXhci);
  if (XhciDeviceTable == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Missing PcdPciEXhci\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  UtmiDeviceTableSize = PcdGetSize (PcdUtmiControllersEnabled);

  /* Check if PCD with UTMI PHYs is correctly defined */
  if (UtmiDeviceTableSize > UtmiCount ||
      UtmiDeviceTableSize > PcdGetSize (PcdPciEXhci)) {
    DEBUG ((DEBUG_ERROR,
      "%a: Wrong PcdUtmiControllersEnabled format\n",
      __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  /* Obtain port type table */
  UtmiPortType = (UINT8 *)PcdGetPtr (PcdUtmiPortType);
  if (UtmiPortType == NULL ||
      PcdGetSize (PcdUtmiPortType) != UtmiDeviceTableSize) {
    DEBUG ((DEBUG_ERROR, "%a: Wrong PcdUtmiPortType format\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  /* Allocate and fill board description */
  BoardDesc = AllocateZeroPool (UtmiDeviceTableSize * sizeof (MV_BOARD_UTMI_DESC));
  if (BoardDesc == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot allocate memory\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  UtmiIndex = 0;
  for (Index = 0; Index < UtmiDeviceTableSize; Index++) {
    if (!MVHW_DEV_ENABLED (Utmi, Index)) {
      continue;
    }

    /* UTMI PHY without enabled XHCI controller is useless */
    if (!MVHW_DEV_ENABLED (Xhci, Index)) {
      DEBUG ((DEBUG_ERROR,
             "%a: Disabled Xhci controller %d\n",
             Index,
             __FUNCTION__));
      return EFI_INVALID_PARAMETER;
    }

    BoardDesc[UtmiIndex].SoC = &SoCDesc[Index];
    BoardDesc[UtmiIndex].UtmiPortType = UtmiPortType[Index];
    UtmiIndex++;
  }

  BoardDesc->UtmiDevCount = UtmiIndex;

  *UtmiDesc = BoardDesc;

  return EFI_SUCCESS;
}

STATIC
VOID
MvBoardDescFree (
  IN VOID *BoardDesc
  )
{
  if (BoardDesc != NULL) {
    FreePool (BoardDesc);
  }
}

STATIC
EFI_STATUS
MvBoardDescInitProtocol (
  IN MARVELL_BOARD_DESC_PROTOCOL *BoardDescProtocol
  )
{
  BoardDescProtocol->BoardDescPp2Get = MvBoardDescPp2Get;
  BoardDescProtocol->BoardDescUtmiGet = MvBoardDescUtmiGet;
  BoardDescProtocol->BoardDescFree = MvBoardDescFree;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MvBoardDescEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;

  mBoardDescInstance = AllocateZeroPool (sizeof (MV_BOARD_DESC));
  if (mBoardDescInstance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  MvBoardDescInitProtocol (&mBoardDescInstance->BoardDescProtocol);

  mBoardDescInstance->Signature = BOARD_DESC_SIGNATURE;

  Status = gBS->InstallMultipleProtocolInterfaces (&(mBoardDescInstance->Handle),
                  &gMarvellBoardDescProtocolGuid,
                  &(mBoardDescInstance->BoardDescProtocol));
  if (EFI_ERROR (Status)) {
    FreePool (mBoardDescInstance);
    return Status;
  }

  return EFI_SUCCESS;
}

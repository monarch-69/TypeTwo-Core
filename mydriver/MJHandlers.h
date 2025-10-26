#pragma once
#include <ntddk.h>
#include <wdf.h>
#include <wdm.h>

NTSTATUS DrvUnsupported(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS DrvCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS DrvClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS DrvRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS DrvWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS DrvIoctlDispatcher(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);
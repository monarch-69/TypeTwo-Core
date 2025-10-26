#include <ntddk.h>
#include <wdf.h>
#include <wdm.h>
#include "mydriver/MJHandlers.h"
#include "mydriver/Source.h"

extern "C" {
    NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
    VOID DrvUnload(PDRIVER_OBJECT DriverObject);
}

// DriverEntry is an entry point for the driver and will act as the parent or root for all the refered objects
// in this driver such as spinlocks, queues, etc
//DRIVER_INITIALIZE DriverEntry;

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DrvUnload)

NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    
    NTSTATUS       NtStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT DeviceObject = NULL;
    UNICODE_STRING DriverName, DosDeviceName;

    DbgPrint("DriverEntry Called.");

    RtlInitUnicodeString(&DriverName, L"\\Device\\mydriver");
    RtlInitUnicodeString(&DosDeviceName, L"\\DosDevices\\mydriver");

    NtStatus = IoCreateDevice(DriverObject, 0, &DriverName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &DeviceObject);

    if (NtStatus == STATUS_SUCCESS)
    {
        // Initial initialization of the table with DrvUnsupported function
        for (ULONG index = 0; index < IRP_MJ_MAXIMUM_FUNCTION; index++)
            DriverObject->MajorFunction[index] = DrvUnsupported;
        
        DbgPrint("[*] Setting up the MJ functions...");

        DriverObject->MajorFunction[IRP_MJ_CREATE] = DrvCreate;
        DriverObject->MajorFunction[IRP_MJ_CLOSE] = DrvClose;
        DriverObject->MajorFunction[IRP_MJ_READ] = DrvRead;
        DriverObject->MajorFunction[IRP_MJ_WRITE] = DrvWrite;
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DrvIoctlDispatcher;

        DriverObject->DriverUnload = DrvUnload;
        
        DeviceObject->Flags |= IO_TYPE_DEVICE;
        DeviceObject->Flags &= (~DO_DEVICE_INITIALIZING);
        IoCreateSymbolicLink(&DosDeviceName, &DriverName);
        DbgPrint("Created symbolic link");
    }
    return NtStatus;
}

VOID DrvUnload(
    PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING DosDeviceName;
    
    DbgPrint("Unloading driver...");
    RtlInitUnicodeString(&DosDeviceName, L"\\DosDevices\\mydriver");
    IoDeleteSymbolicLink(&DosDeviceName); // delete the symboliclink created
    IoDeleteDevice(DriverObject->DeviceObject); // delete the device object
}
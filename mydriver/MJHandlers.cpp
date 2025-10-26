#include "MJHandlers.h"
#include "Source.h"
#include "Helpers.h"
#include "Memory.h"

// An example IOCTL Code
#define IOCTL_METHOD_BUFFERED \
        CTL_CODE(FILE_DEVICE_UNKNOWN, \
                0x800, \
                METHOD_BUFFERED, \
                FILE_ANY_ACCESS)

#define IOCTL_METHOD_NEITHER \
        CTL_CODE(FILE_DEVICE_UNKNOWN, \
                0x801, \
                METHOD_NEITHER, \
                FILE_ANY_ACCESS)

#define IOCTL_METHOD_IN_DIRECT \
        CTL_CODE(FILE_DEVICE_UNKNOWN, \
                0x802, \
                METHOD_IN_DIRECT, \
                FILE_ANY_ACCESS)

#define IOCTL_METHOD_OUT_DIRECT \
        CTL_CODE(FILE_DEVICE_UNKNOWN, \
                0x803, \
                METHOD_OUT_DIRECT, \
                FILE_ANY_ACCESS)

NTSTATUS DrvUnsupported(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    DbgPrint("[*] Not supported yet :( !");
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS DrvCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    DbgPrint("Entered DrvCreate....\n");
    UNREFERENCED_PARAMETER(DeviceObject);
    
    if (Vm_Run())
        DbgPrint("VMRUN excecuted successfully\n");
    else
        DbgPrint("VMRUN DIDN'T excecute successfully\n");

    //DbgPrint("Skipped the VMRUN....\n");

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS DrvRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    DbgPrint("[*] Not implemented yet :( !");
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS DrvWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    DbgPrint("[*] Not implemented yet :( !");
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS DrvClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    DbgPrint("[*] Entered DrvClose...\n");

    DbgPrint("Executing VMSTOP...\n");
    Vm_Stop();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS DrvIoctlDispatcher(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    DbgPrint("DeviceIoctlDispatcher called ...\n");

    NTSTATUS NtStatus = STATUS_SUCCESS;
    // We want the stack location of the Irp
    PIO_STACK_LOCATION IrpStackLocation = NULL;
    IrpStackLocation = IoGetCurrentIrpStackLocation(Irp);
    ULONG DataWritten = 0;


    // Foremost step to check whether the Irp Stack location in not NULL
    if (!IrpStackLocation)
    {
        DbgPrint("IprStackLocation is NULL\n");
        NtStatus = STATUS_INVALID_PARAMETER;
    }
    else
    {
        // Lets print info about the Irp and IrpStackLocation
        // We just need to pass the Irp and IrpStackLocation will be created in this function
        PrintIrpInfo(Irp);
        // it comes from Irp stack location
        switch (IrpStackLocation->Parameters.DeviceIoControl.IoControlCode)
        {
            case IOCTL_METHOD_BUFFERED:
                DbgPrint("In the METHOD_BUFFERED switch case...\n");
                NtStatus = IoctlMethodBufferedWriteHandle(&DataWritten, Irp, IrpStackLocation);
                break;
            
            case IOCTL_METHOD_NEITHER:
                break;

            case IOCTL_METHOD_IN_DIRECT:
                break;

            case IOCTL_METHOD_OUT_DIRECT:
                break;

            default: 
                break;
        }
    }
   
    Irp->IoStatus.Status = NtStatus;
    Irp->IoStatus.Information = DataWritten;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return NtStatus;
}
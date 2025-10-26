#include <ntddk.h>
#include <wdm.h>
#include <intrin.h>
#include "Helpers.h"


int MathPower(
	int Base, 
	size_t Exponent)
{
	int Result = 1;
	
	while (true)
	{
		if (Exponent & 1)
			Result *= Base;
		Exponent >>= 1;
		if (!Exponent)
			break;
		Base *= Base;
	}

	return Result;
}

ULONG64 VirtualToPhysicalAddress(
	void *VirtualAddress)
{
	return MmGetPhysicalAddress(VirtualAddress).QuadPart;
}

ULONG64 PhysicalToVirtualAddress(
	ULONG64 PhysicalAddress)
{
	PHYSICAL_ADDRESS PhysicalAddVar;
	PhysicalAddVar.QuadPart = PhysicalAddress;

	return (ULONG64)MmGetVirtualForPhysical(PhysicalAddVar);
}

VOID PrintIrpInfo(
	IN PIRP Irp)

{
	PIO_STACK_LOCATION IrpStackLocation = IoGetCurrentIrpStackLocation(Irp);

	PAGED_CODE();

	// Now print out all the Irp Info
	// print out the location of system buffer address
	DbgPrint("\nIrp->AssociatedIrp.SystemBuffer: 0x%p",
		Irp->AssociatedIrp.SystemBuffer);

	// print out the location of user buffer address
	DbgPrint("\nIrp->UserBuffer: 0x%p",
		Irp->UserBuffer);

	// print out the location of Type 3 input buffer
	// this comes from the Irp stack location
	DbgPrint("\nIrpStackLocation->Parameters.DeviceIoControl.Type3InputBuffer: 0x%p",
		IrpStackLocation->Parameters.DeviceIoControl.Type3InputBuffer);

	// now print the input and output buffer lengths from the stack location
	DbgPrint("\nIrpStackLocation->Parameters.DeviceIoControl.InputBufferLength: %d",
		IrpStackLocation->Parameters.DeviceIoControl.InputBufferLength);
	DbgPrint("\nIrpStackLocation->Parameters.DeviceIoControl.OutputBufferLength: %d",
		IrpStackLocation->Parameters.DeviceIoControl.OutputBufferLength);
}

bool IsStringNullTerminated(
	PCHAR InputBuffer,
	ULONG InputBufferLen)
{
	if ( *(InputBuffer + (InputBufferLen - 1) ) != '\0')
		return false;

	return true;
}

NTSTATUS IoctlMethodBufferedWriteHandle(
	ULONG* DataWritten,
	PIRP Irp,
	PIO_STACK_LOCATION IrpStackLocation)
{
	NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
	ULONG InputBufferSize = IrpStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	ULONG OutputBufferSize = IrpStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
	PCHAR DataToWrite = "Hello from the kernel! I hope you get the message";
	ULONG DataToWriteLen = (ULONG)strlen("Hello from the kernel! I hope you get the message");
	PCHAR InputBuffer = (PCHAR)Irp->AssociatedIrp.SystemBuffer;
	PCHAR OutputBuffer = (PCHAR)Irp->AssociatedIrp.SystemBuffer;

	DbgPrint("METHOD_BUFFERED Write handler called...\n");

	// Always check for validity of the input and output buffer
	// conditions to check for the different methods differ
	if (InputBuffer && OutputBuffer)
	{
		// Let's print the user's message
		// But first check whether the string is NULL terminated or not.
		if (!IsStringNullTerminated(InputBuffer, InputBufferSize))
		{
			DbgPrint("The input data is not NULL terminated!");
			return STATUS_INVALID_PARAMETER;
		}

		// Let's try printing the User message
		DbgPrint("User's Message: %s\n", InputBuffer);

		// Let's check for the output buffer length with the lenght of the data we want to write from our kernel
		if (OutputBufferSize < DataToWriteLen)
		{
			DbgPrint("Output buffer size is less than the size of the message kernel wants to write!");
			*DataWritten = 0;
			NtStatus = STATUS_BUFFER_TOO_SMALL;
		}
		else
		{
			// Everything seems fine...let's write to the ouput buffer our kernel's message
			DbgPrint("\nKernel writing it's response...");
			if (OutputBuffer)
			{
				RtlCopyMemory(OutputBuffer, DataToWrite, DataToWriteLen);
				*DataWritten = DataToWriteLen;
				NtStatus = STATUS_SUCCESS;
			}
			else
			{
				DbgPrint("Ouput Buffer is NULL\n");
				NtStatus = STATUS_INVALID_PARAMETER;
			}
		}
	}

	if (NtStatus == STATUS_UNSUCCESSFUL)
		DbgPrint("The problem is in the input or output buffer address!");

	return NtStatus;
}


BOOLEAN IsSVMSupported()
{
	// Get the data in the CPUID type variable
	CPUID Cpu = { 0 };
	__cpuid( (int*)&Cpu, 0x80000001 );
	
	// Check whether the CPU supports the Virtualization or not
	if (!(Cpu.ecx & (1ULL << 2)))
	{
		DbgPrint("This system doesn't support Virtualization!\n");
		return FALSE;
	}

	// If above is false then check whether its disabled in BIOS
	ULONG64 VM_CR_REG_CONTENT = __readmsr(MSR_VM_CR_ADDRESS);
	if (VM_CR_REG_CONTENT & (1ULL << 4))
	{
		// If the result is 1 then VM_CR.SVMDIS(able) = 1 i.e SVM is disabled from the BIOS
		DbgPrint("Virtualization is disabled in the BIOS!\n");
		return FALSE;
	}

	// IF above is false then let's check whether the SVM is enabled or not
	MSR_EFER EFER_REG = { 0 };
	EFER_REG.All = __readmsr(MSR_EFER_ADDRESS);
	if (!EFER_REG.Fields.SVME)
	{
		DbgPrint("SVM bit is not set...\nSetting it...\n");
		EFER_REG.Fields.SVME = TRUE;
		// __writemsr(ADDRESS_OF_REGISTER, REGISTER_CONTENT);
		__writemsr(MSR_EFER_ADDRESS, EFER_REG.All);
	}
	else
		DbgPrint("SVM bit is already set...\n");
	
	return TRUE;
}
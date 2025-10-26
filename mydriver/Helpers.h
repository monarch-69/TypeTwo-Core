#pragma once

constexpr ULONG64 MSR_EFER_ADDRESS = 0xC0000080;
constexpr ULONG64 MSR_VM_CR_ADDRESS = 0xC0010114;
constexpr ULONG64 MSR_VM_HSAVE_PA = 0xC0010117;

typedef union _MSR_EFER
{
	ULONG64 All;

	struct
	{
		ULONG64 SCE : 1;   // [0] SYSCALL Enable
		ULONG64 Reserved1 : 7; // [1–7] Reserved
		ULONG64 LME : 1;   // [8] Long Mode Enable
		ULONG64 Reserved2 : 1; // [9]
		ULONG64 LMA : 1;   // [10] Long Mode Active (read-only)
		ULONG64 NXE : 1;   // [11] No-Execute Enable
		ULONG64 SVME : 1;   // [12] Secure Virtual Machine Enable
		ULONG64 LMSLE : 1;   // [13] Long Mode Segment Limit Enable
		ULONG64 FFXSR : 1;   // [14] Fast FXSAVE/FXRSTOR
		ULONG64 TCE : 1;   // [15] Translation Cache Extension
		ULONG64 Reserved3 : 48; // [16–63]
	} Fields;
} MSR_EFER, *PMSR_EFER;

typedef struct _CPUID
{
	int eax;
	int ebx;
	int ecx;
	int edx;
} CPUID, *PCPUID;

typedef struct _VIRTUAL_MACHINE_STATE
{
	ULONG64 VMMStack;
	ULONG64 MsrBitMap;
	// Raw or Unanligned pointer to the Contiguous memory
	PVOID VmcbRawVirtualAddress;
	// Pointer to Contiguous Physical Memory
	ULONG64 VmcbPointer;
	PVOID HostStateSaveAreaRawVirtualAddress;
	ULONG64 HostStateSaveArea;
	// The below memory would be used for guest RIP and RSP (stack)
	PUCHAR GuestMemory;
} VIRTUAL_MACHINE_STATE, *PVIRTUAL_MACHINE_STATE;

ULONG64 VirtualToPhysicalAddress(
	void* VirtualAddress);

ULONG64 PhysicalToVirtualAddress(
	ULONG64 PhysicalAddress);

int MathPower(int Base, size_t Exponent);

VOID PrintIrpInfo(IN PIRP Irp);

NTSTATUS IoctlMethodBufferedWriteHandle(
	ULONG* DataWritten,
	PIRP Irp,
	PIO_STACK_LOCATION IrpStackLocation);

bool IsStringNullTerminated(
	PCHAR InputBuffer,
	ULONG StringLen);

BOOLEAN IsSVMSupported();
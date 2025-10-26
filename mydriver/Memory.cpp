//#include <wdm.h>
#include <ntddk.h>
#include <wdm.h>
#include <intrin.h>
#include "Memory.h"
#include "TableEntries.h"
#include "Helpers.h"
#include "Source.h"

//#define VMCB_SIZE 4096;
#define VMCB_SIZE 0x1000;

constexpr auto NCR3_TAG = 'NCR3';
constexpr auto PML4E_TAG = 'PL4E';
constexpr auto PDPE_TAG = 'PDPE';
constexpr auto PDE_TAG = 'PDET'; // T means tag
constexpr auto PTE_TAG = 'PTET'; // T means tag
constexpr auto GUESTMEM_TAG = 'GMEM';
constexpr auto VMM_STACK_VA_TAG = 'VMST';

extern "C" uint64_t g_StackPointerForReturning = 0;
extern "C" uint64_t g_BasePointerForReturning = 0;

PVIRTUAL_MACHINE_STATE g_GuestState = NULL;
ULONG64 Proccessor_Count = 0;

BOOLEAN CreateStackForVMMExitHandler(size_t ProcessorId)
{
	PUCHAR VMMStack_VA = (PUCHAR)ExAllocatePool2(POOL_FLAG_NON_PAGED, PAGE_SIZE, VMM_STACK_VA_TAG);
	DbgPrint("Creating stack for VM exit handler...\n");
	if (!VMMStack_VA)
	{
		DbgPrint("VMMStack_VA is NULL\n");
		return FALSE;
	}
	g_GuestState[ProcessorId].VMMStack = (ULONG64)VMMStack_VA;
	DbgPrint("VMM stack is at %llp\n", VMMStack_VA);
	RtlZeroMemory((PVOID)g_GuestState[ProcessorId].VMMStack, PAGE_SIZE);

	return TRUE;
}

ULONG64 GetPML4TableBaseAddress()
{
	PAGED_CODE();

	// First initalize PML4 table and keep its address in the nCR3 part of VMCB
	PNPT_PML4E NptPml4 = (PNPT_PML4E)ExAllocatePool2(POOL_FLAG_NON_PAGED, PAGE_SIZE, PML4E_TAG);
	if (!NptPml4)
	{
		DbgPrint("NptPml4 is NULL\n");
		return NULL;
	}
	RtlZeroMemory(NptPml4, PAGE_SIZE);

	// Initalize PDPT table and keep its address in the nCR3 part of VMCB

	PNPT_PDPE NptPdpe = (PNPT_PDPE)ExAllocatePool2(POOL_FLAG_NON_PAGED, PAGE_SIZE, PDPE_TAG);
	if (!NptPdpe)
	{
		DbgPrint("NptPdpe is NULL\n");
		ExFreePool2((PVOID)NptPml4, PML4E_TAG, 0, 0);
		return NULL;
	}
	RtlZeroMemory(NptPdpe, PAGE_SIZE);

	PNPT_PDE NptPde = (PNPT_PDE)ExAllocatePool2(POOL_FLAG_NON_PAGED, PAGE_SIZE, PDE_TAG);
	if (!NptPde)
	{
		DbgPrint("NptPde is NULL\n");
		ExFreePool2((PVOID)NptPml4, PML4E_TAG, 0, 0);
		ExFreePool2((PVOID)NptPdpe, PDPE_TAG, 0, 0);
		return NULL;
	}
	RtlZeroMemory(NptPde, PAGE_SIZE);

	PNPT_PTE NptPte = (PNPT_PTE)ExAllocatePool2(POOL_FLAG_NON_PAGED, PAGE_SIZE, PTE_TAG);
	if (!NptPte)
	{
		DbgPrint("NptPte is NULL\n");
		ExFreePool2((PVOID)NptPml4, PML4E_TAG, 0, 0);
		ExFreePool2((PVOID)NptPdpe, PDPE_TAG, 0, 0);
		ExFreePool2((PVOID)NptPde, PDE_TAG, 0, 0);
		return NULL;
	}
	RtlZeroMemory(NptPte, PAGE_SIZE);

	// Now allocate 2 pages. Each for RIP and RSP (for stack)
	// But we would allocate 10 pages since guest would pages anyways
	const UINT16 PagesToAllocate = 10;
	PUCHAR GuestMemory = (PUCHAR)ExAllocatePool2(POOL_FLAG_NON_PAGED,
													(ULONG64)PagesToAllocate * PAGE_SIZE,
													GUESTMEM_TAG);
	if (!GuestMemory)
	{
		DbgPrint("GuestMemory is NULL....can't allocate 10 pages for guest memory\n");
		return NULL;
	}
	RtlZeroMemory(GuestMemory, (ULONG64)PagesToAllocate * PAGE_SIZE);
	g_GuestState[0].GuestMemory = GuestMemory;

	// Here we have made entries of 10 pages (their base addresses) in page table
	for (size_t i = 0; i < PagesToAllocate; i++)
	{
		NptPte[i].Fields.P = 1;
		NptPte[i].Fields.RW = 1;
		NptPte[i].Fields.PWT = 0;
		NptPte[i].Fields.PCD = 0;
		NptPte[i].Fields.A = 0;
		NptPte[i].Fields.D = 0;
		NptPte[i].Fields.PAT = 0;
		NptPte[i].Fields.G = 0;
		NptPte[i].Fields.AVL1 = 0;

		PVOID VirtualAddress = GuestMemory + (i * PAGE_SIZE);
		if (!VirtualAddress)
		{
			DbgPrint("VirtualAddress got from GuestMemory is NULL\n");
			return NULL;
		}
		NptPte[i].Fields.PPAddress = VirtualToPhysicalAddress(VirtualAddress) / PAGE_SIZE;
		NptPte[i].Fields.AVL2 = 0;
		NptPte[i].Fields.CR4OPKE = 0;
		NptPte[i].Fields.NX = 0;
	}

	// Now we will put address of page tables (their base addresses)
	// for now just entry of a single page table
	NptPde->Fields.P = 1;
	NptPde->Fields.RW = 1;
	NptPde->Fields.PWT = 1;
	NptPde->Fields.PCD = 1;
	NptPde->Fields.A = 0;
	NptPde->Fields.IGN1 = 0;
	NptPde->Fields.MBZ = 0;
	NptPde->Fields.IGN2 = 0;
	NptPde->Fields.AVL1 = 0;
	NptPde->Fields.PTPAddress = VirtualToPhysicalAddress((PVOID)NptPte) / PAGE_SIZE;
	NptPde->Fields.AVL2 = 0;
	NptPde->Fields.NX = 0;

	// Now we will put address of page directories (their base addresses)
	// for now just entry of a single page directory
	NptPdpe->Fields.P = 1;
	NptPdpe->Fields.RW = 1;
	NptPdpe->Fields.PWT = 1;
	NptPdpe->Fields.PCD = 1;
	NptPdpe->Fields.A = 0;
	NptPdpe->Fields.IGN1 = 0;
	NptPdpe->Fields.MBZ = 0;
	NptPdpe->Fields.IGN2 = 0;
	NptPdpe->Fields.AVL1 = 0;
	NptPdpe->Fields.PDPAddress = VirtualToPhysicalAddress((PVOID)NptPde) / PAGE_SIZE;
	NptPdpe->Fields.AVL2 = 0;
	NptPdpe->Fields.NX = 0;

	// Now we will put address of page directories pointer tables (their base addresses)
	// for now just entry of a single page directory pointer table
	NptPml4->Fields.P = 1;
	NptPml4->Fields.RW = 1;
	NptPml4->Fields.PWT = 1;
	NptPml4->Fields.PCD = 1;
	NptPml4->Fields.A = 0;
	NptPml4->Fields.IGN = 0;
	NptPml4->Fields.MBZ1 = 0;
	NptPml4->Fields.MBZ2 = 0;
	NptPml4->Fields.AVL1 = 0;
	NptPml4->Fields.PDPTPAddress = VirtualToPhysicalAddress((PVOID)NptPdpe) / PAGE_SIZE;
	NptPml4->Fields.AVL2 = 0;
	NptPml4->Fields.NX = 0;

	// Now let's return the PML4 table's base
	return VirtualToPhysicalAddress((PVOID)NptPml4) / PAGE_SIZE;
}

BOOLEAN AllocateVmcbRegion(
	IN PVIRTUAL_MACHINE_STATE Vm_State)
{
	// At first check for the IRQL and if the level is greater than the dispatch level then the memory
	// allocation routines won't work.
	// Set the IRQL to dispatch i.e DISPATCH_LEVEL
	if (KeGetCurrentIrql() > DISPATCH_LEVEL)
		KeRaiseIrqlToDpcLevel();

	PHYSICAL_ADDRESS Py_Address = { 0 }, Highest = { 0 }, Lowest = { 0 };
	Py_Address.QuadPart = MAXULONG64;
	Highest.QuadPart = ~0;

	// We got virtual address
	ULONG64 Vmcb_Size = (ULONG64)2 * VMCB_SIZE;
	PVOID Buffer = MmAllocateContiguousMemory(Vmcb_Size + PAGE_SIZE, Py_Address);

	if (Buffer == NULL)
	{
		DbgPrint("Couldn't allocate buffer for VMCB...\n");
		return FALSE;
	}

	// Let's get the physical address for the vmcb's virtual address
	ULONG64 Buffer_Physical_Address = VirtualToPhysicalAddress(Buffer);

	// Let's initialize the Buffer with 0's
	RtlZeroMemory(Buffer, Vmcb_Size + PAGE_SIZE);

	// Now we will actually align the Buffer w.r.t virtual and physical memory
	ULONG64 AlignedPhysicalBuffer = (Buffer_Physical_Address + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	ULONG64 AlignedVirtualBuffer = (ULONG64)((ULONG_PTR)Buffer + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

	DbgPrint("[*] Virtual allocated buffer for VMCB at %llx\n", (ULONG_PTR)Buffer);
	DbgPrint("[*] Virtual aligned allocated buffer for VMCB at %llx\n", AlignedVirtualBuffer);
	DbgPrint("[*] Aligned physical buffer allocated for VMCB at %llx\n", AlignedPhysicalBuffer);

	Vm_State->VmcbRawVirtualAddress = Buffer;
	Vm_State->VmcbPointer = AlignedPhysicalBuffer;

	return TRUE;
}

BOOLEAN AllocateHostStateSaveArea(IN PVIRTUAL_MACHINE_STATE VmState)
{
	if (KeGetCurrentIrql() > DISPATCH_LEVEL)
		KeRaiseIrqlToDpcLevel();

	PHYSICAL_ADDRESS HighestPossibleAddress = { 0 };
	HighestPossibleAddress.QuadPart = MAXULONG64;

	PVOID Buffer = MmAllocateContiguousMemory(3ULL * PAGE_SIZE, HighestPossibleAddress);
	if (!Buffer)
	{
		DbgPrint("Can't allocate buffer space for Host State Save Area\n");
		return FALSE;
	}
	RtlZeroMemory(Buffer, 3ULL * PAGE_SIZE);

	ULONG64 BufferPA = VirtualToPhysicalAddress(Buffer);
	ULONG64 AlignedBufferPA = (BufferPA + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	ULONG64 AlignedBufferVA = ((ULONG64)Buffer + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

	DbgPrint("[*] Virtual allocated buffer for Host State Save Area at %llx\n", (ULONG_PTR)Buffer);
	DbgPrint("[*] Virtual aligned allocated buffer for Host State Save Area at %llx\n", AlignedBufferVA);
	DbgPrint("[*] Aligned physical buffer allocated for Host State Save Area at %llx\n", AlignedBufferPA);

	VmState->HostStateSaveAreaRawVirtualAddress = Buffer;
	VmState->HostStateSaveArea = AlignedBufferPA;

	return TRUE;
}

BOOLEAN Vm_Run()
{
	if (KeGetCurrentIrql() > DISPATCH_LEVEL)
		KeRaiseIrqlToDpcLevel();
	
	DbgPrint("Executing VMRUN...\n");


	if (IsSVMSupported() == FALSE)
	{
		DbgPrint("SVM is not supported!\n");
		return FALSE;
	}
	DbgPrint("SVM is supported and the control bit might have been enabled too!\n");

	DbgPrint("Getting proccessor count using KeQueryActiveProcessorCountEx...\n");
	Proccessor_Count = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);
	DbgPrint("Processor count = %lld\n", Proccessor_Count);
	

	DbgPrint("Allocating memory space of NonPagedPool with ExAllocatePool2...\n");
	DbgPrint("Size of VIRTUAL_MACHINE_STATE is %lld\n", sizeof(VIRTUAL_MACHINE_STATE));
	g_GuestState = (PVIRTUAL_MACHINE_STATE)ExAllocatePool2(POOL_FLAG_NON_PAGED,
														sizeof(VIRTUAL_MACHINE_STATE) * Proccessor_Count,
														'GTST'); // Guest State
	
	if (g_GuestState == NULL)
	{
		DbgPrint("Can't create buffers for VMCB's!\n");
		return FALSE;
	}
	DbgPrint("g_GuestState pointer is not NULL...proceeding forward\n");
	DbgPrint("=====================================================\n");

	

	KAFFINITY Kaffinity;

	for (size_t i = 0; i < Proccessor_Count; i++)
	{
		Kaffinity = MathPower(2, i);
		KeSetSystemAffinityThread(Kaffinity);

		DbgPrint("Enabling SVM...!\n");
		EnableSVMOperation();
		DbgPrint("SVM enabled successfully!\n");

		AllocateVmcbRegion(&g_GuestState[i]);
		AllocateHostStateSaveArea(&g_GuestState[i]);

		__writemsr(MSR_VM_HSAVE_PA, g_GuestState[i].HostStateSaveArea);

		DbgPrint("[*] VMCB Region is allocated at  ===============> %llx", g_GuestState[i].VmcbPointer);

		DbgPrint("\n=====================================================\n");
	}
	DbgPrint("LaunchVM...\n");
	LaunchVM(0);
	return TRUE;
}

VOID Vm_Stop()
{
	DbgPrint("Stopping the VM...\n");

	MSR_EFER Efer_Msr = { 0 };
	Efer_Msr.All = __readmsr(MSR_EFER_ADDRESS);
	DbgPrint("Setting the SVME bit to 0\n");
	Efer_Msr.Fields.SVME = 0;
	KAFFINITY Kaffinity;

	if (g_GuestState == NULL)
	{
		DbgPrint("g_GuestState is NULL\n");
		return;
	}

	for (size_t i = 0; i < Proccessor_Count; i++)
	{
		Kaffinity = MathPower(2, i);
		KeSetSystemAffinityThread(Kaffinity);
		DbgPrint("\tCurrent thread is executing in %d th logical processor.\n", (int)i);

		__writemsr(MSR_EFER_ADDRESS, Efer_Msr.All);

		if (g_GuestState[i].VmcbRawVirtualAddress == NULL)
		{
			DbgPrint("The VMCB Raw Virtual Address is NULL, can't free the continuous memory");
			continue;
		}

		// Free up the VMCB buffer with the virtual address not the pyshical
		MmFreeContiguousMemory(g_GuestState[i].VmcbRawVirtualAddress);
	}
}

InitializeVmcb InitializeVMCB(PVMCB VMCBPointer, size_t ProcessorId)
{
	UNREFERENCED_PARAMETER(ProcessorId);
	
	DbgPrint("In InitializeVMCB function...\n");
	if (!VMCBPointer)
	{
		DbgPrint("VMCBPointer is NULL\n");
		return InitializeVmcb::NullPointer;
	}

	DbgPrint("Initializing VMCB Control Area...\n");
	RtlFillMemory(&VMCBPointer->RESERVED, 8, 0);
	VMCBPointer->CONTROL_AREA.Intercept_CR_Reads = VMCBPointer->CONTROL_AREA.Intercept_CR_Writes = 0;
	VMCBPointer->CONTROL_AREA.Intercept_DR_Reads = VMCBPointer->CONTROL_AREA.Intercept_DR_Writes = 0;

	// Instruction, Exceptions and Harware Interupts to intercept
	VMCBPointer->CONTROL_AREA.Intercept_Vector_Exceptions = 0; // Exception intecepts
	VMCBPointer->CONTROL_AREA.Intercept_Instructions |= HLT_INSTRUCTION_INTERCEPT; // system instruction interupts
	// for HLT instruction bit no 2nd(i.e 3rd bit) should be 1 -> ...100
	// for SVM instructions bit no 1st(i.e 1st bit) should be 1 -> ...001
	// below would be ...101
	VMCBPointer->CONTROL_AREA.Intercept_SVM_Instructions |= VMMCALL_INSTRUCTION_INTERCEPT;

	// PAUSE filter threshold and count
	VMCBPointer->CONTROL_AREA.Pause_Filter_Threshold = 100;
	VMCBPointer->CONTROL_AREA.Pause_Filter_Count = 0;

	// Guest ASID (Address Space Identifier)
	VMCBPointer->CONTROL_AREA.Guest_ASID = 1;

	// TLB control. Setting it to 0 means don't flush guest's entries automatically
	VMCBPointer->CONTROL_AREA.TLB_Control = 0;

	DbgPrint("Initializing VMCB State Save Area...\n");

	// Let's initalize the RIP, RSP, RFLAGS and segments
	VMCBPointer->STATE_SAVE_AREA.RIP = (ULONG64)g_GuestState[0].GuestMemory;
	VMCBPointer->STATE_SAVE_AREA.RSP = (ULONG64)(g_GuestState[0].GuestMemory + 0x1000);
	VMCBPointer->STATE_SAVE_AREA.RFLAGS = 0x2ULL;
	

	VMCBPointer->STATE_SAVE_AREA.CS.Selector = 0x08;
	VMCBPointer->STATE_SAVE_AREA.CS.Limit = 0xFFFFFFFF;
	VMCBPointer->STATE_SAVE_AREA.CS.Base = 0x0ULL;
	VMCBPointer->STATE_SAVE_AREA.CS.Attribute = 0x209B;

	VMCBPointer->STATE_SAVE_AREA.DS.Selector = 0x10;
	VMCBPointer->STATE_SAVE_AREA.DS.Limit = 0xFFFFFFFF;
	VMCBPointer->STATE_SAVE_AREA.DS.Base = 0x0ULL;
	VMCBPointer->STATE_SAVE_AREA.DS.Attribute = 0x2093;

	RtlCopyMemory(&VMCBPointer->STATE_SAVE_AREA.ES, &VMCBPointer->STATE_SAVE_AREA.DS, sizeof(VMCBPointer->STATE_SAVE_AREA.DS));
	RtlCopyMemory(&VMCBPointer->STATE_SAVE_AREA.SS, &VMCBPointer->STATE_SAVE_AREA.DS, sizeof(VMCBPointer->STATE_SAVE_AREA.DS));
	RtlCopyMemory(&VMCBPointer->STATE_SAVE_AREA.FS, &VMCBPointer->STATE_SAVE_AREA.DS, sizeof(VMCBPointer->STATE_SAVE_AREA.DS));
	RtlCopyMemory(&VMCBPointer->STATE_SAVE_AREA.GS, &VMCBPointer->STATE_SAVE_AREA.DS, sizeof(VMCBPointer->STATE_SAVE_AREA.DS));

	VMCBPointer->STATE_SAVE_AREA.GDTR.Base = VMCBPointer->STATE_SAVE_AREA.GDTR.Limit = 0;
	VMCBPointer->STATE_SAVE_AREA.IDTR.Base = VMCBPointer->STATE_SAVE_AREA.IDTR.Limit = 0;

	// Let's fill in the control registers
	VMCBPointer->STATE_SAVE_AREA.CR0 = __readcr0();
	VMCBPointer->STATE_SAVE_AREA.CR2 = __readcr2();
	VMCBPointer->STATE_SAVE_AREA.CR3 = GetPML4TableBaseAddress();
	VMCBPointer->STATE_SAVE_AREA.CR4 = __readcr4();
	VMCBPointer->STATE_SAVE_AREA.EFER = __readmsr(MSR_EFER_ADDRESS);

	// Let's fill in the data registers
	VMCBPointer->STATE_SAVE_AREA.DR6 = 0xFFFF0FF0;
	VMCBPointer->STATE_SAVE_AREA.DR7 = 0x400;

	VMCBPointer->STATE_SAVE_AREA.RAX = 0;

	// We can intialize the RBX, RCX,..., R15 in VMSA

	return InitializeVmcb::Success;
}

VOID SetRipWithHLT()
{
	uint8_t* GuestMemoryUint8Pointer = (uint8_t*)(g_GuestState[0].GuestMemory);
	GuestMemoryUint8Pointer[0] = 0x90;
	GuestMemoryUint8Pointer[1] = 0x90;
	GuestMemoryUint8Pointer[2] = 0x90;
	GuestMemoryUint8Pointer[3] = 0xF4;
}

VOID HandleVmExit()
{
	if (KeGetCurrentIrql() > DISPATCH_LEVEL)
		KeRaiseIrqlToDpcLevel();

	__svm_vmload(g_GuestState[0].HostStateSaveArea);
	
	if (!g_GuestState)
	{
		DbgPrint("g_GuestState is NULL in HandleVmExit()\n");
		return;
	}
	
	PVMCB VmcbVA = (PVMCB)PhysicalToVirtualAddress(g_GuestState[0].VmcbPointer);
	
	if (!VmcbVA)
	{
		DbgPrint("VmcbVA is NULL in HandleVmExit()\n");
		return;
	}

	auto ExitCode = VmcbVA->CONTROL_AREA.Exitcode;

	switch (ExitCode)
	{
	case 0x78: DbgPrint("Intercepted the HTL instruction\n"); break;
	default:   DbgPrint("Unknown interception\n"); break;
	}
	AsmRestoreState();
}

VOID LaunchVM(size_t ProcessorId)
{
	UNREFERENCED_PARAMETER(ProcessorId);
	if (!(PVOID)(g_GuestState[ProcessorId].VmcbPointer))
	{
		DbgPrint("The VMCB pointer is NULL in g_GuestState\n");
		return;
	}
	DbgPrint("VMCB pointer is not NULL\n");
	DbgPrint("Size of VMCB : %lld\n", sizeof(VMCB));
	DbgPrint("Size of VMCB Control Area: %lld\n", sizeof(VMCB_CTRL_AREA));
	DbgPrint("Size of VMCB State Save Area: %lld\n", sizeof(VMCB_STATE_SAVE_AREA));

	// Remember when you're initializing the VMCB you need to use the virtual memory of the VMCB
	// So here before passing the VMCB memory we would convert it to the virtual memory
	PVMCB VmcbVirtualMemory = (PVMCB)PhysicalToVirtualAddress(g_GuestState[0].VmcbPointer);
	InitializeVmcb VmcbInitialized = InitializeVMCB(VmcbVirtualMemory, 0);

	/*BOOLEAN l_AllocateHostStateSaveArea = AllocateHostStateSaveArea(&g_GuestState[0]);
	if (!l_AllocateHostStateSaveArea)
		return;
	DbgPrint("Allocated the Host State Save Area\n");*/

	switch (VmcbInitialized)
	{
		case InitializeVmcb::Success: DbgPrint("VMCB initialized successfully\n"); break;
		case InitializeVmcb::NullPointer: DbgPrint("VMCB DIDN'T initialize successfully\n"); return;
		default: break;
	}
	
	// At first would come affinity
	// i.e we must first setup affinity...for now we are just going set it for 0th logical processor
	DbgPrint("======================== Launching VM =============================\n");
	KAFFINITY KAffinity = 0;
	KAffinity = MathPower(2, ProcessorId); // for now ProcessorId will be 0
	KeSetSystemAffinityThread(KAffinity);
	DbgPrint("Current thread is running in %lld logical processor\n", ProcessorId);
	ULONG64 EFERContent = __readmsr(MSR_EFER_ADDRESS);
	__writemsr(MSR_EFER_ADDRESS, EFERContent | (1ULL << 12));
	__writemsr(MSR_VM_HSAVE_PA, g_GuestState[0].HostStateSaveArea);

	PAGED_CODE();

	//// At 2nd we would allocate the stack for VM exit handler
	//BOOLEAN IsStackCreated = CreateStackForVMMExitHandler(ProcessorId);
	//if (!IsStackCreated)
	//{
	//	DbgPrint("Couldn't create stack for VM exit handler\nExiting LaunchVM\n");
	//	return;
	//}

	// Let's set the RIP
	SetRipWithHLT();
	//DbgPrint("RIP has value = %d\n", *(uint8_t*)(g_GuestState[0].VM));
	// For now we would skip the above and below steps and straight jump to create the guest entry point
	AsmSaveState(); // First the save the state such as RSP, RBP
	DbgPrint("About to execute VMRUN\n");
	//__svm_vmrun(g_GuestState[0].VmcbPointer);
	GuestEntry(g_GuestState[0].VmcbPointer);
	//AsmRestoreState();
	//HandleVmExit();
	
	
	// For now just print the size's of the control and state save area of the VMCB

	// At 3rd we would allocate the BitMap for MSR's 
}
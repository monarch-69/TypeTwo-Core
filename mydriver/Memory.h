#pragma once
#include <wdm.h>
#include <stdint.h>
#include "Helpers.h"

typedef struct _SEG_REGISTER
{
	uint16_t Selector;
	uint16_t Attribute;
	uint32_t Limit;
	uint64_t Base;
} SEG_REGISTER, *PSEG_REGISTER;

// Declaration of VMCB structure
// Control block of VMCB
typedef struct _VMCB_CTRL_AREA
{
	uint16_t Intercept_CR_Reads; // Vector 0 Part 1
	uint16_t Intercept_CR_Writes; // Vector 0 Part 2
	uint16_t Intercept_DR_Reads; // Vector 1 Part 2
	uint16_t Intercept_DR_Writes; // Vector 1 Part 2

	uint32_t Intercept_Vector_Exceptions; // Vector 2
	uint32_t Intercept_Instructions; // Vector 3
	uint32_t Intercept_SVM_Instructions; // Vector 4
	
	uint32_t Intercept_Vector5_Exceptions : 7; // Vector 5 Part 1
	uint32_t Intercept_Vector5_Exceptions_SBZ : 25; // Vector 5 Part 2

	uint8_t Reserved1[(0x3b - 0x18) + 0x1];

	uint16_t Pause_Filter_Threshold;
	uint16_t Pause_Filter_Count;

	uint64_t IOPM_Base_PA;
	uint64_t MSRPM_Base_PA;
	uint64_t TSC_Offset;

	uint64_t Guest_ASID : 32;
	uint64_t TLB_Control : 8;
	uint64_t Reserved2 : 24; // SBZ

	uint64_t Virtual_Vector;
	uint64_t Interrupt_Shadow_And_Mask;
	uint64_t Exitcode;
	uint64_t Exitinfo1;
	uint64_t Exitinfo2;
	uint64_t Exit_Int_Info;
	uint64_t Enable_Vector;
	uint64_t Avic_Apic_Bar;
	uint64_t Guest_PA_GHCB;
	uint64_t Eventinj;
	uint64_t N_CR3; // Nested paging CR3
	uint64_t LBR_Virtualization_Enable;
	uint64_t VMCB_Clean_Bits;
	uint64_t N_RIP;
	
	uint8_t Bytes_Fetched;
	uint8_t Guest_Instruction_Bytes[15];

	uint64_t Avic_Apic_Backing_Page_Pointer;

	uint8_t Reserved3[(0xef - 0xe8) + 0x1];

	uint64_t Avic_Logical_Table_Pointer;
	uint64_t Avic_Physical_Table_Pointer;

	uint8_t Reserved4[(0x107 - 0x100) + 0x1];

	uint64_t VMSA_Pointer;
	
	uint8_t Reserved5[0x400 - 0x110];

} VMCB_CTRL_AREA, *PVMCB_CTRL_AREA;

// VMCB State Save Area
typedef struct _VMCB_STATE_SAVE_AREA
{
	SEG_REGISTER ES;
	SEG_REGISTER CS;
	SEG_REGISTER SS;
	SEG_REGISTER DS;
	SEG_REGISTER FS;
	SEG_REGISTER GS;
	SEG_REGISTER GDTR;
	SEG_REGISTER LDTR;
	SEG_REGISTER IDTR;
	SEG_REGISTER TR;

	uint8_t Reserved1[(0xca - 0xa0) + 0x1];
	uint8_t CPL;
	
	uint32_t Reserved2;

	uint64_t EFER;

	uint8_t Reserved3[(0xdf - 0xd8) + 0x1];

	uint64_t PERF_CTL0;
	uint64_t PERF_CTR0;
	uint64_t PERF_CTL1;
	uint64_t PERF_CTR1;
	uint64_t PERF_CTL2;
	uint64_t PERF_CTR2;
	uint64_t PERF_CTL3;
	uint64_t PERF_CTR3;
	uint64_t PERF_CTL4;
	uint64_t PERF_CTR4;
	uint64_t PERF_CTL5;
	uint64_t PERF_CTR5;
	uint64_t CR4;
	uint64_t CR3;
	uint64_t CR0;
	uint64_t DR7;
	uint64_t DR6;
	uint64_t RFLAGS;
	uint64_t RIP;

	uint8_t Reserved4[(0x1d7 - 0x180) + 0x1];

	uint64_t RSP;
	uint64_t S_CET;
	uint64_t SSP;
	uint64_t ISST_ADDR;
	uint64_t RAX;
	uint64_t STAR;
	uint64_t LSTAR;
	uint64_t CSTAR;
	uint64_t SFMASK;
	uint64_t KernelGsBase;
	uint64_t SYSENTER_CS;
	uint64_t SYSENTER_ESP;
	uint64_t SYSENTER_EIP;
	uint64_t CR2;

	uint8_t Reserved5[(0x267 - 0x248) + 0x1];

	uint64_t G_PAT;
	uint64_t DBGCTL;
	uint64_t BR_FROM;
	uint64_t BR_TO;
	uint64_t LASTEXCPFROM;
	uint64_t LASTEXCPTO;
	uint64_t DBGEXTNCTL;

	uint8_t Reserved6[(0x2df - 0x2a0) + 0x1];

	uint64_t SPEC_CTRL;

	uint8_t Reserved7[(0x1000 - 0x2e9 - 0x400) + 0x1];

} VMCB_STATE_SAVE_AREA, *PVMCB_STATE_SAVE_AREA;

typedef struct _VMCB
{
	VMCB_CTRL_AREA CONTROL_AREA;
	VMCB_STATE_SAVE_AREA STATE_SAVE_AREA;
	// For some reason the total size of state save area was not equal to 3072
	// 8 bytes was short
	uint8_t RESERVED[8];
} VMCB, *PVMCB;

enum InitializeVmcb
{
	NullPointer,
	Success
	// other enums
};

// Defining the intercepts
#define VMRUN_INSTRUCTION_INTERCEPT (1UL << 0)
#define VMMCALL_INSTRUCTION_INTERCEPT (1ULL << 1);
#define HLT_INSTRUCTION_INTERCEPT (1ULL << 24);

extern "C" {
	void AsmSaveState(void);
	void AsmRestoreState(void);
	void GuestEntry(ULONG64 VmcbPhysicalAddress);
}

VOID SetRipWithHLT();
BOOLEAN AllocateVmcbRegion(
	IN PVIRTUAL_MACHINE_STATE Vm_State);
VOID LaunchVM(size_t ProcessorId);
BOOLEAN Vm_Run();
VOID Vm_Stop();
BOOLEAN CreateStackForVMMExitHandler(size_t ProcessorId);
ULONG64 GetPML4TableBaseAddress();
BOOLEAN AllocateHostStateSaveArea(
	IN PVIRTUAL_MACHINE_STATE VmState);
InitializeVmcb InitializeVMCB(PVMCB VMCBPointer, size_t ProcessorId);
extern "C" VOID HandleVmExit();
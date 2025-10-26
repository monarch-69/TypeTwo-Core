#pragma once

#include <ntddk.h>

// All the type definitions for NPT { PML4E, PDPTE, PDE, PTE} are exactly as same as the original
// implementations

// This below can be considered as analogous to EPTP
// this is the nCR3 register used for nest paging and contains the address of the PML4
typedef union _NCR3
{
	ULONG64 All;

	struct
	{
		ULONG64 MBZ1 : 12;
		ULONG64 PML4PAddress : 40;
		ULONG64 MBZ2 : 12;
	} Fields;
} NCR3, *PNCR3;

typedef union _NPT_PML4E
{
	ULONG64 All;

	struct
	{
		ULONG64 P : 1;
		ULONG64 RW : 1;
		ULONG64 US : 1;
		ULONG64 PWT : 1;
		ULONG64 PCD : 1;
		ULONG64 A : 1;
		ULONG64 IGN : 1; // this is ignored, it's meant for the lowest level the bit is D (dirty)
		ULONG64 MBZ1 : 1;
		ULONG64 MBZ2: 1;
		ULONG64 AVL1 : 3;
		ULONG64 PDPTPAddress : 40;
		ULONG64 AVL2 : 11;
		ULONG64 NX : 1;
	} Fields;

} NPT_PML4E, *PNPT_PML4E;

typedef union _NPT_PDPE
{
	ULONG64 All;

	struct
	{
		ULONG64 P : 1;
		ULONG64 RW : 1;
		ULONG64 US : 1;
		ULONG64 PWT : 1;
		ULONG64 PCD : 1;
		ULONG64 A : 1;
		ULONG64 IGN1 : 1; // this is ignored, it's meant for the lowest level the bit is D (dirty)
		ULONG64 MBZ : 1;
		ULONG64 IGN2 : 1;
		ULONG64 AVL1 : 3;
		ULONG64 PDPAddress : 40;
		ULONG64 AVL2 : 11;
		ULONG64 NX : 1;
	} Fields;

} NPT_PDPE, *PNPT_PDPE;

typedef union _NPT_PDE
{
	ULONG64 All;

	struct
	{
		ULONG64 P : 1;
		ULONG64 RW : 1;
		ULONG64 US : 1;
		ULONG64 PWT : 1;
		ULONG64 PCD : 1;
		ULONG64 A : 1;
		ULONG64 IGN1 : 1; // this is ignored, it's meant for the lowest level the bit is D (dirty)
		ULONG64 MBZ : 1;
		ULONG64 IGN2 : 1;
		ULONG64 AVL1 : 3;
		ULONG64 PTPAddress : 40;
		ULONG64 AVL2 : 11;
		ULONG64 NX : 1;
	} Fields;

} NPT_PDE, *PNPT_PDE;

typedef union _NPT_PTE
{
	ULONG64 All;

	struct
	{
		ULONG64 P : 1;
		ULONG64 RW : 1;
		ULONG64 US : 1;
		ULONG64 PWT : 1;
		ULONG64 PCD : 1;
		ULONG64 A : 1;
		ULONG64 D : 1;
		ULONG64 PAT : 1;
		ULONG64 G : 1;
		ULONG64 AVL1 : 3;
		ULONG64 PPAddress : 40;
		ULONG64 AVL2 : 7;
		ULONG64 CR4OPKE : 4;
		ULONG64 NX : 1;
	} Fields;
} NPT_PTE, *PNPT_PTE;
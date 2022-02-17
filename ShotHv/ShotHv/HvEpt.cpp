#include "HvPch.h"

static SHV_MTRR_RANGE CoreMtrrData[16];
static ULONG NumberOfEnabledMemoryRanges = 0;
static KSPIN_LOCK g_HvEptLock = {};

_Use_decl_annotations_
NTSTATUS
WINAPI
CheckHvEptSupported()
{
	Ia32VmxEptVpidCapMsr VpidRegister = { 0 };

	Ia32MtrrDefTypeRegister MTRRDefType = { 0 };

	VpidRegister.all = __readmsr(MSR_IA32_VMX_EPT_VPID_CAP);

	///See: 11.11.2.1 IA32_MTRR_DEF_TYPE MSR
	MTRRDefType.Flags = __readmsr(MSR_IA32_MTRR_DEF_TYPE); /* IA32_MTRR_DEF_TYPE MSR */

	if (!VpidRegister.fields.support_page_walk_length4
		|| !VpidRegister.fields.support_write_back_memory_type
		|| !VpidRegister.fields.support_pde_2mb_pages)
	{
		return STATUS_UNSUCCESSFUL;
	}

	if (!MTRRDefType.MtrrEnable)
	{
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
WINAPI
InitlizetionHvEpt()
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	// ����EPT�ڴ�
	ntStatus = BuildHvEptMemory();
	if (!NT_SUCCESS(ntStatus)) {
		return ntStatus;
	}

	// ��ʼ�� EPT_DYNAMIC_SPLIT
	ntStatus = BuildHvEptDynamicSplit();
	if (!NT_SUCCESS(ntStatus)) {
		return ntStatus;
	}

	DBG_PRINT("EPT ��ʼ�����!\r\n");

	return ntStatus;
}

_Use_decl_annotations_
VOID
WINAPI
UnInitlizetionHvEpt()
{
	auto ContextEntry = GetHvContextEntry();

	if (ContextEntry->VmxEpt)
	{
		// �ͷ�Ept�����ڴ�
		ExFreePool(ContextEntry->VmxEpt);
	}
}

_Use_decl_annotations_
NTSTATUS
WINAPI
GetHvEptMtrrInFo()
{
	MTRR_CAPABILITIES mtrrCapabilities = {};
	MTRR_VARIABLE_BASE mtrrBase = {};
	MTRR_VARIABLE_MASK mtrrMask = {};
	PSHV_MTRR_RANGE	 Descriptor = {};
	unsigned long bit = 0;

	// ��ȡ��Χ
	/// See: 11.11.1 MTRR Feature Identification
	mtrrCapabilities.AsUlonglong = __readmsr(MTRR_MSR_CAPABILITIES); /* ��ȡMTRR�����Ϣ */

	for (int i = 0; i < mtrrCapabilities.u.VarCnt; i++)
	{
		mtrrBase.AsUlonglong = __readmsr(MTRR_MSR_VARIABLE_BASE + i * 2);
		mtrrMask.AsUlonglong = __readmsr(MTRR_MSR_VARIABLE_MASK + i * 2);

		//����Ƿ�����
		if (mtrrMask.u.Enabled) /*mtrrData[i].Enabled != FALSE && */
		{
			Descriptor = &CoreMtrrData[NumberOfEnabledMemoryRanges++];
			Descriptor->Type = (UINT32)mtrrBase.u.Type;
			Descriptor->Enabled = (UINT32)mtrrMask.u.Enabled;

			//���û���ַ
			Descriptor->PhysicalAddressMin = mtrrBase.u.PhysBase * PAGE_SIZE;

			_BitScanForward64(&bit, mtrrMask.u.PhysMask * PAGE_SIZE);
			Descriptor->PhysicalAddressMax = Descriptor->PhysicalAddressMin + ((1ULL << bit) - 1);

			if (CoreMtrrData[i].Type == MTRR_TYPE_WB) {
				NumberOfEnabledMemoryRanges--;
			}
		}
	}

	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS 
WINAPI
BuildHvEptMemory()
{
	HvContextEntry* ContextEntry = GetHvContextEntry();

	if (!ContextEntry) {
		return STATUS_UNSUCCESSFUL;
	}

	ContextEntry->VmxEpt = (pHvEptEntry)ExAllocatePool(NonPagedPoolNx, sizeof(HvEptEntry));
	if (NULL == ContextEntry->VmxEpt)
	{
		return STATUS_MEMORY_NOT_ALLOCATED;
	}

	// ��� EPT �ڴ���Ϣ
	ContextEntry->VmxEpt->PML4T[0].Flags = 0;
	ContextEntry->VmxEpt->PML4T[0].ReadAccess = 1;
	ContextEntry->VmxEpt->PML4T[0].WriteAccess = 1;
	ContextEntry->VmxEpt->PML4T[0].ExecuteAccess = 1;
	ContextEntry->VmxEpt->PML4T[0].PageFrameNumber = MmGetPhysicalAddress(&ContextEntry->VmxEpt->PDPT).QuadPart / PAGE_SIZE; // ��ȡ PFN

	for (int i = 0; i < PDPTE_ENTRY_COUNT; i++)
	{
		// ����PDPT��ҳ������
		ContextEntry->VmxEpt->PDPT[i].Flags = 0;
		ContextEntry->VmxEpt->PDPT[i].ReadAccess = 1;
		ContextEntry->VmxEpt->PDPT[i].WriteAccess = 1;
		ContextEntry->VmxEpt->PDPT[i].ExecuteAccess = 1;
		ContextEntry->VmxEpt->PDPT[i].PageFrameNumber = MmGetPhysicalAddress(&ContextEntry->VmxEpt->PDT[i][0]).QuadPart / PAGE_SIZE; // ��ȡ PFN
	}

	for (int i = 0; i < PDPTE_ENTRY_COUNT; i++)
	{
		// ����PDT��ÿ2MΪһ��ҳ��
		for (int j = 0; j < PDE_ENTRY_COUNT; j++)
		{
			ContextEntry->VmxEpt->PDT[i][j].Flags = 0;
			ContextEntry->VmxEpt->PDT[i][j].ReadAccess = 1;
			ContextEntry->VmxEpt->PDT[i][j].WriteAccess = 1;
			ContextEntry->VmxEpt->PDT[i][j].ExecuteAccess = 1;
			ContextEntry->VmxEpt->PDT[i][j].LargePage = 1;
			ContextEntry->VmxEpt->PDT[i][j].PageFrameNumber = ((uintptr_t)i * 512) + j;

			// �����ڴ�����
			SetEptMemoryByMttrInfo(ContextEntry, i, j);
		}
	}

	// ���� Eptp
	auto ntStatus = SetEptp(ContextEntry);

	return ntStatus;
}

_Use_decl_annotations_
NTSTATUS 
WINAPI
SetEptMemoryByMttrInfo(
	_In_ HvContextEntry* ContextEntry, 
	_In_ INT i, 
	_In_ INT j
)
{
	ULONG_PTR LargePageAddress = 0;
	ULONG_PTR CandidateMemoryType = 0;

	LargePageAddress = ContextEntry->VmxEpt->PDT[i][j].PageFrameNumber * _2MB;

	/* Ĭ��WB�ڴ����� */
	CandidateMemoryType = MTRR_TYPE_WB;

	for (ULONG k = 0; k < NumberOfEnabledMemoryRanges; k++)
	{
		///See: 11.11.9 Large Page Size Considerations
		// ��һ��ҳ������ΪUC����(��Ϊ���п���ΪMMIO����Ҫ)
		// Ԥ��4KB��ַI/O (UC)
		if (ContextEntry->VmxEpt->PDT[i][j].PageFrameNumber == 0) {
			CandidateMemoryType = MTRR_TYPE_UC;
			break;
		}

		// ����ڴ��Ƿ�����
		if (CoreMtrrData[k].Enabled != FALSE)
		{
			///See: 11.11.4 Range Size and Alignment Requirement
			// ����ҳ�������ַ�ı߽�,���������ҳ��Ϊ4KB,���д���Ϊ2MB��MemType
			// If this page's address is below or equal to the max physical address of the range
			if ((LargePageAddress <= CoreMtrrData[k].PhysicalAddressMax) &&
				// And this page's last address is above or equal to the base physical address of the range
				((LargePageAddress + _2MB - 1) >= CoreMtrrData[k].PhysicalAddressMin))
			{
				///See:11.11.4.1 MTRR Precedences
				// ��д��ѡ�ڴ�����
				CandidateMemoryType = CoreMtrrData[k].Type;
				// UC��������
				if (CandidateMemoryType == MTRR_TYPE_UC) {
					break;
				}
			}
		}
	}

	ContextEntry->VmxEpt->PDT[i][j].MemoryType = CandidateMemoryType;

	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS 
WINAPI
SetEptp(
	_In_ HvContextEntry* ContextEntry
)
{
	Ia32VmxEptVpidCapMsr ia32Eptinfo = { __readmsr(MSR_IA32_VMX_EPT_VPID_CAP) };

	if (ia32Eptinfo.fields.support_page_walk_length4)
	{
		ContextEntry->VmxEptp.PageWalkLength = 3; // ����Ϊ 4 ��ҳ��
	}

	if (ia32Eptinfo.fields.support_uncacheble_memory_type)
	{
		ContextEntry->VmxEptp.MemoryType = MEMORY_TYPE_UNCACHEABLE; // UC(�޻������͵��ڴ�)
	}

	if (ia32Eptinfo.fields.support_write_back_memory_type)
	{
		ContextEntry->VmxEptp.MemoryType = MEMORY_TYPE_WRITE_BACK;  // WB(�ɻ�д���͵��ڴ�, ֧������������)
	}

	if (ia32Eptinfo.fields.support_accessed_and_dirty_flag) // Ept dirty ��־λ�Ƿ���Ч
	{
		ContextEntry->VmxEptp.EnableAccessAndDirtyFlags = TRUE;
	}
	else
	{
		ContextEntry->VmxEptp.EnableAccessAndDirtyFlags = FALSE;
	}

	ContextEntry->VmxEptp.PageFrameNumber = MmGetPhysicalAddress(&(ContextEntry->VmxEpt->PML4T[0])).QuadPart / PAGE_SIZE;

	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
WINAPI
BuildHvEptDynamicSplit()
{
	// ��ȡ��ǰ�˵� HvContextEntry
	HvContextEntry* pContextEntry = GetHvContextEntry();

	// ����DynSplits����Ҫ�ı����ڴ�(Ϊ����ִ�еķǷ�ҳ�ڴ�, ÿ�������ֻ��HOOK 30������)
	pContextEntry->DynSplits = (EPT_DYNAMIC_SPLIT*)ExAllocatePool(NonPagedPoolNx, MAX_EPTHOOK_NUMBER * sizeof(EPT_DYNAMIC_SPLIT));
	if (NULL == pContextEntry->DynSplits) {
		return STATUS_MEMORY_NOT_ALLOCATED;
	}

	RtlSecureZeroMemory(pContextEntry->DynSplits, MAX_EPTHOOK_NUMBER * sizeof(HvContextEntry));

	return STATUS_SUCCESS;
}

_Use_decl_annotations_
EPT_DYNAMIC_SPLIT*
WINAPI
GetHvEptDynamicSplit()
{
	KIRQL OldIrql = {};

	EPT_DYNAMIC_SPLIT* pRet = NULL;

	// ��ȡ��ǰ�˵� HvContextEntry
	HvContextEntry* pContextEntry = GetHvContextEntry();

	if (MAX_EPTHOOK_NUMBER <= pContextEntry->DynSplitCount) {
		return NULL;
	}

	KeAcquireSpinLock(&g_HvEptLock, &OldIrql);
	
	for (ULONG i = 0; i < MAX_EPTHOOK_NUMBER; i++)
	{
		// �ж��Ƿ���ʹ��
		if (FALSE == pContextEntry->DynSplits[i].IsUse)
		{
			pRet = &pContextEntry->DynSplits[i];
			pContextEntry->DynSplits[i].IsUse = TRUE;
			pContextEntry->DynSplitCount++;
			break;
		}
	}

	KeReleaseSpinLock(&g_HvEptLock, OldIrql);

	return pRet;
}

_Use_decl_annotations_
NTSTATUS 
WINAPI
EptUpdateTable(
	_In_ HvEptEntry* Table,
	_In_ EPT_ACCESS Access, 
	_In_ ULONG64 PA, 
	_In_ ULONG64 PFN
)
{
	PEPTE Epte = EptGetEpteEntry(Table, PA);
	if (Epte && Access)
	{
		Epte->ReadAccess = (Access & EPT_ACCESS_READ) != 0;
		Epte->WriteAccess = (Access & EPT_ACCESS_WRITE) != 0;
		Epte->ExecuteAccess = (Access & EPT_ACCESS_EXEC) != 0;
		Epte->PageFrameNumber = PFN;
	}

	return STATUS_SUCCESS;
}

_Use_decl_annotations_
PEPTE 
WINAPI
EptGetEpteEntry(
	_In_ HvEptEntry* Table,
	_In_ ULONG64 PA
)
{
	// ��ȡĿ��PTE
	ULONG64 PhyA = PA;
	PEPDE_2MB Epde = EptGetPml2Entry(Table, PhyA);
	
	if (Epde && Epde->LargePage) {
		//PEPT_DYNAMIC_SPLIT Dynentry = GetHvEptDynamicSplit();
		PEPT_DYNAMIC_SPLIT Dynentry = (EPT_DYNAMIC_SPLIT*)ExAllocatePool(NonPagedPoolNx, sizeof(EPT_DYNAMIC_SPLIT));
		if (!Dynentry || !EptSplitLargePage(Epde, Dynentry)) {
			if (Dynentry) {
				ExFreePool(Dynentry);
			}
			return NULL;
		}
	}

	PEPTE Epte = EptGetPml1Entry(Table, PhyA);
	if (!Epte) return NULL;

	return Epte;
}

_Use_decl_annotations_
BOOLEAN
WINAPI
EptSplitLargePage(
	_In_ EPDE_2MB* LargeEptPde, 
	_In_ EPT_DYNAMIC_SPLIT* PreAllocatedBuffer
)
{
	PEPT_DYNAMIC_SPLIT	    NewSplit;
	EPTE					EntryTemplate;
	SIZE_T                  EntryIndex;
	PEPDE_2MB				TargetEntry;
	EPDE					NewPointer;

	if (!LargeEptPde)
	{
		return FALSE;
	}

	if (!LargeEptPde->LargePage)
	{
		return TRUE;
	}

	TargetEntry = LargeEptPde;

	NewSplit = PreAllocatedBuffer;
	RtlSecureZeroMemory(NewSplit, sizeof(EPT_DYNAMIC_SPLIT));

	EntryTemplate.Flags = 0;
	EntryTemplate.ReadAccess = 1;
	EntryTemplate.WriteAccess = 1;
	EntryTemplate.ExecuteAccess = 1;

	__stosq((SIZE_T*)&NewSplit->PTT[0], EntryTemplate.Flags, PTE_ENTRY_COUNT);

	for (EntryIndex = 0; EntryIndex < PTE_ENTRY_COUNT; EntryIndex++)
	{
		NewSplit->PTT[EntryIndex].PageFrameNumber = ((TargetEntry->PageFrameNumber * _2MB) / PAGE_SIZE) + EntryIndex;
	}

	NewPointer.Flags = 0;
	NewPointer.WriteAccess = 1;
	NewPointer.ReadAccess = 1;
	NewPointer.ExecuteAccess = 1;
	NewPointer.PageFrameNumber = PFN( MmGetPhysicalAddress(&NewSplit->PTT[0]).QuadPart );

	RtlCopyMemory(TargetEntry, &NewPointer, sizeof(NewPointer));

	return TRUE;
}

_Use_decl_annotations_
PEPTE
WINAPI
EptGetPml1Entry(
	_In_ HvEptEntry* EptPageTable,
	_In_ SIZE_T PhysicalAddress
)
{
	SIZE_T          Directory, DirectoryPointer, PML4Entry;
	PEPDE_2MB		LargePde;
	PEPDE			PdePointer;
	PEPTE			Pte;

	Directory = ADDRMASK_EPT_PML2_INDEX(PhysicalAddress);
	DirectoryPointer = ADDRMASK_EPT_PML3_INDEX(PhysicalAddress);
	PML4Entry = ADDRMASK_EPT_PML4_INDEX(PhysicalAddress);

	if (PML4Entry > 0)
	{
		return NULL;
	}

	LargePde = &EptPageTable->PDT[DirectoryPointer][Directory];
	if (LargePde->LargePage)
	{
		return NULL;
	}

	PdePointer = (PEPDE)LargePde;

	// ת��ΪPTEָ��
	Pte = (PEPTE)UtilPhysicalAddressToVirtualAddress( (UINT64)(PdePointer->PageFrameNumber * PAGE_SIZE) );
	if (!Pte)
	{
		return NULL;
	}

	Pte = &Pte[ADDRMASK_EPT_PML1_INDEX(PhysicalAddress)];

	return Pte;
}

_Use_decl_annotations_
PEPDE_2MB
WINAPI
EptGetPml2Entry(
	_In_ HvEptEntry* EptPageTable,
	_In_ SIZE_T PhysicalAddress
)
{
	SIZE_T          Directory, DirectoryPointer, PML4Entry;
	PEPDE_2MB		PML2;

	Directory = ADDRMASK_EPT_PML2_INDEX(PhysicalAddress);
	DirectoryPointer = ADDRMASK_EPT_PML3_INDEX(PhysicalAddress);
	PML4Entry = ADDRMASK_EPT_PML4_INDEX(PhysicalAddress);

	if (PML4Entry > 0)
	{
		return NULL;
	}

	PML2 = &EptPageTable->PDT[DirectoryPointer][Directory];
	return PML2;
}

_Use_decl_annotations_
VOID
WINAPI
EptViolationHandler(
	_In_ GuestReg* Registers
)
{
	UNREFERENCED_PARAMETER( Registers );

	ULONG64 GuestAddress = 0;
	PHYSICAL_ADDRESS GuestPhyaddress = {};
	PPAGE_HOOK_CONTEXT pHookEntry = NULL;
	HvContextEntry* pContextEntry = NULL;
	EptAttributePae EptAttrInfo = { 0 };

	GuestAddress = VmxCsRead( GUEST_LINEAR_ADDRESS );
	GuestPhyaddress.LowPart  = (ULONG)VmxCsRead( GUEST_PHYSICAL_ADDRESS );
	GuestPhyaddress.HighPart = (ULONG)VmxCsRead( GUEST_PHYSICAL_ADDRESS_HIGH );

	EptAttrInfo.Flag = VmxCsRead( EXIT_QUALIFICATION );

	// ��ȡ��ǰ�˵� HvContextEntry
	pContextEntry = GetHvContextEntry();

	// �ж��Ƿ������ǵ�HOOK�й�
	pHookEntry = PHGetHookContextByPFN( GuestPhyaddress.QuadPart, DATA_PAGE );
	
	if (pHookEntry && (Activiti == pHookEntry->State))
	{
		ULONG64 TargetPFN = pHookEntry->DataPagePFN;
		EPT_ACCESS TargetAccess = EPT_ACCESS_ALL;

		if (EptAttrInfo.Read)
		{
			TargetPFN = pHookEntry->DataPagePFN;
			TargetAccess = EPT_ACCESS_RW;
		}
		else if (EptAttrInfo.Write)
		{
			TargetPFN = pHookEntry->CodePagePFN;
			TargetAccess = EPT_ACCESS_RW;
		}
		else if (EptAttrInfo.Execute)
		{
			TargetPFN = pHookEntry->CodePagePFN;
			TargetAccess = EPT_ACCESS_EXEC;
		}
		else
		{
			DBG_PRINT( "δ֪��EptViolation!\r\n" );
		}

		EptUpdateTable( pContextEntry->VmxEpt, TargetAccess, GuestPhyaddress.QuadPart, TargetPFN );
	}
	else
	{
		EptUpdateTable( pContextEntry->VmxEpt, EPT_ACCESS_ALL, GuestPhyaddress.QuadPart, PFN( GuestPhyaddress.QuadPart ) );
	}

	// ˢ�� EPT
	// INVEPT ָ������ṩ�� EPTP.PML4T ��ַ��ˢ�� Guest PhySical Mapping �Լ� Combined Mapping ��ص� Cache ��Ϣ
	// Type Ϊ 2 ʱ(���л���), ˢ�� EPTP.PML4T ��ַ�µ����� Cache ��Ϣ
	__invept(INV_ALL_CONTEXTS, &pContextEntry->VmxEptp.Flags);
}
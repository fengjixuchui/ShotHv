#pragma once

extern "C"
NTKERNELAPI
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
VOID
KeGenericCallDpc(
	_In_ PKDEFERRED_ROUTINE Routine,
	_In_opt_ PVOID Context
);

extern "C"
NTKERNELAPI
_IRQL_requires_max_(DISPATCH_LEVEL)
LOGICAL
KeSignalCallDpcSynchronize(
	_In_opt_ PVOID SystemArgument2
);

extern "C"
NTKERNELAPI
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
KeSignalCallDpcDone(
	_In_opt_ PVOID SystemArgument1
);

extern "C"
NTKERNELAPI
PVOID
NTAPI
PsGetProcessDebugPort(
	_In_ PEPROCESS Process
);

extern "C"
NTSTATUS 
NTAPI 
MmCopyVirtualMemory(
	PEPROCESS SourceProcess, 
	PVOID SourceAddress, 
	PEPROCESS TargetProcess, 
	PVOID TargetAddress, 
	SIZE_T BufferSize, 
	KPROCESSOR_MODE PreviousMode, 
	PSIZE_T ReturnSize
);

extern "C"
void* 
NTAPI
PsGetProcessWow64Process(PEPROCESS Process);

extern "C"
NTSTATUS ZwQuerySystemInformation(ULONG InfoClass, PVOID Buffer, ULONG Length, PULONG ReturnLength);

extern "C"
{
	extern POBJECT_TYPE* IoDriverObjectType;
	extern PSHORT NtBuildNumber;
}

extern "C"
{
	void  AsmStackPointer(ULONG_PTR* StackPointer);			// ��ȡ��ǰRsp
	void  AsmNextInstructionPointer(ULONG_PTR* RipPointer);	// ��ȡ��һָ��Rip
	void  AsmCallVmxExitHandler();							// ����VmxExitHandler
	void  AsmUpdateRspAndRip(ULONG_PTR Rsp, ULONG_PTR Rip); // �޸ĵ�ǰ Rsp\Rip
	void  AsmReloadGdtr(void* GdtBase, unsigned long GdtLimit);
	void  AsmReloadIdtr(void* GdtBase, unsigned long GdtLimit);
	void  AsmVmxCall(ULONG CallNumber/*���*/, ULONG64 arg1 = 0, ULONG64 arg2 = 0, ULONG64 arg3 = 0);
	ULONG_PTR __readcs(void);
	ULONG_PTR __readds(void);
	ULONG_PTR __readss(void);
	ULONG_PTR __reades(void);
	ULONG_PTR __readfs(void);
	ULONG_PTR __readgs(void);
	ULONG_PTR __sldt(void);
	ULONG_PTR __str(void);
	ULONG_PTR __sgdt(PGDT gdtr);
	void __writecr2(ULONG_PTR cr2);
	void __writeds(ULONG_PTR ds);
	void __writees(ULONG_PTR es);
	void __writefs(ULONG_PTR fs);
	void __invept(ULONG_PTR Type, ULONG_PTR* EptpPhyAddr); // ˢ�� EPT
	void __invvpid(ULONG_PTR Type, ULONG_PTR* EptpPhyAddr);
}
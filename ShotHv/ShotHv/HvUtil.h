#pragma once

/*
	APC�ȼ����µ�ȫ�߼��ں˵��÷ַ�
*/
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
WINAPI
UtilForEachProcessor(
	_In_ NTSTATUS(*callback_routine)(void*),
	_In_opt_ void* context
);

/*
	DPC�ȼ���ȫ�߼��ں˵��÷ַ�
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
WINAPI
UtilForEachProcessorDpc(
	_In_ PKDEFERRED_ROUTINE deferred_routine,
	_In_opt_ void* context
);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
WINAPI
UtilGetSelectorInfoBySelector(
	ULONG_PTR selector,
	ULONG_PTR* base,
	ULONG_PTR* limit,
	ULONG_PTR* attribute
);

/*
	ע��ػ��ص�
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
WINAPI
RegisterShutdownCallBack(
	_In_ NTSTATUS (*ShutDownCallBack)(_In_ PDEVICE_OBJECT, _In_ PIRP)
);

/*
	ж�عػ��ص�
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
WINAPI
UnRegisterShutdownCallBack();

BOOLEAN
WINAPI
BuildShellCode1(
	_Inout_ PHOOK_SHELLCODE1 pThunk,
	_In_	ULONG64 Pointer,
	_In_    BOOLEAN isX64
);

ULONG64
WINAPI
UtilPhysicalAddressToVirtualAddress(
	_In_ ULONG64 PhysicalAddress
);

ULONG64
WINAPI
UtilVirtualAddressToPhysicalAddress(
	_In_ ULONG64 VrtualAddress
);

/*
	R3 ��ַУ��
*/
BOOLEAN
WINAPI
ProbeUserAddress(
	_In_ PVOID addr,
	_In_ SIZE_T size,
	_In_ ULONG alignment
);

/*
	R0 ��ַУ��
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOL
WINAPI
ProbeKernelAddress(
	_In_ PVOID  addr,
	_In_ SIZE_T size,
	_In_ ULONG  alignment
);

/*
	��ȫ��������
*/
_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
WINAPI
SafeCopy(
	_In_ PVOID dest,
	_In_ PVOID src,
	_In_ SIZE_T size
);

/*
	������ǰģʽ
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
KPROCESSOR_MODE
WINAPI
KeSetPreviousMode(
	_In_ KPROCESSOR_MODE Mode
);

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG_PTR
WINAPI
QueryKernelModule(
	_In_	PUCHAR moduleName,
	_Inout_ ULONG_PTR* moduleSize
);

DWORD
WINAPI
GetUserCr3Offset();
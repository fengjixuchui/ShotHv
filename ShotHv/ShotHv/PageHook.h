#pragma once

/*
	PAGE HOOK��ʼ��
*/
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS 
WINAPI
PHInitlizetion();

/*
	����PFN��ȡHOOK CONTEXT
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
PPAGE_HOOK_CONTEXT
WINAPI
PHGetHookContextByPFN(
	_In_ ULONG64 PA,
	_In_ PAGE_TYPE Type
);

/*
	����HookAddress VA��ȡHOOK CONTEXT
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
PPAGE_HOOK_CONTEXT
WINAPI
PHGetHookContextByVA(
	_In_ ULONG64 VA,
	_In_ PAGE_TYPE Type
);

/*
	����VA��ȡ��ҳ�汻HOOK�Ĵ���
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
ULONG
WINAPI
PHPageHookCount(
	_In_ ULONG64 VA,
	_In_ PAGE_TYPE Type
);

/*
	EPT HOOK
*/
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS 
WINAPI
PHHook(
	_In_	PVOID  pFunc, 
	_In_	PVOID  pHook,
	_Inout_ PVOID* pOriFun
);

/*
	EPT UNHOOK
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
WINAPI
PHUnHook(
	_In_ PVOID pFunc
);

/*
	ALL EPT UNHOOK
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
WINAPI
PHUnAllHook();

/*
	�������о���״̬�� EPT HOOK
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
WINAPI
PHActivateHooks();

/*
	�޸� EPT HOOK STATE
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
WINAPI
PHUpdateHookState(
	_In_ PVOID	 pFunc,
	_In_ ULONG64 State
);

/*
	�����ڴ�CODE
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
WINAPI
PHHideMem(
	_In_ PVOID Address,
	_In_ PVOID Code, 
	_In_ ULONG Size
);

/*
	��ȡĿ��Cr3
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
ULONG64
WINAPI
PHGetHookCr3(
	_In_ PEPROCESS Process,
	_In_ BOOL IsKernel
);
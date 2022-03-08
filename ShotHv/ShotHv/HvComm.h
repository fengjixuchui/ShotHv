#pragma once

/*
	IOCTL ͨ��
*/
#define CTL_SHOTHV CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define SHOTHV_HOOK_ADD     0x1	/* ���Hook */
#define SHOTHV_HOOK_ENABLE  0x2	/* ����Hook */
#define SHOTHV_HOOK_DISABLE 0x3 /* �ر�Hook */
#define SHOTHV_HOOK_DELETE  0x4 /* ɾ��Hook */
#define SHOTHV_HIDE_WRITE	0x5 /* R3��д */

typedef struct _HOOK_CONTEXT
{
	ULONG64	TargetPid;
	PVOID   TargetAddress;
	PVOID   DetourAddress;
	PVOID*  OriAddress;
	ULONG64 State;
	PVOID   Code;
	ULONG64 Size;
}HOOK_CONTEXT, * PHOOK_CONTEXT;

typedef struct _COMM_ENTRY
{
	ULONG64 CommCode;	// ͨ����
	ULONG64 NtStatus;	// ����ֵ
	ULONG64 inData;		// ����
}COMM_ENTRY, * PCOMM_ENTRY;


/*
	��ʼ���ں�ͨ��
*/
NTSTATUS
WINAPI
InitKernelComm(
	_In_ PDRIVER_OBJECT DriverObject
);

/*
	ж���ں�ͨ��
*/
VOID
WINAPI
UnInitKernelComm(
	_In_ PDRIVER_OBJECT DriverObject
);

/* 
	Ĭ�����IRP���� 
*/
NTSTATUS
WINAPI
ShotHvDispathRoutine(
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_ PIRP Irp
);

/*
	IRP_MJ_DEVICE_CONTROL
*/
NTSTATUS
WINAPI
ShotHvDeviceIoControl(
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_ PIRP Irp
);

NTSTATUS
WINAPI
ShotHvCommHandler(
	_In_ COMM_ENTRY* instructions
);
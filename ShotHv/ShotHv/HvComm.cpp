#include "HvPch.h"

#define DEVICE_NAME         L"\\Device\\ShotHvDDK"
#define SYMBOLICLINK_NAME   L"\\??\\ShotHvDDK"

NTSTATUS
WINAPI
InitKernelComm(
	_In_ PDRIVER_OBJECT DriverObject
)
{
    // �����豸����
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(DEVICE_NAME);
    UNICODE_STRING SysmolicLinkName = RTL_CONSTANT_STRING(SYMBOLICLINK_NAME);

    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    ntStatus = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);
    if (!NT_SUCCESS(ntStatus)){
        return ntStatus;
    }

    // ������������
    ntStatus = IoCreateSymbolicLink(&SysmolicLinkName, &DeviceName);
    if (!NT_SUCCESS(ntStatus)){
        return ntStatus;
    }
    
    // ��3��ִ�� ָ�� ����ʱִ��ָ������
    for (uintptr_t i = 0; i < IRP_MJ_MAXIMUM_FUNCTION + 1; i++)
    {
        // ������ֵIRP��Ӧ�ĺ���ָ��
        DriverObject->MajorFunction[i] = ShotHvDispathRoutine;
    }

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ShotHvDeviceIoControl;
    
    // ���ý���ģʽ����ʼ���豸����
    DeviceObject->Flags |= DO_DIRECT_IO; // MDL
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

VOID 
WINAPI
UnInitKernelComm(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    UNICODE_STRING SymbolicLink = RTL_CONSTANT_STRING(SYMBOLICLINK_NAME);
    IoDeleteDevice(DriverObject->DeviceObject);
    IoDeleteSymbolicLink(&SymbolicLink);
}

NTSTATUS 
WINAPI
ShotHvDispathRoutine(
    _In_ PDEVICE_OBJECT DeviceObject, 
    _In_ PIRP Irp
)
{
    // �õ���ǰIRP�� ջ�ṹ
    PIO_STACK_LOCATION sTack = IoGetCurrentIrpStackLocation(Irp);

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(sTack);

    // ����IRP��Ϣ, һ��Ҫ����
    // ���ظ�3����������,û����0
    Irp->IoStatus.Information = 0;
    // ����IRP����״̬, ����Getlasterror()������ȡ��ֵ
    Irp->IoStatus.Status = STATUS_SUCCESS;
    // �������ȼ�����IRP��������
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS 
WINAPI
ShotHvDeviceIoControl(
    _In_ PDEVICE_OBJECT DeviceObject, 
    _In_ PIRP Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    NTSTATUS ntStatus = STATUS_SUCCESS;
    PCHAR InputBuffer = NULL;
    ULONG InputLength = 0;
    PCHAR OutputBuffer = NULL;
    ULONG OutputLength = 0;
    ULONG Length = 0;
    PIO_STACK_LOCATION sTack = IoGetCurrentIrpStackLocation(Irp);
    // ��ȡ IO������
    ULONG IoCode = sTack->Parameters.DeviceIoControl.IoControlCode;
   
    switch (IoCode)
    {
    case CTL_SHOTHV: // �����0x800������, ����������
    {
        // ��ȡ���뻺�����ĵ�ַ
        InputBuffer  = (PCHAR)Irp->AssociatedIrp.SystemBuffer;
        // ��ȡ���뻺�����ĳ���
        InputLength  = sTack->Parameters.DeviceIoControl.InputBufferLength;
        // ��ȡ����������ĵ�ַ
        OutputBuffer = (PCHAR)Irp->AssociatedIrp.SystemBuffer;
        // ��ȡ����������ĳ���
        OutputLength = sTack->Parameters.DeviceIoControl.OutputBufferLength;

        if (InputBuffer && SafeCopy(InputBuffer, InputBuffer, sizeof(COMM_ENTRY))) {
            ((COMM_ENTRY*)InputBuffer)->NtStatus = ShotHvCommHandler((COMM_ENTRY*)InputBuffer);
        }

        Length = 0;
        ntStatus = STATUS_SUCCESS;
    }break;
    default:
    {
        Length = 0;
        ntStatus = STATUS_SUCCESS;
    }break;
    }

    // ����IRP��Ҫ����
    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = Length;
    // ��������
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return ntStatus;
}

NTSTATUS
WINAPI
ShotHvCommHandler(
    _In_ COMM_ENTRY* instructions
)
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    if (UserMode != ExGetPreviousMode()) {
        return STATUS_NOT_SUPPORTED;
    }
   
    switch (instructions->CommCode)
    {
    case SHOTHV_HOOK_ADD:       // ���HOOK
    {
        PHOOK_CONTEXT Context = (PHOOK_CONTEXT)instructions->inData;

        PEPROCESS TarProcess = NULL;

        KAPC_STATE kApc = { 0 };

        ntStatus = PsLookupProcessByProcessId((HANDLE)Context->TargetPid, &TarProcess);

        if (NT_SUCCESS(ntStatus)) {

            KeStackAttachProcess(TarProcess, &kApc);

            ntStatus = PHHook(Context->TargetAddress, Context->DetourAddress, Context->OriAddress);

            KeUnstackDetachProcess(&kApc);
        }     
    }
        break;
    case SHOTHV_HOOK_ENABLE:    // ����HOOK
    {  
        ntStatus = PHActivateHooks();
    }
        break;
    case SHOTHV_HOOK_DISABLE:   // �ر�HOOK
    {
        PHOOK_CONTEXT Context = (PHOOK_CONTEXT)instructions->inData;

        PEPROCESS TarProcess = NULL;

        KAPC_STATE kApc = { 0 };

        ntStatus = PsLookupProcessByProcessId((HANDLE)Context->TargetPid, &TarProcess);

        if (NT_SUCCESS(ntStatus)) {

            KeStackAttachProcess(TarProcess, &kApc);

            ntStatus = PHUpdateHookState(Context->TargetAddress, Context->State);

            KeUnstackDetachProcess(&kApc);
        }
    }
        break;
    case SHOTHV_HOOK_DELETE:    // ɾ��HOOK
    {
        PHOOK_CONTEXT Context = (PHOOK_CONTEXT)instructions->inData;

        PEPROCESS TarProcess = NULL;

        KAPC_STATE kApc = { 0 };

        ntStatus = PsLookupProcessByProcessId((HANDLE)Context->TargetPid, &TarProcess);

        if (NT_SUCCESS(ntStatus)) {

            KeStackAttachProcess(TarProcess, &kApc);

            ntStatus = PHUnHook(Context->TargetAddress);

            KeUnstackDetachProcess(&kApc);
        }
    }
        break;
    case SHOTHV_HIDE_WRITE:
    {
        PHOOK_CONTEXT Context = (PHOOK_CONTEXT)instructions->inData;

        PEPROCESS TarProcess = NULL;

        KAPC_STATE kApc = { 0 };

        ntStatus = PsLookupProcessByProcessId((HANDLE)Context->TargetPid, &TarProcess);

        if (NT_SUCCESS(ntStatus)) {

            KeStackAttachProcess(TarProcess, &kApc);

            ntStatus = PHHideMem(Context->TargetAddress, Context->Code, (ULONG)Context->Size);

            KeUnstackDetachProcess(&kApc);
        }     
    }
        break;
    default:
        break;
    }

    return ntStatus;
}
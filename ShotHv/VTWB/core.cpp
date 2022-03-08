#include "pch.h"

HANDLE g_hDevice;

// ��ʼ�����ں˵�IOCTLͨ��
bool VtWb::InitIoCtlComm()
{
    // ��3���з��ʷ��������� \\.\ShotHvDDK
    g_hDevice = CreateFile(TEXT("\\\\.\\ShotHvDDK"),
        GENERIC_ALL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_ENCRYPTED,
        NULL
    );

    if (g_hDevice == INVALID_HANDLE_VALUE) {
        return false;
    }

    return true;
}

// ����ͨ�Ű�
bool VtWb::SendData(
    _In_ ULONG_PTR CommCode, 
    _In_ PVOID InData
)
{
    DWORD dwRet = 0;

    COMM_ENTRY Comm = { 0 };

    Comm.CommCode = CommCode;
    Comm.inData = (ULONG64)InData;

    bool Flag = DeviceIoControl(
        g_hDevice,          // �豸���Or�ļ����
        CTL_SHOTHV,         // Io������
        &Comm,              // ���뻺����
        sizeof(COMM_ENTRY), // ���뻺��������
        NULL,               // ���������
        NULL,               // �������������
        &dwRet,             // ʵ�ʲ��������ݳ���
        NULL);

    return Comm.NtStatus == 0 ? true : false;
}

// ���Ept Hook(֧��R0\R3)
bool VtWb::ShotHvAddHook(
    _In_ unsigned __int64 target_pid, 
    _In_ void* target_address, 
    _In_ void* detour_address
)
{
    HOOK_CONTEXT Context = { 0 };

    PVOID ori_address = nullptr;

    Context.TargetPid = target_pid;
    Context.TargetAddress = target_address;
    Context.DetourAddress = detour_address;
    Context.OriAddress = &ori_address;

    return SendData(SHOTHV_HOOK_ADD, &Context);
}

// ��������Hook
bool VtWb::ShotHvHookEnable()
{
    HOOK_CONTEXT Context = { 0 };

    return SendData(SHOTHV_HOOK_ENABLE, &Context);
}

// �޸�Hook״̬
bool VtWb::ShotHvUpdateHookState(
    _In_ unsigned __int64 target_pid, 
    _In_ void* target_address, 
    _In_ unsigned __int64 state
)
{
    HOOK_CONTEXT Context = { 0 };

    Context.TargetPid = target_pid;
    Context.TargetAddress = target_address;
    Context.State = state;

    return SendData(SHOTHV_HOOK_DISABLE, &Context);
}

// ɾ��ָ��Ept Hook
bool VtWb::ShotHvDelHook(
    _In_ unsigned __int64 target_pid, 
    _In_ void* target_address
)
{
    HOOK_CONTEXT Context = { 0 };

    Context.TargetPid = target_pid;
    Context.TargetAddress = target_address;

    return SendData(SHOTHV_HOOK_DELETE, &Context);
}

// ��д
bool VtWb::ShotHvHideWrite(
    _In_ unsigned __int64 target_pid, 
    _In_ void* target_address, 
    _In_ void* code, 
    _In_ int size
)
{
    HOOK_CONTEXT Context = { 0 };

    Context.TargetPid = target_pid;
    Context.TargetAddress = target_address;
    Context.Code = code;
    Context.Size = size;

    return SendData(SHOTHV_HIDE_WRITE, &Context);
}

#include "pch.h"

PVOID OriFun;

void Test1()
{
    printf("Test1!\r\n");
}

void Test2()
{
    printf("Test2!\r\n");
}

int WINAPI DetourMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption,
    UINT uType) {

    VtWb::ShotHvUpdateHookState(GetCurrentProcessId(), &MessageBoxW, Stop);

    auto result = MessageBoxW(hWnd, L"Hooked!", lpCaption, uType);

    VtWb::ShotHvUpdateHookState(GetCurrentProcessId(), &MessageBoxW, Activiti);

    return result;
}

int main()
{
    if (VtWb::InitIoCtlComm())  // ��Ҫ�����м�������
    {
        UCHAR Int3 = { 0xCC };

        VtWb::ShotHvHideWrite(GetCurrentProcessId(), &MessageBoxW, &Int3, 1); // ��д
        VtWb::ShotHvHookEnable();                                             // ����Hook
        system("pause");
        MessageBoxW(NULL, L"Not hooked...", L"MinHook Sample", MB_OK);

        if (VtWb::ShotHvAddHook(GetCurrentProcessId(), &MessageBoxW, &DetourMessageBoxW)) // ���Hook
        {
            printf("ShotHvAddHook Success!\r\n");
            system("pause");

            VtWb::ShotHvUpdateHookState(GetCurrentProcessId(), Test1, Stop);  // ����Hook״̬
            VtWb::ShotHvDelHook(GetCurrentProcessId(), &MessageBoxW);         // ɾ��Hook
        }
        else
        {
            printf("Error1:%x!\r\n", GetLastError());
        }
    }
    else
    {
        printf("Error2:%x!\r\n", GetLastError());
    }

    system("pause");

    MessageBoxW(NULL, L"Not hooked...", L"MinHook Sample", MB_OK);
    MessageBoxW(NULL, L"Not hooked...", L"MinHook Sample", MB_OK);

DONE:
    printf("����!\r\n");
    CloseHandle(g_hDevice);
    system("pause");
    return 0;
}



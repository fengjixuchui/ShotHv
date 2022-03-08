#include "HvPch.h"

SSDT_ENTRY* SSDT;
SSDT_ENTRY* SSSDT;

/*
	See: ����SSDT�������ƻ�ȡ��Ӧ������ַ
	RETURN: ���ض�ӦSSDT������ַ
*/
_Use_decl_annotations_
PVOID
WINAPI
GetSsdtFunctionAddress(
	_In_ CHAR* ApiName	// Ҫ��ȡ��SSDT API����
)
{
	if (!SSDT) {
		SSDT = GetSstdEntry();
		if (!SSDT) {
			DBG_PRINT("SSDT not found...\r\n");
			return NULL;
		}
	}

	ULONG_PTR SSDTbase = (ULONG_PTR)SSDT->pServiceTable;
	if (!SSDTbase)
	{
		DBG_PRINT("ServiceTable not found...\r\n");
		return 0;
	}

	/* ��ȡϵͳ����� */
	ULONG Offset = GetSsdtFunctionIndex(ApiName);
	if (Offset == 0)
		return NULL;
	else if (Offset >= SSDT->NumberOfServices)
	{
		DBG_PRINT("Invalid Offset...\r\n");
		return 0;
	}

	return (PVOID)((SSDT->pServiceTable[Offset] >> 4) + SSDTbase);
}

/*
	See: ��ȡSSDT�����±�
	RETURN: ���ض�ӦSSDT�����±�
*/
_Use_decl_annotations_
ULONG
WINAPI
GetSsdtFunctionIndex(
	_In_ PCHAR funName	// SSDT API����
)
{
	ULONG ulFunctionIndex = 0;
	NTSTATUS status = STATUS_SUCCESS;
	HANDLE hFile = NULL;
	HANDLE hSection = NULL;
	PVOID pBaseAddress = NULL;
	WCHAR NtdllPath[MAX_PATH] = { 0 };
	UNICODE_STRING ustrDllFileName = { 0 };

	/* ��ȡ ntdll ·�� */
	wcscat(NtdllPath, L"\\??\\");
	wcscat(NtdllPath, ((KUSER_SHARED_DATA*)KI_USER_SHARED_DATA)->NtSystemRoot);
	wcscat(NtdllPath, L"\\System32\\ntdll.dll");

	RtlInitUnicodeString(&ustrDllFileName, NtdllPath);

	// �ڴ�ӳ���ļ�
	status = DllFileMap(ustrDllFileName, &hFile, &hSection, &pBaseAddress);
	if (!NT_SUCCESS(status))
	{
		DBG_PRINT("DllFileMap Error!!!\n");
		return ulFunctionIndex;
	}

	// ���ݵ������ȡ����������ַ, �Ӷ���ȡ����������
	ulFunctionIndex = GetIndexFromExportTable(pBaseAddress, funName);

	// �ͷ�
	ZwUnmapViewOfSection(NtCurrentProcess(), pBaseAddress);
	ZwClose(hSection);
	ZwClose(hFile);

	return ulFunctionIndex;
}

/*
	See: ����NTDLL�������ȡ����������ַ, �Ӷ���ȡ����������
	RETURN: ���ض�ӦSSDT����������
*/
_Use_decl_annotations_
ULONG
WINAPI
GetIndexFromExportTable(
	_In_ PVOID pBaseAddress,	// BASE
	_In_ PCHAR pszFunctionName	// Export Api Name
)
{
	ULONG ulFunctionIndex = 0;
	// Dos Header
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pBaseAddress;
	// NT Header
	PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((PUCHAR)pDosHeader + pDosHeader->e_lfanew);
	// Export Table
	PIMAGE_EXPORT_DIRECTORY pExportTable = (PIMAGE_EXPORT_DIRECTORY)((PUCHAR)pDosHeader + pNtHeaders->OptionalHeader.DataDirectory[0].VirtualAddress);
	// �����Ƶĵ�����������
	ULONG ulNumberOfNames = pExportTable->NumberOfNames;
	// �����������Ƶ�ַ��
	PULONG lpNameArray = (PULONG)((PUCHAR)pDosHeader + pExportTable->AddressOfNames);
	PCHAR lpName = NULL;
	// ��ʼ����������
	for (ULONG i = 0; i < ulNumberOfNames; i++)
	{
		lpName = (PCHAR)((PUCHAR)pDosHeader + lpNameArray[i]);
		// �ж��Ƿ���ҵĺ���
		if (0 == _strnicmp(pszFunctionName, lpName, strlen(pszFunctionName)))
		{
			// ��ȡ����������ַ
			USHORT uHint = *(USHORT*)((PUCHAR)pDosHeader + pExportTable->AddressOfNameOrdinals + 2 * i);
			ULONG ulFuncAddr = *(PULONG)((PUCHAR)pDosHeader + pExportTable->AddressOfFunctions + 4 * uHint);
			PVOID lpFuncAddr = (PVOID)((PUCHAR)pDosHeader + ulFuncAddr);
			// ��ȡ SSDT ���� Index
#ifdef _WIN64 // 64bit
			ulFunctionIndex = *(ULONG*)((PUCHAR)lpFuncAddr + 4);
#else		  // 32bits
			ulFunctionIndex = *(ULONG*)((PUCHAR)lpFuncAddr + 1);
#endif
			break;
		}
	}

	return ulFunctionIndex;
}

/*
	See: ��ȡSSDT��
	RETURN: ���� SSDT_ENTRY*
*/
_Use_decl_annotations_
SSDT_ENTRY*
WINAPI
GetSstdEntry()
{
	if (!SSDT)
	{
		// x64 code
		ULONG_PTR kernelSize = 0;
		ULONG_PTR kernelBase = QueryKernelModule((PUCHAR)"ntoskrnl.exe", &kernelSize);
		if (kernelBase == 0 || kernelSize == 0)
			return NULL;

		// Find KiSystemServiceStart
		const unsigned char KiSystemServiceStartPattern[] = { 0x8B, 0xF8, 0xC1, 0xEF, 0x07, 0x83, 0xE7, 0x20, 0x25, 0xFF, 0x0F, 0x00, 0x00 };
		const ULONG signatureSize = sizeof(KiSystemServiceStartPattern);
		bool found = false;
		ULONG KiSSSOffset;
		for (KiSSSOffset = 0; KiSSSOffset < kernelSize - signatureSize; KiSSSOffset++)
		{
			if (RtlCompareMemory(((unsigned char*)kernelBase + KiSSSOffset), KiSystemServiceStartPattern, signatureSize) == signatureSize)
			{
				found = true;
				break;
			}
		}
		if (!found)
			return NULL;

		// lea r10, KeServiceDescriptorTable
		ULONG_PTR address = kernelBase + KiSSSOffset + signatureSize;
		LONG relativeOffset = 0;
		if ((*(unsigned char*)address == 0x4c) &&
			(*(unsigned char*)(address + 1) == 0x8d) &&
			(*(unsigned char*)(address + 2) == 0x15))
		{
			relativeOffset = *(LONG*)(address + 3);
		}
		if (relativeOffset == 0)
			return NULL;

		SSDT = (SSDT_ENTRY*)(address + relativeOffset + 7);
	}

	return SSDT;
}

/*
	See: ����DLL���ƣ�ӳ��ָ���ļ�PE����
	RETURN: ������ȷ����STATUS_SUCCESS
*/
_Use_decl_annotations_
NTSTATUS
WINAPI
DllFileMap(
	_In_	UNICODE_STRING ustrDllFileName,	// DLL����
	_Inout_ HANDLE* phFile,					// ���ص��ļ����
	_Inout_ HANDLE* phSection,				// ���صĽ������
	_Inout_ PVOID* ppBaseAddress			// ����ӳ��ĵ�ַ
)
{
	NTSTATUS status = STATUS_SUCCESS;
	HANDLE hFile = NULL;
	HANDLE hSection = NULL;
	OBJECT_ATTRIBUTES objectAttributes = { 0 };
	IO_STATUS_BLOCK iosb = { 0 };
	PVOID pBaseAddress = NULL;
	SIZE_T viewSize = 0;

	// �� DLL �ļ�, ����ȡ�ļ����
	InitializeObjectAttributes(&objectAttributes, &ustrDllFileName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwOpenFile(&hFile, GENERIC_READ, &objectAttributes, &iosb,
		FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);
	if (!NT_SUCCESS(status))
	{
		DBG_PRINT("ZwOpenFile Error! [error code: 0x%X]\r\n", status);
		return status;
	}
	// ����һ���ڶ���, �� PE �ṹ�е� SectionALignment ��С����ӳ���ļ�
	status = ZwCreateSection(&hSection, SECTION_MAP_READ | SECTION_MAP_WRITE, NULL, 0, PAGE_READWRITE, 0x1000000, hFile);
	if (!NT_SUCCESS(status))
	{
		ZwClose(hFile);
		DBG_PRINT("ZwCreateSection Error! [error code: 0x%X]\r\n", status);
		return status;
	}
	// ӳ�䵽�ڴ�
	status = ZwMapViewOfSection(hSection, NtCurrentProcess(), &pBaseAddress, 0, 1024, 0, &viewSize, ViewShare, MEM_TOP_DOWN, PAGE_READWRITE);
	if (!NT_SUCCESS(status))
	{
		ZwClose(hSection);
		ZwClose(hFile);
		DBG_PRINT("ZwMapViewOfSection Error! [error code: 0x%X]\r\n", status);
		return status;
	}

	// ��������
	*phFile = hFile;
	*phSection = hSection;
	*ppBaseAddress = pBaseAddress;

	return status;
}
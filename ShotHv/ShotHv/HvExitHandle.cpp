#include "HvPch.h"

extern "C"
{
	void VmxExitHandler(_In_ GuestReg* Registers)
	{
		VmxExitInfo dwExitReason = { 0 };
		FlagReg GuestRflag = { 0 };

		dwExitReason.all = (ULONG32)VmxCsRead(VM_EXIT_REASON); // ��ȡ VM-exit ԭ��

		switch (dwExitReason.fields.reason)
		{
		case ExitExceptionOrNmi:	// ���� Nmi �ж�(��������)
			NmiExceptionVtExitHandler(Registers);
			break;
		case ExitExternalInterrupt: // �����ⲿ�ж�(������)
			break;
		case ExitCpuid:			// ���� cpuid
			CpuidVmExitHandler(Registers);
			break;
		case ExitRdtsc:			// ���� Rdtsc
			RdtscVtExitHandler(Registers);
			break;
		case ExitRdtscp:		// ���� Rdtscp
			RdtscpVtExitHandler(Registers);
			break;
		case ExitVmcall:		// ���� vmcall
			VmCallVmExitHandler(Registers);
			break;
		case ExitCrAccess:		// ���ط��� CrX �Ĵ���
			CrAccessVtExitHandler(Registers);
			break;
		case ExitMsrRead:		// ����msr�Ĵ�������,��������,��Ȼ�κη�msr�Ĳ������ᵼ��vmexit		
			MsrReadVtExitHandler(Registers);
			break;
		case ExitMsrWrite:		// ����msr�Ĵ��� д��	
			MsrWriteVtExitHandler(Registers);
			break;
		case ExitMonitorTrapFlag: // MTF �쳣
			MonitorTrapFlagVtExitHandler(Registers);
			break;
		case ExitGdtrOrIdtrAccess:	// ���� LGDT��LIDT��SGDT or SIDT ָ��
			break;
		case ExitLdtrOrTrAccess:	// ���� LLDT, LTR, SLDT, or STR ָ��
			break;
		case ExitEptViolation:		// EPT Violation ���µ� VM-EXIT
			EptViolationVtExitHandler(Registers);
			break;
		case ExitEptMisconfig:		// Ept ���ô���
			DBG_PRINT("ExitEptMisconfig!\r\n");
			DbgBreakPoint();
			break;
		case ExitTripleFault:		// 3���쳣,�����Ĵ���ֱ������;
			DBG_PRINT("ExitTripleFault 0x%llx!\r\n", VmxCsRead(GUEST_RIP));
			DbgBreakPoint();
			break;
		case ExitXsetbv:			// Win10 ���봦����ٻ���
			_xsetbv((ULONG32)Registers->Rcx, MAKEQWORD(Registers->Rax, Registers->Rdx));
			break;
		case ExitInvd:
			__wbinvd();
			break;
		case ExitInvpcid:			// ���������ܻ��õ�
			ExitInvpcidVtExitHandler(Registers);
			break;
		case ExitVmclear:			// ��ʵ��VTָ��, �ܾ� VT Ƕ��
		case ExitVmptrld:
		case ExitVmptrst:
		case ExitVmread:
		case ExitVmwrite:
		case ExitVmresume:
		case ExitVmoff:
		case ExitVmon:
		case ExitVmlaunch:
		case ExitVmfunc:
		case ExitInvept:
		case ExitInvvpid:
		{
			// ���� Rflags �� cf λ, ��Ϊ1(����ʧ��)
			GuestRflag.all = VmxCsRead(GUEST_RFLAGS);
			GuestRflag.fields.cf = 1;
			VmxCsWrite(GUEST_RFLAGS, GuestRflag.all);
			// ��Ĭ������
			DefaultVmExitHandler(Registers);
		}
		break;
		default:		// Ĭ������
			DefaultVmExitHandler(Registers);
			DBG_PRINT("δ֪�� VM_EIXT ԭ��:0x%x\n", dwExitReason.all);
			break;
		}
		return;
	}

	void CpuidVmExitHandler(_In_ GuestReg* Registers)
	{
		CpuId dwCpuidRegisters = { 0 };

		switch (Registers->Rax)
		{
		case 0x1:
		{
			CpuidFeatureByEcx CpuidEcx = { 0 };
			__cpuidex((int*)&dwCpuidRegisters, (int)Registers->Rax, (int)Registers->Rcx);
			CpuidEcx.all = (ULONG_PTR)dwCpuidRegisters.ecx;
			CpuidEcx.fields.vmx = false; // ��֧�����⻯
			dwCpuidRegisters.ecx = CpuidEcx.all;
			Registers->Rax = (ULONG_PTR)dwCpuidRegisters.eax;
			Registers->Rbx = (ULONG_PTR)dwCpuidRegisters.ebx;
			Registers->Rcx = (ULONG_PTR)dwCpuidRegisters.ecx;
			Registers->Rdx = (ULONG_PTR)dwCpuidRegisters.edx;
		}
		break;
		case CupidHvCheck:
		{
			// ������֤����
			Registers->Rax = 'SBTX';
		}
		break;
		default:
		{
			// Ĭ����������
			__cpuidex((int*)&dwCpuidRegisters, (int)Registers->Rax, (int)Registers->Rcx);
			Registers->Rax = (ULONG_PTR)dwCpuidRegisters.eax;
			Registers->Rbx = (ULONG_PTR)dwCpuidRegisters.ebx;
			Registers->Rcx = (ULONG_PTR)dwCpuidRegisters.ecx;
			Registers->Rdx = (ULONG_PTR)dwCpuidRegisters.edx;
		}
		break;
		}

		// ��Ĭ������
		DefaultVmExitHandler(Registers);
	}

	void MsrReadVtExitHandler(_In_ GuestReg* Registers)
	{
		ULONGLONG MsrValue = __readmsr((ULONG)Registers->Rcx);
		switch (Registers->Rcx)
		{
		case MSR_LSTAR: // ��ȡ MSR RIP
		{
		}
		break;
		case MSR_IA32_EFER:
		{
			// EFER HOOK ע�� Hypervisor Guard
		}
		break;
		case MSR_IA32_FEATURE_CONTROL:
		{
			// αװMSR��ϵͳ��ΪBIOS��û�п���VT-x
			Ia32FeatureControlMsr FeatureControlMsr = { 0 };
			FeatureControlMsr.all = MsrValue;
			FeatureControlMsr.fields.lock = false;
			FeatureControlMsr.fields.enable_vmxon = false;
			MsrValue = FeatureControlMsr.all;
		}
		break;
		default:
			// Ĭ��MSR��������
			break;
		}

		Registers->Rax = LODWORD(MsrValue);
		Registers->Rdx = HIDWORD(MsrValue);

		// ��Ĭ������
		DefaultVmExitHandler(Registers);

		return VOID();
	}

	void MsrWriteVtExitHandler(_In_ GuestReg* Registers)
	{
		ULONGLONG MsrValue = MAKEQWORD(Registers->Rax, Registers->Rdx);

		switch (Registers->Rcx)
		{
		case IA32_SYSENTER_EIP: // д�� MSR 0x176
		case IA32_SYSENTER_ESP: // д�� MSR 0x175
		case IA32_SYSENTER_CS:	// д�� MSR 0x174
		default:
		{
			// Ĭ����������
			__writemsr((ULONG)Registers->Rcx, MsrValue);
		}
		break;
		}

		// ��Ĭ������
		DefaultVmExitHandler(Registers);

		return VOID();
	}

	void VmCallVmExitHandler(_In_ GuestReg* Registers)
	{
		ULONG_PTR jmpRip = 0;
		ULONG_PTR GuestRIP = 0, GuestRSP = 0;
		ULONG_PTR ExitInstructionLength = 0;
		HvContextEntry* VmxEntry = NULL;

		GuestRIP = VmxCsRead(GUEST_RIP);
		GuestRSP = VmxCsRead(GUEST_RSP);
		ExitInstructionLength = VmxCsRead(VM_EXIT_INSTRUCTION_LEN);
		VmxEntry = GetHvContextEntry();

		switch (Registers->Rax)
		{
		case CallHookPage:		// R0 HOOK
		{
			auto pHvContext = GetHvContextEntry();
			EptUpdateTable(
				pHvContext->VmxEpt,
				EPT_ACCESS_EXEC,
				MmGetPhysicalAddress((PVOID)Registers->Rdx).QuadPart,
				Registers->R8
			);
			__invept(INV_ALL_CONTEXTS, &pHvContext->VmxEptp.Flags);
		}
		break;
		case CallUnHookPage:	// R0 UNHOOK
		{
			auto pHvContext = GetHvContextEntry();
			EptUpdateTable(
				pHvContext->VmxEpt,
				EPT_ACCESS_ALL,
				MmGetPhysicalAddress((PVOID)Registers->Rdx).QuadPart,
				Registers->R8
			);
			__invept(INV_ALL_CONTEXTS, &pHvContext->VmxEptp.Flags);
		}
		break;
		case CallExitVt: // �˳���ǰ���⻯
		{
			DBG_PRINT("�˳�Intel VT!\r\n");
			
			// ��ԭ�Ĵ���
			HvRestoreRegisters();

			// �� VMCS ��״̬���Ϊ�ǻ״̬
			if (__vmx_vmclear(&VmxEntry->VmxCsRegionPhyAddress)) {
				__vmx_off();
			}

			// �˳���ǰ���⻯
			__vmx_off();

			VmxEntry->VmxOnOFF = FALSE;

			jmpRip = GuestRIP + ExitInstructionLength; // Խ������ VM-EXIT ��ָ��

			// CR4.VMXE ��Ϊ 0
			__writecr4(__readcr4() & (~X86_CR4_VMXE));

			// �޸� Rsp\Rip ���ص� Guest ��
			AsmUpdateRspAndRip(GuestRSP, jmpRip);
		}
		break;
		default:
			break;
		}

		DefaultVmExitHandler(Registers);
	}

	void NmiExceptionVtExitHandler(_In_ GuestReg* Registers)
	{
		UNREFERENCED_PARAMETER(Registers);

		VmxExitInterruptInfo exception = { 0 }; // ����ֱ�������¼�
		InterruptionType interruption_type = InterruptionType::kExternalInterrupt; // Ĭ�ϳ�ʼ��
		InterruptionVector vector = InterruptionVector::EXCEPTION_VECTOR_DIVIDE_ERROR;
		ULONG32 error_code_valid = 0;

		/*
			"ֱ�������¼�" ��ֱָ������ VM-exit �������¼��������������֣�
			(1). Ӳ���쳣�������쳣���������� exception bitmap ��Ӧ��λΪ1��ֱ�ӵ��� VM-exit.
			(2). ����쳣(#BP��#OF)�������쳣���������� exception bitmap ��Ӧ��λΪ1��ֱ�ӵ��� VM-exit.
			(3). �ⲿ�жϣ������ⲿ�ж�����ʱ, ����"exception-interrupt exiting"Ϊ1��ֱ�ӵ��� VM-exit.
			(4). NMI������NMI����ʱ, ����"NMI exiting"Ϊ1��ֱ�ӵ��� VM-exit.
		*/

		// �����ж�ʱ, ��ȡ VM-Exit Interruption-Information �ֶ�
		exception.all = static_cast<ULONG32>(VmxCsRead(VM_EXIT_INTR_INFO));

		interruption_type = static_cast<InterruptionType>(exception.fields.interruption_type); // ��ȡ�ж�����
		vector = static_cast<InterruptionVector>(exception.fields.vector); // ��ȡ�ж�������
		error_code_valid = exception.fields.error_code_valid; // �Ƿ��д�����

		if (interruption_type == InterruptionType::kHardwareException)
		{
			// �����Ӳ���쳣, ����������ڴ���쳣
			if (vector == InterruptionVector::EXCEPTION_VECTOR_PAGE_FAULT)
			{

				// ���Ϊ #PF �쳣
				// exit qualification �ֶδ洢���� #PF �쳣�����Ե�ֵַ (�ο������������⻯������(��3.10.1.6��))
				auto fault_address = VmxCsRead(EXIT_QUALIFICATION);

				// VM-exit interruption error code �ֶ�ָ����� Page-Fault Error Code (�ο������������⻯������(��3.10.2��))
				PageFaultErrorCode fault_code = { 0 };
				fault_code.all = static_cast<ULONG32>(VmxCsRead(VM_EXIT_INTR_ERROR_CODE));

				// �ж��쳣�Ƿ���Hook���
				//auto ntStatus = intException::PfExceptionHandler(fault_address, fault_code);

				//if (!NT_SUCCESS(ntStatus))
				{
					// Ĭ�ϲ��޸ģ�����ע���ȥ
					InjectInterruption(interruption_type, vector, true, fault_code.all);

					// ע��ͬ�� cr2 �Ĵ���
					__writecr2(fault_address);

					VmxCsWrite(VM_ENTRY_INTR_INFO, exception.all);

					if (error_code_valid) {
						VmxCsWrite(VM_ENTRY_EXCEPTION_ERROR_CODE, VmxCsRead(VM_EXIT_INTR_ERROR_CODE));
					}
				}
			}
			else if (vector == InterruptionVector::EXCEPTION_VECTOR_GENERAL_PROTECTION) {
				// ���Ϊ #GP �쳣

				auto error_code = VmxCsRead(VM_EXIT_INTR_ERROR_CODE);

				// Ĭ�ϲ��޸ģ�����ע���ȥ
				InjectInterruption(interruption_type, vector, true, (ULONG32)error_code);

			}
			else if (vector == InterruptionVector::EXCEPTION_VECTOR_INVALID_OPCODE) {
				// ����� #UD �쳣

				// Ĭ��ע�� #UD		
				InjectInterruption(interruption_type, vector, false, 0);

			}
		}
		else if (interruption_type == InterruptionType::kSoftwareException) {
			// ����� ����쳣
			if (vector == InterruptionVector::EXCEPTION_VECTOR_BREAKPOINT)
			{
				// #BP
				// int3 ����������쳣, ע���ָ���г���

				// Ĭ�ϲ��޸ģ�����ע���ȥ
				InjectInterruption(interruption_type, vector, false, 0);
				auto exit_inst_length = VmxCsRead(VM_EXIT_INSTRUCTION_LEN); // ��ȡ���� VM-exit ��ָ���
				VmxCsWrite(VM_ENTRY_INSTRUCTION_LEN, exit_inst_length);
			}
		}
		else {
			VmxCsWrite(VM_ENTRY_INTR_INFO, exception.all);

			if (error_code_valid) {
				VmxCsWrite(VM_ENTRY_EXCEPTION_ERROR_CODE, VmxCsRead(VM_EXIT_INTR_ERROR_CODE));
			}
		}
	}

	void MonitorTrapFlagVtExitHandler(_In_ GuestReg* Registers)
	{
		UNREFERENCED_PARAMETER(Registers);

		DisableMTF();
	}

	void EptViolationVtExitHandler(_In_ GuestReg* Registers)
	{
		UNREFERENCED_PARAMETER(Registers);

		EptViolationHandler(Registers);
	}

	void RdtscVtExitHandler(_In_ GuestReg* Registers)
	{
		ULARGE_INTEGER tsc = { 0 };
		tsc.QuadPart = __rdtsc();
		Registers->Rdx = tsc.HighPart;
		Registers->Rax = tsc.LowPart;

		DefaultVmExitHandler(Registers);
	}

	void RdtscpVtExitHandler(_In_ GuestReg* Registers)
	{
		unsigned int tscAux = 0;
		ULARGE_INTEGER tsc = { 0 };
		tsc.QuadPart = __rdtscp(&tscAux);
		Registers->Rdx = tsc.HighPart;
		Registers->Rax = tsc.LowPart;
		Registers->Rcx = tscAux;

		DefaultVmExitHandler(Registers);
	}

	void CrAccessVtExitHandler(_In_ GuestReg* Registers)
	{
		CrxVmExitQualification CrxQualification = { 0 };
		CrxQualification.all = VmxCsRead(EXIT_QUALIFICATION); // ��ȡ�ֶ���Ϣ
		ULONG_PTR* pRegisters = (PULONG_PTR)Registers;

		switch (CrxQualification.Bits.access_type) {
		case MovCrAccessType::kMoveToCr: {
			switch (CrxQualification.Bits.crn) {
			case 0:
			{
				const Cr0Type cr0_fixed0 = { VmxCsRead(IA32_VMX_CR0_FIXED0) };
				const Cr0Type cr0_fixed1 = { VmxCsRead(IA32_VMX_CR0_FIXED1) };
				Cr0Type cr0 = { pRegisters[CrxQualification.Bits.gp_register] };
				cr0.all &= cr0_fixed1.all;
				cr0.all |= cr0_fixed0.all;
				VmxCsWrite(GUEST_CR0, cr0.all);
				VmxCsWrite(CR0_READ_SHADOW, cr0.all);
				break;
			}
			case 4: {
				const Cr4Type cr4_fixed0 = { VmxCsRead(IA32_VMX_CR4_FIXED0) };
				const Cr4Type cr4_fixed1 = { VmxCsRead(IA32_VMX_CR4_FIXED1) };
				Cr4Type cr4 = { pRegisters[CrxQualification.Bits.gp_register] };
				cr4.all &= cr4_fixed1.all;
				cr4.all |= cr4_fixed0.all;
				VmxCsWrite(GUEST_CR4, cr4.all);
				VmxCsWrite(CR4_READ_SHADOW, cr4.all);
				break;
			}
			}
			break;
		}
		}

		// ��Ĭ������
		DefaultVmExitHandler(Registers);
	}

	void ExitInvpcidVtExitHandler(_In_ GuestReg* Registers)
	{
		ULONG64 mrsp = 0;
		ULONG64 instinfo = 0;
		ULONG64 qualification = 0;
		__vmx_vmread(VMX_INSTRUCTION_INFO, &instinfo); //ָ����ϸ��Ϣ
		__vmx_vmread(EXIT_QUALIFICATION, &qualification); //ƫ����
		__vmx_vmread(GUEST_RSP, &mrsp);

		pInvpCid pinfo = (pInvpCid)&instinfo;

		ULONG64 base = 0;
		ULONG64 index = 0;
		ULONG64 scale = pinfo->scale ? 2 ^ pinfo->scale : 0;
		ULONG64 addr = 0;
		ULONG64 regopt = ((PULONG64)Registers)[pinfo->regOpt];;

		if (!pinfo->baseInvaild)
		{
			if (pinfo->base == 4)
			{
				base = mrsp;
			}
			else
			{
				base = ((PULONG64)Registers)[pinfo->base];
			}
		}

		if (!pinfo->indexInvaild)
		{
			if (pinfo->index == 4)
			{
				index = mrsp;
			}
			else
			{
				index = ((PULONG64)Registers)[pinfo->index];
			}
		}

		if (pinfo->addrssSize == 0)
		{
			addr = *(PSHORT)(base + index * scale + qualification);
		}
		else if (pinfo->addrssSize == 1)
		{
			addr = *(PULONG)(base + index * scale + qualification);
		}
		else
		{
			addr = *(PULONG64)(base + index * scale + qualification);
		}

		_invpcid((UINT)regopt, &addr);
		
		DefaultVmExitHandler(Registers);
	}

	void DefaultVmExitHandler(_In_ GuestReg* Registers)
	{
		ULONG_PTR GuestRip = VmxCsRead(GUEST_RIP);
		ULONG_PTR GuestRsp = VmxCsRead(GUEST_RSP);
		ULONG_PTR ExitInstructionLength = VmxCsRead(VM_EXIT_INSTRUCTION_LEN); // �˳���ָ���

		UNREFERENCED_PARAMETER(Registers);

		VmxCsWrite(GUEST_RIP, GuestRip + ExitInstructionLength);
		VmxCsWrite(GUEST_RSP, GuestRsp);
	}
}

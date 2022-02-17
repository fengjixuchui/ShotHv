#pragma once

extern "C"
{
	void VmxExitHandler(_In_ GuestReg* Registers);					// ���봦����
	void DefaultVmExitHandler(_In_ GuestReg* Registers);			// Ĭ�ϴ�����
	void CpuidVmExitHandler(_In_ GuestReg* Registers);				// CPUID ������
	void MsrWriteVtExitHandler(_In_ GuestReg* Registers);			// Msr Write ������
	void MsrReadVtExitHandler(_In_ GuestReg* Registers);			// Msr Read  ������
	void VmCallVmExitHandler(_In_ GuestReg* Registers);				// Vmcall �쳣������
	void NmiExceptionVtExitHandler(_In_ GuestReg* Registers);		// Nmi ������
	void MonitorTrapFlagVtExitHandler(_In_ GuestReg* Registers);	// MTF ������
	void EptViolationVtExitHandler(_In_ GuestReg* Registers);		// EPT ������
	void RdtscVtExitHandler(_In_ GuestReg* Registers);				// Rdtsc ������
	void RdtscpVtExitHandler(_In_ GuestReg* Registers);				// Rdtsc ������
	void CrAccessVtExitHandler(_In_ GuestReg* Registers);			// Crx ������
	void ExitInvpcidVtExitHandler(_In_ GuestReg* Registers);		// ExitInvpcid ������
}


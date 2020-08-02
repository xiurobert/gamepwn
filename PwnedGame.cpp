#include "PwnedGame.h"
#include <TlHelp32.h>

/**
 * Constructs a PwnedGame class with a process name
 * 
 * \param procName process name
 */
PwnedGame::PwnedGame(const wchar_t* procName) {
	_procName = procName;
}

/**
 * Retrieves the procId for the procName stored
 * 
 */
DWORD PwnedGame::getProcId() {
	
	if (_gameProcId == 0)
		_gameProcId = getProcId(_procName);

	return _gameProcId;

}

/**
 * \brief Retrieves the procId for a given procName
 * 
 * \static
 * \param procName process name
 */
DWORD PwnedGame::getProcId(const wchar_t* procName) {

	DWORD procId = 0;
	// Take snapshot of all processes running on system
	HANDLE tlSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	// Only proceed if there are no errors
	if (tlSnap != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32 procEntry;
		procEntry.dwSize = sizeof(procEntry);

		if (Process32First(tlSnap, &procEntry)) {
			// Loop through all processes, compare the name and see if it's equal to the exe file
			do {
				// wide char case insensitive string compare
				if (!_wcsicmp(procEntry.szExeFile, procName)) {
					procId = procEntry.th32ProcessID;
					break;
				}
			} while (Process32Next(tlSnap, &procEntry));
		}
	}
	// Manual memory management
	CloseHandle(tlSnap);
	return procId;
}

/**
 * \brief Retrieves module base address for the stored process name
 *
 * Used to retrieve module base for the main application
 *
 */
uintptr_t PwnedGame::getModBaseAddr() {
	if (_gameProcId == 0)
		getProcId();

	if (_modBaseAddr == 0)
		_modBaseAddr = getModBaseAddr(_gameProcId, _procName);

	return _modBaseAddr;
}

/**
 * \brief Retrieves module base address for a module name
 *
 * Supports both EXE and DLL modules, as long as it is attached to the process with
 * the stored process id \n
 * Stores the module base address in the class object. \n
 * This is fine if you intend to use it throughout, but not so good if you do not
 * intend to do so.
 *
 * \param modName name of the module
 */
uintptr_t PwnedGame::getModBaseAddr(const wchar_t* modName) {

	if (_gameProcId == 0)
		getProcId();
	
	if (_modBaseAddr == 0)
		_modBaseAddr = getModBaseAddr(_gameProcId, modName);
	
	return _modBaseAddr;
}

/**
 * \brief Retrieves module base addr for a procId and a mod name
 *
 * Supports both EXE and DLL modules \n
 * Modules must be attached to the process with the procId as specified
 * 
 * \static
 * \param procId process id
 * \param modName name of the module
 */
uintptr_t PwnedGame::getModBaseAddr(DWORD procId, const wchar_t* modName) {
	uintptr_t modBaseAddr = 0;
	HANDLE tlSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
	if (tlSnap != INVALID_HANDLE_VALUE) {
		MODULEENTRY32 modEntry;
		modEntry.dwSize = sizeof(modEntry);
		if (Module32First(tlSnap, &modEntry)) {
			do {
				if (!_wcsicmp(modEntry.szModule, modName)) {
					modBaseAddr = (uintptr_t) modEntry.modBaseAddr;
					break;
				}
			} while (Module32Next(tlSnap, &modEntry));
		}
	}
	CloseHandle(tlSnap);
	return modBaseAddr;
}

/**
 * \brief Opens a process
 *
 * DesiredAccess is given by the constants provided by Micro$oft \n
 * This will save the handle returned into this object \n
 * Desired access is retained as part of this handle
 *
 * \param desiredAccess Desired access
 */
HANDLE PwnedGame::openProc(DWORD desiredAccess) {
	if (_gameProcId == 0)
		getProcId();

	if (_hProc == 0) 
		_hProc = openProc(desiredAccess, _gameProcId);
	return _hProc;
}

/**
 * \brief Opens a process
 *
 * \static
 * \param desiredAccess Desired access
 */
HANDLE PwnedGame::openProc(DWORD desiredAccess, DWORD procId) {
	return OpenProcess(desiredAccess, NULL, procId);
}

/**
 * \brief Resolves a multi level pointer
 *
 * Uses the handle to the process stored in the class \n
 *  <bold> You need permissions to read the memory.</bold>
 *  
 *
 * \param basePtr base pointer (preferably static pointer)
 * \param offsets The offsets to compute
 */
uintptr_t PwnedGame::resolveMultiLvlPtr(uintptr_t basePtr, std::vector<unsigned int> offsets) {
	if (_hProc == 0)
		return 0;
	return resolveMultiLvlPtr(_hProc, basePtr, offsets);
}

/**
 * \brief Resolves a multi level pointer
 *
 * Uses the handle to the process stored in the class \n
 * Uses the module base address stored in the class\n
 *
 * \param offsets Offsets to compute
 */
uintptr_t PwnedGame::resolveMultiLvlPtr(std::vector<unsigned int> offsets) {
	if (_hProc == 0)
		return 0;
	if (_modBaseAddr == 0)
		return 0;

	return resolveMultiLvlPtr(_hProc, _modBaseAddr, offsets);
}

/**
 * \brief Resolves a multi level pointer
 *
 * \static
 * \param hProc Handle to process. You need permissions to read the memory.
 * \param basePtr Base pointer to go from
 * \param offsets Offsets to compute
 */
uintptr_t PwnedGame::resolveMultiLvlPtr(HANDLE hProc, uintptr_t basePtr, std::vector<unsigned int> offsets) {
	uintptr_t addr = basePtr;
	for (unsigned int i = 0; i < offsets.size(); ++i) {
		// ptr to base addr
		// output
		// 0: output for number of bytes read

		// Dereference pointer
		ReadProcessMemory(hProc, (BYTE*)addr, &addr, sizeof(addr), nullptr);
		// Add offset
		addr += offsets[i];

		//Dereferences next pointer
	}
	return addr;
}

/**
 * \brief ReadProcessMemory wrapper
 * Reads the memory of a process with the stored handle.
 * This is an overloaded method of the static function
 * with the same name.
 *
 * \param baseAddr Address to read from
 * \param output Pointer to output
 */
BOOL PwnedGame::rpm(LPCVOID baseAddr, LPVOID output) const {
	return rpm(_hProc, baseAddr, output);
}

/**
 * \brief ReadProcessMemory wrapper
 * Reads the memory of a process with HANDLE hProc
 * starting from baseAddress baseAddr.
 * Results are placed in pointer output. \n
 * Permissions to read memory of the process must be given in the handle\n
 *
 * size is automatically assumed to be sizeof(output) \n
 * number of bytes read is discarded
 * 
 * \static
 * \param hProc Handle to process
 * \param baseAddr Base address to read from
 * \param output Pointer to store output
 */
BOOL PwnedGame::rpm(HANDLE hProc, LPCVOID baseAddr, LPVOID output) {
	return ReadProcessMemory(hProc, baseAddr, output, sizeof(output), nullptr);
}

/**
 * \brief WriteProcessMemory wrapper
 * Writes to the memory of a process with the stored handle
 * This is an overloaded method of the static function
 * with the same name.
 *
 * \param baseAddr Address to write to
 * \param data Data to write
 */
BOOL PwnedGame::wpm(LPVOID baseAddr, LPCVOID data) const {
	return wpm(_hProc, baseAddr, data);
}

/**
 * \brief WriteProcessMemory wrapper
 * Writes memory to a process with HANDLE hProc
 * starting from the address baseAddr.
 * Data is written from the pointer data. \n
 * Permissions to write to the memory of the process must be given in the handle. \n
 *
 * size is automatically assumed to be the sizeof(data) \n
 * Number of bytes written is discarded
 *
 * \static
 * \param hProc Handle to process
 * \param baseAddr Base address to write to
 * \param data Data to write
 */
BOOL PwnedGame::wpm(HANDLE hProc, LPVOID baseAddr, LPCVOID data) {
	return WriteProcessMemory(hProc, baseAddr, data, sizeof(data), nullptr);;
}


void PwnedGame::patch(BYTE* dest, BYTE* src, unsigned size) const {
	patch(dest, src, size, _hProc);
}

void PwnedGame::patch(BYTE* dest, BYTE* src, unsigned size, HANDLE hProc) {
	DWORD oldProt;
	VirtualProtectEx(hProc, dest, size, PAGE_EXECUTE_READWRITE, &oldProt);
	WriteProcessMemory(hProc, dest, src, size, nullptr);
	VirtualProtectEx(hProc, dest, size, oldProt, &oldProt);
}

void PwnedGame::nop(BYTE* dest, unsigned size) const {
	nop(dest, size, _hProc);
}

void PwnedGame::nop(BYTE* dest, unsigned size, HANDLE hProc) {
	BYTE* nops = new BYTE[size];
	memset(nops, 0x90, size);

	patch(dest, nops, size, hProc);
	delete[] nops;
}

PwnedGame::~PwnedGame() {
	CloseHandle(_hProc);
}

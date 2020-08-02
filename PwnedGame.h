#pragma once
#include <Windows.h>
#include <vector>

class PwnedGame {
private:
	const wchar_t* _procName;
	DWORD _gameProcId{ 0 };
	HANDLE _hProc{ nullptr };

	uintptr_t _modBaseAddr{ 0 };


public:
	PwnedGame(const wchar_t* procName);
	~PwnedGame();

	DWORD getProcId();
	static DWORD getProcId(const wchar_t* procName);

	uintptr_t getModBaseAddr();
	uintptr_t getModBaseAddr(const wchar_t* modName);
	static uintptr_t getModBaseAddr(DWORD procId, const wchar_t* modName);

	HANDLE openProc(DWORD desiredAccess);
	static HANDLE openProc(DWORD desiredAccess, DWORD procId);

	uintptr_t resolveMultiLvlPtr(uintptr_t basePtr, std::vector<unsigned> offsets);
	uintptr_t resolveMultiLvlPtr(std::vector<unsigned> offsets);
	static uintptr_t resolveMultiLvlPtr(HANDLE hProc, uintptr_t basePtr, std::vector<unsigned> offsets);

	BOOL rpm(LPCVOID baseAddr, LPVOID output) const;
	static BOOL rpm(HANDLE hProc, LPCVOID baseAddr, LPVOID output);

	BOOL wpm(LPVOID baseAddr, LPCVOID data) const;
	static BOOL wpm(HANDLE hProc, LPVOID baseAddr, LPCVOID data);

	void patch(BYTE* dest, BYTE* src, unsigned size) const;
	static void patch(BYTE* dest, BYTE* src, unsigned size, HANDLE hProc);

	void nop(BYTE* dest, unsigned size) const;
	static void nop(BYTE* dest, unsigned size, HANDLE hProc);
};

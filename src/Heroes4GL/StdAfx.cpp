/*
	MIT License

	Copyright (c) 2019 Oleksiy Ryabchun

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#include "stdafx.h"

HMODULE hDllModule;
HANDLE hActCtx;

DIRECTDRAWCREATEEX DDCreateEx;

CREATEACTCTXA CreateActCtxC;
RELEASEACTCTX ReleaseActCtxC;
ACTIVATEACTCTX ActivateActCtxC;
DEACTIVATEACTCTX DeactivateActCtxC;

MALLOC MemoryAlloc;
FREE MemoryFree;
MEMSET MemorySet;
MEMCPY MemoryCopy;
CEIL MathCeil;
FLOOR MathFloor;
ROUND MathRound;
SPRINTF StrPrint;
STRCMP StrCompare;
STRCPY StrCopy;
STRCAT StrCat;
STRCHR StrChar;
STRRCHR StrLastChar;
STRSTR StrStr;
WCSTOMBS StrToAnsi;
FOPEN FileOpen;
FCLOSE FileClose;
EXIT Exit;

DWORD
	pAcquireDDThreadLock,
	pCompleteCreateSysmemSurface,
	pD3DParseUnknownCommand,
	pDDGetAttachedSurfaceLcl,
	pDDInternalLock,
	pDDInternalUnlock,
	pDSoundHelp,
	pDirectDrawCreate,
	pDirectDrawCreateClipper,
	pDirectDrawEnumerateA,
	pDirectDrawEnumerateExA,
	pDirectDrawEnumerateExW,
	pDirectDrawEnumerateW,
	pDllCanUnloadNow,
	pDllGetClassObject,
	pGetDDSurfaceLocal,
	pGetOLEThunkData,
	pGetSurfaceFromDC,
	pRegisterSpecialCase,
	pReleaseDDThreadLock,
	pSetAppCompatData;

VOID _declspec(naked) __stdcall exAcquireDDThreadLock() { _asm { JMP pAcquireDDThreadLock } }
VOID _declspec(naked) __stdcall exCompleteCreateSysmemSurface() { _asm { JMP pCompleteCreateSysmemSurface } }
VOID _declspec(naked) __stdcall exD3DParseUnknownCommand() { _asm { JMP pD3DParseUnknownCommand } }
VOID _declspec(naked) __stdcall exDDGetAttachedSurfaceLcl() { _asm { JMP pDDGetAttachedSurfaceLcl } }
VOID _declspec(naked) __stdcall exDDInternalLock() { _asm { JMP pDDInternalLock } }
VOID _declspec(naked) __stdcall exDDInternalUnlock() { _asm { JMP pDDInternalUnlock } }
VOID _declspec(naked) __stdcall exDSoundHelp() { _asm { JMP pDSoundHelp } }
VOID _declspec(naked) __stdcall exDirectDrawCreate() { _asm { JMP pDirectDrawCreate } }
VOID _declspec(naked) __stdcall exDirectDrawCreateClipper() { _asm { JMP pDirectDrawCreateClipper } }
VOID _declspec(naked) __stdcall exDirectDrawCreateEx() { _asm { JMP DDCreateEx } }
VOID _declspec(naked) __stdcall exDirectDrawEnumerateA() { _asm { JMP pDirectDrawEnumerateA } }
VOID _declspec(naked) __stdcall exDirectDrawEnumerateExA() { _asm { JMP pDirectDrawEnumerateExA } }
VOID _declspec(naked) __stdcall exDirectDrawEnumerateExW() { _asm { JMP pDirectDrawEnumerateExW } }
VOID _declspec(naked) __stdcall exDirectDrawEnumerateW() { _asm { JMP pDirectDrawEnumerateW } }
VOID _declspec(naked) __stdcall exDllCanUnloadNow() { _asm { JMP pDllCanUnloadNow } }
VOID _declspec(naked) __stdcall exDllGetClassObject() { _asm { JMP pDllGetClassObject } }
VOID _declspec(naked) __stdcall exGetDDSurfaceLocal() { _asm { JMP pGetDDSurfaceLocal } }
VOID _declspec(naked) __stdcall exGetOLEThunkData() { _asm { JMP pGetOLEThunkData } }
VOID _declspec(naked) __stdcall exGetSurfaceFromDC() { _asm { JMP pGetSurfaceFromDC } }
VOID _declspec(naked) __stdcall exRegisterSpecialCase() { _asm { JMP pRegisterSpecialCase } }
VOID _declspec(naked) __stdcall exReleaseDDThreadLock() { _asm { JMP pReleaseDDThreadLock } }
VOID _declspec(naked) __stdcall exSetAppCompatData() { _asm { JMP pSetAppCompatData } }

double __cdecl round(double number)
{
	double floorVal = MathFloor(number);
	return floorVal + 0.5f > number ? floorVal : MathCeil(number);
}

VOID LoadKernel32()
{
	HMODULE hLib = GetModuleHandle("KERNEL32.dll");
	if (hLib)
	{
		CreateActCtxC = (CREATEACTCTXA)GetProcAddress(hLib, "CreateActCtxA");
		ReleaseActCtxC = (RELEASEACTCTX)GetProcAddress(hLib, "ReleaseActCtx");
		ActivateActCtxC = (ACTIVATEACTCTX)GetProcAddress(hLib, "ActivateActCtx");
		DeactivateActCtxC = (DEACTIVATEACTCTX)GetProcAddress(hLib, "DeactivateActCtx");
	}
}

VOID LoadMsvCRT()
{
	HMODULE hLib = LoadLibrary("MSVCRT.dll");
	if (hLib)
	{
		StrPrint = (SPRINTF)GetProcAddress(hLib, "sprintf");

		CHAR libName[MAX_PATH];
		for (DWORD i = 12; i >= 7; --i)
		{
			StrPrint(libName, "MSVCR%d0.dll", i);
			HMODULE hCrtLib = LoadLibrary(libName);
			if (hCrtLib)
			{
				FreeLibrary(hLib);
				hLib = hCrtLib;
				StrPrint = (SPRINTF)GetProcAddress(hLib, "sprintf");
				break;
			}
		}

		MemoryAlloc = (MALLOC)GetProcAddress(hLib, "malloc");
		MemoryFree = (FREE)GetProcAddress(hLib, "free");
		MemorySet = (MEMSET)GetProcAddress(hLib, "memset");
		MemoryCopy = (MEMCPY)GetProcAddress(hLib, "memcpy");

		MathCeil = (CEIL)GetProcAddress(hLib, "ceil");
		MathFloor = (FLOOR)GetProcAddress(hLib, "floor");
		MathRound = (ROUND)GetProcAddress(hLib, "round");
		if (!MathRound)
			MathRound = round;

		StrCompare = (STRCMP)GetProcAddress(hLib, "strcmp");
		StrCopy = (STRCPY)GetProcAddress(hLib, "strcpy");
		StrCat = (STRCAT)GetProcAddress(hLib, "strcat");
		StrChar = (STRCHR)GetProcAddress(hLib, "strchr");
		StrLastChar = (STRRCHR)GetProcAddress(hLib, "strrchr");
		StrStr = (STRSTR)GetProcAddress(hLib, "strstr");
		StrToAnsi = (WCSTOMBS)GetProcAddress(hLib, "wcstombs");

		FileOpen = (FOPEN)GetProcAddress(hLib, "fopen");
		FileClose = (FCLOSE)GetProcAddress(hLib, "fclose");

		Exit = (EXIT)GetProcAddress(hLib, "exit");
	}
}

VOID LoadDDraw()
{
	CHAR dir[MAX_PATH];
	if (GetSystemDirectory(dir, MAX_PATH))
	{
		StrCat(dir, "\\DDRAW.dll");
		HMODULE hLib = LoadLibrary(dir);
		if (hLib)
		{
			pAcquireDDThreadLock = (DWORD)GetProcAddress(hLib, "AcquireDDThreadLock");
			pCompleteCreateSysmemSurface = (DWORD)GetProcAddress(hLib, "CompleteCreateSysmemSurface");
			pD3DParseUnknownCommand = (DWORD)GetProcAddress(hLib, "D3DParseUnknownCommand");
			pDDGetAttachedSurfaceLcl = (DWORD)GetProcAddress(hLib, "DDGetAttachedSurfaceLcl");
			pDDInternalLock = (DWORD)GetProcAddress(hLib, "DDInternalLock");
			pDDInternalUnlock = (DWORD)GetProcAddress(hLib, "DDInternalUnlock");
			pDSoundHelp = (DWORD)GetProcAddress(hLib, "DSoundHelp");
			pDirectDrawCreate = (DWORD)GetProcAddress(hLib, "DirectDrawCreate");
			pDirectDrawCreateClipper = (DWORD)GetProcAddress(hLib, "DirectDrawCreateClipper");
			DDCreateEx = (DIRECTDRAWCREATEEX)GetProcAddress(hLib, "DirectDrawCreateEx");
			pDirectDrawEnumerateA = (DWORD)GetProcAddress(hLib, "DirectDrawEnumerateA");
			pDirectDrawEnumerateExA = (DWORD)GetProcAddress(hLib, "DirectDrawEnumerateExA");
			pDirectDrawEnumerateExW = (DWORD)GetProcAddress(hLib, "DirectDrawEnumerateExW");
			pDirectDrawEnumerateW = (DWORD)GetProcAddress(hLib, "DirectDrawEnumerateW");
			pDllCanUnloadNow = (DWORD)GetProcAddress(hLib, "DllCanUnloadNow");
			pDllGetClassObject = (DWORD)GetProcAddress(hLib, "DllGetClassObject");
			pGetDDSurfaceLocal = (DWORD)GetProcAddress(hLib, "GetDDSurfaceLocal");
			pGetOLEThunkData = (DWORD)GetProcAddress(hLib, "GetOLEThunkData");
			pGetSurfaceFromDC = (DWORD)GetProcAddress(hLib, "GetSurfaceFromDC");
			pRegisterSpecialCase = (DWORD)GetProcAddress(hLib, "RegisterSpecialCase");
			pReleaseDDThreadLock = (DWORD)GetProcAddress(hLib, "ReleaseDDThreadLock");
			pSetAppCompatData = (DWORD)GetProcAddress(hLib, "SetAppCompatData");
		}
	}
}
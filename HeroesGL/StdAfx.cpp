/*
	MIT License

	Copyright (c) 2018 Oleksiy Ryabchun

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

CREATEACTCTXA CreateActCtxC;
RELEASEACTCTX ReleaseActCtxC;
ACTIVATEACTCTX ActivateActCtxC;
DEACTIVATEACTCTX DeactivateActCtxC;

MALLOC MemoryAlloc;
FREE MemoryFree;
MEMSET MemorySet;
CEIL MathCeil;
FLOOR MathFloor;
ROUND MathRound;
SPRINTF StrPrint;
STRSTR StrStr;
WCSTOMBS StrToAnsi;
EXIT Exit;

DWORD
	pWinGRecommendDIBFormat,
	pWinGSetDIBColorTable,
	pWinGStretchBlt,
	pWinGBitBlt,
	pWinGCreateDC,
	pWinGCreateBitmap;

VOID _declspec(naked) __stdcall WinGRecommendDIBFormat() { _asm { JMP pWinGRecommendDIBFormat } }
VOID _declspec(naked) __stdcall WinGSetDIBColorTable() { _asm { JMP pWinGSetDIBColorTable } }
VOID _declspec(naked) __stdcall WinGStretchBlt() { _asm { JMP pWinGStretchBlt } }
VOID _declspec(naked) __stdcall WinGBitBlt() { _asm { JMP pWinGBitBlt } }
VOID _declspec(naked) __stdcall WinGCreateDC() { _asm { JMP pWinGCreateDC } }
VOID _declspec(naked) __stdcall WinGCreateBitmap() { _asm { JMP pWinGCreateBitmap } }

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

		CHAR libName[256];
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

		MathCeil = (CEIL)GetProcAddress(hLib, "ceil");
		MathFloor = (FLOOR)GetProcAddress(hLib, "floor");
		MathRound = (ROUND)GetProcAddress(hLib, "round");
		if (!MathRound)
			MathRound = round;

		StrStr = (STRSTR)GetProcAddress(hLib, "strstr");
		StrToAnsi = (WCSTOMBS)GetProcAddress(hLib, "wcstombs");
	}
}

VOID LoadWinG32()
{
	CHAR dir[MAX_PATH];
	if (GetSystemDirectory(dir, MAX_PATH))
	{
		strcat(dir, "\\WING32.dll");
		HMODULE hLib = LoadLibrary(dir);
		if (hLib)
		{
			pWinGRecommendDIBFormat = (DWORD)GetProcAddress(hLib, "WinGRecommendDIBFormat");
			pWinGSetDIBColorTable = (DWORD)GetProcAddress(hLib, "WinGSetDIBColorTable");
			pWinGStretchBlt = (DWORD)GetProcAddress(hLib, "WinGStretchBlt");
			pWinGBitBlt = (DWORD)GetProcAddress(hLib, "WinGBitBlt");
			pWinGCreateDC = (DWORD)GetProcAddress(hLib, "WinGCreateDC");
			pWinGCreateBitmap = (DWORD)GetProcAddress(hLib, "WinGCreateBitmap");
		}
	}
}
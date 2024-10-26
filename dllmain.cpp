// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#include <Windows.h>
#include <WinUser.h>
#include <WinBase.h>
#include <debugapi.h>
#include <wchar.h>
#include "dllmain.h"

static void DbgPrint(const wchar_t* message)
{
	wchar_t buffer[120];

	wcscpy_s(buffer, L"[ELDEN_OPTIMIZE] ");
	wcscat_s(buffer, 100, message);

	OutputDebugString(buffer);
}

// get the process ID associated with the foun window
// if it's the same as the one containing the DLL 
// then the game's main window is found 
static BOOL CALLBACK EnumWindowsCallBack(HWND hwnd, LPARAM ptrHWND)
{
	DWORD processId;

	GetWindowThreadProcessId(hwnd, &processId);

	if (processId == GetCurrentProcessId())
	{
		DbgPrint(L"Window found");
		*((HWND*)ptrHWND) = hwnd;
		return false;
	}
	return true;
}

typedef enum Profiles
{
	DefaultProfile = 1,
	GamingProfile,
}Profile;

static void SwitchAfterBurnerProfile(Profile profile)
{
	// the following code basically configures "inputs" to contain:
	// inputs[0] = ctrl key down
	// inputs[1] = shift key down
	// inputs[2] = F1 or F2 key down
	// inputs[3] = ctrl key up
	// inputs[4] = shift key up
	// inputs[5] = F1 or F2 key up
	
	INPUT inputs[6] = {};
	ZeroMemory(inputs, sizeof(inputs));

	// configure all inputs as keyboard
	// make the 3 last inputs as keyup (first one are keydown)
	for (int i = 0; i < ARRAYSIZE(inputs); i++)
	{
		inputs[i].type = INPUT_KEYBOARD;
		if (i > 2)
			inputs[i].ki.dwFlags = KEYEVENTF_KEYUP;
	}

	// first and fourth key is control
	// second and fifth key is shift
	inputs[0].ki.wVk = inputs[3].ki.wVk = VK_CONTROL;
	inputs[1].ki.wVk = inputs[4].ki.wVk = VK_SHIFT;


	// third and sixth input is either F1 or F2 (depends on the selected profile)
	switch (profile)
	{
	case DefaultProfile:
		inputs[2].ki.wVk = inputs[5].ki.wVk = VK_F1;
		DbgPrint(L"Switching to default profile");
		break;
	case GamingProfile:
		inputs[2].ki.wVk = inputs[5].ki.wVk = VK_F2;
		DbgPrint(L"Switching to gaming profile");
		break;
	default:
		break;
	}
	wchar_t buffer[100];
	auto result =  SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
	wsprintf(buffer, L"SendInput result %d", result);
	DbgPrint(buffer);

}

// keep checking if the previously foun main window is still valid
// otherwise it has been closed
void WaitUntilWindowIsClosed(HWND hwnd)
{
	DWORD currentProcessId = GetCurrentProcessId();
	DWORD windowProcessId;

	DbgPrint(L"Waiting for game window to close");
	do
	{
		windowProcessId = 0;
		GetWindowThreadProcessId(hwnd, &windowProcessId);
	} while (windowProcessId == currentProcessId);
	DbgPrint(L"Game window closed");
}

// disable the usage of the first core of the cpu
void SetAffinity()
{
	if (auto hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, GetCurrentProcessId()))
	{
		unsigned long long processAffinity;
		unsigned long long systemAffinity;
		GetProcessAffinityMask(hProcess, &processAffinity, &systemAffinity);
		processAffinity &= ~1;
		SetProcessAffinityMask(hProcess, processAffinity);
		CloseHandle(hProcess);
		DbgPrint(L"Affinity set");
	}
	else
	{
		DbgPrint(L"Couldn't open process, affinity NOT set");
	}
}

HWND WaitForMainWindow()
{
	DbgPrint(L"Waiting for game's main window");
	HWND hwnd = 0;
	do
	{
		EnumWindows(EnumWindowsCallBack, (LPARAM)&hwnd);
	} while (hwnd == 0);
	return hwnd;
}

unsigned long MainLoop(LPVOID arg)
{
	DbgPrint(L"Started, waiting for 5s for stability");
	Sleep(5000);

	auto hwnd = WaitForMainWindow();

	SetAffinity();

	SwitchAfterBurnerProfile(GamingProfile);

	WaitUntilWindowIsClosed(hwnd);

	SwitchAfterBurnerProfile(DefaultProfile);

	return 1;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CreateThread(0, 0, MainLoop, 0, 0, 0);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


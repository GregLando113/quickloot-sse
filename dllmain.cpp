#include <string>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdio>
#include "skse64/PluginAPI.h"


#include "dbg.h"
#include "QuickLoot.h"

IDebugLog gLog("qldbg.txt");

SKSEMessagingInterface* g_messaging;
SKSEScaleformInterface* g_scaleform;
SKSETaskInterface*      g_tasks;

EventDispatcher<SKSECrosshairRefEvent>* g_crosshaireventdispatcher;


#define REQUIRE(expr) if(! (expr) ) return false;


extern "C"
{
	__declspec(dllexport) bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
	{
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name 		  = "QuickLoot SSE";
		info->version     = 1;

		return true;
	}

	__declspec(dllexport) bool SKSEPlugin_Load(const SKSEInterface * skse)
	{

		g_messaging = (SKSEMessagingInterface*)skse->QueryInterface(kInterface_Messaging);
		g_scaleform = (SKSEScaleformInterface*)skse->QueryInterface(kInterface_Scaleform);
		g_tasks     = (SKSETaskInterface*)     skse->QueryInterface(kInterface_Task);

		g_quickloot.Initialize();

		printf("SKSEPlugin_Load skse v. = %X\n", skse->skseVersion);
		TESObjectREFR* r = nullptr;
		printf("GetBaseScale = %p\n", r->_GetBaseScale_GetPtr());
		printf("IsOffLimits = %p\n", r->_IsOffLimits_GetPtr());

		return true;
	}
};

#if BLD_DEBUG
BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		AllocConsole();
		freopen("CONOUT$", "w", stdout);
		SetWindowPos(GetConsoleWindow(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		FreeConsole();
	}
	return TRUE;
}
#endif
#include <string>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdio>
#include "skse64/PluginAPI.h"
#include "skse64/gamethreads.h"

#include "dbg.h"
#include "QuickLoot.h"

IDebugLog gLog("qldbg.txt");

PluginHandle			g_pluginHandle = kPluginHandle_Invalid;

SKSEMessagingInterface* g_messaging;
SKSEScaleformInterface* g_scaleform;
SKSETaskInterface*      g_tasks;

bool isDisabled = false;
bool isOpen = false;

#define REQUIRE(expr) if(! (expr) ) return false;

class DelayedUpdater : public TaskDelegate
{
public:
	virtual void Run() override
	{
		if (g_quickloot)
		{
			g_quickloot->Update();
		}
	}
	virtual void Dispose() override
	{
	}

	static void Register()
	{
		static DelayedUpdater singleton;
		g_tasks->AddTask(&singleton);
	}
};

class OnCrosshairRefChanged : public BSTEventSink<SKSECrosshairRefEvent>
{
public:
	virtual EventResult ReceiveEvent(SKSECrosshairRefEvent *evn, EventDispatcher<SKSECrosshairRefEvent> * dispatcher)
	{
		if (!g_quickloot || !isDisabled)
		{
			BSFixedString s("LootMenu");
			if (evn->crosshairRef )//&& LootMenu::CanOpen(evn->crosshairRef))
			{
				if (isOpen && evn->crosshairRef != g_quickloot->containerRef)
				{
					//if (g_quickloot->containerRef_->formType != kFormType_Character)
					//	g_quickloot->PlayAnimationClose();

					g_quickloot->containerRef = nullptr;
					CALL_MEMBER_FN(UIManager::GetSingleton(), AddMessage)(&s, UIMessage::kMessage_Close, NULL);
					CALL_MEMBER_FN(UIManager::GetSingleton(), AddMessage)(&s, UIMessage::kMessage_Open, NULL);
					isOpen = true;
					g_quickloot->containerRef = evn->crosshairRef;
				}
				else
				{
					CALL_MEMBER_FN(UIManager::GetSingleton(), AddMessage)(&s, UIMessage::kMessage_Open, NULL);
					isOpen = true;
					g_quickloot->containerRef = evn->crosshairRef;
				}
			}
			else
			{
				if (isOpen)
				{
					//if (g_quickloot->containerRef_->formType != kFormType_Character)
					//	g_quickloot->PlayAnimationClose();

					g_quickloot->containerRef = nullptr;
					isOpen = false;
					CALL_MEMBER_FN(UIManager::GetSingleton(), AddMessage)(&s, UIMessage::kMessage_Close, NULL);
				}
			}
		}

		return kEvent_Continue;
	}
};

class LootMenuEventHandler : public BSTEventSink<MenuOpenCloseEvent>
{
public:
	virtual EventResult ReceiveEvent(MenuOpenCloseEvent *evn, EventDispatcher<MenuOpenCloseEvent> * dispatcher)
	{
		SimpleLocker lock(&QuickLoot::tlock);
		if (isOpen && !isDisabled)
		{
			UIStringHolder *holder = UIStringHolder::GetSingleton();
			BSFixedString s("LootMenu");
			MenuManager *mm = MenuManager::GetSingleton();
			if (!mm)
				return kEvent_Continue;

			GFxMovieView * view = mm->GetMovieView(&s);
			if (!view)
				return kEvent_Continue;

			if (evn->opening)
			{
				IMenu *menu = mm->GetMenu(&evn->menuName);
				if (menu)
				{
					if ((menu->flags & 0x4000) != 0 && (evn->menuName != holder->tweenMenu))
					{
						if (g_quickloot)
							g_quickloot->Close();
					}
					else if ((menu->flags & 0x1) != 0)
					{
						view->Unk_08(false);
					}
				}
			}
			else if (!evn->opening)
			{
				if (/*mm->GetNumPauseGame() == 0 &&*/ view->Unk_09() == false)
				{
					view->Unk_08(true);

					if (g_quickloot && g_quickloot->flags.GetAll(kQuickLoot_RequestUpdate))
					{
						_MESSAGE("UPDATING CONTAINER");
						_MESSAGE("    %p %s", g_quickloot->containerRef->formID, CALL_MEMBER_FN(g_quickloot->containerRef, GetReferenceName)());

						DelayedUpdater::Register();
					}
				}
				if (evn->menuName == holder->containerMenu)
				{
					if (g_quickloot)
						g_quickloot->flags.UnSet(kQuickLoot_OpenAnimation);
				}
			}
		}

		return kEvent_Continue;
	}
};

class ContainerChangedEventHandler : public BSTEventSink<TESContainerChangedEvent>
{
public:
	virtual EventResult ReceiveEvent(TESContainerChangedEvent *evn, EventDispatcher<TESContainerChangedEvent> * dispatcher)
	{
		SimpleLocker lock(&QuickLoot::tlock);
		if (g_quickloot && g_quickloot->containerRef && !isDisabled)
		{
			if (!g_quickloot || g_quickloot->flags.GetAll(kQuickLoot_NowTaking))
				return kEvent_Continue;

			UInt32 containerFormID = g_quickloot->containerRef->formID;
			if (evn->fromFormId == containerFormID || evn->toFormId == containerFormID)
			{
				if (!g_quickloot->flags.GetAll(kQuickLoot_RequestUpdate))
				{
					_MESSAGE("CONTAINER HAS BEEN UPDATED WITHOUT QUICK LOOTING");
					_MESSAGE("    %p %s\n", containerFormID, CALL_MEMBER_FN(g_quickloot->containerRef, GetReferenceName)());
				}

				MenuManager *mm = MenuManager::GetSingleton();
				if (!mm)
					return kEvent_Continue;


				DelayedUpdater::Register();
			}
		}

		return kEvent_Continue;
	}
};

OnCrosshairRefChanged			g_onCrosshairRefChanged;
LootMenuEventHandler			g_lootMenuEventHandler;
ContainerChangedEventHandler	g_containerChangedEventHandler;


void MessageHandler(SKSEMessagingInterface::Message * msg)
{
	switch (msg->type)
	{
	case SKSEMessagingInterface::kMessage_InputLoaded:
	{
		D_MSG("SKSEMessagingInterface::kMessage_InputLoaded");
		QuickLoot::Initialize();


		auto crosshairrefdispatch = (EventDispatcher<SKSECrosshairRefEvent>*)g_messaging->GetEventDispatcher(SKSEMessagingInterface::kDispatcher_CrosshairEvent);
		crosshairrefdispatch->AddEventSink(&g_onCrosshairRefChanged);

		MenuManager *mm = MenuManager::GetSingleton();
		mm->MenuOpenCloseEventDispatcher()->AddEventSink(&g_lootMenuEventHandler);

		GetEventDispatcherList()->unk370.AddEventSink(&g_containerChangedEventHandler);
	}
	break;
	}
}


extern "C"
{
	__declspec(dllexport) bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
	{
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name 		  = "QuickLoot SSE";
		info->version     = 1;

		g_pluginHandle = skse->GetPluginHandle();

		return true;
	}

	__declspec(dllexport) bool SKSEPlugin_Load(const SKSEInterface * skse)
	{

		g_messaging = (SKSEMessagingInterface*)skse->QueryInterface(kInterface_Messaging);
		g_scaleform = (SKSEScaleformInterface*)skse->QueryInterface(kInterface_Scaleform);
		g_tasks     = (SKSETaskInterface*)     skse->QueryInterface(kInterface_Task);

		g_messaging->RegisterListener(g_pluginHandle, "SKSE", MessageHandler);

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
#pragma once

#include <vector>

#include "skse64/GameReferences.h"
#include "skse64/GameAPI.h"
#include "skse64/GameEvents.h"
#include "skse64/PapyrusEvents.h"
#include "skse64/GameTypes.h"
#include "skse64/GameMenus.h"
#include "skse64/ScaleformLoader.h"

#include "ItemData.h"
#include "Flags.h"


class QuickLootMenu
	: public IMenu
{
public:

	static IMenu* Create(void);
	QuickLootMenu(const char* swfPath);
};

class QuickLoot
	: public BSTEventSink<SKSECrosshairRefEvent>
{
public:

	void Initialize();

	void Update();
	void Sort();

	void Dbg_PrintItems();

	IMenu*             menu;
private:

	void InvokeScaleform_Open();
	void InvokeScaleform_Close();
	void InvokeScaleform_SetIndex();
	void SetScaleformArgs_Open(std::vector<GFxValue> &args);
	void SetScaleformArgs_Close(std::vector<GFxValue> &args);
	void SetScaleformArgs_SetIndex(std::vector<GFxValue> &args);

	EventResult ReceiveEvent(SKSECrosshairRefEvent * evn, EventDispatcher<SKSECrosshairRefEvent> * dispatcher) override;


	TESObjectREFR*    targetRef_;
	TESObjectREFR*	  containerRef_;
	TESForm*          ownerForm_;
	tArray<ItemData>  items_;
	SInt32	          selectedIndex_;
	Flags<UInt32>	  flags_;

	static SimpleLock tlock_;

	enum
	{
		kQuickLoot_IsOpen        = (1 << 0),
		kQuickLoot_IsDisabled    = (1 << 1),
		kQuickLoot_RequestUpdate = (1 << 2)
	};
};


extern QuickLoot g_quickloot;
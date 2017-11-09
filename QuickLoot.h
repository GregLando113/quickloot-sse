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

class QuickLoot
	: public BSTEventSink<SKSECrosshairRefEvent>,
	  public IMenu
{
public:

	void Initialize();

	void Update();
	void Sort();

	void Dbg_PrintItems();
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
		kQuickLootFlag_IsOpen     = (1 << 0),
		kQuickLootFlag_IsDisabled = (1 << 1)
	};
};


extern QuickLoot g_quickloot;
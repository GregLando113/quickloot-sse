#pragma once


#include "skse64/GameReferences.h"
#include "skse64/GameAPI.h"
#include "skse64/GameEvents.h"
#include "skse64/PapyrusEvents.h"
#include "skse64/GameTypes.h"

#include "ItemData.h"
#include "Flags.h"

class QuickLoot
	: public BSTEventSink<SKSECrosshairRefEvent>
{
public:

	void Initialize();

	void Update();
	void Sort();

	void Dbg_PrintItems();
private:

	EventResult ReceiveEvent(SKSECrosshairRefEvent * evn, EventDispatcher<SKSECrosshairRefEvent> * dispatcher) override;


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
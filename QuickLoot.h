#pragma once


#include "skse64/GameReferences.h"
#include "skse64/GameAPI.h"
#include "skse64/GameEvents.h"
#include "skse64/PapyrusEvents.h"
#include "skse64/GameTypes.h"

#include "ItemData.h"

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
	UInt32	          flags_;

	static SimpleLock tlock_;

	enum
	{
		kQuickLootFlag_IsOpen     = (1 << 0),
		kQuickLootFlag_IsDisabled = (1 << 1)
	};

	bool GetFlags(UInt32 flags) { return (flags_ & flags) != 0; }
	void SetFlags(UInt32 flags) { flags_ |= flags; }
	void UnsetFlags(UInt32 flags) { flags_ &= ~flags;  }
};


extern QuickLoot g_quickloot;
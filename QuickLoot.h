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



enum
{
	kQuickLoot_IsOpen = (1 << 0),
	kQuickLoot_IsDisabled = (1 << 1),
	kQuickLoot_RequestUpdate = (1 << 2),
	kQuickLoot_OpenAnimation = (1 << 3),
	kQuickLoot_NowTaking = (1 << 4)
};

struct QuickLootMenuGen
{
	static IMenu* Create(void);
};

struct QuickLoot
	: public IMenu
{


	QuickLoot(const char* swfPath);

	static void Initialize();


	void Close();

	void Setup();
	void Update();
	void Sort();


	void OnMenuOpen();
	void OnMenuClose();
	void Clear();

	void SetIndex(SInt32 index);

	void Dbg_PrintItems();

	IMenu*             menu;
	Flags<UInt32>	  flags;

	static SimpleLock tlock;

	virtual UInt32 ProcessMessage(UIMessage * message) override;

	void InvokeScaleform_Open();
	void InvokeScaleform_Close();
	void InvokeScaleform_SetIndex();
	void SetScaleformArgs_Open(std::vector<GFxValue> &args);
	void SetScaleformArgs_Close(std::vector<GFxValue> &args);
	void SetScaleformArgs_SetIndex(std::vector<GFxValue> &args);

	TESObjectREFR*    targetRef;
	TESObjectREFR*	  containerRef;
	TESForm*          ownerForm;
	tArray<ItemData>  items;
	SInt32	          selectedIndex;

};


extern QuickLoot* g_quickloot;
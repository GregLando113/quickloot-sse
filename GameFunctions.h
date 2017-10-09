#pragma once

#include "skse64_common\Relocation.h"

#include "skse64/GameObjects.h"
#include "skse64/GameExtraData.h"

class TESObjectREFR;
class TESForm;


/* 
	Added Game Functions not included in base SKSE.
	Most are actually class members as labeled, however I declare as basic functions to avoid modifying the base skse
	Credits to https://github.com/himika/libSkyrim for oldrim addresses
*/

// TESObjectREFR::
typedef UInt32 TESObjectREFR_GetInventoryItemCount_t(TESObjectREFR* o, bool u1, bool u2);
extern RelocAddr<TESObjectREFR_GetInventoryItemCount_t*> TESObjectREFR_GetInventoryItemCount;

// TESForm::
typedef TESForm* TESForm_GetOwner_t(TESForm* o);
typedef double   TESForm_GetWeight_t(TESForm* o);

extern RelocAddr<TESForm_GetOwner_t*>                    TESForm_GetOwner;
extern RelocAddr<TESForm_GetWeight_t*>                   TESForm_GetWeight;

// static TESForm::
typedef TESForm* TESForm_LookupFormByID_t(UInt32 formid);
extern RelocAddr<TESForm_LookupFormByID_t*>              TESForm_LookupFormByID;

// ExtraContainerChanges::Data::
typedef ExtraContainerChanges::Data* ECCData_ctor_t(ExtraContainerChanges::Data* o, TESObjectREFR* ref);
typedef void ECCData_dtor_t(ExtraContainerChanges::Data* o);
typedef void ECCData_InitContainer_t(ExtraContainerChanges::Data* o);

extern RelocAddr<ECCData_ctor_t*>                        ECCData_ctor;
extern RelocAddr<ECCData_dtor_t*>                        ECCData_dtor;
extern RelocAddr<ECCData_InitContainer_t*>               ECCData_InitContainer;

// BaseExtraList::
typedef void BaseExtraList_SetInventoryChanges_t(BaseExtraList* o, ExtraContainerChanges::Data* changes);
typedef SInt16 BaseExtraList_GetItemCount_t(const BaseExtraList* o);

extern RelocAddr<BaseExtraList_SetInventoryChanges_t*>   BaseExtraList_SetInventoryChanges;
extern RelocAddr<BaseExtraList_GetItemCount_t*>			 BaseExtraList_GetItemCount;

// InventoryEntryData::
typedef bool InventoryEntryData_IsOwnedBy_t(InventoryEntryData* o, TESForm *actor, TESForm *itemOwner, bool unk1);
typedef bool InventoryEntryData_IsQuestItem_t(InventoryEntryData* o);

extern RelocAddr<InventoryEntryData_IsOwnedBy_t*>		 InventoryEntryData_IsOwnedBy;
extern RelocAddr<InventoryEntryData_IsQuestItem_t*>		 InventoryEntryData_IsQuestItem;

// Magicitem::
typedef MagicItem::EffectItem* MagicItem_GetCostliestEffectItem_t(MagicItem* o, int arg1, bool arg2);
extern RelocAddr<MagicItem_GetCostliestEffectItem_t*>	 MagicItem_GetCostliestEffectItem;
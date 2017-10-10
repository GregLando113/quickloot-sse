#include "QuickLoot.h"

#include "Interfaces.h"
#include "GameFunctions.h"

#include <map>

QuickLoot g_quickloot;

class ExtraDroppedItemList : public BSExtraData
{
public:
	virtual ~ExtraDroppedItemList();					// 00416BB0
	tList<UInt32>	handles;	// 08
private:
};

#define EVENT_REQUIRE(expr) if(! (expr) ) return kEvent_Continue

static char* GetFormName(TESForm* form)
{
	return *(char**)((uintptr_t)form + 0x28);
}

static bool IsValidItem(TESForm *item)
{
	if (!item)
		return false;

	if (item->GetFormType() == kFormType_LeveledItem)
		return false;

	if (item->GetFormType() == kFormType_Light)
	{
		const UInt32 kFlagCanbeCarried = (1 << 1);
		TESObjectLIGH *light = static_cast<TESObjectLIGH *>(item);
		if (!(light->unkE0.unk0C & kFlagCanbeCarried))
			return false;
	}
	else
	{
		if (!item->IsPlayable())
			return false;
	}

	TESFullName *fullName = dynamic_cast<TESFullName*>(item);
	if (!fullName)
		return false;

	const char *name = fullName->GetName();
	if (!name || !name[0])
		return false;

	return true;
}

static void AddEntryList(InventoryEntryData * newentry,BaseExtraList* list)
{
	if (!list)
		return;

	if (!newentry->extendDataList)
		newentry->extendDataList = new ExtendDataList;
	newentry->extendDataList->Push(list);
}


void
QuickLoot::Initialize()
{
	auto crosshairrefdispatch = (EventDispatcher<SKSECrosshairRefEvent>*)g_messaging->GetEventDispatcher(SKSEMessagingInterface::kDispatcher_CrosshairEvent);
	crosshairrefdispatch->AddEventSink(this);
}

void
QuickLoot::Update()
{
	items_.Clear();

	UInt32 numItems = TESObjectREFR_GetInventoryItemCount(containerRef_, false, false);
	printf("numItems = %d\n", numItems);

	if (numItems > items_.capacity)
	{
		items_.Resize(numItems);
	}


	ownerForm_ = nullptr;
	if (containerRef_->GetFormType() != kFormType_Character)
	{
		ownerForm_ = TESForm_GetOwner(containerRef_);
		if(ownerForm_)
			printf("owner=%p (%s)\n", ownerForm_, GetFormName(ownerForm_));
	}

	TESContainer* container = nullptr;
	if (containerRef_->baseForm->GetFormType() == kFormType_Container)
		container = &dynamic_cast<TESObjectCONT*>(containerRef_->baseForm)->container;
	else if (containerRef_->baseForm->GetFormType() == kFormType_NPC)
		container = &dynamic_cast<TESActorBase* >(containerRef_->baseForm)->container;

	//===================================
	// default items
	//===================================

	// TODO: Please optimize this, std::map is shit
	std::map<TESForm*, SInt32> itemMap;


	TESContainer::Entry *entry;
	for (UInt32 i = 0; i < container->numEntries; ++i)
	{
		entry = container->entries[i];
		if (!entry)
			continue;

		if (!IsValidItem(entry->form))
			continue;

		itemMap[entry->form] = entry->count;
	}

	//================================
	// changes
	//================================
	ExtraContainerChanges* exChanges = static_cast<ExtraContainerChanges*>(containerRef_->extraData.GetByType(kExtraData_ContainerChanges));
	ExtraContainerChanges::Data* changes = (exChanges) ? exChanges->data : nullptr;

	if (!changes)
	{
		_MESSAGE("FORCES CONTAINER TO SPAWN");
		_MESSAGE("  %p [%s]\n", containerRef_->formID, CALL_MEMBER_FN(containerRef_,GetReferenceName)());

		changes = (ExtraContainerChanges::Data*)Heap_Allocate(sizeof(ExtraContainerChanges::Data));
		ECCData_ctor(changes, containerRef_);
		BaseExtraList_SetInventoryChanges(&containerRef_->extraData, changes);
		ECCData_InitContainer(changes);
	}

	if (changes->objList)
	{
		for(auto it = changes->objList->Begin(); it.End(); it++)
		{
			auto pEntry = *it;
			if (!pEntry)
				continue;

			TESForm *item = pEntry->type;
			if (!IsValidItem(item))
				continue;

			SInt32 totalCount = pEntry->countDelta;
			SInt32 baseCount = 0;
			if (baseCount = itemMap.at(item))
			{
				if (baseCount < 0)			// this entry is already processed. skip.
					continue;

				if (item->formID != 0xF)		// gold (formid 0xf) is special case.
					totalCount += baseCount;
			}
			itemMap[item]  = -1;		// mark as processed.

			if (totalCount <= 0)
				continue;

			InventoryEntryData *defaultEntry = nullptr;
			if (item->formID != 0xF && pEntry->extendDataList)
			{
				for (auto kt = pEntry->extendDataList->Begin(); kt.End(); kt++)
				{
					auto extraList = *kt;
					if (!extraList)
						continue;

					int count = BaseExtraList_GetItemCount(extraList);
					if (count <= 0)
					{
						totalCount += count;
						continue;
					}
					totalCount -= count;

					InventoryEntryData *pNewEntry = nullptr;
					if (extraList->m_presence->HasType(kExtraData_TextDisplayData))
					{
						pNewEntry = new InventoryEntryData(item, count);
						AddEntryList(pNewEntry, const_cast<BaseExtraList*>(extraList));
					}
					else if (extraList->m_presence->HasType(kExtraData_Ownership))
					{
						pNewEntry = new InventoryEntryData(item, count);
						AddEntryList(pNewEntry, const_cast<BaseExtraList*>(extraList));
					}
					else
					{
						if (!defaultEntry)
							defaultEntry = new InventoryEntryData(item, 0);
						AddEntryList(defaultEntry, const_cast<BaseExtraList*>(extraList));
						defaultEntry->countDelta += count;
					}

					if (pNewEntry)
					{
						items_.Push(ItemData(pNewEntry,ownerForm_)); //emplace_back(pNewEntry, ownerForm_);
					}
				}
			}

			if (totalCount > 0)	// rest
			{
				if (!defaultEntry)
					defaultEntry = new InventoryEntryData(item, 0);

				defaultEntry->countDelta += totalCount;
			}

			if (defaultEntry)
			{
				items_.Push(ItemData(defaultEntry, ownerForm_));
			}
		}
	}

	//================================
	// default items that were not processed
	//================================
	for (auto &node : itemMap)
	{
		if (node.second <= 0)
			continue;

		if (!IsValidItem(node.first))
			continue;

		InventoryEntryData *entry = new InventoryEntryData(node.first, node.second);
		items_.Push(ItemData(entry, ownerForm_));
	}

	//================================
	// dropped items
	//================================
	ExtraDroppedItemList *exDroppedItemList = reinterpret_cast<ExtraDroppedItemList*>(containerRef_->extraData.GetByType(kExtraData_DroppedItemList));
	if (exDroppedItemList)
	{
		//for (UInt32 handle : exDroppedItemList->handles.)
		for(UInt32 i = 0; i < exDroppedItemList->handles.Count(); ++i)
		{
			UInt32* handle = exDroppedItemList->handles.GetNthItem(i);
			if (handle && *handle == *g_invalidRefHandle)
				continue;

			TESObjectREFR* refPtr;
			if (!TESObjectREFR_LookupRefByHandle(*handle, refPtr))
				continue;

			if (!IsValidItem(refPtr->baseForm))
				continue;

			InventoryEntryData *entry = new InventoryEntryData(refPtr->baseForm, 1);
			AddEntryList(entry, const_cast<BaseExtraList*>(&refPtr->extraData));
			items_.Push(ItemData(entry, ownerForm_));
		}
	}

#if 0
	if (items_.count != 0)
	{
		Sort();
	}

	InvokeScaleform_Open();

	m_bUpdateRequest = false;
#endif
}

EventResult 
QuickLoot::ReceiveEvent(SKSECrosshairRefEvent * evn, EventDispatcher<SKSECrosshairRefEvent> * dispatcher)
{
	TESObjectREFR* ref = evn->crosshairRef;
	if (ref)
	{
		EVENT_REQUIRE(ref->baseForm);


		printf("SKSECrosshairRefEvent: (%s) form=%p type:%d\n", CALL_MEMBER_FN(ref, GetReferenceName)(), ref, ref->baseForm->formType);

		EVENT_REQUIRE(ref->baseForm->formType == kFormType_Container ||
					  ref->baseForm->formType == kFormType_NPC);

		containerRef_ = ref;

		Update();
	}
	else
	{
		printf("SKSECrosshairRefEvent: CLEAR\n");
	}
	return kEvent_Continue;
}
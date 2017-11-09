#include "QuickLoot.h"

#include "Interfaces.h"
#include "GameFunctions.h"
#include "dbg.h"


#include <cstring>

#include "skse64\GameRTTI.h"

#include "skse64\Hooks_UI.h"
#include "skse64\GameMenus.h"
#include "skse64\GameSettings.h"


#define ForItems(type, container) for(UInt32 i = 0, type &it; i < container.count; ++i, it = container[i])

SimpleLock QuickLoot::tlock_;
QuickLoot g_quickloot;

class ExtraDroppedItemList : public BSExtraData
{
public:
	virtual ~ExtraDroppedItemList();					// 00416BB0
	tList<UInt32>	handles;	// 08
private:
};

class LootMenuUIDelegate : public UIDelegate_v1
{
public:
	typedef void (QuickLoot::*FnCallback)(std::vector<GFxValue> &args);

	const char *	m_target;
	FnCallback		m_callback;

	LootMenuUIDelegate(const char *target, FnCallback callback) 
		: m_target(target), m_callback(callback)
	{
	}

	void Run() override
	{
		GFxMovieView *view = g_quickloot.view;

		char target[64];
		strcpy_s(target, 64, "_root.Menu_mc");
		strcat_s(target, m_target);

		std::vector<GFxValue> args;
		(g_quickloot.*m_callback)(args);

		if (args.empty())
			view->Invoke(target, nullptr, nullptr, 0);
		else
			view->Invoke(target, nullptr, &args[0], args.size());
	}

	void Dispose() override
	{
		Heap_Free(this);
	}

	static void Queue(const char *target, FnCallback callback)
	{
		UIManager* m = UIManager::GetSingleton();
		if (m)
		{
			LootMenuUIDelegate* delg = (LootMenuUIDelegate*)Heap_Allocate(sizeof(LootMenuUIDelegate));
			new (delg) LootMenuUIDelegate(target, callback);
			m->QueueCommand(delg);
		}
	}
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
		newentry->extendDataList = ExtendDataList::Create();
	newentry->extendDataList->Push(list);
}

template <typename _Key, typename _Val>
class ItemMap
{
public:

	struct iterator
	{
		bool next(_Key* k, _Val* v)
		{
			bool cont = kcur != end;
			if (cont)
			{
				*k = *kcur;
				*v = *vcur;
				kcur++;
				vcur++;
			}
			return cont;
		}

		_Key* kcur;
		_Val* vcur;
		_Key* end;
	};


	ItemMap(size_t cap)
	{
		base_ = Heap_Allocate((cap * sizeof(_Key)) + (cap * sizeof(_Val)));
		keyBase_ = reinterpret_cast<_Key*>(base_);
		valBase_ = reinterpret_cast<_Val*>(keyBase_ + cap);
		capacity_ = cap;
	}

	~ItemMap()
	{
		Heap_Free(base_);
	}

	void Add(const _Key& key, const _Val& val)
	{
		keyBase_[count_] = key;
		valBase_[count_] = val;
		count_++;
	}

	bool Set(const _Key& key,const _Val& val)
	{
		for (size_t i = 0; i < count_; ++i)
		{
			if (keyBase_[i] == key)
			{
				valBase_[i] = val;
				return true;
			}
		}
		return false;
	}

	bool Get(const _Key& key, _Val* val)
	{
		for (size_t i = 0; i < count_; ++i)
		{
			if (keyBase_[i] == key)
			{
				*val = valBase_[i];
				return true;
			}
		}
		return false;
	}

#if 1
	iterator begin()
	{
		return iterator{ keyBase_ , valBase_, keyBase_ + count_ };
	}
#endif
private:
	void*  base_;
	_Key*  keyBase_;
	_Val*  valBase_;

	size_t capacity_;
	size_t count_;
};

template <typename T>
bool traverse(tArray<T>& arr,UInt32* idx, T** out)
{
	*out = &arr[(*(idx))++];
	return *idx < arr.count;
}

#if 0
bool QuickLootScaleform_Register(GFxMovieView * view, GFxValue * root)
{
	root->

	return true;
}
#endif

void
QuickLoot::Initialize()
{
	auto crosshairrefdispatch = (EventDispatcher<SKSECrosshairRefEvent>*)g_messaging->GetEventDispatcher(SKSEMessagingInterface::kDispatcher_CrosshairEvent);
	crosshairrefdispatch->AddEventSink(this);
	enum
	{
		kType_PauseGame = 0x01,
		kType_DoNotDeleteOnClose = 0x02,
		kType_ShowCursor = 0x04,
		kType_Unk0008 = 0x08,
		kType_Modal = 0x10,
		kType_StopDrawingWorld = 0x20,
		kType_Open = 0x40,
		kType_PreventGameLoad = 0x80,
		kType_Unk0100 = 0x100,
		kType_HideOther = 0x200,
		kType_Unk0400 = 0x400,
		kType_DoNotPreventGameSave = 0x800,
		kType_Unk1000 = 0x1000,
		kType_ItemMenu = 0x2000,
		kType_StopCrosshairUpdate = 0x4000,
		kType_Unk8000 = 0x8000,
		kType_Unk10000 = 0x10000	// mouse cursor
	};
	if (CALL_MEMBER_FN(GFxLoader::GetSingleton(), LoadMovie)(this, &view, "LootMenu", 1, 0.0f))
	{
		flags = kType_DoNotDeleteOnClose | kType_DoNotPreventGameSave | kType_Unk10000;
	}
}

void
QuickLoot::Update()
{
	//SimpleLocker lock(&tlock_);
	D_VAR(items_.count, "%d");
	D_MSG("Quickloot::Update() START");
	items_.count = 0;

	D_VAR(items_.count, "%d");

	UInt32 numItems = TESObjectREFR_GetInventoryItemCount(containerRef_, false, false);
	D_VAR(numItems, "%d");
	D_VAR(items_.capacity, "%d");
	if (numItems > items_.capacity)
	{
		items_.Allocate(numItems);
		items_.count = 0;
	}

	
	ownerForm_ = nullptr;
	if (containerRef_->GetFormType() != kFormType_Character)
	{
		ownerForm_ = TESForm_GetOwner(containerRef_);
		D_VAR(ownerForm_, "%p");
	}
	
	TESContainer* baseContainer = DYNAMIC_CAST(containerRef_->baseForm, TESForm, TESContainer);
	ASSERT(baseContainer);
	/*if (containerRef_->baseForm->GetFormType() == kFormType_Container)
		container = &dynamic_cast<TESObjectCONT*>(containerRef_->baseForm)->container;
	else if (containerRef_->baseForm->GetFormType() == kFormType_NPC)
		container = &dynamic_cast<TESActorBase* >(containerRef_->baseForm)->container;*/

	//===================================
	// default items
	//===================================
	ItemMap<TESForm*, SInt32> itemMap(baseContainer->numEntries);

	
	D_MSG(" === DEFAULT ITEM START");
	TESContainer::Entry *entry;
	for (UInt32 i = 0; i < baseContainer->numEntries; ++i)
	{
		entry = baseContainer->entries[i];
		if (!entry)
			continue;

		if (!IsValidItem(entry->form))
			continue;

		itemMap.Add(entry->form,entry->count);
		D_MSG("DEFAULT item %d [%p] = %d", i, entry->form, entry->count);
	}
	D_MSG(" === DEFAULT ITEM END");
	D_VAR(items_.count, "%d");
	//================================
	// changes

	//================================
	ExtraContainerChanges* exChanges = reinterpret_cast<ExtraContainerChanges*>(containerRef_->extraData.GetByType(kExtraData_ContainerChanges));
	ExtraContainerChanges::Data* changes = (exChanges) ? exChanges->data : nullptr;
	
	if (!changes)
	{
		_MESSAGE("FORCES CONTAINER TO SPAWN");
		_MESSAGE("  %p [%s]\n", containerRef_->formID, CALL_MEMBER_FN(containerRef_,GetReferenceName)());

		changes = (ExtraContainerChanges::Data*)Heap_Allocate(sizeof(ExtraContainerChanges::Data));
		ECCData_ctor(changes, containerRef_);
		BaseExtraList_SetInventoryChanges(&(containerRef_->extraData), changes);
		ECCData_InitContainer(changes);
	}
	D_VAR(items_.count, "%d");

	D_MSG(" === CHANGES START");
	if (changes->objList)
	{
		D_VAR(changes->objList->Count(), "%d");
		for(auto it = changes->objList->Begin(); !it.End(); ++it)
		{
			auto pEntry = it.Get();
			D_VAR(pEntry, "%p");
			if (!pEntry)
				continue;

			TESForm *item = pEntry->type;
			if (!IsValidItem(item))
			{
				D_MSG("pEntryInvalid!");
				continue;
			}

			SInt32 totalCount = pEntry->countDelta;
			SInt32 baseCount = 0;
			D_VAR(totalCount, "%d");
			if (itemMap.Get(item,&baseCount))
			{
				D_VAR(baseCount, "%d");
				if (baseCount < 0)			// this entry is already processed. skip.
				{
					D_MSG("this entry is already processed. skip.");
					continue;
				}

				if (item->formID != 0xF)		// gold (formid 0xf) is special case.
					totalCount += baseCount;
			}
			if (!itemMap.Set(item, -1))		// mark as processed.
			{
				D_MSG("itemmap not set to -1");
			}
			if (totalCount <= 0)
			{
				D_MSG("totalCount <= 0");
				continue;
			}

			InventoryEntryData *defaultEntry = nullptr;
			D_VAR(item->formID, "%X");
			D_VAR(pEntry->extendDataList, "%p");
			if (item->formID != 0xF && pEntry->extendDataList)
			{
				D_MSG(" ========== extendDataList BEGIN");
				for (auto kt = pEntry->extendDataList->Begin(); !kt.End(); ++kt)
				{
					auto extraList = kt.Get();
					if (!extraList)
						continue;

					int count = BaseExtraList_GetItemCount(extraList);
					D_VAR(count, "%d");
					if (count <= 0)
					{
						totalCount += count;
						continue;
					}
					totalCount -= count;

					InventoryEntryData *pNewEntry = nullptr;
					if (extraList->m_presence->HasType(kExtraData_TextDisplayData))
					{
						pNewEntry = InventoryEntryData::Create(item, count);
						AddEntryList(pNewEntry, const_cast<BaseExtraList*>(extraList));
					}
					else if (extraList->m_presence->HasType(kExtraData_Ownership))
					{
						pNewEntry = InventoryEntryData::Create(item, count);
						AddEntryList(pNewEntry, const_cast<BaseExtraList*>(extraList));
					}
					else
					{
						if (!defaultEntry)
							defaultEntry = InventoryEntryData::Create(item, 0);
						AddEntryList(defaultEntry, const_cast<BaseExtraList*>(extraList));
						defaultEntry->countDelta += count;
					}
					//FormHeap_Allocate
					if (pNewEntry)
					{
						
						items_.Push(ItemData(pNewEntry,ownerForm_)); //emplace_back(pNewEntry, ownerForm_);
					}
				}
				D_MSG(" ========== extendDataList END");
			}

			if (totalCount > 0)	// rest
			{
				if (!defaultEntry)
					defaultEntry = InventoryEntryData::Create(item, 0);

				defaultEntry->countDelta += totalCount;
			}

			if (defaultEntry)
			{
				items_.Push(ItemData(defaultEntry, ownerForm_));
			}
		}
	}
	D_MSG(" === CHANGES END");
	//================================
	// default items that were not processed
	//================================

	{
		auto it = itemMap.begin();
		TESForm* item;
		SInt32   count;
		while (it.next(&item,&count))
		{

			if (count <= 0)
				continue;

			if (!IsValidItem(item))
				continue;

			InventoryEntryData *entry = InventoryEntryData::Create(item, count);

			items_.Push(ItemData(entry, ownerForm_));

		}
	}

	//================================
	// dropped items
	//================================
	D_MSG(" === DROPPED ITEM BEGIN");
	ExtraDroppedItemList *exDroppedItemList = reinterpret_cast<ExtraDroppedItemList*>(containerRef_->extraData.GetByType(kExtraData_DroppedItemList));
	if (exDroppedItemList)
	{
		D_VAR(exDroppedItemList->handles.Count(), "%d");
		for(UInt32 i = 0; i < exDroppedItemList->handles.Count(); ++i)
		{
			UInt32* handle = exDroppedItemList->handles.GetNthItem(i);
			if (handle && *handle == *g_invalidRefHandle)
				continue;

			TESObjectREFR* refPtr;
			if (!TESObjectREFR_LookupRefByHandle(*handle, refPtr))
				continue;
			D_VAR(refPtr, "%p");

			if (!IsValidItem(refPtr->baseForm))
			{
				D_MSG("itemInvalid!");
				continue;
			}

			InventoryEntryData *entry = InventoryEntryData::Create(refPtr->baseForm, 1);
			AddEntryList(entry, const_cast<BaseExtraList*>(&refPtr->extraData));
			items_.Push(ItemData(entry, ownerForm_));
		}
	}
	D_MSG(" === DROPPED ITEM END");
	D_MSG(" === BEGIN ITEMS PRE SORT");
#if BLD_DEBUG
	Dbg_PrintItems();
#endif
	D_MSG(" === END ITEMS PRE SORT");
	if (items_.count != 0)
	{
		D_MSG("=== SORT BEGIN");
		//Sort();
		D_MSG("=== SORT END");
	}

	//D_MSG(" === BEGIN ITEMS POST SORT");
	//Dbg_PrintItems();
	//D_MSG(" === END ITEMS POST SORT");

#if 0
	InvokeScaleform_Open();

	m_bUpdateRequest = false;
#endif

	D_MSG("QuickLoot::Update() END");
}

void 
QuickLoot::Sort()
{
	
	qsort(items_.entries, items_.count, sizeof(ItemData), [](const void *pA, const void *pB) -> int {

		if (!pA)
			return -1;
		if (!pB)
			return 1;

		
		const ItemData &a = *(const ItemData *)pA;
		const ItemData &b = *(const ItemData *)pB;
		
		if (a.pEntry == b.pEntry)
			return 0;
		
		return (a < b) ? -1 : 1;
	});
	
}

void
QuickLoot::Dbg_PrintItems()
{
	_MESSAGE("=== DUMP CONTAINER ===");
	_MESSAGE("%p [%s] numItems=%d owner=%p", containerRef_->formID, CALL_MEMBER_FN(containerRef_,GetReferenceName)(), items_.count, (ownerForm_ ? ownerForm_->formID : 0));

	ItemData* it;
	UInt32 idx = 0;
	while(traverse(items_,&idx, &it))
	{
		InventoryEntryData *pEntry = it->pEntry;
		TESForm *form = pEntry ? pEntry->type : nullptr;

		_MESSAGE("    %p (%X) [%s], count=%d, icon=%s, priority=%d isStolen=%d",
			pEntry, 
			form ? form->formID : 0xBAADC0DE,
			it->GetName(),
			it->GetCount(),
			it->GetIcon(),
			it->priority,
			it->IsStolen()
		);
	}
	_MESSAGE("=== END DUMP CONTAINER ===");
}

void 
QuickLoot::InvokeScaleform_Open()
{
	if (selectedIndex_ >= items_.count)
	{
		selectedIndex_ = items_.count - 1;
	}
	else if (selectedIndex_ < 0)
	{
		selectedIndex_ = 0;
	}
	LootMenuUIDelegate::Queue(".openContainer", &QuickLoot::SetScaleformArgs_Open);
}

void 
QuickLoot::InvokeScaleform_Close()
{
	LootMenuUIDelegate::Queue(".closeContainer", &QuickLoot::SetScaleformArgs_Close);
}

void 
QuickLoot::InvokeScaleform_SetIndex()
{
	LootMenuUIDelegate::Queue(".setSelectedIndex", &QuickLoot::SetScaleformArgs_SetIndex);
}

void 
QuickLoot::SetScaleformArgs_Open(std::vector<GFxValue>& args)
{
	SimpleLocker guard(&tlock_);

	static char *sSteal = (*g_gameSettingCollection)->Get("sSteal")->data.s;
	static char *sTake = (*g_gameSettingCollection)->Get("sTake")->data.s;

	GFxValue argItems;
	view->CreateArray(&argItems);
	GFxValue argRefID; // = (double)m_targetRef->formID;
	argRefID.SetNumber(targetRef_->formID);
	GFxValue argTitle;
	argTitle.SetString(CALL_MEMBER_FN(targetRef_, GetReferenceName)());
	GFxValue argTake;
	argTake.SetString((CALL_MEMBER_FN(targetRef_, IsOffLimits)()) ? sSteal : sTake);
	GFxValue argSearch;
	argSearch.SetString((*g_gameSettingCollection)->Get("sSearch")->data.s);
	GFxValue argSelectedIndex;
	argSelectedIndex.SetNumber((double)selectedIndex_);

	for(UInt32 i = 0; i < items_.count; ++i)
	{
		ItemData& itemData = items_[i];
		GFxValue text;
		argTitle.SetString(itemData.GetName());
		GFxValue count;
		count.SetNumber((double)itemData.GetCount());
		GFxValue value;
		value.SetNumber((double)itemData.GetValue());
		GFxValue weight;
		weight.SetNumber(itemData.GetWeight());
		GFxValue isStolen;
		isStolen.SetBool(itemData.IsStolen());
		GFxValue iconLabel;
		iconLabel.SetString(itemData.GetIcon());
		GFxValue itemIndex;
		itemIndex.SetNumber((double)0);

		GFxValue item;
		view->CreateObject(&item);
		item.SetMember("text", &text);
		item.SetMember("count", &count);
		item.SetMember("value", &value);
		item.SetMember("weight", &weight);
		item.SetMember("isStolen", &isStolen);
		item.SetMember("iconLabel", &iconLabel);

		TESForm *form = itemData.pEntry->type;
		if (form->formType == kFormType_Book)
		{
			TESObjectBOOK *book = static_cast<TESObjectBOOK*>(form);
			GFxValue isRead;
			isRead.SetBool((book->data.flags & TESObjectBOOK::Data::kType_Read) != 0);
			item.SetMember("isRead", &isRead);
		}

		if (form->IsArmor() || form->IsWeapon())
		{
			GFxValue isEnchanted;
			isEnchanted.SetBool(itemData.IsEnchanted());
			item.SetMember("isEnchanted", &isEnchanted);
		}

		argItems.PushBack(&item);
	}

	args.reserve(6);
	args.push_back(argItems);				// arg1
	args.push_back(argRefID);				// arg2
	args.push_back(argTitle);				// arg3
	args.push_back(argTake);				// arg4
	args.push_back(argSearch);				// arg5
	args.push_back(argSelectedIndex);		// arg6
}

void 
QuickLoot::SetScaleformArgs_Close(std::vector<GFxValue>& args)
{
}

void 
QuickLoot::SetScaleformArgs_SetIndex(std::vector<GFxValue>& args)
{
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

		_MESSAGE(" = START UPDATE");
		Update();
		_MESSAGE(" = END UPDATE");
	}
	else
	{
		printf("SKSECrosshairRefEvent: CLEAR\n");
	}
	return kEvent_Continue;
}
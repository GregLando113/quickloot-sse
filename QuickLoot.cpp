#include "QuickLoot.h"
#include <cstring>

#include "skse64\GameRTTI.h"

#include "skse64\Hooks_UI.h"
#include "skse64\GameMenus.h"
#include "skse64\GameSettings.h"
#include "skse64\gamethreads.h"


#include "GFxEvent.h"
#include "Interfaces.h"
#include "GameFunctions.h"
#include "dbg.h"



#define ForItems(type, container) for(UInt32 i = 0, type &it; i < container.count; ++i, it = container[i])

SimpleLock QuickLoot::tlock;
QuickLoot* g_quickloot;

class DelayedUpdater : public TaskDelegate
{
public:
	virtual void Run() override
	{
		g_quickloot->Update();
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

	const char *	target_;
	FnCallback		callback_;

	LootMenuUIDelegate(const char *target, FnCallback callback) 
		: target_(target), callback_(callback)
	{
	}

	void Run() override
	{
		if (g_quickloot)
		{
			GFxMovieView *view = g_quickloot->view;

			char target[64];
			strcpy_s(target, 64, "_root.Menu_mc");
			strcat_s(target, target_);

			std::vector<GFxValue> args;
			(g_quickloot->*callback_)(args);

			if (args.empty())
				view->Invoke(target, nullptr, nullptr, 0);
			else
				view->Invoke(target, nullptr, &args[0], args.size());
		}

	}

	void Dispose() override
	{
		if(this)
			Heap_Free(this);
	}

	static void Queue(const char *target, FnCallback callback)
	{
		if (g_tasks)
		{
			LootMenuUIDelegate* delg = (LootMenuUIDelegate*)Heap_Allocate(sizeof(LootMenuUIDelegate));
			new (delg) LootMenuUIDelegate(target, callback);
			g_tasks->AddUITask(delg);
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

QuickLoot::QuickLoot(const char* swfPath)
{
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
	auto crosshairrefdispatch = (EventDispatcher<SKSECrosshairRefEvent>*)g_messaging->GetEventDispatcher(SKSEMessagingInterface::kDispatcher_CrosshairEvent);
	crosshairrefdispatch->AddEventSink(dynamic_cast<BSTEventSink<SKSECrosshairRefEvent>*>(g_quickloot));

	MenuManager *mm = MenuManager::GetSingleton();
	mm->MenuOpenCloseEventDispatcher()->AddEventSink(dynamic_cast<BSTEventSink<MenuOpenCloseEvent>*>(g_quickloot));

	//GetEventDispatcherList()->unk370.AddEventSink(&g_containerChangedEventHandler);

	if (CALL_MEMBER_FN(GFxLoader::GetSingleton(), LoadMovie)(this, &view, swfPath, 1, 0.0))
	{
		IMenu::flags = 0x02 | 0x800 | 0x10000;
		IMenu::unk0C = 2;

		//m_bNowTaking = false;
		//m_bUpdateRequest = false;
		//m_bOpenAnim = false;
		//m_selectedIndex = -1;
		//m_owner = nullptr;
		//m_bTakeAll = false;
	}
	//auto mvloader = GetGfxSingleton();
	//if (!mvloader) D_MSG("mvloader == NULL"); else D_MSG("mvloader != NULL");
	//if (mvloader && CALL_MEMBER_FN(mvloader, LoadMovie)(this, &view, swfPath, 1, 1.0f))
	//{
	//	_MESSAGE("  loaded Inteface/%s.swf successfully", swfPath);

	//	unk0C = 2;			// set this lower than that of Fader Menu (3)
	//	flags = kType_DoNotDeleteOnClose | kType_DoNotPreventGameSave | kType_Unk10000;
	//}
}

IMenu* QuickLootMenuGen::Create(void)
{
	D_MSG("QuickLootMenuGen::Create(void)");
	void* p = ScaleformHeap_Allocate(sizeof(QuickLoot));
	if (p)
	{
		g_quickloot = new (p) QuickLoot("LootMenu");
		return g_quickloot;
	}
	else
	{
		return nullptr;
	}
}



void
QuickLoot::Initialize()
{
	D_MSG("QuickLoot::Initialize()");
	MenuManager * mm = MenuManager::GetSingleton();
	D_VAR(mm, "%p");
	if (mm)
		mm->Register("LootMenu", QuickLootMenuGen::Create);
}


class GFxMovieDef
{
public: 
	virtual UInt32		GetVersion() const = 0;
	virtual UInt32		GetLoadingFrame() const = 0;
	virtual float		GetWidth() const = 0;
	virtual float		GetHeight() const = 0;
	virtual UInt32		GetFrameCount() const = 0;
	virtual float		GetFrameRate() const = 0;
};

void QuickLoot::Close()
{
	SimpleLocker lock(&tlock);

	if (!containerRef)
		return;

	//if (containerRef_->formType != kFormType_Character)
	//	PlayAnimationClose();

	Clear();

	if (menu && menu->view)
		menu->view->Unk_08(false);
}

void QuickLoot::OnMenuOpen()
{
	SimpleLocker lock(&tlock);

	//if (containerRef)	// is another container already opened ?
	//{
	//	if (containerRef->baseForm->formType == kFormType_Activator)
	//	{
	//		UInt32 refHandle = 0;
	//		if (containerRef->extraData.GetAshPileRefHandle(refHandle) && refHandle != (*g_invalidRefHandle))
	//		{
	//			TESObjectREFR * refPtr = nullptr;
	//			if (LookupREFRByHandle(&refHandle, &refPtr))
	//				containerRef = refPtr;
	//		}
	//	}
	//}

	if (view)
	{
		selectedIndex = 0;

		//RegisterMenuEventHandler(MenuControls::GetSingleton(), this);

		Setup();
		Update();

		view->Unk_08(true);
	}
}

void QuickLoot::OnMenuClose()
{
	SimpleLocker lock(&tlock);

	if (view)
	{

		Clear();
		view->Unk_08(false);

		//RemoveMenuEventHandler(MenuControls::GetSingleton(), this);
	}
}

void QuickLoot::Clear()
{
	SimpleLocker lock(&tlock);

	ownerForm = nullptr;
	selectedIndex = -1;

	items.Clear();

	flags.UnSet((UInt32)-1); // unset all flags
}


void
QuickLoot::Setup()
{
	if (menu && menu->view)
	{
		GFxMovieDef *def = reinterpret_cast<GFxMovieDef *>(menu->view->Unk_01());

		double x = -1;
		double y = -1;
		double scale = -1;
		double opacity = 75;

		x = (0 <= x && x <= 100) ? (x * def->GetWidth() * 0.01) : -1;
		y = (0 <= y && y <= 100) ? (y * def->GetHeight() * 0.01) : -1;
		if (scale >= 0)
		{
			if (scale < 25)
				scale = 25;
			else if (scale > 400)
				scale = 400;
		}
		if (opacity >= 0)
		{
			if (opacity > 100)
				opacity = 100;
		}

		GFxValue args[4];
		args[0].SetNumber(x);
		args[1].SetNumber(y);
		args[2].SetNumber(scale);
		args[3].SetNumber(opacity);
		menu->view->Invoke("_root.Menu_mc.Setup", nullptr, args, 4);
	}
}

void
QuickLoot::Update()
{
	SimpleLocker lock(&tlock);

	if (flags.GetAll(kQuickLoot_IsDisabled))
	{
		return;
	}

	D_VAR(items.count, "%d");
	D_MSG("Quickloot::Update() START");
	items.count = 0;

	D_VAR(items.count, "%d");

	UInt32 numItems = TESObjectREFR_GetInventoryItemCount(containerRef, false, false);
	D_VAR(numItems, "%d");
	D_VAR(items.capacity, "%d");
	if (numItems > items.capacity)
	{
		items.Allocate(numItems);
		items.count = 0;
	}

	
	ownerForm = nullptr;
	if (containerRef->GetFormType() != kFormType_Character)
	{
		ownerForm = TESForm_GetOwner(containerRef);
		D_VAR(ownerForm, "%p");
	}
	
	TESContainer* baseContainer = DYNAMIC_CAST(containerRef->baseForm, TESForm, TESContainer);
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
	D_VAR(items.count, "%d");
	//================================
	// changes

	//================================
	ExtraContainerChanges* exChanges = reinterpret_cast<ExtraContainerChanges*>(containerRef->extraData.GetByType(kExtraData_ContainerChanges));
	ExtraContainerChanges::Data* changes = (exChanges) ? exChanges->data : nullptr;
	
	if (!changes)
	{
		_MESSAGE("FORCES CONTAINER TO SPAWN");
		_MESSAGE("  %p [%s]\n", containerRef->formID, CALL_MEMBER_FN(containerRef,GetReferenceName)());

		changes = (ExtraContainerChanges::Data*)Heap_Allocate(sizeof(ExtraContainerChanges::Data));
		ECCData_ctor(changes, containerRef);
		BaseExtraList_SetInventoryChanges(&(containerRef->extraData), changes);
		ECCData_InitContainer(changes);
	}
	D_VAR(items.count, "%d");

	D_MSG(" === CHANGES START");
	if (changes && changes->objList)
	{
		D_VAR(changes->objList->Count(), "%d");
		for(auto it = changes->objList->Begin(); !it.End(); ++it)
		{
			auto pEntry = it.Get();
			D_VAR(pEntry, "%p");
			if (!pEntry)
				continue;

			TESForm *item = pEntry->type;
			if (!item || !IsValidItem(item))
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
						
						items.Push(ItemData(pNewEntry,ownerForm)); //emplace_back(pNewEntry, ownerForm_);
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
				items.Push(ItemData(defaultEntry, ownerForm));
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

			items.Push(ItemData(entry, ownerForm));

		}
	}

	//================================
	// dropped items
	//================================
	D_MSG(" === DROPPED ITEM BEGIN");
	ExtraDroppedItemList *exDroppedItemList = reinterpret_cast<ExtraDroppedItemList*>(containerRef->extraData.GetByType(kExtraData_DroppedItemList));
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
			items.Push(ItemData(entry, ownerForm));
		}
	}
	D_MSG(" === DROPPED ITEM END");
	D_MSG(" === BEGIN ITEMS PRE SORT");
#if BLD_DEBUG
	Dbg_PrintItems();
#endif
	D_MSG(" === END ITEMS PRE SORT");
	if (items.count != 0)
	{
		D_MSG("=== SORT BEGIN");
		//Sort();
		D_MSG("=== SORT END");
	}

	//D_MSG(" === BEGIN ITEMS POST SORT");
	//Dbg_PrintItems();
	//D_MSG(" === END ITEMS POST SORT");

#if 1
	InvokeScaleform_Open();

	flags.Set(kQuickLoot_RequestUpdate);
#endif

	D_MSG("QuickLoot::Update() END");
}

void 
QuickLoot::Sort()
{
	
	qsort(items.entries, items.count, sizeof(ItemData), [](const void *pA, const void *pB) -> int {

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
	_MESSAGE("%p [%s] numItems=%d owner=%p", containerRef->formID, CALL_MEMBER_FN(containerRef,GetReferenceName)(), items.count, (ownerForm ? ownerForm->formID : 0));

	ItemData* it;
	UInt32 idx = 0;
	while(traverse(items,&idx, &it))
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

void QuickLoot::SetIndex(SInt32 index)
{
	SimpleLocker lock(&tlock);

	if (containerRef)
	{
		const int tail = items.count - 1;
		selectedIndex += index;
		if (selectedIndex > tail)
			selectedIndex = tail;
		else if (selectedIndex < 0)
			selectedIndex = 0;

		InvokeScaleform_SetIndex();
	}

}

UInt32 QuickLoot::ProcessMessage(UIMessage * message)
{
	UInt32 result = 2;

	struct QLBSUIScaleformData
	{
		virtual ~QLBSUIScaleformData() {}

		//	void	** _vtbl;		// 00
		UInt32				unk04; // 04
		GFxEvent*              event;

	};

	if (view)
	{
		switch (message->message)
		{
		case UIMessage::kMessage_Open:
			OnMenuOpen();
			break;
		case UIMessage::kMessage_Close:
			OnMenuClose();
			break;
		case 6: // kMessage_Scaleform
			if (view->Unk_09() && message->objData)
			{
				QLBSUIScaleformData *scaleformData = (QLBSUIScaleformData *)message->objData;

				GFxEvent *event = scaleformData->event;

				if (event->type == GFxEvent::MouseWheel)
				{
					GFxMouseEvent *mouse = (GFxMouseEvent *)event;
					if (mouse->scrollDelta > 0)
						SetIndex(-1);
					else if (mouse->scrollDelta < 0)
						SetIndex(1);
				}
				else if (event->type == GFxEvent::KeyDown)
				{
					GFxKeyEvent *key = (GFxKeyEvent *)event;
					if (key->keyCode == GFxKey::Up)
						SetIndex(-1);
					else if (key->keyCode == GFxKey::Down)
						SetIndex(1);
				}
				else if (event->type == GFxEvent::CharEvent)
				{
					GFxCharEvent *charEvent = (GFxCharEvent *)event;
				}
			}
		}
	}

	return result;
}

void
QuickLoot::InvokeScaleform_Open()
{
	if (items.count == 0)
	{
		//menu->view->Invoke("_root.Menu_mc.openContainer", nullptr, nullptr, 0);
		//InvokeScaleform_SetContainer();

		//CALL_MEMBER_FN((*g_thePlayer), OnCrosshairRefChanged)();

		return;
	}
	if (selectedIndex >= items.count)
	{
		selectedIndex = items.count - 1;
	}
	else if (selectedIndex < 0)
	{
		selectedIndex = 0;
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
	SimpleLocker guard(&tlock);

	static char *sSteal = (*g_gameSettingCollection)->Get("sSteal")->data.s;
	static char *sTake = (*g_gameSettingCollection)->Get("sTake")->data.s;

	GFxValue argItems;
	menu->view->CreateArray(&argItems);
	GFxValue argRefID; // = (double)m_targetRef->formID;
	argRefID.SetNumber(targetRef->formID);
	GFxValue argTitle;
	argTitle.SetString(CALL_MEMBER_FN(targetRef, GetReferenceName)());
	GFxValue argTake;
	argTake.SetString((CALL_MEMBER_FN(targetRef, IsOffLimits)()) ? sSteal : sTake);
	GFxValue argSearch;
	argSearch.SetString((*g_gameSettingCollection)->Get("sSearch")->data.s);
	GFxValue argSelectedIndex;
	argSelectedIndex.SetNumber((double)selectedIndex);

	for(UInt32 i = 0; i < items.count; ++i)
	{
		ItemData& itemData = items[i];
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
		menu->view->CreateObject(&item);
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
	SimpleLocker guard(&tlock);
}

void 
QuickLoot::SetScaleformArgs_SetIndex(std::vector<GFxValue>& args)
{
	SimpleLocker guard(&tlock);
	args.reserve(1);
	GFxValue idx;
	idx.SetNumber((double)selectedIndex);
	args.push_back(idx);
}
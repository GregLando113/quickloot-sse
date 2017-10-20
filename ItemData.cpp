#include "ItemData.h"

#include "skse64/GameRTTI.h"
#include "skse64/GameForms.h"
#include "skse64/GameObjects.h"
#include "skse64/GameReferences.h"
#include "skse64/GameExtraData.h"

#include "GameFunctions.h"

static ItemData::Type GetItemType(TESForm *form);

static const char * strIcons[] = {
	"none",					// 00
	"default_weapon",
	"weapon_sword",
	"weapon_greatsword",
	"weapon_daedra",
	"weapon_dagger",
	"weapon_waraxe",
	"weapon_battleaxe",
	"weapon_mace",
	"weapon_hammer",
	"weapon_staff",			// 10
	"weapon_bow",
	"weapon_arrow",
	"weapon_pickaxe",
	"weapon_woodaxe",
	"weapon_crossbow",
	"weapon_bolt",
	"default_armor",
	"lightarmor_body",
	"lightarmor_head",
	"lightarmor_hands",		// 20
	"lightarmor_forearms",
	"lightarmor_feet",
	"lightarmor_calves",
	"lightarmor_shield",
	"lightarmor_mask",
	"armor_body",
	"armor_head",
	"armor_hands",
	"armor_forearms",
	"armor_feet",			// 30
	"armor_calves",
	"armor_shield",
	"armor_mask",
	"armor_bracer",
	"armor_daedra",
	"clothing_body",
	"clothing_robe",
	"clothing_head",
	"clothing_pants",
	"clothing_hands",		// 40
	"clothing_forearms",
	"clothing_feet",
	"clothing_calves",
	"clothing_shoes",
	"clothing_shield",
	"clothing_mask",
	"armor_amulet",
	"armor_ring",
	"armor_circlet",
	"default_scroll",		// 50
	"default_book",
	"default_book_read",
	"book_tome",
	"book_tome_read",
	"book_journal",
	"book_note",
	"book_map",
	"default_food",
	"food_wine",
	"food_beer",			// 60
	"default_ingredient",
	"default_key",
	"key_house",
	"default_potion",
	"potion_health",
	"potion_stam",
	"potion_magic",
	"potion_poison",
	"potion_frost",
	"potion_fire",			// 70
	"potion_shock",
	"default_misc",
	"misc_artifact",
	"misc_clutter",
	"misc_lockpick",
	"misc_soulgem",
	"soulgem_empty",
	"soulgem_partial",
	"soulgem_full",
	"soulgem_grandempty",	// 80
	"soulgem_grandpartial",
	"soulgem_grandfull",
	"soulgem_azura",
	"misc_gem",
	"misc_ore",
	"misc_ingot",
	"misc_hide",
	"misc_strips",
	"misc_leather",
	"misc_wood",			// 90
	"misc_remains",
	"misc_trollskull",
	"misc_torch",
	"misc_goldsack",
	"misc_gold",
	"misc_dragonclaw"
};

static void chk(const char* file, const char* func, int line)
{
	printf("%s (%d): %s\n", file, line, func);
}

static char* GetFormName(TESForm* form)
{
	return *(char**)((uintptr_t)form + 0x28);
}

ItemData::ItemData(InventoryEntryData *a_pEntry, TESForm *owner) : pEntry(a_pEntry), type(Type::kType_None), priority(0), name(),
isStolen(false), isEnchanted(false), isQuestItem(false)
{
	if (!pEntry || !pEntry->type)
		return;

	TESForm *form = pEntry->type;
	
	// set type
	type = GetItemType(form);

	
	// set name
	name = CALL_MEMBER_FN(pEntry,GenerateName)(); // pEntry->GenerateName();

	
	// set isEnchanted
	if (form->IsArmor() || form->IsWeapon())
	{
		if (pEntry->extendDataList && !pEntry->extendDataList->Count() == 0)
		{
			BaseExtraList *baseExtraList = pEntry->extendDataList->GetNthItem(0);
			ExtraEnchantment *exEnchant = reinterpret_cast<ExtraEnchantment *>(baseExtraList->GetByType(kExtraData_Enchantment));
			if (exEnchant && exEnchant->enchant)
				isEnchanted = true;
		}
		
		TESEnchantableForm *enchantForm = dynamic_cast<TESEnchantableForm *>(pEntry->type);
		if (enchantForm && enchantForm->enchantment)
			isEnchanted = true;
	}

	// set isStolen
	TESForm *itemOwner = InventoryEntryData_GetOwner(pEntry); //pEntry->GetOwner();
	if (!itemOwner)
		itemOwner = owner;
	isStolen = InventoryEntryData_IsOwnedBy(pEntry, *g_thePlayer, itemOwner, true);//!pEntry->IsOwnedBy(g_thePlayer, itemOwner, true);

	// set isQuestItem
	isQuestItem = InventoryEntryData_IsQuestItem(pEntry); // pEntry->IsQuestItem();

	// set priority
	enum Priority
	{
		Key,
		Gold,
		LockPick,
		Ammo,
		SoulGem,
		Potion,
		Poison,
		EnchantedWeapon,
		EnchantedArmor,
		Gem,
		Amulet,
		Ring,
		Weapon,
		Armor,
		Other,
		Food = Other
	};
	
	switch (form->formType)
	{
	case kFormType_Ammo:
		priority = Ammo;
		break;
	case kFormType_SoulGem:
		priority = SoulGem;
		break;
	case kFormType_Potion:
	{
		AlchemyItem *alchemyItem = static_cast<AlchemyItem *>(form);
		if (alchemyItem->IsFood())
			priority = Food;
		else if (alchemyItem->IsPoison())
			priority = Poison;
		else
			priority = Potion;
	}
	break;
	case kFormType_Weapon:
		priority = (IsEnchanted()) ? EnchantedWeapon : Weapon;
		break;
	case kFormType_Armor:
		if (IsEnchanted())
			priority = EnchantedArmor;
		else if (type == kType_ArmorAmulet)
			priority = Amulet;
		else if (type == kType_ArmorRing)
			priority = Ring;
		else
			priority = Armor;
		break;
	case kFormType_Key:
		priority = Key;
		break;
	case kFormType_Misc:
		switch (type)
		{
		case kType_MiscGold:
			priority = Gold;
			break;
		case kType_MiscLockPick:
			priority = LockPick;
			break;
		case kType_MiscGem:
			priority = Gem;
			break;
		default:
			priority = Other;
		}
		break;
	default:
		priority = Other;
	}
	
}


ItemData::ItemData(const ItemData &rhs) : pEntry(rhs.pEntry), type(rhs.type), priority(rhs.priority), name(rhs.name),
isStolen(rhs.isStolen), isEnchanted(rhs.isEnchanted), isQuestItem(rhs.isQuestItem)
{
}

ItemData::ItemData(ItemData &&rhs) : pEntry(rhs.pEntry), type(rhs.type), priority(rhs.priority), name(rhs.name),
isStolen(rhs.isStolen), isEnchanted(rhs.isEnchanted), isQuestItem(rhs.isQuestItem)
{
	rhs.pEntry = nullptr;
}


ItemData::~ItemData()
{
	if (pEntry)
		pEntry->Delete();
}



const char * ItemData::GetName() const
{
	return name;
	//return pEntry->GenerateName();
}

UInt32 ItemData::GetCount() const
{
	return pEntry->countDelta;
}

SInt32 ItemData::GetValue() const
{
	return CALL_MEMBER_FN(pEntry, GetValue)();  // pEntry->GetValue();
}

double ItemData::GetWeight() const
{
	return TESForm_GetWeight(pEntry->type);
}

const char * ItemData::GetIcon() const
{
	return strIcons[type];
}

//===================================================================================
// compare
//===================================================================================

typedef int(*FnCompare)(const ItemData &a, const ItemData &b);

static int CompareByType(const ItemData &a, const ItemData &b)
{
	return a.priority - b.priority;
}

static int CompareByStolen(const ItemData &a, const ItemData &b)
{
	SInt32 valueA = a.IsStolen() ? 1 : 0;
	SInt32 valueB = b.IsStolen() ? 1 : 0;

	return valueA - valueB;
}

static int CompareByQuestItem(const ItemData &a, const ItemData &b)
{
	SInt32 valueA = a.IsQuestItem() ? 0 : 1;
	SInt32 valueB = b.IsQuestItem() ? 0 : 1;

	return valueA - valueB;
}

static int CompareByValue(const ItemData &a, const ItemData &b)
{
	SInt32 valueA = a.GetValue();
	SInt32 valueB = b.GetValue();
	return valueA - valueB;
}

static int CompareByCount(const ItemData &a, const ItemData &b)
{
	SInt32 valueA = a.GetCount();
	SInt32 valueB = b.GetCount();
	return valueA - valueB;
}

static int CompareByName(const ItemData &a, const ItemData &b)
{
	//return a.name.compare(b.name);
	return strcmp(a.name, b.name);
}

bool operator<(const ItemData &a, const ItemData &b)
{
	static FnCompare compares[] = {
		&CompareByQuestItem,
		&CompareByStolen,
		&CompareByType,
		&CompareByName,
		&CompareByValue,
		&CompareByCount
	};

	for (FnCompare compare : compares)
	{
		int cmp = compare(a, b);
		if (cmp == 0)
			continue;

		return cmp < 0;
	}

	return a.pEntry < b.pEntry;
}

//===================================================================================
// get type
//===================================================================================

static ItemData::Type GetItemTypeWeapon(TESObjectWEAP *weap)
{
	ItemData::Type type = ItemData::kType_DefaultWeapon;

	switch (weap->type())
	{
	case TESObjectWEAP::GameData::kType_OneHandSword:
		type = ItemData::kType_WeaponSword;
		break;
	case TESObjectWEAP::GameData::kType_OneHandDagger:
		type = ItemData::kType_WeaponDagger;
		break;
	case TESObjectWEAP::GameData::kType_OneHandAxe:
		type = ItemData::kType_WeaponWarAxe;
		break;
	case TESObjectWEAP::GameData::kType_OneHandMace:
		type = ItemData::kType_WeaponMace;
		break;
	case TESObjectWEAP::GameData::kType_TwoHandSword:
		type = ItemData::kType_WeaponGreatSword;
		break;
	case TESObjectWEAP::GameData::kType_TwoHandAxe:
		static BGSKeyword *keywordWarHammer = (BGSKeyword*)TESForm_LookupFormByID(0x06D930);		// WeapTypeWarhammer
		if (dynamic_cast<BGSKeywordForm*>(weap)->HasKeyword(keywordWarHammer))
			type = ItemData::kType_WeaponHammer;
		else
			type = ItemData::kType_WeaponBattleAxe;
		break;
	case TESObjectWEAP::GameData::kType_Bow:
		type = ItemData::kType_WeaponBow;
		break;
	case TESObjectWEAP::GameData::kType_Staff:
		type = ItemData::kType_WeaponStaff;
		break;
	case TESObjectWEAP::GameData::kType_CrossBow:
		type = ItemData::kType_WeaponCrossbow;
		break;
	}

	return type;
}

static bool HasParts(TESObjectARMO * a, UInt32 parts)
{
	return (a->bipedObject.data.parts & parts) != 0;
}

static ItemData::Type GetItemTypeArmor(TESObjectARMO *armor)
{
	static ItemData::Type types[] = {
		ItemData::kType_LightArmorBody,		// 0
		ItemData::kType_LightArmorHead,
		ItemData::kType_LightArmorHands,
		ItemData::kType_LightArmorForearms,
		ItemData::kType_LightArmorFeet,
		ItemData::kType_LightArmorCalves,
		ItemData::kType_LightArmorShield,
		ItemData::kType_LightArmorMask,

		ItemData::kType_ArmorBody,			// 8
		ItemData::kType_ArmorHead,
		ItemData::kType_ArmorHands,
		ItemData::kType_ArmorForearms,
		ItemData::kType_ArmorFeet,
		ItemData::kType_ArmorCalves,
		ItemData::kType_ArmorShield,
		ItemData::kType_ArmorMask,

		ItemData::kType_ClothingBody,		// 16
		ItemData::kType_ClothingHead,
		ItemData::kType_ClothingHands,
		ItemData::kType_ClothingForearms,
		ItemData::kType_ClothingFeet,
		ItemData::kType_ClothingCalves,
		ItemData::kType_ClothingShield,
		ItemData::kType_ClothingMask,

		ItemData::kType_ArmorAmulet,		// 24
		ItemData::kType_ArmorRing,
		ItemData::kType_Circlet,

		ItemData::kType_DefaultArmor		// 27
	};

	UInt32 index = 0;

	if (armor->bipedObject.data.weightClass == BGSBipedObjectForm::kWeight_Light)
	{
		index = 0;
	}
	else if (armor->bipedObject.data.weightClass == BGSBipedObjectForm::kWeight_Heavy)
	{
		index = 8;
	}
	else
	{
#define HAS_PARTS(a,p) ((a->bipedObject.data.parts & (p) ) != 0)

		static BGSKeyword *keywordJewelry = (BGSKeyword*)TESForm_LookupFormByID(0x08F95A);		// VendorItemJewelry
		static BGSKeyword *keywordClothing = (BGSKeyword*)TESForm_LookupFormByID(0x08F95B);	// VendorItemClothing

		if (dynamic_cast<BGSKeywordForm*>(armor)->HasKeyword(keywordClothing))
		{
			index = 16;
		}
		else if (dynamic_cast<BGSKeywordForm*>(armor)->HasKeyword(keywordJewelry))
		{
			if (HAS_PARTS(armor, BGSBipedObjectForm::kPart_Ring))       //armor->HasPartOf(BGSBipedObjectForm::kPart_Amulet))
				index = 24;
			else if (HAS_PARTS(armor,BGSBipedObjectForm::kPart_Ring))
				index = 25;
			else if ((armor->bipedObject.data.parts &  BGSBipedObjectForm::kPart_Circlet) != 0)
				index = 26;
			else
				index = 27;
		}
		else
		{
			index = 27;
		}
	}

	if (index >= 24)
		return types[index];

	if (HAS_PARTS(armor, BGSBipedObjectForm::kPart_Body | BGSBipedObjectForm::kPart_Unnamed10)) // kPart_Unnamed10 = tail
		index += 0;			// body
	else if (HAS_PARTS(armor, BGSBipedObjectForm::kPart_Head | BGSBipedObjectForm::kPart_Hair | BGSBipedObjectForm::kPart_LongHair))
	{
		index += 1;			// head
		if (armor->formID >= 0x061C8B && armor->formID < 0x061CD7)
			index += 6;		// mask
	}
	else if (HAS_PARTS(armor, BGSBipedObjectForm::kPart_Hands))
		index += 2;			// hands
	else if (HAS_PARTS(armor, BGSBipedObjectForm::kPart_Forearms))
		index += 3;			// forarms
	else if (HAS_PARTS(armor, BGSBipedObjectForm::kPart_Feet))
		index += 4;			// forarms
	else if (HAS_PARTS(armor, BGSBipedObjectForm::kPart_Calves))
		index += 5;			// calves
	else if (HAS_PARTS(armor, BGSBipedObjectForm::kPart_Shield))
		index += 6;			// shield
	else
		index = 27;

	return types[index];
}


static ItemData::Type GetItemTypePotion(AlchemyItem *potion)
{
	enum {
		kActorValue_Aggression = 0,
		kActorValue_Confidence,
		kActorValue_Energy,
		kActorValue_Morality,
		kActorValue_Mood,
		kActorValue_Assistance,
		kActorValue_Onehanded,
		kActorValue_Twohanded,
		kActorValue_Marksman,
		kActorValue_Block,
		kActorValue_Smithing,
		kActorValue_HeavyArmor,
		kActorValue_LightArmor,
		kActorValue_Pickpocket,
		kActorValue_Lockpicking,
		kActorValue_Sneak,
		kActorValue_Alchemy,
		kActorValue_Speechcraft,
		kActorValue_Alteration,
		kActorValue_Conjuration,
		kActorValue_Destruction,
		kActorValue_Illusion,
		kActorValue_Restoration,
		kActorValue_Enchanting,
		kActorValue_Health,
		kActorValue_Magicka,
		kActorValue_Stamina,
		kActorValue_Healrate,
		kActorValue_MagickaRate,
		kActorValue_StaminaRate,
		kActorValue_SpeedMult,
		kActorValue_InventoryWeight,
		kActorValue_CarryWeight,
		kActorValue_CritChance,
		kActorValue_MeleeDamage,
		kActorValue_UnarmedDamage,
		kActorValue_Mass,
		kActorValue_VoicePoints,
		kActorValue_VoiceRate,
		kActorValue_DamageResist,
		kActorValue_PoisonResist,
		kActorValue_FireResist,
		kActorValue_ElectricResist,
		kActorValue_FrostResist,
		kActorValue_MagicResist,
		kActorValue_DiseaseResist,
		kActorValue_PerceptionCondition,
		kActorValue_EnduranceCondition,
		kActorValue_LeftAttackCondition,
		kActorValue_RightAttackCondition,
		kActorValue_LeftMobilityCondition,
		kActorValue_RightMobilityCondition,
		kActorValue_BrainCondition,
		kActorValue_Paralysis,
		kActorValue_Invisibility,
		kActorValue_NightEye,
		kActorValue_DetectLifeRange,
		kActorValue_WaterBreathing,
		kActorValue_WaterWalking,
		kActorValue_IgnoreCrippledLimbs,
		kActorValue_Fame,
		kActorValue_Infamy,
		kActorValue_JumpingBonus,
		kActorValue_WardPower,
		kActorValue_RightItemCharge,
		kActorValue_ArmorPerks,
		kActorValue_ShieldPerks,
		kActorValue_WardDeflection,
		kActorValue_Variable01,
		kActorValue_Variable02,
		kActorValue_Variable03,
		kActorValue_Variable04,
		kActorValue_Variable05,
		kActorValue_Variable06,
		kActorValue_Variable07,
		kActorValue_Variable08,
		kActorValue_Variable09,
		kActorValue_Variable10,
		kActorValue_BowSpeedBonus,
		kActorValue_FavorActive,
		kActorValue_FavorSperDay,
		kActorValue_FavorSperDaytimer,
		kActorValue_LeftItemCharge,
		kActorValue_AbsorbChance,
		kActorValue_Blindness,
		kActorValue_WeaponSpeedMult,
		kActorValue_ShoutRecoveryMult,
		kActorValue_BowStaggerBonus,
		kActorValue_Telekinesis,
		kActorValue_FavorPointsBonus,
		kActorValue_LastBribedIntimidated,
		kActorValue_LastFlattered,
		kActorValue_MovementNoiseMult,
		kActorValue_BypassVendorStolenCheck,
		kActorValue_BypassVendorKeywordCheck,
		kActorValue_WaitingForPlayer,
		kActorValue_OnehandedMod,
		kActorValue_TwohandedMod,
		kActorValue_MarksmanMod,
		kActorValue_BlockMod,
		kActorValue_SmithingMod,
		kActorValue_HeavyArmorMod,
		kActorValue_LightArmorMod,
		kActorValue_PickpocketMod,
		kActorValue_LockpickingMod,
		kActorValue_SneakMod,
		kActorValue_AlchemyMod,
		kActorValue_SpeechcraftMod,
		kActorValue_AlterationMod,
		kActorValue_ConjurationMod,
		kActorValue_DestructionMod,
		kActorValue_IllusionMod,
		kActorValue_RestorationMod,
		kActorValue_EnchantingMod,
		kActorValue_OnehandedSkillAdvance,
		kActorValue_TwohandedSkillAdvance,
		kActorValue_MarksmanSkillAdvance,
		kActorValue_BlockSkillAdvance,
		kActorValue_SmithingSkillAdvance,
		kActorValue_HeavyArmorSkillAdvance,
		kActorValue_LightArmorSkillAdvance,
		kActorValue_PickpocketSkillAdvance,
		kActorValue_LockpickingSkillAdvance,
		kActorValue_SneakSkillAdvance,
		kActorValue_AlchemySkillAdvance,
		kActorValue_SpeechcraftSkillAdvance,
		kActorValue_AlterationSkillAdvance,
		kActorValue_ConjurationSkillAdvance,
		kActorValue_DestructionSkillAdvance,
		kActorValue_IllusionSkillAdvance,
		kActorValue_RestorationSkillAdvance,
		kActorValue_EnchantingSkillAdvance,
		kActorValue_LeftWeaponSpeedMult,
		kActorValue_DragonSouls,
		kActorValue_CombatHealthRegenMult,
		kActorValue_OnehandedPowerMod,
		kActorValue_TwohandedPowerMod,
		kActorValue_MarksmanPowerMod,
		kActorValue_BlockPowerMod,
		kActorValue_SmithingPowerMod,
		kActorValue_HeavyarmorPowerMod,
		kActorValue_LightarmorPowerMod,
		kActorValue_PickpocketPowerMod,
		kActorValue_LockpickingPowerMod,
		kActorValue_SneakPowerMod,
		kActorValue_AlchemyPowerMod,
		kActorValue_SpeechcraftPowerMod,
		kActorValue_AlterationPowerMod,
		kActorValue_ConjurationPowerMod,
		kActorValue_DestructionPowerMod,
		kActorValue_IllusionPowerMod,
		kActorValue_RestorationPowerMod,
		kActorValue_EnchantingPowerMod,
		kActorValue_Dragonrend,
		kActorValue_AttackDamageMult,
		kActorValue_HealRateMult,
		kActorValue_MagickaRateMult,
		kActorValue_StaminaRateMult,
		kActorValue_WerewolfPerks,
		kActorValue_VampirePerks,
		kActorValue_GrabActorOffset,
		kActorValue_Grabbed,
		kActorValue_Deprecated05,
		kActorValue_ReflectDamage
	};

	ItemData::Type type = ItemData::kType_DefaultPotion;

	if (potion->IsFood())
	{
		type = ItemData::kType_DefaultFood;

		const static UInt32 ITMPosionUse = 0x000B6435;
		if (potion->itemData.useSound && potion->itemData.useSound->formID == ITMPosionUse)
			type = ItemData::kType_FoodWine;
	}
	else if (potion->IsPoison())
	{
		type = ItemData::kType_PotionPoison;
	}
	else
	{
		type = ItemData::kType_DefaultPotion;

		MagicItem::EffectItem *pEffect = MagicItem_GetCostliestEffectItem(potion, 5, false); //potion->GetCostliestEffectItem(5, false);
		if (pEffect && pEffect->mgef)
		{
			UInt32 primaryValue = pEffect->mgef->properties.primaryValue;
			switch (primaryValue)
			{
			case kActorValue_Health:
				type = ItemData::kType_PotionHealth;
				break;
			case kActorValue_Magicka:
				type = ItemData::kType_PotionMagic;
				break;
			case kActorValue_Stamina:
				type = ItemData::kType_PotionStam;
				break;
			case kActorValue_FireResist:
				type = ItemData::kType_PotionFire;
				break;
			case kActorValue_ElectricResist:
				type = ItemData::kType_PotionShock;
				break;
			case kActorValue_FrostResist:
				type = ItemData::kType_PotionFrost;
				break;
			}
		}
	}

	return type;
}

bool HasKeyword(BGSKeywordForm* o, UInt32 formID)
{
	bool result = false;

	if (o->keywords)
	{
		for (UInt32 idx = 0; idx < o->numKeywords; ++idx)
		{
			if (o->keywords[idx] && o->keywords[idx]->formID == formID)
			{
				result = true;
				break;
			}
		}
	}

	return result;
}

static ItemData::Type GetItemTypeMisc(TESObjectMISC *misc)
{
	ItemData::Type type = ItemData::kType_DefaultMisc;

	static const UInt32 LockPick = 0x00000A;
	static const UInt32 Gold = 0x00000F;
	static const UInt32 Leather01 = 0x000DB5D2;
	static const UInt32 LeatherStrips = 0x000800E4;

	static const UInt32 VendorItemAnimalHide = 0x0914EA;
	static const UInt32 VendorItemDaedricArtifact = 0x000917E8;
	static const UInt32 VendorItemGem = 0x000914ED;
	static const UInt32 VendorItemTool = 0x000914EE;
	static const UInt32 VendorItemAnimalPart = 0x000914EB;
	static const UInt32 VendorItemOreIngot = 0x000914EC;
	static const UInt32 VendorItemClutter = 0x000914E9;
	static const UInt32 VendorItemFireword = 0x000BECD7;

	static const UInt32 RubyDragonClaw = 0x04B56C;
	static const UInt32 IvoryDragonClaw = 0x0AB7BB;
	static const UInt32 GlassCraw = 0x07C260;
	static const UInt32 EbonyCraw = 0x05AF48;
	static const UInt32 EmeraldDragonClaw = 0x0ED417;
	static const UInt32 DiamondClaw = 0x0AB375;
	static const UInt32 IronClaw = 0x08CDFA;
	static const UInt32 CoralDragonClaw = 0x0B634C;
	static const UInt32 E3GoldenClaw = 0x0999E7;
	static const UInt32 SapphireDragonClaw = 0x0663D7;
	static const UInt32 MS13GoldenClaw = 0x039647;

	if (misc->formID == LockPick)
		type = ItemData::kType_MiscLockPick;
	else if (misc->formID == Gold)
		type = ItemData::kType_MiscGold;
	else if (misc->formID == Leather01)
		type = ItemData::kType_MiscLeather;
	else if (misc->formID == LeatherStrips)
		type = ItemData::kType_MiscStrips;
	else if (HasKeyword(&misc->keyword,VendorItemAnimalHide))
		type = ItemData::kType_MiscHide;
	else if (HasKeyword(&misc->keyword,VendorItemDaedricArtifact))
		type = ItemData::kType_MiscArtifact;
	else if (HasKeyword(&misc->keyword,VendorItemGem))
		type = ItemData::kType_MiscGem;
	else if (HasKeyword(&misc->keyword,VendorItemAnimalPart))
		type = ItemData::kType_MiscRemains;
	else if (HasKeyword(&misc->keyword,VendorItemOreIngot))
		type = ItemData::kType_MiscIngot;
	else if (HasKeyword(&misc->keyword,VendorItemClutter))
		type = ItemData::kType_MiscClutter;
	else if (HasKeyword(&misc->keyword,VendorItemFireword))
		type = ItemData::kType_MiscWood;
	else if (misc->formID == RubyDragonClaw
		|| misc->formID == IvoryDragonClaw
		|| misc->formID == GlassCraw
		|| misc->formID == EbonyCraw
		|| misc->formID == EmeraldDragonClaw
		|| misc->formID == DiamondClaw
		|| misc->formID == IronClaw
		|| misc->formID == CoralDragonClaw
		|| misc->formID == E3GoldenClaw
		|| misc->formID == SapphireDragonClaw
		|| misc->formID == MS13GoldenClaw
		)
		type = ItemData::kType_MiscDragonClaw;

	return type;
}


static ItemData::Type GetItemTypeSoulGem(TESSoulGem *gem)
{
	ItemData::Type type = ItemData::kType_MiscSoulGem;

	const static UInt32 DA01SoulGemAzurasStar = 0x063B27;
	const static UInt32 DA01SoulGemBlackStar = 0x063B29;

	if (gem->formID == DA01SoulGemBlackStar || gem->formID == DA01SoulGemAzurasStar)
	{
		type = ItemData::kType_SoulGemAzura;
	}
	else
	{
		if (gem->gemSize < 4)
		{
			if (gem->soulSize == 0)
				type = ItemData::kType_SoulGemEmpty;
			else if (gem->soulSize >= gem->gemSize)
				type = ItemData::kType_SoulGemFull;
			else
				type = ItemData::kType_SoulGemPartial;
		}
		else
		{
			if (gem->soulSize == 0)
				type = ItemData::kType_SoulGemGrandEmpty;
			else if (gem->soulSize >= gem->gemSize)
				type = ItemData::kType_SoulGemGrandFull;
			else
				type = ItemData::kType_SoulGemGrandPartial;
		}
	}

	return type;
}


const ItemData::Type GetItemTypeBook(TESObjectBOOK *book)
{
	ItemData::Type type = ItemData::kType_DefaultBook;

	const static UInt32 VendorItemRecipe = 0x000F5CB0;
	const static UInt32 VendorItemSpellTome = 0x000937A5;

	if (book->data.type == 0xFF || HasKeyword(&book->keyword, VendorItemRecipe))
	{
		type = ItemData::kType_BookNote;
	}
	else if (HasKeyword(&book->keyword,VendorItemSpellTome))
	{
		type = ItemData::kType_BookTome;
	}

	return type;
}

static ItemData::Type GetItemType(TESForm *form)
{
	ItemData::Type type = ItemData::kType_None;

	switch (form->formType)
	{
	case kFormType_ScrollItem:
		type = ItemData::kType_DefaultScroll;
		break;
	case kFormType_Armor:
		type = GetItemTypeArmor(static_cast<TESObjectARMO*>(form));
		break;
	case kFormType_Book:
		type = GetItemTypeBook(static_cast<TESObjectBOOK*>(form));
		break;
	case kFormType_Ingredient:
		type = ItemData::kType_DefaultIngredient;
		break;
	case kFormType_Light:
		type = ItemData::kType_MiscTorch;
		break;
	case kFormType_Misc:
		type = GetItemTypeMisc(static_cast<TESObjectMISC*>(form));
		break;
	case kFormType_Weapon:
		type = GetItemTypeWeapon(static_cast<TESObjectWEAP*>(form));
		break;
	case kFormType_Ammo:
		type = (static_cast<TESAmmo*>(form)->isBolt()) ? ItemData::kType_WeaponBolt : ItemData::kType_WeaponArrow;
		break;
	case kFormType_Key:
		type = ItemData::kType_DefaultKey;
		break;
	case kFormType_Potion:
		type = GetItemTypePotion(static_cast<AlchemyItem*>(form));
		break;
	case kFormType_SoulGem:
		type = GetItemTypeSoulGem(static_cast<TESSoulGem*>(form));
		break;
	}

	return type;
}

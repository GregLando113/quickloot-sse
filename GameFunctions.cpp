#include "GameFunctions.h"


/* For Skyrim SE v1.5.3 */

// \x33\xDB\x41\x0F\xB6\xF0\x8B\xFB\x48\x85\xC0\x74\x1E
// xxxxxxxxxxxxx
// -0x13
RelocAddr<TESObjectREFR_GetInventoryItemCount_t*> TESObjectREFR_GetInventoryItemCount (0x0028E320);


// \x48\x0F\x44\xD6\x48\x85\xD2\x0F\x85\x00\x00\x00\x00\x48\x85\xC0\x0F\x85
// xxxxxxxxx????xxxxx
// -0x2E
RelocAddr<TESForm_GetOwner_t*> TESForm_GetOwner (0x002A6740);

// 48 85 C9 0F 84 C1 00 00 00 0F B6 41 1A 83 C0 E9 83 F8 1D 0F 87 B1 00 00 00 -0x16
RelocAddr<TESForm_GetWeight_t*>  TESForm_GetWeight (0x001A1800);

// TODO: FIND THIS BITCH
RelocAddr<TESForm_LookupFormByID_t*>   TESForm_LookupFormByID ();

/*
00007FF7295D72F0 | 48 8D 15 B9 61 2F 01         | lea rdx,qword ptr ds:[7FF72A8CD4B0]                      | 7FF72A8CD4B0:"Found ReferenceHandle extra on invalid %s ref '%s' (%08X)"
00007FF7295D72F7 | B9 05 00 00 00               | mov ecx,5                                                |
00007FF7295D72FC | E8 BF 28 EF FF               | call skyrimse.7FF7294C9BC0                               |
00007FF7295D7301 | BA 1C 00 00 00               | mov edx,1C                                               |
00007FF7295D7306 | 49 8D 4E 70                  | lea rcx,qword ptr ds:[r14+70]                            |
00007FF7295D730A | E8 71 C3 EA FF               | call skyrimse.7FF729483680                               |
00007FF7295D730F | 90                           | nop                                                      |
00007FF7295D7310 | 48 85 DB                     | test rbx,rbx                                             |
00007FF7295D7313 | 74 1C                        | je skyrimse.7FF7295D7331                                 |
00007FF7295D7315 | 48 8D 4B 20                  | lea rcx,qword ptr ds:[rbx+20]                            |
00007FF7295D7319 | 83 C8 FF                     | or eax,FFFFFFFF                                          |
00007FF7295D731C | F0 0F C1 41 08               | lock xadd dword ptr ds:[rcx+8],eax                       |
00007FF7295D7321 | FF C8                        | dec eax                                                  |
00007FF7295D7323 | A9 FF 03 00 00               | test eax,3FF                                             |
00007FF7295D7328 | 75 07                        | jne skyrimse.7FF7295D7331                                |
00007FF7295D732A | 48 8B 01                     | mov rax,qword ptr ds:[rcx]                               |
00007FF7295D732D | FF 50 08                     | call qword ptr ds:[rax+8]                                |
00007FF7295D7330 | 90                           | nop                                                      |
00007FF7295D7331 | 41 F7 47 40 20 00 00 08      | test dword ptr ds:[r15+40],8000020                       |
00007FF7295D7339 | 74 49                        | je skyrimse.7FF7295D7384                                 |
00007FF7295D733B | 49 8D 4E 70                  | lea rcx,qword ptr ds:[r14+70]                            |
00007FF7295D733F | E8 7C 96 E9 FF               | call skyrimse.7FF7294709C0                               |
00007FF7295D7344 | 49 8B CE                     | mov rcx,r14                                              |
00007FF7295D7347 | E8 24 1C F5 FF               | call skyrimse.7FF729528F70                               |
00007FF7295D734C | 48 85 C0                     | test rax,rax                                             |
00007FF7295D734F | 74 0D                        | je skyrimse.7FF7295D735E                                 |
00007FF7295D7351 | 49 8B D7                     | mov rdx,r15                                              |
00007FF7295D7354 | 48 8B C8                     | mov rcx,rax                                              |
00007FF7295D7357 | E8 C4 51 F6 FF               | call skyrimse.7FF72953C520                               |
00007FF7295D735C | EB 26                        | jmp skyrimse.7FF7295D7384                                |
00007FF7295D735E | 49 8B D6                     | mov rdx,r14                                              |
00007FF7295D7361 | 48 8D 4C 24 48               | lea rcx,qword ptr ss:[rsp+48]                            |
00007FF7295D7366 | E8 C5 1F F5 FF               | call skyrimse.7FF729529330                               | < ====== ctor
00007FF7295D736B | 90                           | nop                                                      |
00007FF7295D736C | 49 8B D7                     | mov rdx,r15                                              |
00007FF7295D736F | 48 8D 4C 24 48               | lea rcx,qword ptr ss:[rsp+48]                            |
00007FF7295D7374 | E8 A7 51 F6 FF               | call skyrimse.7FF72953C520                               |
00007FF7295D7379 | 90                           | nop                                                      |
00007FF7295D737A | 48 8D 4C 24 48               | lea rcx,qword ptr ss:[rsp+48]                            |
00007FF7295D737F | E8 9C 20 F5 FF               | call skyrimse.7FF729529420                               |< ======= dtor
00007FF7295D7384 | 41 8B 47 40                  | mov eax,dword ptr ds:[r15+40]                            |
*/
RelocAddr<ECCData_ctor_t*>  ECCData_ctor (0x001D9330);
RelocAddr<ECCData_dtor_t*>  ECCData_dtor (0x001D9420);

// 33 FF 44 8B E7 48 8B 49 08 0F B7 D7 66 89 55 40 48 85 C9 74 15 8B C7 80 79 1A 3E -0x32
RelocAddr<ECCData_InitContainer_t*> ECCData_InitContainer (0x001E9E70);

// 65 48 8B 04 25 58 00 00 00 B9 68 07 00 00 4E 8B 34 C0 4C 03 F1 41 8B 3E 89 7C 24 60 41 C7 06 61 00 00 00 49 8D 5F 10 48 89 5C 24 68 48 8B CB
// first entry
RelocAddr<BaseExtraList_SetInventoryChanges_t*> BaseExtraList_SetInventoryChanges (0x0010F6C0);

// 48 85 FF 74 06 0F B7 47 10 EB 05 B8 01 00 00 00 48 8B 5C 24 48 48 83 C4 30 5F C3 -0x47
RelocAddr<BaseExtraList_GetItemCount_t*>	    BaseExtraList_GetItemCount (0x001138B0);

// 48 8B F0 48 85 C0 75 04 48 8B 73 40 33 C0 80 7E 1A 2B 48 0F 44 C6 48 3B EE 0F 84 80 00 00 00 80 7D 1A 0B -0x4D
RelocAddr<InventoryEntryData_IsOwnedBy_t*>		 InventoryEntryData_IsOwnedBy (0x001D76C0);

// 48 8B 59 08 40 32 FF 48 85 DB 74 4A 48 89 74 24 30 0F 1F 44 00 00 48 83 7B 08 00 75 06 -0xA
RelocAddr<InventoryEntryData_IsQuestItem_t*>		 InventoryEntryData_IsQuestItem (0x001D6CD0);
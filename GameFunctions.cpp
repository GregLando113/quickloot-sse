#include "GameFunctions.h"


/* For Skyrim SE v1.5.16 */
// 1.5.16 = +0x80 bytes from 1.5.3

// \x33\xDB\x41\x0F\xB6\xF0\x8B\xFB\x48\x85\xC0\x74\x1E
// xxxxxxxxxxxxx
// -0x13
RelocAddr<TESObjectREFR_GetInventoryItemCount_t*> TESObjectREFR_GetInventoryItemCount (0x0028E320 + 0x80);

/*
00007FF644240DC7 | 48 C7 44 24 30 00 00 00 00   | mov qword ptr ss:[rsp+30],0                              |
00007FF644240DD0 | 48 8D 54 24 30               | lea rdx,qword ptr ss:[rsp+30]                            |
00007FF644240DD5 | 48 8B C8                     | mov rcx,rax                                              |
00007FF644240DD8 | E8 E3 23 89 FF               | call skyrimse.7FF643AD31C0                               | < =================== TESObjectREFR_LookupRefByHandle
00007FF644240DDD | 90                           | nop                                                      |
00007FF644240DDE | 48 8B 5C 24 30               | mov rbx,qword ptr ss:[rsp+30]                            |
00007FF644240DE3 | 48 85 DB                     | test rbx,rbx                                             |
00007FF644240DE6 | 74 3A                        | je skyrimse.7FF644240E22                                 |
00007FF644240DE8 | 45 33 C0                     | xor r8d,r8d                                              |
00007FF644240DEB | 48 8B 15 66 F6 5B 02         | mov rdx,qword ptr ds:[7FF646800458]                      |
00007FF644240DF2 | 48 8B CB                     | mov rcx,rbx                                              |
00007FF644240DF5 | E8 76 C3 C5 FF               | call skyrimse.7FF643E9D170                               |
00007FF644240DFA | 8B 83 E0 00 00 00            | mov eax,dword ptr ds:[rbx+E0]                            |
00007FF644240E00 | C1 E8 1E                     | shr eax,1E                                               |
00007FF644240E03 | A8 01                        | test al,1                                                |
00007FF644240E05 | 75 35                        | jne skyrimse.7FF644240E3C                                |
00007FF644240E07 | 45 33 C9                     | xor r9d,r9d                                              |
00007FF644240E0A | 4C 8B 05 47 F6 5B 02         | mov r8,qword ptr ds:[7FF646800458]                       |
00007FF644240E11 | 48 8B D3                     | mov rdx,rbx                                              |
00007FF644240E14 | 48 8B 8B F0 00 00 00         | mov rcx,qword ptr ds:[rbx+F0]                            |
00007FF644240E1B | E8 90 D7 CC FF               | call skyrimse.7FF643F0E5B0                               |
00007FF644240E20 | EB 1A                        | jmp skyrimse.7FF644240E3C                                |
00007FF644240E22 | 48 8B 07                     | mov rax,qword ptr ds:[rdi]                               |
00007FF644240E25 | 41 B9 01 00 00 00            | mov r9d,1                                                |
00007FF644240E2B | 44 8B C6                     | mov r8d,esi                                              |
00007FF644240E2E | 48 8D 15 4B 21 D7 00         | lea rdx,qword ptr ds:[7FF644FB2F80]                      | 7FF644FB2F80:"No nearby NPC found to sound alarm, alarm aborted."
00007FF644240E35 | 48 8B CF                     | mov rcx,rdi                                              |
00007FF644240E38 | FF 50 10                     | call qword ptr ds:[rax+10]                               |*/
RelocAddr<TESObjectREFR_LookupRefByHandle_t*>     TESObjectREFR_LookupRefByHandle   (0x002131C0 + 0x80);

// \x48\x0F\x44\xD6\x48\x85\xD2\x0F\x85\x00\x00\x00\x00\x48\x85\xC0\x0F\x85
// xxxxxxxxx????xxxxx
// -0x2E
RelocAddr<TESForm_GetOwner_t*> TESForm_GetOwner (0x002A6740 + 0x80);

// 48 85 C9 0F 84 C1 00 00 00 0F B6 41 1A 83 C0 E9 83 F8 1D 0F 87 B1 00 00 00 -0x16
RelocAddr<TESForm_GetWeight_t*>  TESForm_GetWeight (0x001A1800 + 0x80);

// 48 85 D2 74 2F 8B 4B 0C FF C9 8B 44 24 48 48 23 C8 48 8D 04 49 48 8D 04 C2 48 83 78 10 00 74 14 -0x4C
RelocAddr<TESForm_LookupFormByID_t*>   TESForm_LookupFormByID (0x00194300 + 0x80);

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
RelocAddr<ECCData_ctor_t*>  ECCData_ctor (0x001D9330 + 0x80);
RelocAddr<ECCData_dtor_t*>  ECCData_dtor (0x001D9420 + 0x80);

// 33 FF 44 8B E7 48 8B 49 08 0F B7 D7 66 89 55 40 48 85 C9 74 15 8B C7 80 79 1A 3E -0x32
RelocAddr<ECCData_InitContainer_t*> ECCData_InitContainer (0x001E9E70 + 0x80);

// 65 48 8B 04 25 58 00 00 00 B9 68 07 00 00 4E 8B 34 C0 4C 03 F1 41 8B 3E 89 7C 24 60 41 C7 06 61 00 00 00 49 8D 5F 10 48 89 5C 24 68 48 8B CB
// first entry
RelocAddr<BaseExtraList_SetInventoryChanges_t*> BaseExtraList_SetInventoryChanges (0x0010F6C0 + 0x80);

// 48 85 FF 74 06 0F B7 47 10 EB 05 B8 01 00 00 00 48 8B 5C 24 48 48 83 C4 30 5F C3 -0x47
RelocAddr<BaseExtraList_GetItemCount_t*>	    BaseExtraList_GetItemCount (0x001138B0 + 0x80);

// 48 8B 59 08 48 85 DB 74 22 48 8B 1B 48 85 DB 74 1A 48 8B CB (first match)
RelocAddr<InventoryEntryData_GetOwner_t*>		 InventoryEntryData_GetOwner (0x001D6750 + 0x80);

// 48 8B F0 48 85 C0 75 04 48 8B 73 40 33 C0 80 7E 1A 2B 48 0F 44 C6 48 3B EE 0F 84 80 00 00 00 80 7D 1A 0B -0x4D
RelocAddr<InventoryEntryData_IsOwnedBy_t*>		InventoryEntryData_IsOwnedBy (0x001D76C0 + 0x80);

// 48 8B 59 08 40 32 FF 48 85 DB 74 4A 48 89 74 24 30 0F 1F 44 00 00 48 83 7B 08 00 75 06 -0xA
RelocAddr<InventoryEntryData_IsQuestItem_t*>    InventoryEntryData_IsQuestItem (0x001D6CD0 + 0x80);

// 49 89 5B 10 49 89 73 18 33 C0 49 89 43 D8 -0x10
RelocAddr<MagicItem_GetCostliestEffectItem_t*>	 MagicItem_GetCostliestEffectItem (0x00101DC0 + 0x80);
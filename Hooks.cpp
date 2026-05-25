#include "Hooks.h"

#include "obse/GameAPI.h"
#include "obse/GameObjects.h"
#include "obse/NiNodes.h"
#include "obse/NiControllers.h"
#include <obse_common/SafeWrite.h>

#include <windows.h>
#include <cstdint>
#include <cstring>

#include <Conditions.h>
#include <Core.h>
#include <unordered_set>
#include <utility.h>
#include <GameTasks.h>

#include "MinHook/include/MinHook.h"

#define AnimString "_CAF"

bool g_cafDebug = true;

// Use maps instead:
std::unordered_map<AnimSequenceSingle*, BSAnimGroupSequence*> g_singleCAFCache;
std::unordered_map<AnimSequenceMultiple*, std::unordered_map<int, BSAnimGroupSequence*>> g_multipleCAFCache;

static std::unordered_set<AnimSequenceMultiple*> g_cafInjectedMultiples;

static std::unordered_map<AnimSequenceMultiple*, Actor*> g_multipleToActor;

static std::unordered_map<ActorAnimData*, Actor*> g_animDataToActor;

static std::unordered_map<AnimSequenceSingle*, BSAnimGroupSequence*> g_activeCafAnims;

std::unordered_map<NiControllerManager*, Actor*> g_managerToActorMap;

std::unordered_map<NiControllerManager*, std::unordered_map<UInt32, bool>> g_groupBusy;

static thread_local int g_loadAnimDepth = 0;

static UInt32 g_animationFrame = 0;

std::unordered_map<NiControllerManager*, UInt32> g_mgrLastTickSingle;
std::unordered_map<NiControllerManager*, UInt32> g_mgrLastTickMultiple;

using _65E900 = char(__thiscall*)(TESObjectREFR*);
_65E900 g_original_65E900 = nullptr;

void __fastcall Hook_65E900(TESObjectREFR* thisPtr, void*)
{
	g_animationFrame++;
	ThisStdCall(0x65E900, thisPtr);
}

void InstallOnUpdateHook()
{
	WriteRelCall(0x40DC70, (UInt32)&Hook_65E900);
}

struct CAFKey
{
	Actor* actor;
	NiControllerManager* mgr;
	UInt32 group;
	const AnimOverrideRule* rule;
};

struct CAFKeyHash
{
	size_t operator()(const CAFKey& k) const noexcept
	{
		return ((size_t)k.actor >> 4) ^
			((size_t)k.mgr << 1) ^
			(k.group * 2654435761u) ^
			(size_t)k.rule;
	}
};

struct CAFKeyEq
{
	bool operator()(const CAFKey& a, const CAFKey& b) const noexcept
	{
		return a.actor == b.actor &&
			a.mgr == b.mgr &&
			a.group == b.group &&
			a.rule == b.rule;
	}
};

static std::unordered_map<
	CAFKey,
	BSAnimGroupSequence*,
	CAFKeyHash,
	CAFKeyEq
> g_actorCAFSeqs;

struct SeqKey
{
	NiControllerManager* mgr;
	BSAnimGroupSequence* seq;

	bool operator==(const SeqKey& o) const
	{
		return mgr == o.mgr && seq == o.seq;
	}
};

struct SeqKeyHash
{
	size_t operator()(const SeqKey& k) const
	{
		return (size_t)k.mgr ^ (size_t)k.seq;
	}
};

static std::unordered_set<SeqKey, SeqKeyHash> g_added;

struct CAFGroupLock
{
	bool* flag;

	CAFGroupLock(std::unordered_map<NiControllerManager*, std::unordered_map<UInt32, bool>>& map,
		NiControllerManager* mgr,
		UInt32 group)
	{
		flag = &map[mgr][group];
		*flag = true;
	}

	~CAFGroupLock()
	{
		*flag = false;
	}
};

struct CAFSequence
{
	BSAnimGroupSequence* seq;          // the CAF animation sequence
	const AnimOverrideRule* rule;      // rule that owns it
	bool firstPerson;
};

std::unordered_map<
	UInt32,
	std::vector<CAFSequence>
> g_cafSequencesByGroupFP;

std::unordered_map<
	UInt32,
	std::vector<CAFSequence>
> g_cafSequencesByGroupTP;

thread_local bool g_inLoadAnimGroup = false;
thread_local BSAnimGroupSequence* g_pendingCAFSequence = nullptr;

UInt32 g_originalAddMultiple;
UInt32 g_originalGetSingle;
UInt32 g_originalGetMultiple;
UInt32 g_originalAddAnimation;

void* g_trampAddMultiple = nullptr;
void* g_trampGetSingle = nullptr;
void* g_trampGetMultiple = nullptr;
void* g_trampAddAnimation = nullptr;

ActorAnimData* g_currentAnimData = nullptr;
UInt32 g_currentActorRefID = 0;


typedef void(__thiscall* _AddSingle)(
	AnimSequenceSingle*,
	BSAnimGroupSequence*
	);

_AddSingle g_originalAddSingle = nullptr;
void* g_trampAddSingle = nullptr;



// Hook the KF model loader - IT'S __thiscall, NOT __cdecl!
typedef int(__thiscall* _LoadKFModel)(void* This, const char* path);
_LoadKFModel g_originalLoadKFModel = nullptr;

typedef BSAnimGroupSequence* (__thiscall* _BSAnimGroupSequenceCtor)(
	BSAnimGroupSequence*, // this (0x6C bytes)
	void*,               // BSAnimGroup*   (kfModel+0x08)
	void*                // NiSequenceData* (kfModel+0x04)
	);

static _BSAnimGroupSequenceCtor g_BSAnimGroupSequenceCtor =
(_BSAnimGroupSequenceCtor)0x0049FD90;

struct CAFCondition
{
	AnimConditionFn fn;
	std::string     arg;
};

struct CAFAnimEntry
{
	BSAnimGroupSequence* seq;
	const AnimOverrideRule* rule;
};

struct CAFAnimData
{
	std::vector<CAFAnimEntry> entries;
};

std::unordered_map<UInt32, CAFAnimData> g_cafAnimData;

static std::unordered_map<
	UInt32,
	std::vector<BSAnimGroupSequence*>
> g_cafSequences;


inline const char* GetFileName(const char* path)
{
	if (!path)
		return nullptr;

	const char* lastSlash = strrchr(path, '\\');
	if (lastSlash)
		return lastSlash + 1;

	return path;
}

static bool StrContainsI(const char* haystack, const char* needle)
{
	if (!haystack || !needle || !*needle)
		return false;

	std::string h(haystack);
	std::string n(needle);

	std::transform(h.begin(), h.end(), h.begin(),
		[](unsigned char c) { return std::tolower(c); });

	std::transform(n.begin(), n.end(), n.begin(),
		[](unsigned char c) { return std::tolower(c); });

	return h.find(n) != std::string::npos;
}


static bool EndsWithI(const char* fullPath, const char* fileName)
{
	if (!fullPath || !fileName)
		return false;
	_MESSAGE("fullPath: %s", fullPath);
	_MESSAGE("fileName: %s", fileName);
	size_t fullLen = strlen(fullPath);
	size_t fileLen = strlen(fileName);

	if (fileLen == 0 || fileLen > fullLen)
		return false;

	const char* tail = fullPath + (fullLen - fileLen);

	return _stricmp(tail, fileName) == 0;
}

static std::string NormalizeAnimString(const char* s)
{
	std::string out;
	if (!s) return out;

	for (; *s; ++s)
	{
		char c = *s;
		if (isalnum((unsigned char)c))
			out.push_back((char)tolower((unsigned char)c));
	}
	return out;
}

static bool AnimStringMatch(const char* filePath, const char* rule)
{
	if (!filePath || !rule)
		return false;

	std::string f = NormalizeAnimString(GetFileName(filePath));
	std::string r = NormalizeAnimString(GetFileName(rule));

	// remove trailing "kf"
	auto stripKF = [](std::string& s) {
		if (s.size() >= 2 &&
			s.substr(s.size() - 2) == "kf")
		{
			s.erase(s.size() - 2);
		}
		};

	stripKF(f);
	stripKF(r);

	return f.find(r) != std::string::npos
		|| r.find(f) != std::string::npos;
}

static void AddCAFAnimGroupSequence(
	UInt32 refID,
	BSAnimGroupSequence* seq,
	const AnimOverrideRule& rule
)
{
	if (!seq)
		return;

	CAFAnimData& caf = g_cafAnimData[refID];

	caf.entries.push_back({
		seq,
		&rule
		});

}

void RegisterCAFSequenceFP(BSAnimGroupSequence* seq, UInt32 expectedGroup)
{
	auto it = g_animOverrideRules.find(expectedGroup);
	if (it == g_animOverrideRules.end()) return;

	for (AnimOverrideRule& rule : it->second)
	{
		if (AnimStringMatch(seq->filePath, rule.replacementFile.c_str()))
		{
			g_cafSequencesByGroupFP[expectedGroup].push_back({ seq, &rule });
			break;
		}
	}
}

void RegisterCAFSequence(BSAnimGroupSequence* seq)
{

	if (!seq || !seq->animGroup || !seq->filePath)
	{
		_MESSAGE("CAF: RegisterCAFSequence — null check failed seq=%p", seq);
		return;
	}

	UInt32 group = seq->animGroup->animGroup;

	const char* fileName = GetFileName(seq->filePath);

	auto it = g_animOverrideRules.find(group);
	if (it == g_animOverrideRules.end())
	{
		_MESSAGE("CAF REGISTER: NO RULES FOUND for group %u", group);
		return;
	}

	for (AnimOverrideRule& rule : it->second)
	{

		if (AnimStringMatch(fileName, rule.replacementFile.c_str()))
		{
			bool isFPFile = StrContainsI(rule.replacementFile.c_str(), "_1stPerson")
				|| StrContainsI(fileName, "_1stPerson");

			auto& target = isFPFile
				? g_cafSequencesByGroupFP[group]
				: g_cafSequencesByGroupTP[group];

			target.push_back({ seq, &rule, isFPFile });

			//AddCAFAnimGroupSequence(g_currentActorRefID, seq, rule);

			break;
		}
	}
}

static std::unordered_map<AnimSequenceSingle*, Actor*> g_singleToActor;

void __fastcall AddSingleHook(
	AnimSequenceSingle* This,
	void*,
	BSAnimGroupSequence* anim
)
{
	// IMPORTANT: do NOT touch actor globals here

	// Let engine behave normally first
	ThisStdCall((UInt32)g_originalAddSingle, This, anim);

	// Only bind if we already know actor from a SAFE source
	auto it = g_animDataToActor.begin();

	for (; it != g_animDataToActor.end(); ++it)
	{
		ActorAnimData* data = it->first;
		Actor* actor = it->second;

		if (!data || !actor)
			continue;

		// bind ONLY if this AnimSequenceSingle belongs to this actor's animData
		if (actor->GetAnimData() == data)
		{
			g_singleToActor[This] = actor;
			break;
		}
	}
}

// The function you already hook — call it directly for preloading
typedef AnimSequenceSingle* (__thiscall* _ActorAnimData_AddAnimation)(
	ActorAnimData*,   // this
	kfModel*,         // the loaded KF
	bool              // bDeferred — pass 0!
	);
static _ActorAnimData_AddAnimation g_addAnimation =
(_ActorAnimData_AddAnimation)0x00474070;

thread_local bool g_loadingCAF = false;


static bool g_cafNeedsReregistration = false;

static std::unordered_map<AnimSequenceSingle*, BSAnimGroupSequence*> g_vanillaSingleAnims;


ActorAnimData* (__thiscall* DisposeActorAnimData)(ActorAnimData*) = (ActorAnimData * (__thiscall*)(ActorAnimData*))0x475B60;
ActorAnimData* __fastcall DisposeActorAnimDataHook(ActorAnimData* This, UInt32 edx)
{
	g_animDataToActor.erase(This);
	g_vanillaSingleAnims.clear();
	return DisposeActorAnimData(This);
}

void InstallDisposeActorAnimDataHook()
{
	WriteRelCall(0x41E9FB, (UInt32)&DisposeActorAnimDataHook);
	WriteRelCall(0x429920, (UInt32)&DisposeActorAnimDataHook);
	WriteRelCall(0x650009, (UInt32)&DisposeActorAnimDataHook);
	WriteRelCall(0x6520ED, (UInt32)&DisposeActorAnimDataHook);
	WriteRelCall(0x65F9BE, (UInt32)&DisposeActorAnimDataHook);
	WriteRelCall(0x66960C, (UInt32)&DisposeActorAnimDataHook);
	WriteRelCall(0x66B3FB, (UInt32)&DisposeActorAnimDataHook);
}

bool(__thiscall* AddAnimation)(ActorAnimData*, kfModel*, UInt8) = (bool(__thiscall*)(ActorAnimData*, kfModel*, UInt8))0x474070;
bool __fastcall AddAnimationHook(ActorAnimData* This, UInt32 edx, kfModel* Model, UInt8 Arg)
{
	if (g_cafDebug)
		_MESSAGE("AddAnimation called, model=%p", Model);

	// Call original
	bool result = (bool)ThisStdCall(g_originalAddAnimation, This, Model, Arg);

	// After vanilla animations are added, add CAF animations
	if (g_currentAnimActor && result)
	{
		UInt32 group = g_currentAnimGroup;

		auto ruleIt = g_animOverrideRules.find(group);
		if (ruleIt != g_animOverrideRules.end())
		{
			for (const AnimOverrideRule& rule : ruleIt->second)
			{
				if (ConditionsPass(rule, g_currentAnimActor, group))
				{
					// TODO: Load the CAF KF file here
					// You need to create a kfModel* for the CAF file
					// Then call AddAnimation again with it

					if (g_cafDebug)
					{
						_MESSAGE(
							"CAF: Should load additional KF: %s",
							rule.replacementFile.c_str()
						);
					}
				}
			}
		}
	}

	return result;
}


struct PendingCAF
{
	UInt32 group;
	std::string path;
};

static std::vector<PendingCAF> g_pendingCAF;

BSAnimGroupSequence* FindCAFSequenceForGroup(
	UInt32 animGroup,
	const AnimOverrideRule& rule,
	bool firstPerson
)
{
	auto& map = firstPerson
		? g_cafSequencesByGroupFP
		: g_cafSequencesByGroupTP;

	auto it = map.find(animGroup);
	if (it == map.end())
		return nullptr;

	BSAnimGroupSequence* fallback = nullptr;

	for (const CAFSequence& caf : it->second)
	{
		if (!caf.seq || !caf.rule)
			continue;

		if (!AnimStringMatch(caf.seq->filePath, rule.replacementFile.c_str()))
			continue;

		// exact match wins immediately
		return caf.seq;
	}

	// optional fallback behavior (important for debugging)
	return fallback;
}


typedef bool(__thiscall* _NiControllerManagerAddSequence)(
	void* mgr, BSAnimGroupSequence* seq, int, int);
static _NiControllerManagerAddSequence g_addSequence =
(_NiControllerManagerAddSequence)0x006C5760;


void(__thiscall* AddMultiple)(AnimSequenceMultiple*, BSAnimGroupSequence*) = (void(__thiscall*)(AnimSequenceMultiple*, BSAnimGroupSequence*))0x471930;
void __fastcall AddMultipleHook(
	AnimSequenceMultiple* This,
	UInt32,
	BSAnimGroupSequence* seq
)
{

	ThisStdCall(g_originalAddMultiple, This, seq);
}

void RemoveAnimGroupSequence(AnimSequenceBase* This, BSAnimGroupSequence* seqToRemove)
{
	// 1. Cast to the manager's sequence array
	// Assuming the NiTArray is at offset 0x3C
	NiTArray<BSAnimGroupSequence*>* seqList = (NiTArray<BSAnimGroupSequence*>*)((char*)This + 0x3C);

	for (UInt16 i = 0; i < seqList->numObjs; i++)
	{
		if (seqList->data[i] == seqToRemove)
		{
			// 2. Perform the shift
			// Move all subsequent pointers down by 1 to fill the hole
			for (UInt16 j = i; j < seqList->numObjs - 1; j++)
			{
				seqList->data[j] = seqList->data[j + 1];
			}

			// 3. Decrement the count
			seqList->numObjs--;
			seqList->data[seqList->numObjs] = nullptr; // Null the tail
			break;
		}
	}
}



typedef bool(__thiscall* _NiTMapGetAt)(void* map, UInt16 key, void** outVal);
static _NiTMapGetAt g_animsMapGetAt = (_NiTMapGetAt)0x00470960;

static std::unordered_set<BSAnimGroupSequence*> g_registeredWithManager;

// Force the controller manager to update its sequence cache
typedef void(__thiscall* _UpdateSequence)(NiControllerManager* mgr, NiControllerSequence* seq);
_UpdateSequence UpdateSeq = (_UpdateSequence)0x006C7A20; // Verify this address in your binary

BSAnimGroupSequence* (__thiscall* GetAnimGroupSequenceSingle)(AnimSequenceSingle*, int) = (BSAnimGroupSequence * (__thiscall*)(AnimSequenceSingle*, int))0x00471710;
// Global tracking
std::map<UInt32, bool> g_actorRegistrationState;

// Track the LAST sequence object we registered for this actor
std::unordered_set<AnimSequenceSingle*> g_registeredSingles;

std::unordered_map<NiControllerManager*, Actor*> g_managerActorCache;

bool IsFirstPersonContext(Actor* actor, AnimSequenceSingle* seq)
{
	PlayerCharacter* pc = OBLIVION_CAST(actor, Actor, PlayerCharacter);
	if (!pc || !g_thePlayer || pc != *g_thePlayer) return false;
	return pc->isThirdPerson == 0;
}

typedef void(__thiscall* sub_474510_t)(ActorAnimData* animData, TESObjectREFR* a2);
sub_474510_t Original_sub_474510 = nullptr;

void __fastcall sub_474510_Detour(ActorAnimData* animData, void* edx, TESObjectREFR* a2)
{
	// A2 is the Actor, 'This' is the AnimData. 
	// This is the point of truth for the link.
	if (a2 && animData->manager)
	{
		g_managerActorCache[animData->manager] = (Actor*)a2;
	}

	// Call the original to ensure the engine initializes the AnimData properly
	Original_sub_474510(animData, a2);
}

void InstallSub_474510_Detour()
{
	if (MH_CreateHook((LPVOID)0x00474510, (LPVOID)&sub_474510_Detour, (LPVOID*)&Original_sub_474510) == MH_OK)
	{
		MH_EnableHook((LPVOID)0x00474510);
	}
}

BSAnimGroupSequence* GetOrCreateActorCAFSeqFP(
	Actor* actor, UInt32 group, const AnimOverrideRule& rule,
	NiControllerManager* mgr)
{
	CAFKey key{ actor, mgr, group, &rule };

	auto it = g_actorCAFSeqs.find(key);
	if (it != g_actorCAFSeqs.end())
	{
		BSAnimGroupSequence* cached = it->second;

		// IMPORTANT: validate pointer still belongs to this manager
		if (cached && cached->controllerMgr == mgr)
			return cached;

		// stale entry cleanup
		g_actorCAFSeqs.erase(it);
	}

	// Find the template sequence
	auto cafIt = g_cafSequencesByGroupFP.find(group);
	if (cafIt == g_cafSequencesByGroupFP.end()) return nullptr;
	BSAnimGroupSequence* tmpl = cafIt->second[0].seq;
	if (!tmpl) return nullptr;

	// Clone it
	void* modelLoader = *(void**)0x00B33A1C;
	std::string path = "Characters\\_1stPerson\\" + rule.replacementFile + ".kf";

	g_loadingCAF = true;
	kfModel* model = (kfModel*)g_originalLoadKFModel(modelLoader, path.c_str());
	g_loadingCAF = false;

	if (!model || !model->animGroup) return nullptr;

	BSAnimGroupSequence* seq =
		(BSAnimGroupSequence*)FormHeap_Allocate(0x6C);
	if (!seq) return nullptr;

	//memset(seq, 0, 0x6C);
	g_BSAnimGroupSequenceCtor(seq, model->animGroup, model->controllerSequence);
	//seq->controllerMgr = mgr;
	//seq->m_uiRefCount = 1;

	RegisterCAFSequence(seq); // reuses existing matching logic
	if (seq && seq->filePath)
	{
		// Prefix with marker that idle picker won't load
		std::string fakePath = "_CAF_INTERNAL_" + std::string(seq->filePath);
		char* newPath = (char*)FormHeap_Allocate(fakePath.size() + 1);
		strcpy_s(newPath, fakePath.size() + 1, fakePath.c_str());
		seq->filePath = newPath;
	}

	g_actorCAFSeqs[key] = seq;

	return seq;
}

BSAnimGroupSequence* GetOrCreateActorCAFSeqTP(
	Actor* actor, UInt32 group, const AnimOverrideRule& rule,
	NiControllerManager* mgr)
{
	CAFKey key{ actor, mgr, group, &rule };

	auto it = g_actorCAFSeqs.find(key);
	if (it != g_actorCAFSeqs.end())
	{
		BSAnimGroupSequence* cached = it->second;

		// IMPORTANT: validate pointer still belongs to this manager
		if (cached && cached->controllerMgr == mgr)
			return cached;

		// stale entry cleanup
		g_actorCAFSeqs.erase(it);
	}

	// Find the template sequence
	auto cafIt = g_cafSequencesByGroupTP.find(group);
	if (cafIt == g_cafSequencesByGroupTP.end()) return nullptr;
	BSAnimGroupSequence* tmpl = cafIt->second[0].seq;
	if (!tmpl) return nullptr;

	// Clone it
	void* modelLoader = *(void**)0x00B33A1C;
	std::string path = "Characters\\_Male\\" + rule.replacementFile + ".kf";

	g_loadingCAF = true;
	kfModel* model = (kfModel*)g_originalLoadKFModel(modelLoader, path.c_str());
	g_loadingCAF = false;

	if (!model || !model->animGroup) return nullptr;

	BSAnimGroupSequence* seq =
		(BSAnimGroupSequence*)FormHeap_Allocate(0x6C);
	if (!seq) return nullptr;

	//memset(seq, 0, 0x6C);
	g_BSAnimGroupSequenceCtor(seq, model->animGroup, model->controllerSequence);
	//seq->controllerMgr = mgr;
	//seq->m_uiRefCount = 1;

	RegisterCAFSequence(seq); // reuses existing matching logic
	if (seq && seq->filePath)
	{
		// Prefix with marker that idle picker won't load
		std::string fakePath = "_CAF_INTERNAL_" + std::string(seq->filePath);
		char* newPath = (char*)FormHeap_Allocate(fakePath.size() + 1);
		strcpy_s(newPath, fakePath.size() + 1, fakePath.c_str());
		seq->filePath = newPath;
	}

	g_actorCAFSeqs[key] = seq;

	return seq;
}

typedef void(__thiscall* _ToggleBody)(PlayerCharacter*, char);
static _ToggleBody OriginalToggleBody = nullptr;

static bool g_isFirstPerson = false;

void __fastcall HookedToggleBody(PlayerCharacter* thisPtr, void*, char a3)
{
	// Ensure this is actually the player
	if (thisPtr == *g_thePlayer)
	{
		// a3 == 1 → first person, a3 == 0 → third person
		g_isFirstPerson = (a3 != 0);

	}

	// Call original engine function
	OriginalToggleBody(thisPtr, a3);
}

void InstallToggleBodyHook()
{
	if (MH_CreateHook((LPVOID)0x00664F70, (LPVOID)&HookedToggleBody, (LPVOID*)&OriginalToggleBody) == MH_OK)
	{
		MH_EnableHook((LPVOID)0x00664F70);
	}
}

std::unordered_map<AnimSequenceMultiple*, BSAnimGroupSequence*> g_overrideMap;
std::unordered_map<AnimSequenceSingle*, BSAnimGroupSequence*> g_overrideSingleMap;
static std::unordered_set<BSAnimGroupSequence*> g_registeredSequences;

BSAnimGroupSequence* __fastcall GetAnimGroupSequenceSingleHook(AnimSequenceSingle* This, void*, int index)
{
	BSAnimGroupSequence* base = (BSAnimGroupSequence*)ThisStdCall(g_originalGetSingle, This, index);

	if (!This || !This->Anim) return base;

	NiControllerManager* mgr = This->Anim ? This->Anim->controllerMgr : nullptr;
	if (!mgr) return base;

	Actor* actor = nullptr;
	auto itActor = g_managerToActorMap.find(mgr);
	if (itActor != g_managerToActorMap.end())
	{
		actor = itActor->second;
	}

	if (!actor || !InterfaceManager::GetSingleton()->IsGameMode()) return base;

	UInt32 actorId = actor->refID;
	UInt32 group = base->animGroup->animGroup;

	if (g_groupBusy[mgr][group])
		return base;
	CAFGroupLock lock(g_groupBusy, mgr, group);
	PlayerCharacter* pc = OBLIVION_CAST(actor, Actor, PlayerCharacter);

	bool useFirstPerson = false;

	auto& map = useFirstPerson ? g_cafSequencesByGroupFP : g_cafSequencesByGroupTP;

	auto it = map.find(group);
	if (it == map.end())
	{
		return base;
	}


	BSAnimGroupSequence* chosen = nullptr;

	for (const CAFSequence& caf : it->second)
	{
		if (!caf.seq || !caf.rule)
			continue;

		if (ConditionsPass(*caf.rule, actor, group))
		{
			chosen = GetOrCreateActorCAFSeqTP(actor, group, *caf.rule, mgr);
			break;
		}
	}

	if (chosen)
	{
		g_addSequence(base->controllerMgr, chosen, 0, 1);
		g_overrideSingleMap[This] = chosen;
		chosen->m_uiRefCount++;
		return chosen;
	}
	else
	{
		auto it = g_overrideSingleMap.find(This);
		if (it != g_overrideSingleMap.end())
			g_overrideSingleMap.erase(it);
	}

	return base;
}

AnimSequenceMultiple* (__thiscall* NewAnimSequenceMultiple)(AnimSequenceMultiple*, AnimSequenceSingle*) = (AnimSequenceMultiple * (__thiscall*)(AnimSequenceMultiple*, AnimSequenceSingle*))0x00473D90;

static std::unordered_map<
	AnimSequenceMultiple*,
	std::vector<BSAnimGroupSequence*>
> g_vanillaNodesBySequence;

BSAnimGroupSequence* (__thiscall* GetAnimGroupSequenceMultiple)(AnimSequenceMultiple*, int) = (BSAnimGroupSequence * (__thiscall*)(AnimSequenceMultiple*, int))0x00470BF0;
static std::unordered_map<AnimSequenceMultiple*, BSAnimGroupSequence*> g_vanillaBySequence;

BSAnimGroupSequence* __fastcall GetAnimGroupSequenceMultipleHook(
	AnimSequenceMultiple* sequence,
	void*,
	int index)
{
	BSAnimGroupSequence* base =
		(BSAnimGroupSequence*)ThisStdCall(g_originalGetMultiple, sequence, index);

	if (!base || !base->animGroup)
		return base;

	NiControllerManager* mgr = base->controllerMgr;

	Actor* actor = nullptr;
	auto itActor = g_managerToActorMap.find(mgr);
	if (itActor == g_managerToActorMap.end())
		return base;

	actor = itActor->second;

	if (!actor || !InterfaceManager::GetSingleton()->IsGameMode())
		return base;

	UInt32 group = base->animGroup->animGroup;

	if (g_groupBusy[mgr][group])
		return base;

	CAFGroupLock lock(g_groupBusy, mgr, group);

	bool useFirstPerson = false;

	auto& map = useFirstPerson ?
		g_cafSequencesByGroupFP :
		g_cafSequencesByGroupTP;

	auto cafIt = map.find(group);
	if (cafIt == map.end())
		return base;

	BSAnimGroupSequence* chosen = nullptr;

	// ---------------------------------------
	// 1. PURE EVALUATION (NO MUTATION)
	// ---------------------------------------
	for (const CAFSequence& caf : cafIt->second)
	{
		if (!caf.seq || !caf.rule)
			continue;

		if (ConditionsPass(*caf.rule, actor, group))
		{
			chosen = GetOrCreateActorCAFSeqTP(actor, group, *caf.rule, mgr);
			break;
		}
	}

	// ---------------------------------------
	// 2. APPLY OR CLEAR OVERRIDE
	// ---------------------------------------
	if (chosen)
	{
		SeqKey key{ mgr, chosen };

		if (g_added.find(key) == g_added.end())
		{
			g_addSequence(mgr, chosen, 0, 1);
			g_added.insert(key);
		}

		g_overrideMap[sequence] = chosen;

		return chosen;
	}

	// ---------------------------------------
	// 3. IMPORTANT: CLEAR STALE OVERRIDE
	// ---------------------------------------
	auto it = g_overrideMap.find(sequence);
	if (it != g_overrideMap.end())
	{
		g_overrideMap.erase(it);
	}

	return base;
}


void* CreateTrampoline(uintptr_t src, size_t patchSize)
{
	if (patchSize < 5)
		return nullptr;

	uint8_t* tramp = (uint8_t*)VirtualAlloc(
		nullptr,
		patchSize + 5,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE
	);

	if (!tramp)
		return nullptr;

	// copy stolen bytes
	memcpy(tramp, (void*)src, patchSize);

	// calculate jump back
	uintptr_t srcNext = src + patchSize;
	uintptr_t trampNext = (uintptr_t)(tramp + patchSize);

	tramp[patchSize] = 0xE9;
	*(int32_t*)(tramp + patchSize + 1) =
		(int32_t)(srcNext - (trampNext + 5));

	// IMPORTANT: ensure CPU sees new code
	FlushInstructionCache(GetCurrentProcess(), tramp, patchSize + 5);

	return tramp;
}

void InstallHook(uintptr_t addr, void* hook, size_t patchSize)
{
	void* tramp = CreateTrampoline(addr, patchSize);
	if (!tramp)
		return;

	DWORD old;
	VirtualProtect((void*)addr, patchSize, PAGE_EXECUTE_READWRITE, &old);

	*(uint8_t*)addr = 0xE9;
	*(int32_t*)(addr + 1) =
		(int32_t)((uintptr_t)hook - (addr + 5));

	// NOP remaining bytes
	for (size_t i = 5; i < patchSize; i++)
		*(uint8_t*)(addr + i) = 0x90;

	VirtualProtect((void*)addr, patchSize, old, &old);
}

using Actor_LoadAnimGroup_t =
signed __int16(__thiscall*)(
	Actor*,
	int,
	int,
	char
	);

Actor_LoadAnimGroup_t g_originalLoadAnimGroup = nullptr;

signed __int16 __fastcall ActorLoadAnimGroupHook(
	Actor* actor,
	void*,
	int animGroup,
	int a3,
	char a4
)
{
	g_inLoadAnimGroup = true;
	g_currentAnimActor = actor;
	g_currentAnimGroup = animGroup;
	g_currentActorRefID = actor->refID;

	ActorAnimData* animData = actor->GetAnimData();

	if (animData)
		g_animDataToActor[animData] = actor;

	PlayerCharacter* pc = OBLIVION_CAST(actor, Actor, PlayerCharacter);

	bool isFP = (pc && pc->firstPersonAnimData == actor->GetAnimData());

	auto& map = isFP ? g_cafSequencesByGroupFP : g_cafSequencesByGroupTP;

	auto it = map.find(animGroup);

	g_loadAnimDepth++;

	if (animData && g_loadAnimDepth == 1)
	{
		auto& cafData = g_cafAnimData[g_currentActorRefID];
		cafData.entries.clear();

		auto it = g_animOverrideRules.find(animGroup);
		if (it != g_animOverrideRules.end())
		{
			for (const AnimOverrideRule& rule : it->second)
			{
				if (!ConditionsPass(rule, actor, animGroup))
					continue;

				BSAnimGroupSequence* cafSeq =
					FindCAFSequenceForGroup(animGroup, rule, isFP);

				if (!cafSeq)
					continue;

				CAFAnimEntry entry;
				entry.seq = cafSeq;
				entry.rule = &rule;

				void* mgr = *(void**)((char*)animData + 0x98);
				if (mgr)
				{
					for (auto& [group, cafVec] : map)
					{
						for (CAFSequence& caf : cafVec)
						{
							if (!caf.seq) continue;
							g_addSequence(mgr, caf.seq, 0, 1);
						}
					}
					g_cafNeedsReregistration = false;
				}
			}
		}
	}

	signed __int16 result =
		g_originalLoadAnimGroup(actor, animGroup, a3, a4);

	g_loadAnimDepth--;

	if (g_loadAnimDepth == 0)
	{
		g_inLoadAnimGroup = false;
		g_currentAnimActor = nullptr;
	}

	return result;
}


// AnimSequenceSingle dtor address — find via the calltrace
// 0x004717F1 - 0x91 = 0x00471760
typedef void(__thiscall* _AnimSequenceSingleDtor)(AnimSequenceSingle*);
static _AnimSequenceSingleDtor g_originalAnimSequenceSingleDtor =
(_AnimSequenceSingleDtor)CreateTrampoline(0x00471760, 7);

void __fastcall AnimSequenceSingleDtorHook(AnimSequenceSingle* This, void*)
{
	auto cafIt = g_activeCafAnims.find(This);
	if (cafIt != g_activeCafAnims.end())
	{
		InterlockedDecrement((volatile LONG*)((char*)cafIt->second + 4));
		g_activeCafAnims.erase(cafIt);
	}

	auto vanillaIt = g_vanillaSingleAnims.find(This);
	if (vanillaIt != g_vanillaSingleAnims.end())
	{
		This->Anim = vanillaIt->second;
		g_vanillaSingleAnims.erase(vanillaIt);
	}

	g_originalAnimSequenceSingleDtor(This);
}

void InstallSingleDtorHook()
{
	WriteRelJump(0x00471760, (UInt32)AnimSequenceSingleDtorHook);
}


void InstallAddSingleHook()
{
	g_originalAddSingle =
		(void(__thiscall*)(AnimSequenceSingle*, BSAnimGroupSequence*))DetourVtable(
			0x00A3C730, // your AddSingle vtable address
			(UInt32)&AddSingleHook
		);

	_MESSAGE("CAF: AddSingleHook installed");
}

// Credits to lStewieAl
[[nodiscard]] __declspec(noinline) UInt32 __stdcall DetourVtable(UInt32 addr, UInt32 dst)
{
	UInt32 originalFunction = *(UInt32*)addr;
	SafeWrite32(addr, dst);
	return originalFunction;
}

void PreloadCAFSequencesTP()
{

	_MESSAGE("CAF PRELOAD: ENTER PreloadCAFSequencesTP()");
	_MESSAGE("CAF PRELOAD: g_animOverrideRules groups=%zu",
		g_animOverrideRules.size());

	void* modelLoader = *(void**)0x00B33A1C;
	if (!modelLoader)
	{
		_MESSAGE("CAF: PreloadCAF — ModelLoader not ready");
		return;
	}

	for (auto& [group, rules] : g_animOverrideRules)
	{
		_MESSAGE("CAF PRELOAD: group=%u rules=%zu",
			group,
			rules.size()
		);

		for (const AnimOverrideRule& rule : rules)
		{
			_MESSAGE("CAF PRELOAD: rule='%s' group=%u",
				rule.replacementFile.c_str(),
				group);

			std::string path = "Characters\\_Male\\" + rule.replacementFile + ".kf";

			g_loadingCAF = true;
			kfModel* model = (kfModel*)g_originalLoadKFModel(modelLoader, path.c_str());
			_MESSAGE("CAF PRELOAD: KF LOAD result model=%p path=%s",
				model,
				path.c_str());

			g_loadingCAF = false;

			if (!model || !model->animGroup)
			{
				_MESSAGE("CAF: PreloadCAF FAILED %s", path.c_str());
				continue;
			}

			BSAnimGroupSequence* seq =
				(BSAnimGroupSequence*)FormHeap_Allocate(0x6C);
			if (!seq) continue;

			g_BSAnimGroupSequenceCtor(seq, model->animGroup, model->controllerSequence);

			_MESSAGE("CAF PRELOAD: seq created=%p animGroup=%p controller=%p",
				seq,
				model->animGroup,
				model->controllerSequence);

			_MESSAGE("CAF PRELOAD: calling RegisterCAFSequence seq=%p filePath=%s",
				seq,
				seq ? seq->filePath : "<null>");

			RegisterCAFSequence(seq); // reuses existing matching logic
			if (seq && seq->filePath)
			{
				// Prefix with marker that idle picker won't load
				std::string fakePath = "_CAF_INTERNAL_" + std::string(seq->filePath);
				char* newPath = (char*)FormHeap_Allocate(fakePath.size() + 1);
				strcpy_s(newPath, fakePath.size() + 1, fakePath.c_str());
				seq->filePath = newPath;
			}

			_MESSAGE("CAF: PreloadCAF OK group=%u %s", group, path.c_str());

			_MESSAGE("CAF: seq+0x24=%f seq+0x28=%f seq+0x2C=%f",
				*(float*)((char*)seq + 0x24),
				*(float*)((char*)seq + 0x28),
				*(float*)((char*)seq + 0x2C));
		}
	}

	_MESSAGE("CAF: PreloadCAF TP complete, %zu groups populated",
		g_cafSequencesByGroupTP.size());
}

void PreloadCAFSequencesFP()
{

	_MESSAGE("CAF PRELOAD: ENTER PreloadCAFSequencesFP()");
	_MESSAGE("CAF PRELOAD: g_animOverrideRules groups=%zu",
		g_animOverrideRules.size());

	void* modelLoader = *(void**)0x00B33A1C;
	if (!modelLoader)
	{
		_MESSAGE("CAF: PreloadCAF — ModelLoader not ready");
		return;
	}

	for (auto& [group, rules] : g_animOverrideRules)
	{
		_MESSAGE("CAF PRELOAD: group=%u rules=%zu",
			group,
			rules.size()
		);

		for (const AnimOverrideRule& rule : rules)
		{
			_MESSAGE("CAF PRELOAD: rule='%s' group=%u",
				rule.replacementFile.c_str(),
				group);

			std::string path = "Characters\\_1stPerson\\" + rule.replacementFile + ".kf";

			g_loadingCAF = true;
			kfModel* model = (kfModel*)g_originalLoadKFModel(modelLoader, path.c_str());
			_MESSAGE("CAF PRELOAD: KF LOAD result model=%p path=%s",
				model,
				path.c_str());

			g_loadingCAF = false;

			if (!model || !model->animGroup)
			{
				_MESSAGE("CAF: PreloadCAF FAILED %s", path.c_str());
				continue;
			}

			BSAnimGroupSequence* seq =
				(BSAnimGroupSequence*)FormHeap_Allocate(0x6C);
			if (!seq) continue;

			g_BSAnimGroupSequenceCtor(seq, model->animGroup, model->controllerSequence);

			_MESSAGE("CAF PRELOAD: seq created=%p animGroup=%p controller=%p",
				seq,
				model->animGroup,
				model->controllerSequence);

			_MESSAGE("CAF PRELOAD: calling RegisterCAFSequence seq=%p filePath=%s",
				seq,
				seq ? seq->filePath : "<null>");

			RegisterCAFSequenceFP(seq, group); // reuses existing matching logic
			if (seq && seq->filePath)
			{
				// Prefix with marker that idle picker won't load
				std::string fakePath = "_CAF_INTERNAL_" + std::string(seq->filePath);
				char* newPath = (char*)FormHeap_Allocate(fakePath.size() + 1);
				strcpy_s(newPath, fakePath.size() + 1, fakePath.c_str());
				seq->filePath = newPath;
			}

			_MESSAGE("CAF: PreloadCAF OK group=%u %s", group, path.c_str());

			_MESSAGE("CAF: seq+0x24=%f seq+0x28=%f seq+0x2C=%f",
				*(float*)((char*)seq + 0x24),
				*(float*)((char*)seq + 0x28),
				*(float*)((char*)seq + 0x2C));
		}
	}

	_MESSAGE("CAF: PreloadCAF FP complete, %zu groups populated",
		g_cafSequencesByGroupFP.size());
}

int __fastcall LoadKFModelHook(void* This, UInt32 edx, const char* path)
{
	return g_originalLoadKFModel(This, path);
}

// Install hook
void InstallLoadKFModelHook()
{
	UInt32 hookAddr = 0x00439FF0;

	// Allocate executable trampoline
	static UInt8 trampoline[32];

	// Make trampoline executable
	DWORD oldProtect;
	VirtualProtect(trampoline, sizeof(trampoline), PAGE_EXECUTE_READWRITE, &oldProtect);

	// Copy original bytes
	memcpy(trampoline, (void*)hookAddr, 7);

	// Add jump back to original function
	UInt32 returnAddr = hookAddr + 7;
	trampoline[7] = 0xE9; // JMP opcode
	*(UInt32*)(trampoline + 8) = returnAddr - ((UInt32)trampoline + 12);

	// Set function pointer
	g_originalLoadKFModel = (_LoadKFModel)(void*)trampoline;

	// Install hook at original location
	WriteRelJump(hookAddr, (UInt32)LoadKFModelHook);

	_MESSAGE("CAF: Installed LoadKFModel hook at 0x%08X", hookAddr);
}

inline Actor** GetCurrentAnimActorPtr()
{
	return &g_currentAnimActor;
}

static const UInt32 loc_65D797 = 0x0065D797;

static Actor* s_tempActor = nullptr;

void LogActorProcessAction(Actor* actor)
{
	if (!actor)
		return;

	if (!actor->GetAnimData())
		return;

	g_managerToActorMap[actor->GetAnimData()->manager] = actor;
}

__declspec(naked) void Sub65D790Hook()
{
	__asm
	{
		mov[s_tempActor], ecx      // save actor* before pushad

		pushad

		mov eax, [s_tempActor]
		test eax, eax
		jz skip
		test eax, 0x3
		jnz skip
		cmp eax, 0x10000
		jb skip
		mov edx, [eax]
		test edx, edx
		jz skip

		push eax
		call LogActorProcessAction
		add esp, 4

		push[s_tempActor]
		call GetCurrentAnimActorPtr
		add esp, 4
		test eax, eax
		jz skip
		mov ecx, [s_tempActor]
		mov[eax], ecx

		skip :
		popad
			push ebx
			mov ebx, [esp + 8]
			push ebp
			push esi
			jmp loc_65D797
	}
}

void Install65D790Hook()
{
	WriteRelJump(0x0065D790, (UInt32)Sub65D790Hook);
}

typedef void* OriginalFnPtr;
OriginalFnPtr OriginalActorProcessAction = nullptr;

__declspec(naked) void ActorProcessAction_Detour_Naked()
{
	__asm
	{
		test ecx, ecx
		jz skip

		push ecx
		call LogActorProcessAction
		add esp, 4

		skip:
		jmp[OriginalActorProcessAction]
	}
}

void InstallActorProcessActionHook()
{
	if (MH_CreateHook((LPVOID)0x005FCAB0, (LPVOID)&ActorProcessAction_Detour_Naked, (LPVOID*)&OriginalActorProcessAction) == MH_OK)
	{
		MH_EnableHook((LPVOID)0x005FCAB0);
	}
}

typedef void(__thiscall* _ModelCacheRelease)(void*, const char*, int);
static _ModelCacheRelease g_originalModelCacheRelease = nullptr;

void __fastcall Hook_ModelCacheRelease(void* This, void*, const char* filename, int a2)
{
	if (filename)
	{
		if (strncmp(filename, "_CAF_INTERNAL_", 14) == 0)
		{
			_MESSAGE("CAF: blocked release for internal seq %s", filename);
			return;
		}
	}
	g_originalModelCacheRelease(This, filename, a2);
}

void InstallSub43E680Hook()
{
	g_originalModelCacheRelease = (_ModelCacheRelease)DetourVtable(
		0x00A371E0,
		(UInt32)Hook_ModelCacheRelease
	);
}

void InitializeMyHooks()
{
	if (MH_Initialize() != MH_OK)
	{
		_MESSAGE("CAF: MinHook failed to initialize");
	}
}

void Install()
{
	InitializeMyHooks();

	InstallOnUpdateHook();

	g_originalGetMultiple = DetourVtable(
		0x00A3C768,
		(UInt32)&GetAnimGroupSequenceMultipleHook
	);

	g_originalGetSingle = DetourVtable(
		0x00A3C73C,
		(UInt32)&GetAnimGroupSequenceSingleHook
	);

	Install65D790Hook();
	InstallActorProcessActionHook();
	//InstallSub_474510_Detour();

	//WriteRelCall(0x4741D9, (UInt32)&NewAnimSequenceMultipleHook);

	//g_originalLoadAnimGroup =
		//(Actor_LoadAnimGroup_t)CreateTrampoline(0x005E5690, 7);

	//WriteRelJump(0x005E5690, (UInt32)ActorLoadAnimGroupHook);

	//InstallSingleDtorHook();
	InstallDisposeActorAnimDataHook();

	//InstallAddSingleHook();

	//InstallToggleBodyHook();

	InstallLoadKFModelHook();
	PreloadCAFSequencesTP();
	//PreloadCAFSequencesFP();

	_MESSAGE("CAF: All hooks installed");
}
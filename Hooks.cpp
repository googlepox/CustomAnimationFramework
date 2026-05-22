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

#define AnimString "_CAF"

bool g_cafDebug = true;

// Use maps instead:
std::unordered_map<AnimSequenceSingle*, BSAnimGroupSequence*> g_singleCAFCache;
std::unordered_map<AnimSequenceMultiple*, std::unordered_map<int, BSAnimGroupSequence*>> g_multipleCAFCache;

static std::unordered_set<AnimSequenceMultiple*> g_cafInjectedMultiples;

static std::unordered_map<AnimSequenceMultiple*, Actor*> g_multipleToActor;

static std::unordered_map<ActorAnimData*, Actor*> g_animDataToActor;


struct CAFSequence
{
	BSAnimGroupSequence* seq;          // the CAF animation sequence
	const AnimOverrideRule* rule;      // rule that owns it
};

static std::unordered_map<
	UInt32,                            // animGroup
	std::vector<CAFSequence>
> g_cafSequencesByGroup;



class Jumpers
{
public:
	struct CreateDevice
	{
		static const UInt32 Hook = 0x0076A542;
		static const UInt32 Return = 0x0076A547;
	};
	struct SetRegionEditorName
	{
		static const UInt32 Hook = 0x004A32A6;
		static const UInt32 Return = 0x004A33A6;
	};
	struct SetWeatherEditorName
	{
		static const UInt32 Hook = 0x004EE04E;
		static const UInt32 Return = 0x004EE0EA;
	};
	struct SkipFogPass
	{
		static const UInt32 Hook = 0x007AE6F5;
		static const UInt32 Return = 0x007AE6FB;
	};
	struct DetectorWindow
	{
		static const UInt32 CreateTreeViewHook = 0x00495E1F;
		static const UInt32 CreateTreeViewReturn = 0x00495E27;
		static const UInt32 DumpAttributesHook = 0x004967C7;
		static const UInt32 DumpAttributesReturn = 0x004967CD;
		static const UInt32 ConsoleCommandHook = 0x0040CC6C;
		static const UInt32 ConsoleCommandReturn = 0x0040CC73;
		static const UInt32 SetNodeName = 0x0049658E;
	};
	struct RenderInterface
	{
		static const UInt32 Hook = 0x0057F3F3;
		static const UInt32 Return = 0x0057F3F8;
		static const UInt32 Method = 0x0070E0A0;
	};
	struct HitEvent
	{
		static const UInt32 Hook = 0x005FF613;
		static const UInt32 Return = 0x005FF618;
	};
	struct NewAnimSequenceSingle
	{
		static const UInt32 Hook = 0x0047414D;
		static const UInt32 Return = 0x00474157;
	};
	struct RemoveSequence
	{
		static const UInt32 Hook = 0x004742B7;
		static const UInt32 Return1 = 0x004742BF;
		static const UInt32 Return2 = 0x004742CD;
	};
	struct Shadows
	{
		static const UInt32 RenderShadowMapHook = 0x0040C919;
		static const UInt32 RenderShadowMapReturn = 0x0040C920;
		static const UInt32 OriginalRenderShadowPass = 0x004073D0;
		static const UInt32 AddCastShadowFlagHook = 0x004B1A25;
		static const UInt32 AddCastShadowFlagReturn = 0x004B1A2A;
		static const UInt32 EditorCastShadowFlagHook = 0x005498DD;
		static const UInt32 EditorCastShadowFlagReturn = 0x005498E3;
	};
	struct WaterHeightMap
	{
		static const UInt32 Hook = 0x0049D9FA;
		static const UInt32 Return = 0x0049D9FF;
	};
	struct EndProcess
	{
		static const UInt32 Hook = 0x0040F488;
	};
	struct Occlusion
	{
		static const UInt32 New1CollisionObjectHook = 0x00564529;
		static const UInt32 New1CollisionObjectReturn = 0x0056452E;
		static const UInt32 New2CollisionObjectHook = 0x0089E989;
		static const UInt32 New2CollisionObjectReturn = 0x0089E98E;
		static const UInt32 New3CollisionObjectHook = 0x0089EA1C;
		static const UInt32 New3CollisionObjectReturn = 0x0089EA21;
		static const UInt32 DisposeCollisionObjectHook = 0x00532DD1;
		static const UInt32 DisposeCollisionObjectReturn = 0x00532DD8;
		static const UInt32 MaterialPropertyHook = 0x0089F7C6;
		static const UInt32 MaterialPropertyReturn1 = 0x0089F7CE;
		static const UInt32 MaterialPropertyReturn2 = 0x0089F8A0;
		static const UInt32 CoordinateJackHook = 0x008A3101;
		static const UInt32 CoordinateJackReturn1 = 0x008A3107;
		static const UInt32 CoordinateJackReturn2 = 0x008A3165;
		static const UInt32 ObjectCullHook = 0x007073D6;
		static const UInt32 ObjectCullReturn1 = 0x007073DC;
		static const UInt32 ObjectCullReturn2 = 0x007073E7;
	};
	struct Camera
	{
		static const UInt32 UpdateCameraHook = 0x0066BE6E;
		static const UInt32 UpdateCameraReturn = 0x0066BE7C;
		static const UInt32 SwitchCameraHook = 0x00671AC9;
		static const UInt32 SwitchCameraReturn = 0x00671AD0;
		static const UInt32 SwitchCameraPOVHook = 0x00672FDA;
		static const UInt32 SwitchCameraPOVReturn = 0x00672FE2;
		static const UInt32 HeadTrackingHook = 0x0055D6A8;
		static const UInt32 HeadTrackingReturn = 0x0055D6B5;
		static const UInt32 HeadTrackingReturn1 = 0x0055D7E6;
		static const UInt32 SpineTrackingHook = 0x00603C55;
		static const UInt32 SpineTrackingReturn = 0x00603C5E;
		static const UInt32 SpineTrackingReturn1 = 0x00603C95;
		static const UInt32 SetReticleOffsetHook = 0x00580796;
		static const UInt32 SetReticleOffsetReturn = 0x0058079C;
	};
	struct UpdateGrass
	{
		static const UInt32 Hook = 0x004EBF87;
		static const UInt32 Return = 0x004EC4E8;
	};
	struct Memory
	{
		static const UInt32 MemReallocHook = 0x00401E66;
		static const UInt32 MemReallocReturn = 0x00401EC7;
		static const UInt32 CreateTextureFromFileInMemory = 0x007610D3;
	};
	struct Equipment
	{
		static const UInt32 PrnHook = 0x0047927B;
		static const UInt32 PrnReturn = 0x0047928A;
		static const UInt32 MenuMouseButtonHook = 0x0058251B;
		static const UInt32 MenuMouseButtonReturn1 = 0x00582525;
		static const UInt32 MenuMouseButtonReturn2 = 0x0058264F;
		static const UInt32 UnequipTorchHook = 0x0048A7AD;
		static const UInt32 UnequipTorchReturn = 0x0048A7B5;
		static const UInt32 EquipItemWornHook = 0x00489E0A;
		static const UInt32 EquipItemWornReturn = 0x00489E13;
		static const UInt32 SetWeaponRotationPositionHook = 0x006563F3;
		static const UInt32 SetWeaponRotationPositionReturn = 0x006563FC;
	};
	struct Mounted
	{
		static const UInt32 PlayerReadyWeaponHook = 0x00671E37;
		static const UInt32 ActorReadyWeaponHook = 0x005FDA4F;
		static const UInt32 ActorReadyWeaponSittingHook = 0x005FD904;
		static const UInt32 ActorReadyWeaponSittingReturn = 0x005FD910;
		static const UInt32 PlayerAttackHook = 0x00672612;
		static const UInt32 PlayerAttackReturn = 0x0067261F;
		static const UInt32 HittingMountedCreatureHook = 0x005FF017;
		static const UInt32 HittingMountedCreatureReturn = 0x005FF01C;
		static const UInt32 BowEquipHook = 0x004E1AC4;
		static const UInt32 BowEquipReturn = 0x004E1AC9;
		static const UInt32 AnimControllerHook = 0x004732F4;
		static const UInt32 AnimControllerReturn1 = 0x004732FA;
		static const UInt32 AnimControllerReturn2 = 0x00473314;
		static const UInt32 HorsePaletteHook = 0x007165B9;
		static const UInt32 HorsePaletteReturn1 = 0x007165C2;
		static const UInt32 HorsePaletteReturn2 = 0x0071661B;
		static const UInt32 BowUnequipHook = 0x005F34AB;
		static const UInt32 BowUnequipReturn = 0x005F34B0;
		static const UInt32 HideWeaponHook = 0x00654DC8;
		static const UInt32 HideWeaponReturn = 0x00654E5F;
	};
	struct Dodge
	{
		static const UInt32 JumpPressedHook = 0x00672A79;
		static const UInt32 JumpPressedReturn1 = 0x00672A80;
		static const UInt32 JumpPressedReturn2 = 0x00672B94;
		static const UInt32 DoubleTapHook = 0x006729DA;
		static const UInt32 DoubleTapReturn = 0x006729EA;
	};
	struct FlyCam
	{
		static const UInt32 UpdateForwardFlyCamHook = 0x0066446C;
		static const UInt32 UpdateForwardFlyCamReturn = 0x0066447A;
		static const UInt32 UpdateBackwardFlyCamHook = 0x00664489;
		static const UInt32 UpdateBackwardFlyCamReturn = 0x00664497;
		static const UInt32 UpdateRightFlyCamHook = 0x006644A6;
		static const UInt32 UpdateRightFlyCamReturn = 0x006644B4;
		static const UInt32 UpdateLeftFlyCamHook = 0x006644C3;
		static const UInt32 UpdateLeftFlyCamReturn = 0x006644D1;
	};
	struct UpdateTimeInfo
	{
		static const UInt32 Hook = 0x0040D8AB;
		static const UInt32 Return = 0x0040D8B0;
	};
};

thread_local bool g_inLoadAnimGroup = false;
thread_local BSAnimGroupSequence* g_pendingCAFSequence = nullptr;

UInt32 g_originalAddSingle;
UInt32 g_originalAddMultiple;
UInt32 g_originalGetSingle;
UInt32 g_originalGetMultiple;
UInt32 g_originalAddAnimation;

void* g_trampAddSingle = nullptr;
void* g_trampAddMultiple = nullptr;
void* g_trampGetSingle = nullptr;
void* g_trampGetMultiple = nullptr;
void* g_trampAddAnimation = nullptr;

ActorAnimData* g_currentAnimData = nullptr;
UInt32 g_currentActorRefID = 0;

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

	_MESSAGE("MATCH CHECK '%s' vs '%s'",
		f.c_str(),
		r.c_str());

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

	_MESSAGE(
		"CAF: registered CAF anim '%s'",
		seq->filePath
	);
}


void RegisterCAFSequence(BSAnimGroupSequence* seq)
{
	_MESSAGE("CAF REGISTER: ENTER seq=%p groupPtr=%p filePath=%s",
		seq,
		seq ? seq->animGroup : nullptr,
		seq ? seq->filePath : "<null>");

	if (!seq || !seq->animGroup || !seq->filePath)
	{
		_MESSAGE("CAF: RegisterCAFSequence — null check failed seq=%p", seq);
		return;
	}

	UInt32 group = seq->animGroup->animGroup;
	_MESSAGE("CAF REGISTER: group=%u", group);

	const char* fileName = GetFileName(seq->filePath);
	_MESSAGE("CAF: RegisterCAFSequence group=%u filePath='%s' fileName='%s'",
		group, seq->filePath, fileName ? fileName : "<null>");

	_MESSAGE("CAF REGISTER: lookup group=%u in rules map", group);

	auto it = g_animOverrideRules.find(group);
	if (it == g_animOverrideRules.end())
	{
		_MESSAGE("CAF REGISTER: NO RULES FOUND for group %u", group);
		return;
	}

	for (AnimOverrideRule& rule : it->second)
	{
		_MESSAGE("CAF REGISTER: testing rule='%s' against file='%s'",
			rule.replacementFile.c_str(),
			fileName ? fileName : "<null>");

		if (AnimStringMatch(fileName, rule.replacementFile.c_str()))
		{
			g_cafSequencesByGroup[group].push_back({ seq, &rule });
			_MESSAGE("CAF REGISTER: MATCH SUCCESS group=%u file=%s",
				group,
				fileName ? fileName : "<null>");

			//AddCAFAnimGroupSequence(g_currentActorRefID, seq, rule);

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
	g_cafNeedsReregistration = true;
	return DisposeActorAnimData(This);
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
	const AnimOverrideRule& rule
)
{

	auto it = g_cafSequencesByGroup.find(animGroup);
	if (it == g_cafSequencesByGroup.end())
		return nullptr;

	for (const CAFSequence& caf : it->second)
	{

		if (!caf.seq || !caf.rule)
			continue;

		if (!AnimStringMatch(caf.seq->filePath, rule.replacementFile.c_str()))
			continue;

		return caf.seq;
	}

	return nullptr;
}


void(__thiscall* AddSingle)(AnimSequenceSingle*, BSAnimGroupSequence*) = (void(__thiscall*)(AnimSequenceSingle*, BSAnimGroupSequence*))0x470BA0;
void __fastcall AddSingleHook(
	AnimSequenceSingle* This,
	void*,
	BSAnimGroupSequence* anim
)
{
	_MESSAGE("AddSingleHook HIT");
	RegisterCAFSequence(anim);
	if (g_inLoadAnimGroup &&
		g_pendingCAFSequence &&
		anim &&
		anim->animGroup &&
		anim->animGroup->animGroup == g_currentAnimGroup)
	{
		anim->filePath = CopyString(g_pendingCAFSequence->filePath);

		if (g_cafDebug)
			_MESSAGE("CAF: Replaced SINGLE sequence path");
	}

	ThisStdCall(g_originalAddSingle, This, anim);
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

static std::unordered_set<BSAnimGroupSequence*> g_registeredWithManager;

BSAnimGroupSequence* (__thiscall* GetAnimGroupSequenceSingle)(AnimSequenceSingle*, int) = (BSAnimGroupSequence * (__thiscall*)(AnimSequenceSingle*, int))0x00471710;
BSAnimGroupSequence* __fastcall GetAnimGroupSequenceSingleHook(
	AnimSequenceSingle* This,
	void*,
	int index
)
{
	BSAnimGroupSequence* base =
		(BSAnimGroupSequence*)ThisStdCall(
			g_originalGetSingle,
			This,
			index
		);

	if (!base || !base->animGroup)
		return base;

	UInt32 group = base->animGroup->animGroup;

	Actor* actor = g_currentAnimActor;
	if (!actor)
	{
		for (auto& [animData, candidate] : g_animDataToActor)
		{
			if (!animData || !candidate) continue;
			for (int i = 0; i < 5; i++)
			{
				BSAnimGroupSequence* slot =
					(BSAnimGroupSequence*)animData->animSequences[i];
				if (slot == base)
				{
					actor = candidate;
					goto found;
				}
			}
		}
	found:;
	}

	if (!actor) return base;

	_MESSAGE("actor %08X with filename %s", actor->refID, This->Anim->filePath);
	// Find CAF override for this group
	auto it = g_cafSequencesByGroup.find(group);
	if (it == g_cafSequencesByGroup.end())
		return base;

	for (const CAFSequence& caf : it->second)
	{
		if (!caf.seq || !caf.rule)
			continue;
		
		if (!g_vanillaSingleAnims.count(This))
			g_vanillaSingleAnims[This] = This->Anim;

		BSAnimGroupSequence* vanilla = g_vanillaSingleAnims[This];

		auto fixAnimSequencesSlot = [&](BSAnimGroupSequence* from, BSAnimGroupSequence* to) {
			Actor* a = g_currentAnimActor;
			if (!a) return;
			ActorAnimData* animData = a->GetAnimData();
			if (!animData) return;
			for (int i = 0; i < 5; i++)
			{
				if (animData->animSequences[i] == from)
				{
					animData->animSequences[i] = to;
					_MESSAGE("CAF: fixed animSequences[%d] %p -> %p", i, from, to);
					break;
				}
			}
			};

		if (ConditionsPass(*caf.rule, actor, group))
		{
			fixAnimSequencesSlot(vanilla, caf.seq);
			This->Anim = caf.seq;
			return caf.seq;
		}
		else
		{
			fixAnimSequencesSlot(caf.seq, vanilla);
			This->Anim = vanilla;
			return vanilla;
		}
	}

	return base;
}



bool SequenceContains(AnimSequenceMultiple* sequence, BSAnimGroupSequence* target)
{
	if (!sequence || !sequence->Anims || !target)
		return false;

	for (auto* node = sequence->Anims->start; node; node = node->next)
	{
		if (node->data == target)
			return true;
	}

	return false;
}

typedef bool(__thiscall* _NiTMapGetAt)(void* map, UInt16 key, void** outVal);
static _NiTMapGetAt g_animsMapGetAt = (_NiTMapGetAt)0x00470960;

BSAnimGroupSequence* GetCAFOverride(
	BSAnimGroupSequence* vanilla,
	Actor* actor,
	UInt32 group)
{
	auto it = g_cafSequencesByGroup.find(group);
	if (it == g_cafSequencesByGroup.end())
		return vanilla;

	for (auto& caf : it->second)
	{
		if (!caf.seq || !caf.rule)
			continue;

		if (ConditionsPass(*caf.rule, actor, group))
		{
			return caf.seq;
		}
	}

	return vanilla;
}

void DumpSequenceMultiple(AnimSequenceMultiple* sequence)
{
	if (!sequence || !sequence->Anims)
	{
		_MESSAGE("DumpSequenceMultiple: invalid sequence");
		return;
	}

	int i = 0;

	for (auto* node = sequence->Anims->start; node; node = node->next, i++)
	{
		BSAnimGroupSequence* seq = node->data;

		if (!seq)
		{
			_MESSAGE("  [%d] NULL seq", i);
			continue;
		}

		const char* file = seq->filePath ? seq->filePath : "<null>";
		UInt32 group = seq->animGroup ? seq->animGroup->animGroup : 0;

		_MESSAGE("  [%d] seq=%p group=%u file=%s", i, seq, group, file);
	}
}

AnimSequenceMultiple* (__thiscall* NewAnimSequenceMultiple)(AnimSequenceMultiple*, AnimSequenceSingle*) = (AnimSequenceMultiple * (__thiscall*)(AnimSequenceMultiple*, AnimSequenceSingle*))0x00473D90;


struct SequenceBackup
{
	std::vector<BSAnimGroupSequence*> vanilla;
	bool modified = false;
};

static std::unordered_map<
	AnimSequenceMultiple*,
	std::vector<BSAnimGroupSequence*>
> g_vanillaNodesBySequence;

BSAnimGroupSequence* (__thiscall* GetAnimGroupSequenceMultiple)(AnimSequenceMultiple*, int) = (BSAnimGroupSequence * (__thiscall*)(AnimSequenceMultiple*, int))0x00470BF0;
static std::unordered_map<AnimSequenceMultiple*, BSAnimGroupSequence*> g_vanillaBySequence;

BSAnimGroupSequence* __fastcall GetAnimGroupSequenceMultipleHook(
	AnimSequenceMultiple* sequence,
	void*,
	int index
)
{
	BSAnimGroupSequence* base =
		(BSAnimGroupSequence*)ThisStdCall(g_originalGetMultiple, sequence, index);

	if (!base || !base->animGroup) return base;

	UInt32 group = base->animGroup->animGroup;

	Actor* actor = g_currentAnimActor;
	if (!actor) return base;

	auto cafIt = g_cafSequencesByGroup.find(group);
	if (cafIt == g_cafSequencesByGroup.end()) return base;

	BSAnimGroupSequence* cafSeq = cafIt->second[0].seq;
	const AnimOverrideRule* rule = cafIt->second[0].rule;
	if (!cafSeq || !rule) return base;

	if (!g_vanillaBySequence.count(sequence) && base != cafSeq)
		g_vanillaBySequence[sequence] = base;

	BSAnimGroupSequence* vanilla = g_vanillaBySequence.count(sequence)
		? g_vanillaBySequence[sequence]
		: base;

	bool condPass = ConditionsPass(*rule, actor, group);

	if (sequence->Anims)
	{
		for (auto* node = sequence->Anims->start; node; node = node->next)
		{
			if (condPass && node->data != cafSeq)
			{
				auto& vec = g_vanillaNodesBySequence[sequence];
				vec.push_back(node->data);
				node->data = cafSeq;
			}
			else if (!condPass)
			{
				auto vanillaIt = g_vanillaNodesBySequence.find(sequence);
				if (vanillaIt != g_vanillaNodesBySequence.end())
				{
					auto* node = sequence->Anims->start;
					for (BSAnimGroupSequence* v : vanillaIt->second)
					{
						if (!node) break;
						node->data = v;
						node = node->next;
					}
				}
			}
		}
	}

	return condPass ? cafSeq : vanilla;
}

static std::unordered_map<AnimSequenceSingle*, Actor*> g_singleToActor;

AnimSequenceMultiple* __fastcall NewAnimSequenceMultipleHook(
	AnimSequenceMultiple* This,
	UInt32,
	AnimSequenceSingle* sourceSingle
)
{
	AnimSequenceMultiple* seq = NewAnimSequenceMultiple(This, sourceSingle);
	if (!seq) return seq;

	// Store actor context while we have it
	if (g_currentAnimActor)
	{
		g_multipleToActor[seq] = g_currentAnimActor;
		_MESSAGE("CAF[NASM]: bound seq=%p to actor=%08X",
			seq, g_currentAnimActor->refID);
	}

	if (g_cafInjectedMultiples.contains(seq)) return seq;

	for (auto& [group, cafVec] : g_cafSequencesByGroup)
	{
		for (CAFSequence& caf : cafVec)
		{
			if (!caf.seq) continue;
			seq->AddAnimGroupSequence(caf.seq);
			_MESSAGE("CAF[NASM]: INJECTED caf.seq=%p group=%u", caf.seq, group);
		}
	}

	g_cafInjectedMultiples.insert(seq);
	return seq;
}


void __stdcall CAF_NewAnimSequenceSingle_PostCtor(
	AnimSequenceSingle* seq,
	BSAnimGroupSequence* source
)
{
	_MESSAGE("CAF_NewAnimSequenceSingle_PostCtor");
	if (!seq || !source || !source->animGroup)
		return;

	UInt32 group = source->animGroup->animGroup;

	if (g_currentAnimActor)
	{
		g_singleToActor[seq] = g_currentAnimActor;

		_MESSAGE("CAF[SINGLE]: bound seq=%p actor=%08X",
			seq,
			g_currentAnimActor->refID);
	}

	auto it = g_animOverrideRules.find(group);
	if (it == g_animOverrideRules.end())
		return;

	for (const AnimOverrideRule& rule : it->second)
	{
		BSAnimGroupSequence* cafSeq =
			FindCAFSequenceForGroup(group, rule);

		if (cafSeq)
		{
			if (g_cafDebug)
			{
				_MESSAGE(
					"CAF: SINGLE ctor replace group %u\n  %s\n  -> %s",
					group,
					source->filePath,
					cafSeq->filePath
				);
			}

			seq->Anim = cafSeq;
			break;
		}
	}
}


__declspec(naked) void NewAnimSequenceSingleHook()
{
	__asm
	{
		// eax = AnimSequenceSingle*
		// esi = BSAnimGroupSequence* source

		// --- original constructor tail ---
		mov[eax + 0x8], esi
		mov[eax + 0x4], esi
		push eax
		mov dword ptr[eax], 0x00A3C72C

		// --- CAF injection ---
		push esi        // source
		push eax        // this
		call CAF_NewAnimSequenceSingle_PostCtor
		add esp, 8

		// --- return to engine ---
		jmp Jumpers::NewAnimSequenceSingle::Return
	}
}


__declspec(naked) void RemoveSequenceHook()
{

	__asm {
		mov		ecx, [eax + 0x68]
		mov		edx, [ecx + 0x0A]
		test	dl, dl
		jnz		short skip_removal
		push	eax
		lea		ecx, [esp + 0x1C]
		push	ecx
		mov		ecx, edi
		jmp		Jumpers::RemoveSequence::Return1

		skip_removal :
		jmp		Jumpers::RemoveSequence::Return2
	}

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


static thread_local int g_loadAnimDepth = 0;

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
	_MESSAGE("ActorLoadAnimGroupHook triggered for group %u", animGroup);
	g_inLoadAnimGroup = true;
	g_currentAnimActor = actor;
	g_currentAnimGroup = animGroup;
	g_currentActorRefID = actor->refID;

	ActorAnimData* animData = actor->GetAnimData();

	if (animData)
		g_animDataToActor[animData] = actor;

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
					FindCAFSequenceForGroup(animGroup, rule);

				if (!cafSeq)
					continue;

				CAFAnimEntry entry;
				entry.seq = cafSeq;
				entry.rule = &rule;

				void* mgr = *(void**)((char*)animData + 0x98);
				if (mgr)
				{
					for (auto& [group, cafVec] : g_cafSequencesByGroup)
					{
						for (CAFSequence& caf : cafVec)
						{
							if (!caf.seq) continue;
							// Re-register if back-pointer was cleared by unequip
							void* backPtr = *(void**)((char*)caf.seq + 0x40);
							if (!backPtr || g_cafNeedsReregistration)
							{
								g_addSequence(mgr, caf.seq, 0, 1);
								_MESSAGE("CAF: re-registered seq=%p +0x40=%p",
									caf.seq, *(void**)((char*)caf.seq + 0x40));
							}
						}
					}
					g_cafNeedsReregistration = false;
				}
			}
		}
	}

	signed __int16 result =
		g_originalLoadAnimGroup(actor, animGroup, a3, a4);

	/*void* animsMap = *(void**)((char*)animData + 0x9C);
	void* outSeq = nullptr;
	if (animsMap && g_animsMapGetAt(animsMap, (UInt16)animGroup, &outSeq) && outSeq)
	{
		UInt32 vtable = *(UInt32*)outSeq;
		if (vtable != 0x00A3C758)
		{
			AnimSequenceMultiple* multiple = (AnimSequenceMultiple*)outSeq;
			if (!g_multipleToActor.count(multiple))
			{
				g_multipleToActor[multiple] = actor;
				_MESSAGE("CAF: late-mapped multiple=%p to actor=%08X group=%u",
					multiple, actor->refID, animGroup);
			}
		}
	}*/

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
	_MESSAGE("CAF: dtor This=%p Anim=%p vanillaEntry=%d",
		This, This ? This->Anim : nullptr,
		g_vanillaSingleAnims.count(This));

	auto it = g_vanillaSingleAnims.find(This);
	if (it != g_vanillaSingleAnims.end())
	{
		This->Anim = it->second;
		g_vanillaSingleAnims.erase(it);
	}

	g_originalAnimSequenceSingleDtor(This);
}

void InstallSingleDtorHook()
{
	WriteRelJump(0x00471760, (UInt32)AnimSequenceSingleDtorHook);
}

// Credits to lStewieAl
[[nodiscard]] __declspec(noinline) UInt32 __stdcall DetourVtable(UInt32 addr, UInt32 dst)
{
	UInt32 originalFunction = *(UInt32*)addr;
	SafeWrite32(addr, dst);
	return originalFunction;
}

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

void PreloadCAFSequences()
{

	_MESSAGE("CAF PRELOAD: ENTER PreloadCAFSequences()");
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

			void* src = model->controllerSequence;

			// Map NiSequenceData fields -> BSAnimGroupSequence fields
			*(float*)((char*)seq + 0x24) = 0.0f;                              // startTime
			*(float*)((char*)seq + 0x28) = *(float*)((char*)src + 0x1C);      // frequency (1.0)
			*(float*)((char*)seq + 0x2C) = 0.0f;                              // phase
			*(float*)((char*)seq + 0x30) = *(float*)((char*)src + 0x30);      // duration
			*(float*)((char*)seq + 0x34) = 0.0f;                              // currentTime
			*(float*)((char*)seq + 0x38) = 0.0f;                              // lastTime

			_MESSAGE("CAF: seq timing set duration=%f",
				*(float*)((char*)seq + 0x30));

			_MESSAGE("CAF PRELOAD: seq created=%p animGroup=%p controller=%p",
				seq,
				model->animGroup,
				model->controllerSequence);

			_MESSAGE("CAF PRELOAD: calling RegisterCAFSequence seq=%p filePath=%s",
				seq,
				seq ? seq->filePath : "<null>");

			RegisterCAFSequence(seq); // reuses existing matching logic

			_MESSAGE("CAF: PreloadCAF OK group=%u %s", group, path.c_str());

			_MESSAGE("CAF: seq+0x24=%f seq+0x28=%f seq+0x2C=%f",
				*(float*)((char*)seq + 0x24),
				*(float*)((char*)seq + 0x28),
				*(float*)((char*)seq + 0x2C));
		}
	}

	_MESSAGE("CAF: PreloadCAF complete, %zu groups populated",
		g_cafSequencesByGroup.size());
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

__declspec(naked) void Sub65D790Hook()
{
	__asm
	{
		pushad

		mov eax, ecx
		test eax, eax
		jz skip

		// basic sanity: must be aligned pointer
		test eax, 0x3
		jnz skip

		cmp eax, 0x10000
		jb skip

		// vtable check (Actor RTTI sanity)
		mov edx, [eax]
		test edx, edx
		jz skip

		// OPTIONAL but highly recommended:
		// compare against known Actor vtable region
		// (you already saw PlayerCharacter vtable in logs)
		cmp edx, 0x00A6E074
		jne skip

		push eax
		call GetCurrentAnimActorPtr
		pop ecx
		test eax, eax
		jz skip

		mov[eax], ecx   // safe store

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

void Install()
{

	g_originalGetMultiple = DetourVtable(
		0x00A3C768,
		(UInt32)&GetAnimGroupSequenceMultipleHook
	);

	g_originalGetSingle = DetourVtable(
		0x00A3C73C,
		(UInt32)&GetAnimGroupSequenceSingleHook
	);

	Install65D790Hook();

	//WriteRelCall(0x4741D9, (UInt32)&NewAnimSequenceMultipleHook);

	g_originalLoadAnimGroup =
		(Actor_LoadAnimGroup_t)CreateTrampoline(0x005E5690, 7);

	WriteRelJump(0x005E5690, (UInt32)ActorLoadAnimGroupHook);

	InstallSingleDtorHook();

	InstallLoadKFModelHook();
	PreloadCAFSequences();

	_MESSAGE("CAF: All hooks installed");
}
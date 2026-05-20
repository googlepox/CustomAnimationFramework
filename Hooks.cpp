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

std::unordered_map<ActorAnimData*, CAFAnimData> g_cafAnimData;

static std::unordered_map<
	UInt32,
	std::vector<BSAnimGroupSequence*>
> g_cafSequences;

std::unordered_map<ActorAnimData*, NiTListBase<CAFAnimEntry>*> g_cafAnimsByActor;

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
	if (!filePath || !rule || !*rule)
		return false;

	std::string f = NormalizeAnimString(filePath);
	std::string r = NormalizeAnimString(rule);

	return f.find(r) != std::string::npos;
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
	_MESSAGE("CAF: RegisterCAFSequence group=%u filePath='%s' fileName='%s'",
		group, seq->filePath, fileName ? fileName : "<null>");

	auto it = g_animOverrideRules.find(group);
	if (it == g_animOverrideRules.end())
	{
		_MESSAGE("CAF: RegisterCAFSequence — no rule for group %u", group);
		return;
	}

	for (const AnimOverrideRule& rule : it->second)
	{
		_MESSAGE("CAF: RegisterCAFSequence — comparing '%s' vs rule '%s'",
			fileName, rule.replacementFile.c_str());

		if (AnimStringMatch(fileName, rule.replacementFile.c_str()))
		{
			g_cafSequencesByGroup[group].push_back({ seq, &rule });
			_MESSAGE("CAF: RegisterCAFSequence MATCHED group=%u '%s'", group, fileName);
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

// Capture player AnimData during NewActorAnimDataHook
// Add this to your existing hook:
static ActorAnimData* g_playerAnimData = nullptr;

thread_local bool g_loadingCAF = false;

ActorAnimData* (__thiscall* NewActorAnimData)(ActorAnimData*) = (ActorAnimData * (__thiscall*)(ActorAnimData*))0x473EB0;
ActorAnimData* __fastcall NewActorAnimDataHook(ActorAnimData* This, UInt32 edx)
{
	ActorAnimData* result = NewActorAnimData(This);

	// Allocate CAF list
	auto* cafList = (NiTListBase<CAFAnimEntry>*)FormHeap_Allocate(sizeof(NiTListBase<CAFAnimEntry>));
	*(void**)cafList = (void*)0x00A3C748;
	cafList->start = nullptr;
	cafList->end = nullptr;
	cafList->numItems = 0;

	// Store in map
	g_cafAnimsByActor[result] = cafList;

	return result;
}

ActorAnimData* (__thiscall* DisposeActorAnimData)(ActorAnimData*) = (ActorAnimData * (__thiscall*)(ActorAnimData*))0x475B60;
ActorAnimData* __fastcall DisposeActorAnimDataHook(ActorAnimData* This, UInt32 edx)
{	
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
static void AddCAFAnimGroupSequence(
	ActorAnimData* animData,
	BSAnimGroupSequence* seq,
	const AnimOverrideRule& rule
)
{
	if (!animData || !seq)
		return;

	CAFAnimData& caf = g_cafAnimData[animData];

	caf.entries.push_back({
		seq,
		&rule
		});

	_MESSAGE(
		"CAF: registered CAF anim '%s'",
		seq->filePath
	);
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

		if (caf.rule != &rule)
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





void(__thiscall* AddMultiple)(AnimSequenceMultiple*, BSAnimGroupSequence*) = (void(__thiscall*)(AnimSequenceMultiple*, BSAnimGroupSequence*))0x471930;
void __fastcall AddMultipleHook(
	AnimSequenceMultiple* This,
	UInt32,
	BSAnimGroupSequence* seq
)
{

	ThisStdCall(g_originalAddMultiple, This, seq);
}



BSAnimGroupSequence* FindCAFAnimation(
	Actor* actor,
	UInt32 baseGroup)
{
	if (!actor)
		return nullptr;

	ActorAnimData* animData = actor->GetAnimData();
	if (!animData)
		return nullptr;

	// Use map lookup instead of casting
	auto cafIt = g_cafAnimsByActor.find(animData);
	if (cafIt == g_cafAnimsByActor.end())
		return nullptr;

	NiTListBase<CAFAnimEntry>* cafList = cafIt->second;
	if (!cafList || cafList->numItems == 0)
		return nullptr;

	// Rest of your logic...
	for (auto it = cafList->start; it; it = it->next)
	{
		CAFAnimEntry* entry = it->data;
		if (!entry || !entry->seq || !entry->seq->animGroup)
			continue;

		UInt32 cafGroup = entry->seq->animGroup->animGroup;

		if (cafGroup != baseGroup)
			continue;

		if (!entry->rule)
			continue;

		bool passed = true;
		for (const AnimConditionEntry& cond : entry->rule->conditions)
		{
			if (!cond.fn(actor, baseGroup, cond.arg.c_str()))
			{
				passed = false;
				break;
			}
		}

		if (passed)
			return entry->seq;
	}

	return nullptr;
}


BSAnimGroupSequence* (__thiscall* GetAnimGroupSequenceSingle)(AnimSequenceSingle*, int) = (BSAnimGroupSequence * (__thiscall*)(AnimSequenceSingle*, int))0x00471710;
BSAnimGroupSequence* __fastcall GetAnimGroupSequenceSingleHook(
	AnimSequenceSingle* This,
	UInt32,
	int index
)
{
	BSAnimGroupSequence* base =
		(BSAnimGroupSequence*)ThisStdCall(
			g_originalGetSingle,
			This,
			index
		);

	if (g_inLoadAnimGroup &&
		g_pendingCAFSequence &&
		base &&
		base->animGroup &&
		base->animGroup->animGroup ==
		g_pendingCAFSequence->animGroup->animGroup)
	{
		if (g_cafDebug)
		{
			_MESSAGE(
				"CAF: Replacing SINGLE with %s",
				g_pendingCAFSequence->filePath
			);
		}

		return g_pendingCAFSequence;
	}

	return base;
}






BSAnimGroupSequence* (__thiscall* GetAnimGroupSequenceMultiple)(AnimSequenceMultiple*, int) = (BSAnimGroupSequence * (__thiscall*)(AnimSequenceMultiple*, int))0x00470BF0;
BSAnimGroupSequence* __fastcall GetAnimGroupSequenceMultipleHook(
	AnimSequenceMultiple* sequence,
	void*,
	int index
)
{
	BSAnimGroupSequence* base =
		(BSAnimGroupSequence*)ThisStdCall(g_originalGetMultiple, sequence, index);

	if (!base || !sequence) return base;

	Actor* actor = g_currentAnimActor;
	if (!actor || !base->animGroup) return base;

	UInt32 group = base->animGroup->animGroup;

	auto it = g_cafAnimData.find(actor->GetAnimData());

	_MESSAGE("CAF: GetMultiple actor=%08X group=%u cafDataFound=%d",
		actor->refID, group, it != g_cafAnimData.end());

	if (it == g_cafAnimData.end()) return base;

	_MESSAGE("CAF: GetMultiple entries=%zu", it->second.entries.size());

	// Try CAF overrides
	for (const CAFAnimEntry& entry : it->second.entries)
	{
		if (!entry.seq || !entry.rule)
			continue;

		if (ConditionsPass(*entry.rule, actor, group))
		{
			if (g_cafDebug)
			{
				_MESSAGE(
					"CAF: MULTIPLE select group %u\n  %s\n  -> %s",
					group,
					base->filePath,
					entry.seq->filePath
				);
			}

			bool found = false;
			for (auto* it = sequence->Anims->start; it; it = it->next)
			{
				if (it->data == entry.seq)
				{
					found = true;
					break;
				}
			}

			_MESSAGE("CAF: seq present in MULTIPLE = %d", found);


			return entry.seq;
		}
	}

	// Fallback to vanilla
	return base;
}

AnimSequenceMultiple* (__thiscall* NewAnimSequenceMultiple)(AnimSequenceMultiple*, AnimSequenceSingle*) = (AnimSequenceMultiple * (__thiscall*)(AnimSequenceMultiple*, AnimSequenceSingle*))0x00473D90;
AnimSequenceMultiple* __fastcall NewAnimSequenceMultipleHook(
    AnimSequenceMultiple* This,
    UInt32,
    AnimSequenceSingle* sourceSingle
)
{
    _MESSAGE(
        "CAF[NASM]: ENTER This=%p sourceSingle=%p",
        This,
        sourceSingle
    );

    AnimSequenceMultiple* seq =
        NewAnimSequenceMultiple(This, sourceSingle);

    _MESSAGE(
        "CAF[NASM]: after original seq=%p",
        seq
    );

    if (!seq)
    {
        _MESSAGE("CAF[NASM]: seq == nullptr → bail");
        return seq;
    }

    // one-time guard
    if (g_cafInjectedMultiples.contains(seq))
    {
        _MESSAGE(
            "CAF[NASM]: seq %p already injected → bail",
            seq
        );
        return seq;
    }

    size_t totalInjected = 0;

    _MESSAGE(
        "CAF[NASM]: starting unconditional injection, CAF groups=%zu",
        g_cafSequencesByGroup.size()
    );

    for (auto& [group, cafVec] : g_cafSequencesByGroup)
    {
        _MESSAGE(
            "CAF[NASM]: group %u has %zu CAF sequences",
            group,
            cafVec.size()
        );

        for (CAFSequence& caf : cafVec)
        {
            _MESSAGE(
                "CAF[NASM]: considering caf.seq=%p",
                caf.seq
            );

            if (!caf.seq)
            {
                _MESSAGE("CAF[NASM]: caf.seq == nullptr → skip");
                continue;
            }

            seq->AddAnimGroupSequence(caf.seq);
            totalInjected++;

            _MESSAGE(
                "CAF[NASM]: INJECTED caf.seq=%p",
                caf.seq
            );
        }
    }

    g_cafInjectedMultiples.insert(seq);

    _MESSAGE(
        "CAF[NASM]: injection complete seq=%p totalInjected=%zu",
        seq,
        totalInjected
    );

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
	g_inLoadAnimGroup = true;
	g_currentAnimActor = actor;
	g_currentAnimGroup = animGroup;

	ActorAnimData* animData = actor->GetAnimData();

	g_loadAnimDepth++;

	if (animData && g_loadAnimDepth == 1)
	{
		auto& cafData = g_cafAnimData[animData];
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

				g_cafAnimData[animData].entries.push_back(entry);

				_MESSAGE("CAF: injected runtime entry %s", cafSeq->filePath);
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
	void* modelLoader = *(void**)0x00B33A1C;
	if (!modelLoader)
	{
		_MESSAGE("CAF: PreloadCAF — ModelLoader not ready");
		return;
	}

	for (auto& [group, rules] : g_animOverrideRules)
	{
		for (const AnimOverrideRule& rule : rules)
		{
			std::string path = "Characters\\_Male\\" + rule.replacementFile + ".kf";

			g_loadingCAF = true;
			kfModel* model = (kfModel*)g_originalLoadKFModel(modelLoader, path.c_str());
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

			RegisterCAFSequence(seq); // reuses existing matching logic

			_MESSAGE("CAF: PreloadCAF OK group=%u %s", group, path.c_str());
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

void Install()
{
	g_originalAddMultiple = DetourVtable(
		0x00A3C75C,
		(UInt32)&AddMultipleHook  // can gut this entirely
	);

	g_originalGetMultiple = DetourVtable(
		0x00A3C768,
		(UInt32)&GetAnimGroupSequenceMultipleHook
	);

	WriteRelCall(0x4741D9, (UInt32)&NewAnimSequenceMultipleHook);

	g_originalLoadAnimGroup =
		(Actor_LoadAnimGroup_t)CreateTrampoline(0x005E5690, 7);

	WriteRelJump(0x005E5690, (UInt32)ActorLoadAnimGroupHook);

	InstallLoadKFModelHook();
	PreloadCAFSequences();

	_MESSAGE("CAF: All hooks installed");
}

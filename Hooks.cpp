#include "Hooks.h"

#include "obse/GameAPI.h"
#include "obse/GameObjects.h"
#include "obse/NiNodes.h"
#include <obse_common/SafeWrite.h>

#include <windows.h>
#include <cstdint>
#include <cstring>

#include <Conditions.h>
#include <Core.h>
#include <unordered_set>

#define AnimString "_CAF"

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

class ActorAnimDataEx : public ActorAnimData
{
public:
	NiTListBase<BSAnimGroupSequence>* cafAnims;
};

class AnimSequenceSingleEx : public AnimSequenceSingle
{
public:
	BSAnimGroupSequence* cafAnim = nullptr;
};

class AnimSequenceMultipleEx : public AnimSequenceMultiple
{
public:
	BSAnimGroupSequence* cafAnim = nullptr;
};

static ActorAnimDataEx* AnimDataAnimation = NULL;

ActorAnimData* (__thiscall* NewActorAnimData)(ActorAnimData*) = (ActorAnimData * (__thiscall*)(ActorAnimData*))0x473EB0;
ActorAnimData* __fastcall NewActorAnimDataHook(ActorAnimData* This, UInt32 edx)
{
	return NewActorAnimData(This);

}

ActorAnimData* (__thiscall* DisposeActorAnimData)(ActorAnimData*) = (ActorAnimData * (__thiscall*)(ActorAnimData*))0x475B60;
ActorAnimData* __fastcall DisposeActorAnimDataHook(ActorAnimData* This, UInt32 edx)
{

	
	return DisposeActorAnimData(This);

}

bool(__thiscall* AddAnimation)(ActorAnimData*, kfModel*, UInt8) = (bool(__thiscall*)(ActorAnimData*, kfModel*, UInt8))0x474070;
bool __fastcall AddAnimationHook(ActorAnimData* This, UInt32 edx, kfModel* Model, UInt8 Arg)
{
	return (bool)ThisStdCall(g_originalAddAnimation, This, Model, Arg);

}
static void AddCAFAnimGroupSequence(BSAnimGroupSequence* seq)
{
	if (!seq)
		return;

	// ONLY store pointer, no refcounting here
	NiTListBase<BSAnimGroupSequence>* caf = AnimDataAnimation->cafAnims;

	for (auto it = caf->start; it; it = it->next)
	{
		if (it->data == seq)
			return;
	}

	auto* node = caf->AllocateNode();
	node->data = seq;
	node->next = nullptr;
	node->prev = caf->end;

	if (caf->end)
		caf->end->next = node;
	else
		caf->start = node;

	caf->end = node;
	++caf->numItems;
}


void(__thiscall* AddSingle)(AnimSequenceSingle*, BSAnimGroupSequence*) = (void(__thiscall*)(AnimSequenceSingle*, BSAnimGroupSequence*))0x470BA0;
void __fastcall AddSingleHook(AnimSequenceSingle* This, UInt32 edx, BSAnimGroupSequence* AnimGroupSequence)
{

	//if (AnimGroupSequence && AnimGroupSequence->animGroup) AddCAFAnimGroupSequence(AnimGroupSequence);
	ThisStdCall(g_originalAddSingle, This, AnimGroupSequence);

}

void(__thiscall* AddMultiple)(AnimSequenceMultiple*, BSAnimGroupSequence*) = (void(__thiscall*)(AnimSequenceMultiple*, BSAnimGroupSequence*))0x471930;
void __fastcall AddMultipleHook(AnimSequenceMultiple* This, UInt32 edx, BSAnimGroupSequence* AnimGroupSequence)
{

	if (AnimGroupSequence->animGroup) {}
		//AddCAFAnimGroupSequence(AnimGroupSequence);
	else
		ThisStdCall(g_originalAddMultiple, This, AnimGroupSequence);

}

BSAnimGroupSequence* (__thiscall* GetAnimGroupSequenceSingle)(AnimSequenceSingle*, int) = (BSAnimGroupSequence * (__thiscall*)(AnimSequenceSingle*, int))0x00471710;
BSAnimGroupSequence* __fastcall GetAnimGroupSequenceSingleHook(AnimSequenceSingle* This, UInt32 edx, int Index)
{

	return (BSAnimGroupSequence*)ThisStdCall(g_originalGetSingle, This, Index);

}

BSAnimGroupSequence* (__thiscall* GetAnimGroupSequenceMultiple)(AnimSequenceMultiple*, int) = (BSAnimGroupSequence * (__thiscall*)(AnimSequenceMultiple*, int))0x00470BF0;
BSAnimGroupSequence* __fastcall GetAnimGroupSequenceMultipleHook(AnimSequenceMultiple* This, UInt32 edx, int Index)
{

	return (BSAnimGroupSequence*)ThisStdCall(g_originalGetMultiple, This, Index);

}

AnimSequenceMultiple* (__thiscall* NewAnimSequenceMultiple)(AnimSequenceMultiple*, AnimSequenceSingle*) = (AnimSequenceMultiple * (__thiscall*)(AnimSequenceMultiple*, AnimSequenceSingle*))0x00473D90;
AnimSequenceMultiple* __fastcall NewAnimSequenceMultipleHook(AnimSequenceMultiple* This, UInt32 edx, AnimSequenceSingle* SourceAnimSequence)
{

	return NewAnimSequenceMultiple(This, SourceAnimSequence);

}

TESAnimGroup* (__cdecl* LoadAnimGroup)(NiControllerSequence*, char*) = (TESAnimGroup * (__cdecl*)(NiControllerSequence*, char*))0x0051B490;
TESAnimGroup* __cdecl LoadAnimGroupHook(NiControllerSequence* ControllerSequence, char* FilePath)
{

	return (TESAnimGroup*)LoadAnimGroup(ControllerSequence, FilePath);

}

__declspec(naked) void NewAnimSequenceSingleHook()
{

	__asm {
		mov[eax + 0x8], esi
		mov[eax + 0x4], esi
		push	eax
		mov     dword ptr[eax], 0x00A3C72C
		jmp		Jumpers::NewAnimSequenceSingle::Return
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
    // patchSize MUST be >= 5 and MUST align to instruction boundaries
    if (patchSize < 5)
        return nullptr;

    // Allocate executable memory
    uint8_t* tramp = (uint8_t*)VirtualAlloc(
        nullptr,
        patchSize + 5,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );

    if (!tramp)
        return nullptr;

    // Copy overwritten bytes
    memcpy(tramp, (void*)src, patchSize);

    // Write jump back to original code after overwritten bytes
    uintptr_t srcEnd = src + patchSize;
    uintptr_t trampEnd = (uintptr_t)(tramp + patchSize);

    tramp[patchSize] = 0xE9; // JMP rel32
    *(int32_t*)(tramp + patchSize + 1) =
        (int32_t)(srcEnd - (trampEnd + 5));

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

// Credits to lStewieAl
[[nodiscard]] __declspec(noinline) UInt32 __stdcall DetourVtable(UInt32 addr, UInt32 dst)
{
    UInt32 originalFunction = *(UInt32*)addr;
    SafeWrite32(addr, dst);
    return originalFunction;
}

void Install()
{
	g_originalAddSingle = DetourVtable(
        0x00A3C730,
        (UInt32)&AddSingleHook
    );

	g_originalAddMultiple = DetourVtable(
        0x00A3C75C,
        (UInt32)&AddMultipleHook
    );

	g_originalGetSingle = DetourVtable(
		0x00A3C73C,
		(UInt32)&GetAnimGroupSequenceSingleHook
	);

	g_originalGetMultiple = DetourVtable(
		0x00A3C768,
		(UInt32)&GetAnimGroupSequenceMultipleHook
	);

	g_originalAddAnimation = DetourVtable(
		0x00AD49A4,
		(UInt32)&AddAnimationHook
	);



	constexpr uintptr_t kNewActorAnimDataAddr = 0x473EB0;
	size_t kPatchSize = 20;

	NewActorAnimData =
		(ActorAnimData * (__thiscall*)(ActorAnimData*))
		CreateTrampoline(kNewActorAnimDataAddr, kPatchSize);

	InstallHook(
		kNewActorAnimDataAddr,
		(void*)&NewActorAnimDataHook,
		kPatchSize
	);



	constexpr uintptr_t kDisposeActorAnimDataAddr = 0x475B60;
	kPatchSize = 20;

	DisposeActorAnimData =
		(ActorAnimData * (__thiscall*)(ActorAnimData*))
		CreateTrampoline(kDisposeActorAnimDataAddr, kPatchSize);

	InstallHook(
		kDisposeActorAnimDataAddr,
		(void*)&DisposeActorAnimDataHook,
		kPatchSize
	);

	WriteRelCall(0x4741D9, (UInt32)&NewAnimSequenceMultipleHook);

	WriteRelCall(0x436DCD, (UInt32)&LoadAnimGroupHook);

    _MESSAGE("CAF: All hooks installed");
}

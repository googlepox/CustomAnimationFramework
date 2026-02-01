#include "CustomAnimations.h"

#include "obse/GameAPI.h"
#include "obse/GameObjects.h"
#include "obse/NiNodes.h"
#include <obse_common/SafeWrite.h>

constexpr UInt32 kAddSingleVtableSlot = 0x00A3C730;
constexpr UInt32 kAddMultipleVtableSlot = 0x00A3C75C;
constexpr UInt32 kGetSequenceSingleVtableSlot = 0x00A3C73C;
constexpr UInt32 kGetSequenceMultipleVtableSlot = 0x00A3C768;
constexpr UInt32 kGetAnimDataVtableSlot = 0x00A46DA8;

UInt32 g_originalAddSingle = 0;
UInt32 g_originalAddMultiple = 0;
UInt32 g_originalGetSequenceSingle = 0;
UInt32 g_originalGetSequenceMultiple = 0;
UInt32 g_originalGetAnimData = 0;

std::unordered_map<UInt32, std::vector<AnimOverrideRule>> g_animOverrideRules;
CustomAnimationManager* CustomAnimationManager::instance = nullptr;
thread_local Actor* g_currentAnimActor = nullptr;

CustomAnimationManager* CustomAnimationManager::GetSingleton()
{
    if (!instance)
        instance = new CustomAnimationManager();
    return instance;
}

void CustomAnimationManager::RegisterAnimOverride(
    UInt32 group,
    const char* replacementFile,
    AnimCondition_t condition
)
{
    if (!replacementFile || !replacementFile[0])
        return;

    g_animOverrideRules[group].push_back({
        replacementFile,
        condition
        });

    _MESSAGE(
        "CAF: Registered anim override for group %u -> %s",
        group,
        replacementFile
    );
}



bool CustomAnimationManager::Always(Actor*, UInt32)
{
    return true;
}

bool CustomAnimationManager::PlayerOnly(Actor* actor, UInt32)
{
    return actor == *g_thePlayer;
}

bool CustomAnimationManager::UsingOneHandedBlade(Actor* actor, UInt32)
{
    auto items = actor->GetEquippedItems();
    for (int i = 0; i < items.size(); i++)
    {
        TESForm* eq = items.at(i);

        if (!eq)
            continue;

        TESObjectWEAP* weap = OBLIVION_CAST(eq, TESForm, TESObjectWEAP);
        if (weap)
        {
            return weap->type == TESObjectWEAP::kType_BladeOneHand;
        }
    }
    return false;
}

bool CustomAnimationManager::UsingOneHandedBlunt(Actor* actor, UInt32)
{

    auto items = actor->GetEquippedItems();
    for (int i = 0; i < items.size(); i++)
    {
        TESForm* eq = items.at(i);

        if (!eq)
            continue;

        TESObjectWEAP* weap = OBLIVION_CAST(eq, TESForm, TESObjectWEAP);
        if (weap)
        {
            return weap->type == TESObjectWEAP::kType_BluntOneHand;
        }
    }
    return false;
}

bool CustomAnimationManager::UsingTwoHandedBlade(Actor* actor, UInt32)
{

    auto items = actor->GetEquippedItems();
    for (int i = 0; i < items.size(); i++)
    {
        TESForm* eq = items.at(i);

        if (!eq)
            continue;

        TESObjectWEAP* weap = OBLIVION_CAST(eq, TESForm, TESObjectWEAP);
        if (weap)
        {
            return weap->type == TESObjectWEAP::kType_BladeTwoHand;
        }
    }
    return false;
}

bool CustomAnimationManager::UsingTwoHandedBlunt(Actor* actor, UInt32)
{

    auto items = actor->GetEquippedItems();
    for (int i = 0; i < items.size(); i++)
    {
        TESForm* eq = items.at(i);

        if (!eq)
            continue;

        TESObjectWEAP* weap = OBLIVION_CAST(eq, TESForm, TESObjectWEAP);
        if (weap)
        {
            return weap->type == TESObjectWEAP::kType_BluntTwoHand;
        }
    }
    return false;
}

bool CustomAnimationManager::UsingBow(Actor* actor, UInt32)
{

    auto items = actor->GetEquippedItems();
    for (int i = 0; i < items.size(); i++)
    {
        TESForm* eq = items.at(i);

        if (!eq)
            continue;

        TESObjectWEAP* weap = OBLIVION_CAST(eq, TESForm, TESObjectWEAP);
        if (weap)
        {
            return weap->type == TESObjectWEAP::kType_Bow;
        }
    }
    return false;
}

bool CustomAnimationManager::UsingStaff(Actor* actor, UInt32)
{

    auto items = actor->GetEquippedItems();
    for (int i = 0; i < items.size(); i++)
    {
        TESForm* eq = items.at(i);

        if (!eq)
            continue;

        TESObjectWEAP* weap = OBLIVION_CAST(eq, TESForm, TESObjectWEAP);
        if (weap)
        {
            return weap->type == TESObjectWEAP::kType_Staff;
        }
    }
    return false;
}

bool LowStamina(Actor* actor, UInt32)
{
    return actor->GetActorValue(kActorVal_Fatigue) < 20.0f;
}

void __fastcall GetAnimDataHook(
    Actor* actor,
    void*
)
{
    g_currentAnimActor = actor;
    ThisStdCall(g_originalGetAnimData, actor);
}

static size_t SafeStrLen(const char* s)
{
    __try
    {
        return s ? strlen(s) : 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

static BSAnimGroupSequence* FindSequenceByFileSingle(
    AnimSequenceSingle* sequence,
    const char* targetFile
)
{
    if (!sequence || !targetFile)
        return nullptr;

    BSAnimGroupSequence* seq = sequence->GetAnimGroupSequence(0);
    if (!seq)
        return nullptr;

    size_t len = SafeStrLen(seq->filePath);
    size_t targetLen = SafeStrLen(targetFile);

    if (len >= targetLen &&
        _stricmp(seq->filePath + (len - targetLen), targetFile) == 0)
    {
        return seq;
    }

    return nullptr;
}

static BSAnimGroupSequence* FindSequenceByFileMultiple(
    AnimSequenceMultiple* sequence,
    const char* targetFile
)
{
    if (!sequence || !targetFile || !sequence->Anims)
        return nullptr;

    size_t targetLen = SafeStrLen(targetFile);

    for (auto e = sequence->Anims->start; e; e = e->next)
    {
        BSAnimGroupSequence* seq = e->data;
        if (!seq)
            continue;
        _MESSAGE("CAF: checking anim path = %s", seq->filePath);
        size_t len = SafeStrLen(seq->filePath);
        if (!len)
            continue;

        if (len >= targetLen &&
            _stricmp(seq->filePath + (len - targetLen), targetFile) == 0)
        {
            return seq;
        }
    }

    return nullptr;
}


void __fastcall AddSingleHook(
    AnimSequenceSingle* sequence,
    void*,
    BSAnimGroupSequence* anim
)
{
    ThisStdCall(g_originalAddSingle, sequence, anim);

    if (!anim || !anim->filePath)
        return;

    if (strstr(anim->filePath, "_CAF"))
    {
        _MESSAGE("CAF: Loaded custom anim %s", anim->filePath);
    }
}

void __fastcall AddMultipleHook(
    AnimSequenceMultiple* sequence,
    void*,
    BSAnimGroupSequence* anim
)
{
    ThisStdCall(g_originalAddMultiple, sequence, anim);

    if (!anim || !anim->filePath)
        return;

    if (strstr(anim->filePath, "_CAF"))
    {
        _MESSAGE("CAF: Loaded custom anim %s", anim->filePath);
    }
}

BSAnimGroupSequence* __fastcall GetAnimGroupSequenceSingleHook(
    AnimSequenceSingle* sequence,
    void*,
    int index
)
{
    BSAnimGroupSequence* base =
        reinterpret_cast<BSAnimGroupSequence*>(
            ThisStdCall(
                g_originalGetSequenceSingle,
                sequence,
                index
            )
            );

    if (!base || !base->animGroup)
        return base;
    Actor* actor = g_currentAnimActor;
    if (!actor)
        return base;

    _MESSAGE(
        "CAF: Base anim group=%u file=%s",
        base->animGroup->animGroup,
        base->filePath ? base->filePath : "<null>"
    );

    UInt32 group = base->animGroup->animGroup;

    auto it = g_animOverrideRules.find(group);
    if (it == g_animOverrideRules.end())
        return base;

    for (const auto& rule : it->second)
    {
        if (!rule.condition || rule.condition(actor, group))
        {
            BSAnimGroupSequence* replacement =
                FindSequenceByFileSingle(sequence, rule.replacementFile);
            if (replacement)
            {
                _MESSAGE(
                    "Anim override: %s -> %s",
                    base->filePath,
                    replacement->filePath
                );
                return replacement;
            }
        }
    }

    return base;
}



BSAnimGroupSequence* __fastcall GetAnimGroupSequenceMultipleHook(
    AnimSequenceMultiple* sequence,
    void*,
    int index
)
{
    BSAnimGroupSequence* base =
        reinterpret_cast<BSAnimGroupSequence*>(
            ThisStdCall(
                g_originalGetSequenceMultiple,
                sequence,
                index
            )
            );

    if (!base || !base->animGroup)
        return base;
    Actor* actor = g_currentAnimActor;
    if (!actor)
        return base;

    _MESSAGE(
        "CAF: Base anim group=%u file=%s",
        base->animGroup->animGroup,
        base->filePath ? base->filePath : "<null>"
    );

    UInt32 group = base->animGroup->animGroup;

    auto it = g_animOverrideRules.find(group);
    if (it == g_animOverrideRules.end())
        return base;
    for (const auto& rule : it->second)
    {
        if (!rule.condition || rule.condition(actor, group))
        {
            BSAnimGroupSequence* replacement =
                FindSequenceByFileMultiple(sequence, rule.replacementFile);
            _MESSAGE("replacement");
            if (replacement)
            {
                _MESSAGE(
                    "Anim override: %s -> %s",
                    base->filePath,
                    replacement->filePath
                );
                return replacement;
            }
        }
    }

    return base;
}




// Credits to lStewieAl
[[nodiscard]] __declspec(noinline) UInt32 __stdcall DetourVtable(UInt32 addr, UInt32 dst)
{
    UInt32 originalFunction = *(UInt32*)addr;
    SafeWrite32(addr, dst);
    return originalFunction;
}


void CustomAnimationManager::InstallCustomAnimationHooks()
{
    _MESSAGE("CAF: Installing animation hooks (forward-only mode)...");

    g_originalAddSingle =
        DetourVtable(
            kAddSingleVtableSlot,
            reinterpret_cast<UInt32>(&AddSingleHook)
        );

    g_originalAddMultiple =
        DetourVtable(
            kAddMultipleVtableSlot,
            reinterpret_cast<UInt32>(&AddMultipleHook)
        );

    g_originalGetSequenceSingle =
        DetourVtable(
            kGetSequenceSingleVtableSlot,
            reinterpret_cast<UInt32>(&GetAnimGroupSequenceSingleHook)
        );

    g_originalGetSequenceMultiple =
        DetourVtable(
            kGetSequenceMultipleVtableSlot,
            reinterpret_cast<UInt32>(&GetAnimGroupSequenceMultipleHook)
        );

    g_originalGetAnimData =
        DetourVtable(
            kGetAnimDataVtableSlot,
            reinterpret_cast<UInt32>(&GetAnimDataHook)
        );

    _MESSAGE("CAF: Animation hooks installed.");
}

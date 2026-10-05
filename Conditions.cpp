
#include "Conditions.h"

#include "EditorIDMapper/EditorIDMapperAPI.h"

#include "obse/GameAPI.h"
#include "obse/GameObjects.h"
#include "obse/NiNodes.h"
#include <obse_common/SafeWrite.h>

static std::unordered_map<std::string, AnimConditionFn> g_conditionRegistry;

thread_local Actor* g_currentAnimActor = nullptr;
thread_local UInt32 g_currentAnimGroup = 0;
std::unordered_set<UInt8> g_cafAnimGroups;

static bool Always(Actor*, UInt32, const char* arg)
{
    return true;
}

bool PlayerOnly(Actor* actor, UInt32, const char* arg)
{
    return actor == *g_thePlayer;
}

bool IsPlayableRace(Actor* actor, UInt32, const char* arg)
{
    TESNPC* npc = OBLIVION_CAST(actor, Actor, TESNPC);

    if (!npc || !npc->race.race) return false;

    return npc->race.race->isPlayable;
}

bool UsingOneHandedBlade(Actor* actor, UInt32, const char* arg)
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

bool UsingOneHandedBlunt(Actor* actor, UInt32, const char* arg)
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

bool UsingTwoHandedBlade(Actor* actor, UInt32, const char* arg)
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

bool UsingTwoHandedBlunt(Actor* actor, UInt32, const char* arg)
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

bool UsingBow(Actor* actor, UInt32, const char* arg)
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

bool UsingStaff(Actor* actor, UInt32, const char* arg)
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

bool EditorIDContains(Actor* actor, UInt32, const char* arg)
{
    if (!actor || !arg || !actor->baseExtraList.m_presenceBitfield)
        return false;

    auto items = actor->GetEquippedItems();

    for (size_t i = 0; i < items.size(); i++)
    {
        TESForm* eq = items.at(i);
        if (!eq)
            continue;

        TESObjectWEAP* weap = OBLIVION_CAST(eq, TESForm, TESObjectWEAP);
        if (!weap)
            continue;

        const char* editorID = weap->GetEditorName();

        if (!editorID)
        {
            editorID = EditorIDMapper::ReverseLookup(actor->refID);
        }

        if (!editorID)
            continue;

        const char* p = arg;

        while (*p)
        {
            const char* next = strchr(p, '|');
            size_t len = next ? (size_t)(next - p) : strlen(p);

            char token[256];
            if (len >= sizeof(token))
                len = sizeof(token) - 1;

            memcpy(token, p, len);
            token[len] = '\0';

            auto icontains = [](const char* a, const char* b) -> bool {
                if (!a || !b) return false;

                std::string A(a), B(b);

                std::transform(A.begin(), A.end(), A.begin(),
                    [](unsigned char c) { return std::tolower(c); });

                std::transform(B.begin(), B.end(), B.begin(),
                    [](unsigned char c) { return std::tolower(c); });

                return A.find(B) != std::string::npos;
                };

            if (icontains(editorID, token))
            {
                return true;
            }

            if (!next)
                break;

            p = next + 1;
        }
    }

    return false;
}

bool Race(Actor* actor, UInt32, const char* arg)
{
    if (!actor || !arg)
        return false;

    TESNPC* npc = OBLIVION_CAST(actor, Actor, TESNPC);

    if (!npc || !npc->race.race) return false;

    const char* editorID = npc->race.race->GetEditorName();

    if (!editorID)
        return false;

    const char* p = arg;

    while (*p)
    {
        const char* next = strchr(p, '|');
        size_t len = next ? (size_t)(next - p) : strlen(p);

        char token[256];
        if (len >= sizeof(token))
            len = sizeof(token) - 1;

        memcpy(token, p, len);
        token[len] = '\0';

        auto icontains = [](const char* a, const char* b) -> bool {
            if (!a || !b) return false;

            std::string A(a), B(b);

            std::transform(A.begin(), A.end(), A.begin(),
                [](unsigned char c) { return std::tolower(c); });

            std::transform(B.begin(), B.end(), B.begin(),
                [](unsigned char c) { return std::tolower(c); });

            return A.find(B) != std::string::npos;
            };

        if (icontains(editorID, token))
        {
            return true;
        }

        if (!next)
            break;

        p = next + 1;
    }

    return false;
}

bool WeaponOut(Actor* actor, UInt32, const char* arg)
{
    if (!actor || !actor->process)
        return false;
    return actor->process->GetWeaponOut();
}

bool IsSneaking(Actor* actor, UInt32, const char* arg)
{
    if (!actor || !actor->process)
        return false;

    UInt32 moveFlags = actor->process->GetMovementFlags();

    return (moveFlags & BaseProcess::kMovementFlag_Sneaking) != 0;
}

bool IsSwimming(Actor* actor, UInt32, const char* arg)
{
    if (!actor || !actor->process)
        return false;

    UInt32 moveFlags = actor->process->GetMovementFlags();

    return (moveFlags & BaseProcess::kMovementFlag_Swimming) != 0;
}

bool IsFemale(Actor* actor, UInt32, const char* arg)
{
    return (bool)ThisStdCall(0x5E1DF0, actor);
}

bool LowHealth(Actor* actor, UInt32, const char* arg)
{
    return actor->GetActorValue(kActorVal_Health) < 20.0f;
}

bool LowMagicka(Actor* actor, UInt32, const char* arg)
{
    return actor->GetActorValue(kActorVal_Magicka) < 20.0f;
}


bool LowStamina(Actor* actor, UInt32, const char* arg)
{
    return actor->GetActorValue(kActorVal_Fatigue) < 20.0f;
}

bool InFaction(Actor* actor, UInt32, const char* arg)
{
    if (!actor || !arg) return false;

    TESActorBase* base = OBLIVION_CAST(actor, Actor, TESActorBase);
    if (!base) return false;

    auto* entry = &base->actorBaseData.factionList;

    while (entry && entry->data)
    {
        if (!entry->data) continue;
        const char* editorID = entry->data->faction->GetEditorName();
        if (!editorID) continue;

        const char* p = arg;
        while (*p)
        {
            const char* next = strchr(p, '|');
            size_t len = next ? (size_t)(next - p) : strlen(p);
            char token[256];
            if (len >= sizeof(token)) len = sizeof(token) - 1;
            memcpy(token, p, len);
            token[len] = '\0';
            if (_stricmp(editorID, token) == 0) return true;
            if (!next) break;
            p = next + 1;
        }
    }
    return false;
}

bool HasSpell(Actor* actor, UInt32, const char* arg)
{
    if (!actor || !arg) return false;

    TESActorBase* base = OBLIVION_CAST(actor, Actor, TESActorBase);
    if (!base) return false;

    auto* entry = &base->spellList.spellList;

    while (entry && entry->type)
    {
        if (!entry->type) continue;
        const char* editorID = entry->type->GetEditorName();
        if (!editorID) continue;

        const char* p = arg;
        while (*p)
        {
            const char* next = strchr(p, '|');
            size_t len = next ? (size_t)(next - p) : strlen(p);
            char token[256];
            if (len >= sizeof(token)) len = sizeof(token) - 1;
            memcpy(token, p, len);
            token[len] = '\0';
            if (_stricmp(editorID, token) == 0) return true;
            if (!next) break;
            p = next + 1;
        }
    }
    return false;
}

bool HasItem(Actor* actor, UInt32, const char* arg)
{
    if (!actor || !arg) return false;

    ExtraContainerChanges* xChanges =
        (ExtraContainerChanges*)actor->baseExtraList.GetByType(kExtraData_ContainerChanges);
    if (!xChanges || !xChanges->data) return false;

    for (auto* node = xChanges->data->objList->Head(); node; node = node->next)
    {
        if (!node->item) continue;
        const char* editorID = node->Item()->type->GetEditorName();;
        if (!editorID) continue;

        const char* p = arg;
        while (*p)
        {
            const char* next = strchr(p, '|');
            size_t len = next ? (size_t)(next - p) : strlen(p);
            char token[256];
            if (len >= sizeof(token)) len = sizeof(token) - 1;
            memcpy(token, p, len);
            token[len] = '\0';
            if (_stricmp(editorID, token) == 0) return true;
            if (!next) break;
            p = next + 1;
        }
    }
    return false;
}

bool SkillLevel(Actor* actor, UInt32, const char* arg)
{
    if (!actor || !arg) return false;

    // arg format: "Blade>=50" or "Blade<30" etc.
    // Parse skill name and comparison
    char skillName[64] = {};
    char op[4] = {};
    int threshold = 0;

    if (sscanf_s(arg, "%63[A-Za-z]%3[><=!]%d", skillName, (unsigned)sizeof(skillName),
        op, (unsigned)sizeof(op), &threshold) < 3)
        return false;

    static const std::unordered_map<std::string, UInt32> skillMap = {
        {"Blade",       kActorVal_Blade},
        {"Blunt",       kActorVal_Blunt},
        {"HandToHand",  kActorVal_HandToHand},
        {"Armorer",     kActorVal_Armorer},
        {"Block",       kActorVal_Block},
        {"Athletics",   kActorVal_Athletics},
        {"HeavyArmor",  kActorVal_HeavyArmor},
        {"Sneak",       kActorVal_Sneak},
        {"Marksman",    kActorVal_Marksman},
        {"LightArmor",  kActorVal_LightArmor},
        {"Acrobatics",  kActorVal_Acrobatics},
        {"Security",    kActorVal_Security},
        {"Speechcraft", kActorVal_Speechcraft},
        {"Mercantile",  kActorVal_Mercantile},
        {"Illusion",    kActorVal_Illusion},
        {"Conjuration", kActorVal_Conjuration},
        {"Mysticism",   kActorVal_Mysticism},
        {"Destruction", kActorVal_Destruction},
        {"Alteration",  kActorVal_Alteration},
        {"Restoration", kActorVal_Restoration},
        {"Alchemy",     kActorVal_Alchemy},
    };

    auto it = skillMap.find(skillName);
    if (it == skillMap.end()) return false;

    float val = actor->GetActorValue(it->second);
    int skill = (int)val;

    std::string opStr(op);
    if (opStr == ">=") return skill >= threshold;
    if (opStr == "<=") return skill <= threshold;
    if (opStr == ">") return skill > threshold;
    if (opStr == "<") return skill < threshold;
    if (opStr == "==") return skill == threshold;
    if (opStr == "!=") return skill != threshold;

    return false;
}

void RegisterConditions()
{
    g_conditionRegistry["UsingOneHandedBlade"] = UsingOneHandedBlade;
    g_conditionRegistry["UsingOneHandedBlunt"] = UsingOneHandedBlunt;
    g_conditionRegistry["UsingTwoHandedBlade"] = UsingTwoHandedBlade;
    g_conditionRegistry["UsingTwoHandedBlunt"] = UsingTwoHandedBlunt;
    g_conditionRegistry["UsingBow"] = UsingBow;
    g_conditionRegistry["UsingStaff"] = UsingStaff;
    g_conditionRegistry["PlayerOnly"] = PlayerOnly;
    g_conditionRegistry["LowHealth"] = LowHealth;
    g_conditionRegistry["LowMagicka"] = LowMagicka;
    g_conditionRegistry["LowStamina"] = LowStamina;
    g_conditionRegistry["EditorIDContains"] = EditorIDContains;
    g_conditionRegistry["Always"] = Always;
    g_conditionRegistry["WeaponOut"] = WeaponOut;
    g_conditionRegistry["IsFemale"] = IsFemale;
    g_conditionRegistry["Race"] = Race;
    g_conditionRegistry["IsPlayableRace"] = IsPlayableRace;
    g_conditionRegistry["IsSneaking"] = IsSneaking;
    g_conditionRegistry["IsSwimming"] = IsSwimming;
    g_conditionRegistry["InFaction"] = InFaction;
    g_conditionRegistry["HasSpell"] = HasSpell;
    g_conditionRegistry["HasItem"] = HasItem;
    g_conditionRegistry["SkillLevel"] = SkillLevel;
}

AnimConditionFn GetConditionByName(const std::string& name)
{
    auto it = g_conditionRegistry.find(name);
    return it != g_conditionRegistry.end() ? it->second : nullptr;
}

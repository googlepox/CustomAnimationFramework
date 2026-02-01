
#include "Conditions.h"


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
    if (!actor || !arg)
        return false;
    auto items = actor->GetEquippedItems();
    for (int i = 0; i < items.size(); i++)
    {
        TESForm* eq = items.at(i);

        if (!eq)
            continue;
        TESObjectWEAP* weap = OBLIVION_CAST(eq, TESForm, TESObjectWEAP);
        if (weap)
        {
            const char* editorID = weap->GetEditorName();
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

                if (_stricmp(editorID, token) == 0 ||
                    strstr(editorID, token))
                {
                    return true;
                }

                if (!next)
                    break;

                p = next + 1;
            }

        }
    }

    return false;
}


bool LowStamina(Actor* actor, UInt32, const char* arg)
{
    return actor->GetActorValue(kActorVal_Fatigue) < 20.0f;
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
    g_conditionRegistry["LowStamina"] = LowStamina;
    g_conditionRegistry["EditorIDContains"] = EditorIDContains;
    g_conditionRegistry["Always"] = Always;
}

AnimConditionFn GetConditionByName(const std::string& name)
{
    auto it = g_conditionRegistry.find(name);
    return it != g_conditionRegistry.end() ? it->second : nullptr;
}

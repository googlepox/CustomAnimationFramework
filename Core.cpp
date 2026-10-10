#include "Core.h"

#include <unordered_map>
#include <vector>
#include <string>

#include "obse/GameObjects.h"


std::unordered_map<UInt32, std::vector<AnimOverrideRule>> g_animOverrideRules;

std::unordered_map<UInt32, std::string> g_animFileOverride;

void RegisterAnimOverride(
    UInt32 group,
    const std::string& replacementFile,
    const std::vector<AnimConditionEntry>& conditions
)
{
    if (replacementFile.empty())
        return;

    AnimOverrideRule rule{};
    rule.replacementFile = replacementFile;
    rule.conditions = conditions;

    g_animOverrideRules[group].push_back(rule);

    _MESSAGE(
        "CAF: Registered override | group=%u file=%s conditions=%zu",
        group,
        replacementFile.c_str(),
        conditions.size()
    );
}


const std::vector<AnimOverrideRule>* GetOverridesForGroup(UInt32 group)
{
    auto it = g_animOverrideRules.find(group);
    if (it == g_animOverrideRules.end())
        return nullptr;

    return &it->second;
}

bool __cdecl ConditionsPass(
    const AnimOverrideRule& rule,
    Actor* actor,
    UInt32 group
)
{
    if (rule.conditions.empty())
        return true;
    for (const AnimConditionEntry& c : rule.conditions)
    {
        if (!c.fn)
        {
            continue;
        }

        const char* arg = c.arg.empty() ? nullptr : c.arg.c_str();

        bool r = c.fn(actor, group, c.arg.c_str());
        if (c.exclusion) r = !r;
        if (!r) return false;
    }

    return true;
}

bool ConditionsPassPreActor(
    const AnimOverrideRule& rule,
    UInt32 group
)
{
    for (const AnimConditionEntry& cond : rule.conditions)
    {
        if (!cond.fn(nullptr, group, cond.arg.c_str()))
        {
            return false;
        }
    }

    return true;
}



void DumpAnimOverrides()
{
    _MESSAGE("CAF: Dumping animation overrides");

    for (auto& pair : g_animOverrideRules)
    {
        UInt32 group = pair.first;
        auto& rules = pair.second;

        for (auto& rule : rules)
        {
            _MESSAGE(
                "CAF: group=%u -> %s (%zu conditions)",
                group,
                rule.replacementFile.c_str(),
                rule.conditions.size()
            );
        }
    }
}

void ClearAnimOverrides()
{
    g_animOverrideRules.clear();
    _MESSAGE("CAF: Cleared all animation overrides");
}

#pragma once
#include <Conditions.h>

struct AnimConditionEntry
{
    AnimConditionFn fn;
    std::string     arg;
};

struct AnimOverrideRule
{
    std::string replacementFile;
    std::vector<AnimConditionEntry> conditions;
};


extern std::unordered_map<UInt32, std::vector<AnimOverrideRule>> g_animOverrideRules;
extern std::unordered_map<UInt32, std::string> g_animFileOverride;


void RegisterAnimOverride(
    UInt32 group,
    const std::string& replacementFile,
    const std::vector<AnimConditionEntry>& conditions
);

bool ConditionsPass(
    const AnimOverrideRule& rule,
    Actor* actor,
    UInt32 group
);

bool ConditionsPassPreActor(
    const AnimOverrideRule& rule,
    UInt32 group
);

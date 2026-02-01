#pragma once
#include <unordered_set>

class Actor;

using AnimConditionFn = bool (*)(Actor*, UInt32, const char* arg);

extern thread_local Actor* g_currentAnimActor;
extern thread_local UInt32 g_currentAnimGroup;
extern std::unordered_set<UInt8> g_cafAnimGroups;

static bool Always(Actor*, UInt32, const char* arg);
static bool PlayerOnly(Actor* actor, UInt32, const char* arg);
static bool UsingOneHandedBlade(Actor* actor, UInt32, const char* arg);
static bool UsingOneHandedBlunt(Actor* actor, UInt32, const char* arg);
static bool UsingTwoHandedBlade(Actor* actor, UInt32, const char* arg);
static bool UsingTwoHandedBlunt(Actor* actor, UInt32, const char* arg);
static bool UsingBow(Actor* actor, UInt32, const char* arg);
static bool UsingStaff(Actor* actor, UInt32, const char* arg);
static bool UsingStaff(Actor* actor, UInt32, const char* arg);
static bool LowStamina(Actor* actor, UInt32, const char* arg);

void RegisterConditions();
AnimConditionFn GetConditionByName(const std::string& name);


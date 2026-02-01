#include "Config.h"
#include "Conditions.h"
#include "Core.h"

#include <filesystem>
#include <string>
#include <vector>
#include <cstring>
#include <cctype>

#include "lib/simpleINI.hpp"

namespace fs = std::filesystem;

static char* Trim(char* s)
{
    while (*s && std::isspace(static_cast<unsigned char>(*s)))
        ++s;

    char* end = s + std::strlen(s);
    while (end > s && std::isspace(static_cast<unsigned char>(*(end - 1))))
        --end;

    *end = '\0';
    return s;
}

void LoadCAFInis()
{
    _MESSAGE("CAF: Loading INI files");

    const fs::path cafFolder =
        "Data\\OBSE\\Plugins\\CustomAnimationFramework";

    if (!fs::exists(cafFolder))
    {
        _MESSAGE("CAF: Config folder not found");
        return;
    }

    std::vector<fs::path> iniFiles;

    for (const auto& entry : fs::directory_iterator(cafFolder))
    {
        if (entry.is_regular_file() &&
            entry.path().extension() == ".ini")
        {
            iniFiles.push_back(entry.path());
        }
    }

    if (iniFiles.empty())
    {
        _MESSAGE("CAF: No INI files found");
        return;
    }

    _MESSAGE("CAF: %u INI file(s) found", iniFiles.size());

    for (const auto& path : iniFiles)
    {
        _MESSAGE("CAF: Reading %s", path.string().c_str());

        CSimpleIniA ini;
        ini.SetUnicode();
        ini.SetMultiKey();
        ini.SetAllowKeyOnly();

        if (ini.LoadFile(path.string().c_str()) < 0)
        {
            _WARNING("CAF: Failed to read %s", path.string().c_str());
            continue;
        }

        CSimpleIniA::TNamesDepend keys;
        ini.GetAllKeys("Overrides", keys);
        keys.sort(CSimpleIniA::Entry::LoadOrder());

        if (keys.empty())
            continue;

        for (const auto& key : keys)
        {
            const char* rawKey = key.pItem;
            const char* rawVal = ini.GetValue("Overrides", rawKey);
            if (!rawVal)
                continue;

            UInt32 group = std::atoi(rawKey);
            if (!group)
                continue;

            char temp[512];
            std::strncpy(temp, rawVal, sizeof(temp) - 1);
            temp[sizeof(temp) - 1] = '\0';

            char* rhs = Trim(temp);
            char* pipe = std::strchr(rhs, '|');

            char* filePart = rhs;
            char* condPart = nullptr;

            if (pipe)
            {
                *pipe = '\0';
                condPart = Trim(pipe + 1);
            }

            filePart = Trim(filePart);
            if (!*filePart)
                continue;

            std::vector<AnimConditionEntry> conditions;

            if (condPart && *condPart)
            {
                char* cond = condPart;
                while (cond && *cond)
                {
                    char* comma = std::strchr(cond, ',');
                    if (comma)
                        *comma = '\0';

                    char* token = Trim(cond);

                    char* arg = nullptr;
                    char* bracket = std::strchr(token, '[');
                    if (bracket)
                    {
                        *bracket = '\0';
                        arg = bracket + 1;

                        char* end = std::strchr(arg, ']');
                        if (end)
                            *end = '\0';
                    }

                    AnimConditionFn fn = GetConditionByName(token);

                    if (fn)
                    {
                        AnimConditionEntry entry;
                        entry.fn = fn;
                        entry.arg = arg ? arg : "";

                        conditions.push_back(std::move(entry));
                    }
                    else
                    {
                        _WARNING(
                            "CAF: Unknown condition '%s' in %s",
                            token,
                            path.string().c_str()
                        );
                    }

                    if (!comma)
                        break;

                    cond = comma + 1;
                }
            }
            g_cafAnimGroups.insert(group);
            RegisterAnimOverride(group, filePart, conditions);

            _MESSAGE(
                "CAF: Override group %u -> %s (%zu conditions)",
                group,
                filePart,
                conditions.size()
            );
        }
    }

    _MESSAGE("CAF: INI load complete");
}

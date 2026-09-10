#ifndef ALTBOT_SYNC_CONFIG_H
#define ALTBOT_SYNC_CONFIG_H

#include <unordered_map>
#include <unordered_set>

#include "Define.h"

class AltbotSyncConfig
{
public:
    static AltbotSyncConfig& instance()
    {
        static AltbotSyncConfig instance;

        return instance;
    }

    void Initialize();

    bool debug;

    bool triggerSyncOnGroupJoin;
    bool triggerSyncOnLevelUp;
    bool triggerSyncOnQuestComplete;

    std::unordered_set<uint32> classQuestIgnoreList;
    std::unordered_set<uint32> playerQuestIgnoreList;
    std::unordered_map<uint32, uint32> classQuestDelayList;

private:
    AltbotSyncConfig() = default;
    ~AltbotSyncConfig() = default;

    AltbotSyncConfig(const AltbotSyncConfig&) = delete;
    AltbotSyncConfig& operator=(const AltbotSyncConfig&) = delete;

    AltbotSyncConfig(AltbotSyncConfig&&) = delete;
    AltbotSyncConfig& operator=(AltbotSyncConfig&&) = delete;
};

#define sAltbotSyncConfig AltbotSyncConfig::instance()

#endif
#include "AltbotSyncConfig.h"

#include "Config.h"
#include "Helpers.h"
#include "Log.h"

template <class T>
void LoadSet(std::string const value, T& set)
{
    if (!value.size())
        return;

    std::vector<std::string> ids = split(value, ',');
    for (std::vector<std::string>::iterator i = ids.begin(); i != ids.end(); i++)
    {
        uint32 id = atoi((*i).c_str());
        set.insert(id);
    }
}

template <class T>
void LoadMap(std::string const value, T& map)
{
    if (!value.size())
        return;

    std::vector<std::string> pairs = split(value, ',');
    for (std::vector<std::string>::iterator i = pairs.begin(); i != pairs.end(); i++)
    {
        std::vector<std::string> pair = split((*i).c_str(), ':');
        if (pair.size() != 2)
            continue;

        uint32 key = atoi(pair.at(0).c_str());
        uint32 value = atoi(pair.at(1).c_str());
        map.insert({key, value});
    }
}

void AltbotSyncConfig::Initialize()
{
    debug = sConfigMgr->GetOption<bool>("AltbotSync.Debug", false);
    triggerSyncOnGroupJoin = sConfigMgr->GetOption<bool>("AltbotSync.CatchUp.Triggers.OnGroupJoin", false);
    triggerSyncOnLevelUp = sConfigMgr->GetOption<bool>("AltbotSync.CatchUp.Triggers.OnLevelUp", false);
    triggerSyncOnQuestComplete = sConfigMgr->GetOption<bool>("AltbotSync.CatchUp.Triggers.OnQuestComplete", false);

    LoadSet(sConfigMgr->GetOption<std::string>("AltbotSync.CatchUp.Quests.IgnoreClassQuests", ""),
            classQuestIgnoreList);
    LOG_INFO("module", "[catchup] Loaded {} ignored class quests", classQuestIgnoreList.size());

    LoadSet(sConfigMgr->GetOption<std::string>("AltbotSync.CatchUp.Quests.IgnorePlayerQuests", ""),
            playerQuestIgnoreList);
    LOG_INFO("module", "[catchup] Loaded {} ignored player quests", playerQuestIgnoreList.size());

    LoadMap(sConfigMgr->GetOption<std::string>("AltbotSync.CatchUp.Quests.DelayClassQuests", ""), classQuestDelayList);
    LOG_INFO("module", "[catchup] Loaded {} delayed class quests", classQuestDelayList.size());
}
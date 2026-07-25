#include "CatchUpQuestManager.h"
#include "Log.h"
#include "WorldScript.h"

class AltbotSync_WorldScript : public WorldScript
{
public:
    AltbotSync_WorldScript() : WorldScript("AltbotSync_WorldScript", {WORLDHOOK_ON_BEFORE_WORLD_INITIALIZED}) {}

    void OnBeforeWorldInitialized() override
    {
        LOG_INFO("server.loading", "[altbot-sync] Loading...");

        LOG_INFO("server.loading", "[altbot-sync] Initializing quest manager...");
        CatchUpQuestManager::Init();

        LOG_INFO("server.loading", "[altbot-sync] Ready");
    }
};

void AddWorldScripts() { new AltbotSync_WorldScript(); }

#include "CatchUpCommand.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "ScriptMgr.h"

using namespace Acore::ChatCommands;

class AltbotSync_CommandScript : public CommandScript
{
public:
    AltbotSync_CommandScript() : CommandScript("AltbotSync_CommandScript") {}

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable dcTable = {
            {"catchup", CatchUpCommandHandler::Handle, SEC_PLAYER, Console::No},
        };
        static ChatCommandTable root = {{"altsync", dcTable}};
        return root;
    }
};

void AddCommandScripts() { new AltbotSync_CommandScript(); }

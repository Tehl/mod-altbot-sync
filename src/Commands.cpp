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
            {"test", HandleExampleCommand, SEC_PLAYER, Console::No},
        };
        static ChatCommandTable root = {{"altsync", dcTable}};
        return root;
    }

    static bool HandleExampleCommand(ChatHandler* handler, Optional<std::string> param)
    {
        // ChatHandler could be Console or Player session
        handler->PSendSysMessage("Hello there! This is an example command");
        return true;
    }
};

void AddCommandScripts() { new AltbotSync_CommandScript(); }

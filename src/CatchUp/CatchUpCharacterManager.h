#ifndef ALTBOT_SYNC_CATCHUPCHARACTERMANAGER_H
#define ALTBOT_SYNC_CATCHUPCHARACTERMANAGER_H

#include <unordered_map>

#include "Define.h"
#include "Player.h"

class CatchUpCharacterManager
{
public:
    CatchUpCharacterManager(Player* bot);

    void GiveLevel(Player* player);
    void GiveAbilities();
    void GiveBags(Player* player);

private:
    static std::unordered_map<uint32, uint32> bags_vanilla;
    static std::unordered_map<uint32, uint32> bags_tbc;
    static std::unordered_map<uint32, uint32> bags_wotlk;

    uint32 ChooseBagTypeBySize(uint32 size);

    Player* bot;
};

#endif
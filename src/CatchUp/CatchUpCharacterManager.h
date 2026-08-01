#ifndef ALTBOT_SYNC_CATCHUPCHARACTERMANAGER_H
#define ALTBOT_SYNC_CATCHUPCHARACTERMANAGER_H

#include "Player.h"

class CatchUpCharacterManager
{
public:
    CatchUpCharacterManager(Player* bot);

    void GiveLevel(Player* player);
    void GiveAbilities();

private:
    Player* bot;
};

#endif
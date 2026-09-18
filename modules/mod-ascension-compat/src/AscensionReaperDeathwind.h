/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */

#ifndef ASCENSION_REAPER_DEATHWIND_H
#define ASCENSION_REAPER_DEATHWIND_H

class Player;
class SpellInfo;

void ApplyAscensionReaperDeathwindContracts(SpellInfo* spellInfo);
void HandleAscensionReaperEaterOfSouls(Player* player);
void AddSC_AscensionReaperDeathwind();

#endif

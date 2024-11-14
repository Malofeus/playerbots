
#include "playerbot/playerbot.h"
#include "FactionAction.h"
#include "Tools/Language.h"

using namespace ai;

bool FactionAction::Execute(Event& event)
{
    Player* requester = event.getOwner() ? event.getOwner() : GetMaster();
    std::string cmd = event.getParam();

    bool setWar = (cmd.find("+atwar") == 0);
    bool removeWar = (cmd.find("-atwar") == 0);

    std::string factionName;

    if ((setWar || removeWar) && cmd.size() > 6)
        factionName = cmd.substr(7);


    std::wstring wnamepart;

    if (!Utf8toWStr(factionName, wnamepart))
        return false;

    std::set<uint32> factionIds = ChatHelper::ExtractAllFactionIds(factionName);

    // converting string that we try to find to lower case
    wstrToLower(wnamepart);

    bool factionFound = false;
    std::map<std::string, std::string> args;

    for (uint32 id = 0; id < sFactionStore.GetNumRows(); ++id)
    {
#ifndef MANGOSBOT_ONE
        const FactionEntry* factionEntry = sFactionStore.LookupEntry(id);
#else
        const FactionEntry* factionEntry = sFactionStore.LookupEntry<FactionEntry>(id);
#endif

        if (!factionEntry)
            continue;

        FactionState const* repState = bot->GetReputationMgr().GetState(factionEntry);

        if (!repState)
            continue;

        if(!(repState->Flags & FACTION_FLAG_VISIBLE))
            continue;

        if(repState->Flags & FACTION_FLAG_HIDDEN)
            continue;

        if (repState->Flags & FACTION_FLAG_INVISIBLE_FORCED)
            continue;

        int loc = requester->GetSession()->GetSessionDbcLocale();

        if (!factionName.empty() && factionIds.empty())
        {
            std::string name = factionEntry->name[loc];

            if (name.empty())
                continue;

            if (!Utf8FitTo(name, wnamepart))
            {
                loc = 0;
                for (; loc < MAX_LOCALE; ++loc)
                {
                    if (loc == requester->GetSession()->GetSessionDbcLocale())
                        continue;

                    name = factionEntry->name[loc];
                    if (name.empty())
                        continue;

                    if (Utf8FitTo(name, wnamepart))
                        break;
                }
            }
        }

        if (factionName.empty() || factionIds.find(id) != factionIds.end() || (loc < MAX_LOCALE && factionIds.empty()))
        {
            ReputationMgr& mgr = bot->GetReputationMgr();

            if (setWar || removeWar)
            {
                args["%factionname"] = ChatHelper::formatFaction(id);

                if (!factionName.empty())
                {
                    if (removeWar && mgr.IsAtWar(id) && bot->GetReputationMgr().GetRank(factionEntry) < REP_HOSTILE)
                    {
                        ai->TellPlayerNoFacing(requester, BOT_TEXT2("Unable uncheck at war for %factionname because they hate me too much.", args));
                        return false;
                    }

                    if (repState->Flags & FACTION_FLAG_PEACE_FORCED)
                    {
                        ai->TellPlayerNoFacing(requester, BOT_TEXT2("Unable change at war for %factionname because peace is forced for this faction.", args));
                        return false;
                    }

                    if (removeWar && !mgr.IsAtWar(id))
                    {
                        ai->TellPlayerNoFacing(requester, BOT_TEXT2("I already have at war unchecked for %factionname.", args));
                        return false;
                    }

                    if (setWar && mgr.IsAtWar(id))
                    {
                        ai->TellPlayerNoFacing(requester, BOT_TEXT2("I already have at war checked for %factionname.", args));
                        return false;
                    }
                }
                
                bot->GetReputationMgr().SetAtWar(repState->ReputationListID, setWar);

                if(setWar)
                    ai->TellPlayerNoFacing(requester, BOT_TEXT2("Checked at war for %factionname", args));
                else
                    ai->TellPlayerNoFacing(requester, BOT_TEXT2("Unchecked at war for %factionname", args));

                return true;
            }

            ai->TellPlayerNoFacing(requester, ChatHelper::formatFaction(id, bot));

            factionFound = true;
        }
    }

    if (!factionFound)
    {
        if (factionName.empty())
        {
            if (setWar || removeWar)
                ai->TellPlayerNoFacing(requester, BOT_TEXT2("I do not have any factions where I can toggle at war.", args));
            else
                ai->TellPlayerNoFacing(requester, BOT_TEXT2("I do not have any visible factions.", args));
        }
        else
        {
            args["%factionname"] = factionName;
            ai->TellPlayerNoFacing(requester, BOT_TEXT2("I do not know %factionname.", args));
        }
        return false;
    }

    return true;
}

// 1076 1062 1061  972!!!!!  971
// TO-DO: Глазик против инвизов, рандомные бафы берсы/хасты и тд, ловушки

#include "ScriptPCH.h"
#include "Player.h"
#include "GameEventMgr.h"
#include "Group.h"
#include "ScriptMgr.h"

#define EVENT_DEATHMATCH_QUEUE 120  
#define EVENT_DEATHMATCH 121  
#define MIN_PLAYERS 10
#define MAX_PLAYERS 75

#define SPELL_QUEUE 58169



Position const StartPos[14] =
{
    { 746.492065f, -1725.065186f, 72.075821f, 0.392751f},
    { 754.386108f, -1624.970215f, 61.906303f, 5.530126f},
    { 789.427368f, -1786.345703f, 56.637093f, 5.688951f},
    { 922.640381f, -1779.072144f, 70.955536f, 0.402742f},
    { 1024.858765f, -1782.857178f, 81.509499f, 2.484484f},
    { 965.576355f, -1957.817993f, 67.764488f, 4.492572f},
    { 925.114685f, -2030.373901f, 69.223160f, 2.021492f},
    { 806.257385f, -1994.557739f, 55.892090f, 2.532001f},
    { 792.085327f, -1878.854858f, 66.285423f, 2.924308f},
    { 707.699890f, -1790.597900f, 56.103577f, 2.490376f},
    { 606.883667f, -1748.270142f, 49.379082f, 5.985442f},
    { 630.627258f, -1979.521362f, 78.508888f, 0.224112f},
    { 842.557861f, -2102.044189f, 63.814968f, 1.519235f},
    { 1045.860107f, -2078.038330f, 80.965233f, 2.384088f}
};

struct PlayerAbuse
{
    uint64 victimGUID;
    uint32 whenKilled;
};

std::map<uint64, PlayerAbuse> abuseList;


class starter_deathmatch : public CreatureScript  //900000
{
    public:
    starter_deathmatch() : CreatureScript("starter_deathmatch") { }

    bool OnGossipHello(Player* player, Creature* pCreature)
    {
        LocaleConstant loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
         if(sGameEventMgr->IsActiveEvent(EVENT_DEATHMATCH_QUEUE))
            if (!player->HasAura(SPELL_QUEUE))
                if (sWorld->GetCountQueueOnDM() <= MAX_PLAYERS)
                    if (player->getLevel() == 90)
                        if (!player->HasAura(99413))
                            player->ADD_GOSSIP_ITEM(5, sObjectMgr->GetTrinityString(30016, loc_idx), GOSSIP_SENDER_MAIN, 1);
                
        player->ADD_GOSSIP_ITEM(5, sObjectMgr->GetTrinityString(30017, loc_idx), GOSSIP_SENDER_MAIN, 2);
        player->ADD_GOSSIP_ITEM(5, sObjectMgr->GetTrinityString(30018, loc_idx), GOSSIP_SENDER_MAIN, 3);
        player->SEND_GOSSIP_MENU(100003, pCreature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        LocaleConstant loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
        player->PlayerTalkClass->ClearMenus();
        
        switch(action)
        {
            case 1:
                player->AddAura(SPELL_QUEUE, player); //registration aura
                sWorld->CountQueueOnDMPlus();
                if (!sGameEventMgr->IsActiveEvent(EVENT_DEATHMATCH))
                {
                    player->GetSession()->SendNotification(30012);
                    ChatHandler(player->GetSession()).PSendSysMessage(30012);
                    ChatHandler(player->GetSession()).PSendSysMessage("%u members left before the start of the deathmatch", (10-(sWorld->GetCountQueueOnDM())));
                }
                player->CLOSE_GOSSIP_MENU(); 
                break;
            
            case 2:
                {
                    std::stringstream str1;
                    QueryResult result = CharacterDatabase.PQuery("SELECT * FROM custom_character_deathmatch WHERE guid = '%u';", player->GetGUID()); 
                    if (result)
                    {
                        Field* fields = result->Fetch();
                        uint32 kills = fields[1].GetUInt32();
                        uint32 deads = fields[2].GetUInt32();
                        uint32 count = fields[3].GetUInt32();
                        
                        str1 << "You have " << kills << " kills for " << deads << " deads and for " << count << "games" ;
                    }
                    else
                        str1 << "You have 0 kills for 0 deads and for 0 games" ;
                    
                    player->ADD_GOSSIP_ITEM(5, str1.str(), GOSSIP_SENDER_MAIN, 4);
                    player->ADD_GOSSIP_ITEM(5, sObjectMgr->GetTrinityString(30019, loc_idx), GOSSIP_SENDER_MAIN, 4);
                    player->SEND_GOSSIP_MENU(100003, creature->GetGUID());
                }
                break;
            
            case 4:
                if(sGameEventMgr->IsActiveEvent(EVENT_DEATHMATCH_QUEUE))
                    if (!player->HasAura(SPELL_QUEUE))
                        if (sWorld->GetCountQueueOnDM() <= MAX_PLAYERS)
                            if (player->getLevel() == 90)
                                if (!player->HasAura(99413))
                                    player->ADD_GOSSIP_ITEM(5, sObjectMgr->GetTrinityString(30016, loc_idx), GOSSIP_SENDER_MAIN, 1);
                        
                player->ADD_GOSSIP_ITEM(5, sObjectMgr->GetTrinityString(30017, loc_idx), GOSSIP_SENDER_MAIN, 2);
                player->ADD_GOSSIP_ITEM(5, sObjectMgr->GetTrinityString(30018, loc_idx), GOSSIP_SENDER_MAIN, 3);
                player->SEND_GOSSIP_MENU(100003, creature->GetGUID());
                break;
            
            default:            
                player->CLOSE_GOSSIP_MENU(); 
                break;
            }
        return true;
    }
};

class deathmatch_player_script : public PlayerScript
{
    public:
        deathmatch_player_script() : PlayerScript("deathmatch_player_script") { }

    uint32 rand;
    
    void OnLogin(Player* player)
    {    
        if (!player)
            return;
                
        if (player->HasAura(SPELL_QUEUE))
            player->RemoveAura(SPELL_QUEUE); 
        
        rand = urand(0, 13);
        
        new custom_deathmatch_check(player);
    }
    
    void OnLogout(Player* player)
    {
        if (!player)
            return;
        
        if (player->HasAura(SPELL_QUEUE))
        {
            sWorld->CountQueueOnDMMinus();
            player->RemoveAura(SPELL_QUEUE);
        }
    }
    
    void OnPVPKill(Player* killer, Player* victim)
    {
        if (killer->GetGUID() == victim->GetGUID())
            return;
        
        if (killer->GetMapId() == 972 && victim->GetMapId() == 972)
        {            
            if (!abuseList.empty())
            {
                for (std::map<uint64, PlayerAbuse>::const_iterator itr = abuseList.begin(); itr != abuseList.end(); ++itr)
                {
                    if (itr->first == killer->GetGUID() && itr->second.victimGUID == victim->GetGUID()) // Initial check
                    {
                        if (GetMSTimeDiffToNow(itr->second.whenKilled) < 120000)
                        {  
                            // The player won't be able to kill the same player for another 1 minute
                            ChatHandler(killer->GetSession()).PSendSysMessage("You cannot kill this player for another %u second", CalculateTimeInSeconds(GetMSTimeDiffToNow(itr->second.whenKilled)));
                            return;
                        }
                        else
                            abuseList.erase(killer->GetGUID());
                    }
                }
            }
            // Adding the killer/victimGUID to the abuse list
            abuseList[killer->GetGUID()].victimGUID = victim->GetGUID();
            abuseList[killer->GetGUID()].whenKilled = getMSTime();
            
            Map::PlayerList const& players = killer->GetMap()->GetPlayers();  // announce
            
            killer->AddAura(46392, killer);

            for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
            {
                if (Player* player = itr->getSource())
                   ChatHandler(player->GetSession()).PSendSysMessage(30014, killer->GetName(), victim->GetName());
            }
            
            QueryResult result = CharacterDatabase.PQuery("SELECT * FROM custom_character_deathmatch WHERE guid = '%u';", killer->GetGUID()); 
            if (result)
                CharacterDatabase.PExecute("UPDATE custom_character_deathmatch set `kills` = kills+1 where guid = '%u';", killer->GetGUID());
            else
                CharacterDatabase.PExecute("insert into custom_character_deathmatch values (%u, 1, 0, 1);", killer->GetGUID());
            
            QueryResult resultv = CharacterDatabase.PQuery("SELECT * FROM custom_character_deathmatch WHERE guid = '%u';", victim->GetGUID()); 
            if (resultv)
                CharacterDatabase.PExecute("UPDATE custom_character_deathmatch set `deads` = deads+1 where guid = '%u';", victim->GetGUID());
            else
                CharacterDatabase.PExecute("insert into custom_character_deathmatch values (%u, 0, 1, 1);", victim->GetGUID());
            
            // ORDER BY kills DESC LIMIT 3
            // GetPlayerNameByGUID
        }
    }
    
    uint32 CalculateTimeInSeconds(uint32 m_time)
    {
        m_time = (m_time / 1000);
        return m_time;
    }
    
    void OnPlayerDeath(Player* deadPlayer)
    {
        if (!deadPlayer || !deadPlayer->IsInWorld() || deadPlayer->GetMapId() != 972)
            return;
        
        rand = urand(0, 13);
        
        deadPlayer->AddAura(58729, deadPlayer);
        deadPlayer->TeleportTo(972, StartPos[rand].GetPositionX(), StartPos[rand].GetPositionY(), StartPos[rand].GetPositionZ(), StartPos[rand].GetOrientation());
        deadPlayer->RemoveAllAurasOnDeath();
        deadPlayer->ResurrectPlayer(1.0f);
        deadPlayer->SpawnCorpseBones();
        deadPlayer->SaveToDB();
        
    }
    
    class custom_deathmatch_check : public BasicEvent
    {
        public:
        custom_deathmatch_check(Player* player) : player(player) 
        {
            player->m_Events.AddEvent(this, player->m_Events.CalculateTime(1000));
        }

        bool Execute(uint64 /*time*/, uint32 /*diff*/)
        {        
            player->RemoveAura(58729);
            
            if(sGameEventMgr->IsActiveEvent(EVENT_DEATHMATCH))
            {
                if (player && player->HasAura(SPELL_QUEUE) && player->GetMapId() != 972 && !player->GetBattleground())
                {                 
                    rand = urand(0, 13);
                    player->TeleportTo(972, StartPos[rand].GetPositionX(), StartPos[rand].GetPositionY(), StartPos[rand].GetPositionZ(), StartPos[rand].GetOrientation());
                    player->GetSession()->SendNotification(30013);
                    ChatHandler(player->GetSession()).PSendSysMessage(30013);
                    if (Group* group = player->GetGroup()) // Remove the player from the group
                        group->RemoveMember(player->GetGUID());
                    player->RemoveAllAurasOnDeath(); //нужен дебаф, дабы не настакивались перед входом
                    
                    QueryResult result = CharacterDatabase.PQuery("SELECT * FROM custom_character_deathmatch WHERE guid = '%u';", player->GetGUID()); 
                    if (result)
                        CharacterDatabase.PExecute("UPDATE custom_character_deathmatch set `countmatch` = countmatch+1 where guid = '%u';", player->GetGUID());
                    else
                        CharacterDatabase.PExecute("insert into custom_character_deathmatch values (%u, 0, 0, 1);", player->GetGUID());
            
                }
                
                if (player && !player->HasAura(SPELL_QUEUE) && player->GetMapId() == 972) //бафа нет но на деатматч карте
                {
                    if (player->GetTeam() == HORDE)
                        player->TeleportTo(1, 1573.98f, -4401.63f, 15.78f, 0.84f);  //orgri
                    else
                        player->TeleportTo(0, -8833.62f, 620.64f, 93.73f, 1.12f);  //storm
                }
            }
            else
            {
                if (player->GetMapId() == 972)
                {
                    player->RemoveAura(46392);
                    if (player->GetTeam() == HORDE)
                        player->TeleportTo(1, 1573.98f, -4401.63f, 15.78f, 0.84f);  //orgri
                    else
                        player->TeleportTo(0, -8833.62f, 620.64f, 93.73f, 1.12f);  //storm
                }
                if(!sGameEventMgr->IsActiveEvent(EVENT_DEATHMATCH_QUEUE))
                    player->RemoveAura(SPELL_QUEUE);
            }
            
            player->m_Events.AddEvent(this, player->m_Events.CalculateTime(1000));
            return false; //event will not deleted
        }

        
        Player* player;
        uint32 rand;
    };
};

//58169
class spell_deathmatch_checker : public SpellScriptLoader
{
    public:
        spell_deathmatch_checker() : SpellScriptLoader("spell_deathmatch_checker") {}

        class spell_deathmatch_checker_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_deathmatch_checker_AuraScript);

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                if (!target)
                    return;
                
                if (target->GetTypeId() == TYPEID_PLAYER)
                {
                    if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_CANCEL)  // если сам снял - выкидываем из счетчика
                        if(sGameEventMgr->IsActiveEvent(EVENT_DEATHMATCH_QUEUE))
                            sWorld->CountQueueOnDMMinus();    
                    
                    if (target->GetMapId() == 972)
                    {
                        if (target->ToPlayer()->GetTeam() == HORDE)
                            target->ToPlayer()->TeleportTo(1, 1573.98f, -4401.63f, 15.78f, 0.84f);  //orgri
                        else
                            target->ToPlayer()->TeleportTo(0, -8833.62f, 620.64f, 93.73f, 1.12f);  //storm
                        
                        target->AddAura(99413, target);
                    }
                    else
                    {
                        target->ToPlayer()->GetSession()->SendNotification(30015);
                        ChatHandler(target->ToPlayer()->GetSession()).PSendSysMessage(30015);
                    }
                }                    
            }

            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_deathmatch_checker_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_deathmatch_checker_AuraScript();
        }
};

class deathmatch_event_checker : public WorldScript
{
public:
    deathmatch_event_checker() : WorldScript("deathmatch_event_checker") 
    { 
        timerforannounce = 10000;
    }
    uint32 timerforannounce;

    void OnUpdate(uint32 diff)
    {
       // TC_LOG_ERROR("server", "%u", sWorld->GetCountQueueOnDM());
        
        if(!sGameEventMgr->IsActiveEvent(120) || sWorld->GetCountQueueOnDM() < 0)
            sWorld->CountQueueOnDMNull();
        if (sGameEventMgr->IsActiveEvent(120))
        {
            if (timerforannounce <= diff)
            {   
                std::string name("Организатор Deathmatch`а");
                std::string text("Деатматч уже начался! Спешите принять участие, попасть в `топ-3` и получить великолепные призы! Зарегистрироваться можно у меня в столицах!");
                sWorld->SendWorldText(LANG_ANNOUNCE_COLOR, name.c_str(), text.c_str());
                timerforannounce = urand(1100000, 1200000);
            }
            else
                timerforannounce -= diff;
        }
        
        if(sGameEventMgr->IsActiveEvent(EVENT_DEATHMATCH))  //если детматч идет, но стало мало игроков/истекло время - заканчиваем
        {
            if (sWorld->GetCountQueueOnDM() < MIN_PLAYERS || !sGameEventMgr->IsActiveEvent(EVENT_DEATHMATCH_QUEUE))
                sGameEventMgr->StopEvent(EVENT_DEATHMATCH);
            return;
        }
        
        if (sWorld->GetCountQueueOnDM() >= MIN_PLAYERS && sGameEventMgr->IsActiveEvent(EVENT_DEATHMATCH_QUEUE)) //если игроков хватает и подходящее время - начинаем
            sGameEventMgr->StartEvent(EVENT_DEATHMATCH); 

    }
};

void AddSC_custom_deathmatch()
{
    new starter_deathmatch();
    new deathmatch_player_script();
    new spell_deathmatch_checker();
    new deathmatch_event_checker();
}
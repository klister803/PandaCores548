/*
    Dungeon : Template of the Jade Serpent 85-87
    Instance General Script
    Jade servers
*/

#include "ScriptMgr.h"
#include "InstanceScript.h"

enum eSpells
{
    SPELL_CORRUPTED_WATERS      = 115167,
};

class instance_temple_of_jade_serpent : public InstanceMapScript
{
public:
    instance_temple_of_jade_serpent() : InstanceMapScript("instance_temple_of_jade_serpent", 690) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_temple_of_jade_serpent_InstanceMapScript(map);
    }

    struct instance_temple_of_jade_serpent_InstanceMapScript : public InstanceScript
    {
        Position roomCenter;
        uint32 waterDamageTimer;
        instance_temple_of_jade_serpent_InstanceMapScript(Map* map) : InstanceScript(map)
        {
            roomCenter.m_positionX = 1046.941f;
            roomCenter.m_positionY = -2560.606f;
            roomCenter.m_positionZ = 174.9552f;
            roomCenter.m_orientation = 4.33f;
            waterDamageTimer = 250;
        }

        void Initialize()
        {
        }

        void OnGameObjectCreate(GameObject* go)
        {
        }

         virtual void Update(uint32 diff) 
         {
             if (waterDamageTimer <= diff)
             {
                 // Handle damage of water in wise mari combat
                 // Blizz handle that case with trigger and aura cast every 250 ms, anyway it's work
                 Map::PlayerList const &PlayerList = instance->GetPlayers();

                 if (!PlayerList.isEmpty())
                 {
                     for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                     {
                         Player* plr = i->getSource();
                         if( !plr)
                             continue;

                         // position : center of the wise mari's room
                         Position pos;
                         plr->GetPosition(&pos);

                         if ((plr->GetDistance(roomCenter) < 20.00f && roomCenter.HasInArc(M_PI, &pos)) 
                             || (!roomCenter.HasInArc(M_PI, &pos) && plr->GetDistance(roomCenter) < 14.00f))
                         {
                             if (plr->GetPositionZ() > 174.05f && plr->GetPositionZ() < 174.23f)
                                 plr->CastSpell(plr, SPELL_CORRUPTED_WATERS, true);
                         }

                         if (plr->GetDistance(roomCenter) < 30.00f && plr->GetPositionZ() > 170.19f && plr->GetPositionZ() < 170.215f)
                             plr->CastSpell(plr, SPELL_CORRUPTED_WATERS, true);
                     }
                 }
                 waterDamageTimer = 250;
             } else waterDamageTimer -= diff;
         }

         void SetData(uint32 type, uint32 data)
         {

         }

         uint32 GetData(uint32 type)
         {
            return 0;
         }
    };

};

void AddSC_instance_temple_of_jade_serpent()
{
    new instance_temple_of_jade_serpent();
}

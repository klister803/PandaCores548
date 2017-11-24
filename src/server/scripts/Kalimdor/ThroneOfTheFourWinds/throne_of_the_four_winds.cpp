#include "ScriptPCH.h"
#include "throne_of_the_four_winds.h"

enum SlipstreamEnums
{
    // Land Positions
    DIR_WEST_TO_SOUTH,
    DIR_SOUTH_TO_WEST,
    DIR_NORTH_TO_WEST,
    DIR_WEST_TO_NORTH,
    DIR_EAST_TO_NORTH,
    DIR_NORTH_TO_EAST,
    DIR_SOUTH_TO_EAST,
    DIR_EAST_TO_SOUTH,
    DIR_ERROR,

    // Spells
    SPELL_SLIPSTREAM_BUFF                   = 87740,
    SPELL_SLIPSTREAM_PLAYER_VISUAL          = 85063,
    SPELL_SLEET_STORM_ULTIMATE              = 84644,

    EVENT_SEARCH_PLAYERS                    = 1,
};

Position const SlipstreamPositions[8] =
{
    {-245.141129f,  861.474060f,    197.386398f,    0},
    {-92.116440f,   1010.796448f,   197.475754f,    0},
    {-5.322055f,    1010.573608f,   197.520096f,    0},
    {144.480469f,   857.187927f,    197.594208f,    0},
    {144.221481f,   770.720154f,    197.629150f,    0},
    {-9.268366f,    620.736328f,    197.567032f,    0},
    {-96.089645f,   621.198730f,    197.499115f,    0},
    {-245.653870f,  774.446472f,    197.507156f,    0},
};

class SlipStreamFilter
{
public:
    bool operator()(WorldObject* unit)
    {
        if (!unit->ToPlayer())
            return true;

        if (unit->ToPlayer()->HasAura(SPELL_SLIPSTREAM_BUFF))
            return true;

        return false;
    }
};

//47066
class npc_slipstream_raid : public CreatureScript
{
public:
    npc_slipstream_raid() : CreatureScript("npc_slipstream_raid") { }

    struct npc_slipstream_raidAI : public ScriptedAI
    {
        npc_slipstream_raidAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
            me->SetReactState(REACT_PASSIVE);
            me->SetDisplayId(11686);
        }

        EventMap events;
        uint8 SlipstreamPosition;

        void Reset()
        {
            SlipstreamPosition = 8;

            for (uint8 i = 0; i <= 7; i++)
            {
                if (me->GetDistance2d(SlipstreamPositions[i].GetPositionX(), SlipstreamPositions[i].GetPositionY()) < 10)
                {
                    SlipstreamPosition = i;
                    break;
                }
            }

            if (SlipstreamPosition >= DIR_ERROR)
                return;

            SlipstreamPosition += (SlipstreamPosition == DIR_WEST_TO_SOUTH || SlipstreamPosition == DIR_NORTH_TO_WEST ||
                SlipstreamPosition == DIR_EAST_TO_NORTH || SlipstreamPosition == DIR_SOUTH_TO_EAST) ? 1 : -1;

            events.ScheduleEvent(EVENT_SEARCH_PLAYERS, 750);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_SEARCH_PLAYERS)
                {
                    std::list<Player*>pllist;
                    pllist.clear();
                    GetPlayerListInGrid(pllist, me, 10.0f);
                    if (!pllist.empty())
                    {
                        pllist.remove_if(SlipStreamFilter());
                        if (!pllist.empty())
                        {
                            for (std::list<Player*>::const_iterator Itr = pllist.begin(); Itr != pllist.end(); ++Itr)
                            {
                                me->AddAura(SPELL_SLIPSTREAM_BUFF, *Itr);
                                me->AddAura(SPELL_SLIPSTREAM_PLAYER_VISUAL, *Itr);

                                if ((*Itr)->GetOrientation() != SlipstreamPositions[SlipstreamPosition].GetOrientation())
                                    (*Itr)->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TURNING);

                                me->GetMap()->PlayerRelocation((*Itr), SlipstreamPositions[SlipstreamPosition].GetPositionX(), SlipstreamPositions[SlipstreamPosition].GetPositionY(), SlipstreamPositions[SlipstreamPosition].GetPositionZ(), SlipstreamPositions[SlipstreamPosition].GetOrientation());
                                (*Itr)->GetMotionMaster()->MoveJump(SlipstreamPositions[SlipstreamPosition].GetPositionX(), SlipstreamPositions[SlipstreamPosition].GetPositionY(), 198.458481f, 1, 6);
                            }
                        }
                    }
                    events.ScheduleEvent(EVENT_SEARCH_PLAYERS, 750);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_slipstream_raidAI(creature);
    }
};


void AddSC_throne_of_the_four_winds()
{
    new npc_slipstream_raid();
}
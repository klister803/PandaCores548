/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "pursuing_the_black_harvest.h"
#include "ScriptedCreature.h"

enum Texts
{ };

enum Spells
{ };

enum Events
{
    EVENT_NONE,
};

enum Actions
{
    ACTION_NONE,

};

enum Sounds
{ };

//Area Trigger Id: 8696
class at_stage_1: public AreaTriggerScript
{
public:
    at_stage_1() : AreaTriggerScript("at_stage_1") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*at*/, bool enter)
    {
        if (enter)
        {
            // как добавить ебаный прогресс? шаг должен смениться =/

            //ServerToClient: SMSG_CRITERIA_UPDATE (0x1904) Length: 37 ConnIdx: 0 Time: 01/29/2015 08:42:29.000 Number: 3133
            //Id: 22336
            //Quantity: 1
            //Guid: Full: 0x08190800000000000000000007BFF135 Player/0 R1602/S0 Map: 0 Low: 130019637
            //Flags: 0
            //Date: 01/29/2015 08:42:00
            //TimeFromStart: 01/01/1970 00:00:00
            //TimeFromCreate: 01/01/1970 00:00:00

            //ServerToClient: SMSG_SCENARIO_PROGRESS_UPDATE (0x0324) Length: 37 ConnIdx: 0 Time: 01/29/2015 08:42:29.000 Number: 3134
            //(Progress) Id: 22336
            //(Progress) Quantity: 1
            //(Progress) Player: Full: 0x40313C8B00003200002935000049F152 Scenario/0 R3151/S10549 Map: 1112 Low: 4845906
            //(Progress) Date: 01/29/2015 09:42:00
            //(Progress) TimeFromStart: 01/01/1970 00:00:00
            //(Progress) TimeFromCreate: 01/01/1970 00:00:00
            //(Progress) Flags: 0

            //ServerToClient: SMSG_SCENARIO_STATE (0x152D) Length: 33 ConnIdx: 0 Time: 01/29/2015 08:42:29.000 Number: 3135
            //ScenarioID: 200
            //CurrentStep: 1
            //DifficultyID: 0
            //WaveCurrent: 0
            //WaveMax: 0
            //TimerDuration: 0
            //CriteriaProgressCount: 0
            //BonusObjectiveDataCount: 0
            //ScenarioComplete: False

            //ClientToServer: CMSG_QUERY_SCENARIO_POI (0x09A1) Length: 12 ConnIdx: 0 Time: 01/29/2015 08:42:29.000 Number: 3136
            //MissingScenarioPOITreeCount: 2
            //[0] MissingScenarioPOITreeIDs: 31011
            //[1] MissingScenarioPOITreeIDs: 32958

            //ServerToClient: SMSG_CRITERIA_UPDATE (0x1904) Length: 37 ConnIdx: 0 Time: 01/29/2015 08:42:29.000 Number: 3137
            //Id: 21594
            //Quantity: 0
            //Guid: Full: 0x08190800000000000000000007BFF135 Player/0 R1602/S0 Map: 0 Low: 130019637
            //Flags: 0
            //Date: 01/29/2015 08:42:00
            //TimeFromStart: 01/01/1970 00:00:00
            //TimeFromCreate: 01/01/1970 00:00:00

            //ServerToClient: SMSG_CRITERIA_UPDATE (0x1904) Length: 37 ConnIdx: 0 Time: 01/29/2015 08:42:29.000 Number: 3138
            //Id: 23348
            //Quantity: 0
            //Guid: Full: 0x08190800000000000000000007BFF135 Player/0 R1602/S0 Map: 0 Low: 130019637
            //Flags: 0
            //Date: 01/29/2015 08:42:00
            //TimeFromStart: 01/01/1970 00:00:00
            //TimeFromCreate: 01/01/1970 00:00:00

            //ServerToClient: SMSG_CRITERIA_UPDATE (0x1904) Length: 37 ConnIdx: 0 Time: 01/29/2015 08:42:29.000 Number: 3139
            //Id: 23724
            //Quantity: 0
            //Guid: Full: 0x08190800000000000000000007BFF135 Player/0 R1602/S0 Map: 0 Low: 130019637
            //Flags: 0
            //Date: 01/29/2015 08:42:00
            //TimeFromStart: 01/01/1970 00:00:00
            //TimeFromCreate: 01/01/1970 00:00:00

            return true;
        }
        return false;
    }
};

void AddSC_pursing_the_black_harvest()
{
    new at_stage_1();
}

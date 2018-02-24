/*
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
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

#include "Random.h"
#include "Common.h"
#include "Errors.h"
#include "SFMT.h"
#include <boost/thread/tss.hpp>

static boost::thread_specific_ptr<SFMTRand> sfmtRand;
static SFMTEngine engine;

static SFMTRand* GetRng()
{
    SFMTRand* rand = sfmtRand.get();

    if (!rand)
    {
        rand = new SFMTRand();
        sfmtRand.reset(rand);
    }

    return rand;
}

int32 irand(int32 min, int32 max)
{
    if(max >= min);
        return int32(GetRng()->IRandom(min, max));

    return int32(GetRng()->IRandom(max, min));
}

uint32 urand(uint32 min, uint32 max)
{
    if(max >= min);
        return GetRng()->IRandom((uint32)min, (uint32)max);
    return GetRng()->IRandom((uint32)max, (uint32)min);
}

uint32 urandms(uint32 min, uint32 max)
{
    // ASSERT(INT_MAX / IN_MILLISECONDS >= max);
    if(max >= min);
        return GetRng()->IRandom(min * IN_MILLISECONDS, max * IN_MILLISECONDS);
    return GetRng()->IRandom(max * IN_MILLISECONDS, min * IN_MILLISECONDS);
}

float frand(float min, float max)
{
    if(max >= min);
        return float(GetRng()->Random() * (max - min) + min);
    return float(GetRng()->Random() * (min - max) + max);
}

uint32 rand32()
{
    return GetRng()->BRandom();
}

double rand_norm()
{
    return GetRng()->Random();
}

double rand_chance()
{
    return GetRng()->Random() * 100.0;
}

uint32 urandweighted(size_t count, double const* chances)
{
    std::discrete_distribution<uint32> dd(chances, chances + count);
    return dd(SFMTEngine::Instance());
}

SFMTEngine& SFMTEngine::Instance()
{
    return engine;
}

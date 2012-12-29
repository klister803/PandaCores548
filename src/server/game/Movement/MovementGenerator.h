/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#ifndef TRINITY_MOVEMENTGENERATOR_H
#define TRINITY_MOVEMENTGENERATOR_H

#include "Define.h"
#include <ace/Singleton.h>
#include "ObjectRegistry.h"
#include "FactoryHolder.h"
#include "Common.h"
#include "MotionMaster.h"

class Unit;

class MovementGenerator : public std::enable_shared_from_this<MovementGenerator>
{
    public:
        virtual ~MovementGenerator();

        virtual void Initialize(UnitPtr) = 0;
        virtual void Finalize(UnitPtr) = 0;

        virtual void Reset(UnitPtr) = 0;

        virtual bool Update(UnitPtr, const uint32& time_diff) = 0;

        virtual MovementGeneratorType GetMovementGeneratorType() = 0;

        virtual void unitSpeedChanged() { }
};

template<class T, class D>
class MovementGeneratorMedium : public MovementGenerator
{
    public:
        void Initialize(UnitPtr u)
        {
            //u->AssertIsType<T>();
            (static_cast<D*>(this))->Initialize(CAST(T,u));
        }
        void Finalize(UnitPtr u)
        {
            //u->AssertIsType<T>();
            (static_cast<D*>(this))->Finalize(CAST(T,u));
        }
        void Reset(UnitPtr u)
        {
            //u->AssertIsType<T>();
            (static_cast<D*>(this))->Reset(CAST(T,u));
        }
        bool Update(UnitPtr u, const uint32& time_diff)
        {
            //u->AssertIsType<T>();
            return (static_cast<D*>(this))->Update(CAST(T,u), time_diff);
        }
    public:
        // will not link if not overridden in the generators
        void Initialize(std::shared_ptr<T> u);
        void Finalize(std::shared_ptr<T> u);
        void Reset(std::shared_ptr<T> u);
        bool Update(std::shared_ptr<T> u, const uint32& time_diff);
};

struct SelectableMovement : public FactoryHolder<MovementGenerator, MovementGeneratorType>
{
    SelectableMovement(MovementGeneratorType mgt) : FactoryHolder<MovementGenerator, MovementGeneratorType>(mgt) {}
};

template<class REAL_MOVEMENT>
struct MovementGeneratorFactory : public SelectableMovement
{
    MovementGeneratorFactory(MovementGeneratorType mgt) : SelectableMovement(mgt) {}

    MovementGenerator* Create(void *) const;
};

typedef FactoryHolder<MovementGenerator, MovementGeneratorType> MovementGeneratorCreator;
typedef FactoryHolder<MovementGenerator, MovementGeneratorType>::FactoryHolderRegistry MovementGeneratorRegistry;
typedef FactoryHolder<MovementGenerator, MovementGeneratorType>::FactoryHolderRepository MovementGeneratorRepository;
#endif


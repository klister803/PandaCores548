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

#ifndef _REFMANAGER_H
#define _REFMANAGER_H
//=====================================================

#include "Dynamic/LinkedList.h"
#include "Dynamic/LinkedReference/Reference.h"

template <class TO, class FROM> class RefManager : public LinkedListHead
{
    public:
        typedef LinkedListHead::Iterator< Reference<TO, FROM> > iterator;
        RefManager() { }
        virtual ~RefManager() { clearReferences(); }

        std::shared_ptr<Reference<TO, FROM>> getFirst() { return std::static_pointer_cast<Reference<TO,FROM>>(LinkedListHead::getFirst()); }
        std::shared_ptr<const Reference<TO, FROM>> getFirst() const { return std::static_pointer_cast<const Reference<TO,FROM>>(LinkedListHead::getFirst()); }
        std::shared_ptr<Reference<TO, FROM>> getLast() { return std::static_pointer_cast<Reference<TO,FROM>>(LinkedListHead::getLast()); }
        std::shared_ptr<const Reference<TO, FROM>> getLast() const { return std::static_pointer_cast<const Reference<TO,FROM>>(LinkedListHead::getLast()); }

        iterator begin() { return iterator(getFirst()); }
        iterator end() { return iterator(nullptr); }
        iterator rbegin() { return iterator(getLast()); }
        iterator rend() { return iterator(nullptr); }

        void clearReferences()
        {
            std::shared_ptr<LinkedListElement> ref;
            while ((ref = getFirst()) != nullptr)
            {
                std::static_pointer_cast<Reference<TO,FROM>>(ref)->invalidate();
                ref->delink();                              // the delink might be already done by invalidate(), but doing it here again does not hurt and insures an empty list
            }
        }
};

//=====================================================
#endif


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

#ifndef _LINKEDLIST
#define _LINKEDLIST

#include "Define.h"
#include <iterator>

//============================================
class LinkedListHead;

class LinkedListElement : public std::enable_shared_from_this<LinkedListElement>
{
    private:
        friend class LinkedListHead;

        std::shared_ptr<LinkedListElement> iNext;
        std::shared_ptr<LinkedListElement> iPrev;
    public:
        LinkedListElement(): iNext(nullptr), iPrev(nullptr) {}
        ~LinkedListElement() { delink(); }

        bool hasNext() const { return(iNext && iNext->iNext != nullptr); }
        bool hasPrev() const { return(iPrev && iPrev->iPrev != nullptr); }
        bool isInList() const { return(iNext != nullptr && iPrev != nullptr); }

        std::shared_ptr<LinkedListElement> next()       { return hasNext() ? iNext : nullptr; }
        std::shared_ptr<const LinkedListElement> next() const { return hasNext() ? iNext : nullptr; }
        std::shared_ptr<LinkedListElement> prev()       { return hasPrev() ? iPrev : nullptr; }
        std::shared_ptr<const LinkedListElement> prev() const { return hasPrev() ? iPrev : nullptr; }

        std::shared_ptr<LinkedListElement> nocheck_next()       { return iNext; }
        std::shared_ptr<const LinkedListElement> nocheck_next() const { return iNext; }
        std::shared_ptr<LinkedListElement> nocheck_prev()       { return iPrev; }
        std::shared_ptr<const LinkedListElement> nocheck_prev() const { return iPrev; }

        void delink()
        {
            if (isInList())
            {
                iNext->iPrev = iPrev; iPrev->iNext = iNext; iNext = nullptr; iPrev = nullptr;
            }
        }

        void insertBefore(std::shared_ptr<LinkedListElement> pElem)
        {
            pElem->iNext = shared_from_this();
            pElem->iPrev = iPrev;
            iPrev->iNext = pElem;
            iPrev = pElem;
        }

        void insertAfter(std::shared_ptr<LinkedListElement> pElem)
        {
            pElem->iPrev = shared_from_this();
            pElem->iNext = iNext;
            iNext->iPrev = pElem;
            iNext = pElem;
        }
};

//============================================

class LinkedListHead
{
    private:
        std::shared_ptr<LinkedListElement> iFirst;
        std::shared_ptr<LinkedListElement> iLast;
        uint32 iSize;
    public:
        LinkedListHead(): iSize(0)
        {
            // create empty list

            iFirst->iNext = iLast;
            iLast->iPrev = iFirst;
        }

        bool isEmpty() const { return(!iFirst->iNext->isInList()); }

        std::shared_ptr<LinkedListElement> getFirst()       { return(isEmpty() ? nullptr : iFirst->iNext); }
        std::shared_ptr<const LinkedListElement> getFirst() const { return(isEmpty() ? nullptr : iFirst->iNext); }

        std::shared_ptr<LinkedListElement> getLast() { return(isEmpty() ? nullptr : iLast->iPrev); }
        std::shared_ptr<const LinkedListElement> getLast() const  { return(isEmpty() ? nullptr : iLast->iPrev); }

        void insertFirst(std::shared_ptr<LinkedListElement> pElem)
        {
            iFirst->insertAfter(pElem);
        }

        void insertLast(std::shared_ptr<LinkedListElement> pElem)
        {
            iLast->insertBefore(pElem);
        }

        uint32 getSize() const
        {
            if (!iSize)
            {
                uint32 result = 0;
                std::shared_ptr<const LinkedListElement> e = getFirst();
                while (e)
                {
                    ++result;
                    e = e->next();
                }
                return result;
            }
            else
                return iSize;
        }

        void incSize() { ++iSize; }
        void decSize() { --iSize; }

        template<class _Ty>
            class Iterator
        {
            public:
                typedef std::bidirectional_iterator_tag     iterator_category;
                typedef _Ty                                 value_type;
                typedef ptrdiff_t                           difference_type;
                typedef ptrdiff_t                           distance_type;
                typedef _Ty*                                pointer;
                typedef _Ty const*                          const_pointer;
                typedef _Ty&                                reference;
                typedef _Ty const &                         const_reference;
                typedef std::shared_ptr<_Ty>                value_ptr;
                typedef std::shared_ptr<const _Ty>          const_value_ptr;

                Iterator() : _Ptr(0)
                {                                           // construct with null node pointer
                }

                Iterator(pointer _Pnode) : _Ptr(_Pnode)
                {                                           // construct with node pointer _Pnode
                }

                Iterator(value_ptr _Pnode) : _Ptr(_Pnode)
                {                                           // construct with node pointer _Pnode
                }

                Iterator& operator=(Iterator const &_Right)
                {
                    _Ptr = _Right._Ptr;
                    return *this;
                }

                Iterator& operator=(const_value_ptr const &_Right)
                {
                    _Ptr = NO_CONST(_Ty,_Right);
                    return *this;
                }

                reference operator*()
                {                                           // return designated value
                    return *_Ptr;
                }

                value_ptr operator->()
                {                                           // return pointer to class object
                    return _Ptr;
                }

                Iterator& operator++()
                {                                           // preincrement
                    _Ptr = _Ptr->next();
                    return (*this);
                }

                Iterator operator++(int)
                {                                           // postincrement
                    iterator _Tmp = *this;
                    ++*this;
                    return (_Tmp);
                }

                Iterator& operator--()
                {                                           // predecrement
                    _Ptr = _Ptr->prev();
                    return (*this);
                }

                Iterator operator--(int)
                {                                           // postdecrement
                    iterator _Tmp = *this;
                    --*this;
                    return (_Tmp);
                }

                bool operator==(Iterator const &_Right) const
                {                                           // test for iterator equality
                    return (_Ptr == _Right._Ptr);
                }

                bool operator!=(Iterator const &_Right) const
                {                                           // test for iterator inequality
                    return (!(*this == _Right));
                }

                bool operator==(pointer const &_Right) const
                {                                           // test for pointer equality
                    return (_Ptr != _Right);
                }

                bool operator!=(pointer const &_Right) const
                {                                           // test for pointer equality
                    return (!(*this == _Right));
                }

                bool operator==(reference _Right) const
                {                                           // test for reference equality
                    return (_Ptr.get() == &_Right);
                }

                bool operator!=(reference _Right) const
                {                                           // test for reference equality
                    return (_Ptr.get() != &_Right);
                }

                bool operator==(const_value_ptr _Right) const
                {                                           // test for shared_ptr equality
                    return (_Ptr == _Right);
                }

                bool operator!=(const_value_ptr _Right) const
                {                                           // test for shared_ptr equality
                    return (_Ptr != _Right);
                }

                value_ptr _Mynode()
                {                                           // return node pointer
                    return (_Ptr);
                }

            protected:
                value_ptr _Ptr;                               // pointer to node
        };

        typedef Iterator<LinkedListElement> iterator;
};

//============================================
#endif
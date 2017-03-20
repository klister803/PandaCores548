/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "MovementPacketBuilder.h"
#include "MoveSpline.h"
#include "WorldPacket.h"
#include "Object.h"

namespace Movement
{
    inline void operator << (ByteBuffer& b, const Vector3& v)
    {
        b << v.x << v.y << v.z;
    }

    inline void operator >> (ByteBuffer& b, Vector3& v)
    {
        b >> v.x >> v.y >> v.z;
    }

    enum MonsterMoveType
    {
        MonsterMoveNormal       = 0,
        MonsterMoveStop         = 1,
        MonsterMoveFacingSpot   = 2,
        MonsterMoveFacingTarget = 3,
        MonsterMoveFacingAngle  = 4
    };

    void PacketBuilder::WriteStopMovement(Vector3 const& pos, WorldPacket& data, Unit& unit)
    {
        ObjectGuid guid = unit.GetObjectGuid();
        ObjectGuid transportGuid = unit.GetTransGUID();
        MoveSplineFlag splineFlags = unit.movespline->splineflags;

        data.WriteBit(0);
        data.WriteGuidMask<0>(guid);
        data.WriteBit(1);
        data.WriteBit(1);
        data.WriteBit(1);

        data.WriteBit(!transportGuid);
        data.WriteGuidMask<1, 5, 6, 2, 3, 7, 4, 0>(transportGuid);
        data.WriteGuidMask<4>(guid);
        data.WriteBit(1);
        data.WriteBits(0, 22);
        data.WriteGuidMask<3, 7, 6, 5>(guid);
        data.WriteBit(0);
        data.WriteGuidMask<1, 2>(guid);
        data.WriteBit(1);
        data.WriteBit(!splineFlags.raw());
        data.WriteBit(1);
        data.WriteBits(0, 20);
        data.WriteBit(1);
        data.WriteBits(MonsterMoveStop, 3);

        data.WriteBit(1);
        data.WriteBit(1);

        data << uint32(unit.movespline->GetId());

        data.WriteGuidBytes<1, 7, 4, 6, 0, 2, 5, 3>(transportGuid);

        data << float(0.0f);
        data << float(0.0f);

        data.WriteGuidBytes<0>(guid);
        if (splineFlags.raw())
        {
            MoveSplineFlag splineflags2 = splineFlags;
            splineflags2 &= ~MoveSplineFlag::Mask_No_Monster_Move;
            data << uint32(unit.movespline->splineflags.raw());
        }
        data.WriteGuidBytes<5, 6>(guid);

        data << float(0.0f);

        data.WriteGuidBytes<4, 2>(guid);

        data << pos.y;
        data << pos.z;

        data.WriteGuidBytes<3, 7>(guid);

        data << pos.x;

        data.WriteGuidBytes<1>(guid);
        if (transportGuid)
            data << uint8(unit.GetTransSeat());
    }

    void WriteLinearPath(const Spline<int32>& spline, WorldPacket& data)
    {
        uint32 last_idx = spline.getPointCount() - 3;
        const Vector3 * real_path = &spline.getPoint(1);

        if (last_idx > 1)
        {
            Vector3 middle = (real_path[0] + real_path[last_idx]) / 2.f;
            Vector3 offset;
            // first and last points already appended
            for (uint32 i = 1; i < last_idx; ++i)
            {
                offset = middle - real_path[i];
                data.appendPackXYZ(offset.x, offset.y, offset.z);
            }
        }
    }

    void WriteCatmullRomPath(const Spline<int32>& spline, WorldPacket& data)
    {
        //Full size is -2 (one from start, and one from end) so itr size -1 and start from 1.
        uint32 count = spline.getPointCount()-1;

        //data.append<Vector3>(&spline.getPoint(1), count);         //LK style, I can't find how it put to bufer, but logic is wrong.
        for (uint32 i = 1; i < count; ++i)
        {
            data << spline.getPoint(i);
            //const Vector3& v = spline.getPoint(i);
            //data << v.x << v.y << v.z;
            //sLog->outU(">>>>> patch%i || %f-%f-%f", i, v.x, v.y, v.z);
        }
    }

    void WriteCatmullRomCyclicPath(const Spline<int32>& spline, WorldPacket& data)
    {
        // Full size is -3 (one from start, and two from end) so itr size -2 and start from 1
        // but need fake point, start from 0
        // fake point, client will erase it from the spline after first cycle done
        // --->>> Full size with fake 2, start from 0, itr -2
        uint32 count = spline.getPointCount() - 2;
        for (uint32 i = 1; i < count; ++i)
            data << spline.getPoint(i);
    }

    void PacketBuilder::WriteMonsterMove(const MoveSpline& move_spline, WorldPacket& data, Unit& unit)
    {
        MoveSplineFlag splineflags = move_spline.splineflags;
        ObjectGuid transportGuid = unit.GetTransGUID();
        ObjectGuid guid = unit.GetObjectGuid();
        const Spline<int32>& spline = move_spline.spline;

        MonsterMoveType type;
        switch (splineflags & MoveSplineFlag::Mask_Final_Facing)
        {
            case MoveSplineFlag::Final_Target:
                type = MonsterMoveFacingTarget;
                break;
            case MoveSplineFlag::Final_Angle:
                type = MonsterMoveFacingAngle;
                break;
            case MoveSplineFlag::Final_Point:
                type = MonsterMoveFacingSpot;
                break;
            default:
                type = MonsterMoveNormal;
                break;
        }

        data.WriteBit(0);                                       // sets/unsets MOVEMENTFLAG2_UNK7 (0x40)
        data.WriteGuidMask<0>(guid);
        data.WriteBit(!transportGuid);                          // has transport seat
        data.WriteBit(!splineflags.animation);
        data.WriteBit(1);                                       // !byte64

        data.WriteBit(!transportGuid);                          // transport guid marker
        data.WriteGuidMask<1, 5, 6, 2, 3, 7, 4, 0>(transportGuid);
        data.WriteGuidMask<4>(guid);
        data.WriteBit(!move_spline.Duration());

        // compressed wp count
        if (splineflags & MoveSplineFlag::UncompressedPath)
            data.WriteBits(0, 22);
        else
        {
            int32 cnt = spline.getPointCount() - 4;
            data.WriteBits(cnt > 0 ? cnt : 0, 22);
        }

        data.WriteGuidMask<3, 7, 6, 5>(guid);
        data.WriteBit(0);                                       // byteA8
        data.WriteGuidMask<1, 2>(guid);
        data.WriteBit(!splineflags.parabolic);
        data.WriteBit(!splineflags.raw());

        data.WriteBit(1);                                       // dword44
        // uncompressed wp count
        if (splineflags & MoveSplineFlag::UncompressedPath)
        {
            if (splineflags.cyclic)
                data.WriteBits(spline.getPointCount() - 2, 20);
            else
                data.WriteBits(spline.getPointCount() - 2, 20);
        }
        else
            data.WriteBits(1, 20);

        data.WriteBit(!splineflags.animation && !splineflags.parabolic);    // has effect start time
        data.WriteBits(type, 3);

        if (type == MonsterMoveFacingTarget)
            data.WriteGuidMask<0, 5, 7, 1, 2, 4, 6, 3>(move_spline.facing.target);

        data.WriteBit(1);                                       // byte65
        data.WriteBit(1);                                       // dword40

        if (type == MonsterMoveFacingTarget)
            data.WriteGuidBytes<0, 1, 3, 7, 6, 5, 4, 2>(move_spline.facing.target);

        if (move_spline.splineflags & MoveSplineFlag::UncompressedPath)
        {
            if (move_spline.splineflags.cyclic)
                WriteCatmullRomCyclicPath(move_spline.spline, data);
            else
                WriteCatmullRomPath(move_spline.spline, data);
        }
        else
        {
            uint32 last_idx = spline.getPointCount() - 1;
            data << spline.getPoint(last_idx); // destination
        }

        data << uint32(move_spline.GetId());

        data.WriteGuidBytes<1, 7, 4, 6, 0, 2, 5, 3>(transportGuid);

        if (type == MonsterMoveFacingSpot)
        {
            data << move_spline.facing.f.z;
            data << move_spline.facing.f.y;
            data << move_spline.facing.f.x;
        }

        data << float(0.0f);                                    // float2C
        if (splineflags.animation || splineflags.parabolic)     // has effect start time
            data << uint32(move_spline.effect_start_time);

        if ((splineflags & MoveSplineFlag::UncompressedPath) == 0)
            WriteLinearPath(move_spline.spline, data);

        data << float(0.0f);                                    // float28

        data.WriteGuidBytes<0>(guid);

        if (splineflags.raw())
        {
            MoveSplineFlag splineflags2 = splineflags;
            splineflags2.enter_cycle = move_spline.isCyclic();
            splineflags2 &= ~MoveSplineFlag::Mask_No_Monster_Move;
            data << uint32(splineflags2.raw());
        }
        data.WriteGuidBytes<5, 6>(guid);

        if (type == MonsterMoveFacingAngle)
            data << float(Position::NormalizeOrientation(move_spline.facing.angle));

        Vector3 const& pos = move_spline.spline.getPoint(move_spline.spline.first());

        data << float(0.0f);
        if (move_spline.Duration())
            data << uint32(move_spline.Duration());
        data.WriteGuidBytes<4, 2>(guid);
        data << float(pos.y);
        data << float(pos.z);
        data.WriteGuidBytes<3>(guid);
        if (splineflags.parabolic)
            data << float(move_spline.vertical_acceleration);

        data.WriteGuidBytes<7>(guid);
        if (splineflags.animation)
            data << uint8(splineflags.getAnimationId());
        data << float(pos.x);
        data.WriteGuidBytes<1>(guid);
        if (transportGuid)
            data << uint8(unit.GetTransSeat());
    }

    void PacketBuilder::WriteCreateBits(MoveSpline const& moveSpline, ByteBuffer& data)
    {
        if (!data.WriteBit(!moveSpline.Finalized()))    // has full spline
            return;

        data.WriteBit(0);       // has unknown counters
        data.WriteBit((moveSpline.splineflags & MoveSplineFlag::Parabolic) && moveSpline.effect_start_time < moveSpline.Duration());
        data.WriteBits(uint8(moveSpline.spline.mode()), 2);
        data.WriteBits(moveSpline.splineflags.raw(), 25);
        data.WriteBit(moveSpline.splineflags & (MoveSplineFlag::Parabolic | MoveSplineFlag::Animation));    // hasSplineStartTime
        data.WriteBits(moveSpline.getPath().size(), 20);
    }

    void PacketBuilder::WriteCreateData(MoveSpline const& moveSpline, ByteBuffer& data)
    {
        if (!moveSpline.Finalized())    // has full spline
        {
            uint32 nodes = moveSpline.getPath().size();
            for (uint32 i = 0; i < nodes; ++i)
            {
                data << float(moveSpline.getPath()[i].z);
                data << float(moveSpline.getPath()[i].x);
                data << float(moveSpline.getPath()[i].y);
            }
            data << moveSpline.timePassed();

            MoveSplineFlag splineFlags = moveSpline.splineflags;
            if ((splineFlags & MoveSplineFlag::Parabolic) && moveSpline.effect_start_time < moveSpline.Duration())
                data << moveSpline.vertical_acceleration;   // added in 3.1
            data << moveSpline.Duration();
            if (splineFlags & (MoveSplineFlag::Parabolic | MoveSplineFlag::Animation))
                data << moveSpline.effect_start_time;       // added in 3.1

            switch (splineFlags & MoveSplineFlag::Mask_Final_Facing)
            {
                case MoveSplineFlag::Final_Target:
                    data << uint8(MonsterMoveFacingTarget);
                    break;
                case MoveSplineFlag::Final_Angle:
                    data << uint8(MonsterMoveFacingAngle);
                    break;
                case MoveSplineFlag::Final_Point:
                    data << uint8(MonsterMoveFacingSpot);
                    break;
                default:
                    data << uint8(MonsterMoveNormal);
                    break;
            }

            data << float(1.f);                             // splineInfo.duration_mod; added in 3.1
            data << float(1.f);                             // splineInfo.duration_mod_next; added in 3.1

            if (splineFlags.final_point)
                data << moveSpline.facing.f.y << moveSpline.facing.f.z << moveSpline.facing.f.x;
            if (splineFlags.final_angle)
                data << moveSpline.facing.angle;
        }

        if (!moveSpline.isCyclic())
        {
            Vector3 dest = moveSpline.FinalDestination();
            data << float(dest.y);
            data << float(dest.z);
            data << float(dest.x);
        }
        else
        {
            data << float(0.0f);
            data << float(0.0f);
            data << float(0.0f);
        }

        data << moveSpline.GetId();
    }

    void PacketBuilder::WriteFacingData(MoveSpline const& moveSpline, ByteBuffer& data)
    {
        if (moveSpline.Finalized())
            return;

        if ((moveSpline.splineflags & MoveSplineFlag::Mask_Final_Facing) != MoveSplineFlag::Final_Target)
            return;

        ObjectGuid facingGuid = moveSpline.facing.target;
        data.WriteGuidMask<5, 3, 6, 2, 7, 0, 1, 4>(facingGuid);
        data.WriteGuidBytes<7, 0, 1, 4, 2, 5, 3, 6>(facingGuid);
    }
}

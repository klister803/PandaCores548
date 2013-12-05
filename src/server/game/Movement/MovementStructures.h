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

#ifndef _MOVEMENT_STRUCTURES_H
#define _MOVEMENT_STRUCTURES_H

enum MovementStatusElements
{
    MSEHasGuidByte0,
    MSEHasGuidByte1,
    MSEHasGuidByte2,
    MSEHasGuidByte3,
    MSEHasGuidByte4,
    MSEHasGuidByte5,
    MSEHasGuidByte6,
    MSEHasGuidByte7,
    MSEHasMovementFlags,
    MSEHasMovementFlags2,
    MSEHasTimestamp,
    MSEHasOrientation,
    MSEHasTransportData,
    MSEHasTransportGuidByte0,
    MSEHasTransportGuidByte1,
    MSEHasTransportGuidByte2,
    MSEHasTransportGuidByte3,
    MSEHasTransportGuidByte4,
    MSEHasTransportGuidByte5,
    MSEHasTransportGuidByte6,
    MSEHasTransportGuidByte7,
    MSEHasTransportTime2,
    MSEHasTransportTime3,
    MSEHasPitch,
    MSEHasFallData,
    MSEHasFallDirection,
    MSEHasSplineElevation,
    MSEHasSpline,
    MSEHasUnkInt32,

    MSEGuidByte0,
    MSEGuidByte1,
    MSEGuidByte2,
    MSEGuidByte3,
    MSEGuidByte4,
    MSEGuidByte5,
    MSEGuidByte6,
    MSEGuidByte7,
    MSEMovementFlags,
    MSEMovementFlags2,
    MSETimestamp,
    MSEPositionX,
    MSEPositionY,
    MSEPositionZ,
    MSEOrientation,
    MSETransportGuidByte0,
    MSETransportGuidByte1,
    MSETransportGuidByte2,
    MSETransportGuidByte3,
    MSETransportGuidByte4,
    MSETransportGuidByte5,
    MSETransportGuidByte6,
    MSETransportGuidByte7,
    MSETransportPositionX,
    MSETransportPositionY,
    MSETransportPositionZ,
    MSETransportOrientation,
    MSETransportSeat,
    MSETransportTime,
    MSETransportTime2,
    MSETransportTime3,
    MSEPitch,
    MSEFallTime,
    MSEFallVerticalSpeed,
    MSEFallCosAngle,
    MSEFallSinAngle,
    MSEFallHorizontalSpeed,
    MSESplineElevation,
    MSEBitCounter,
    MSEBitCounterValues,
    MSEUnkInt32,
    // Special
    MSEFlushBits,   //FlushBits()
    MSEBit95,
    MSEBitAC,
    MSEEnd,     // marks end of parsing
    MSE_COUNT
};

//5.4.0 17399
MovementStatusElements MovementStartForwardSequence[] =
{
    MSEPositionZ,
    MSEPositionX,
    MSEPositionY,
    MSEHasGuidByte1,
    MSEHasGuidByte3,
    MSEHasMovementFlags,
    MSEHasGuidByte0,
    MSEHasOrientation,
    MSEHasGuidByte4,
    MSEHasSplineElevation,
    MSEHasGuidByte2,
    MSEBit95,
    MSEHasSpline,
    MSEBitCounter,
    MSEHasFallData,
    MSEHasGuidByte5,
    MSEHasGuidByte7,
    MSEHasPitch,
    MSEHasTransportData,
    MSEBitAC,
    MSEHasUnkInt32,
    MSEHasTimestamp,
    MSEHasMovementFlags2,
    MSEHasGuidByte6,
    MSEHasTransportGuidByte1,
    MSEHasTransportGuidByte2,
    MSEHasTransportGuidByte6,
    MSEHasTransportTime3,
    MSEHasTransportGuidByte3,
    MSEHasTransportTime2,
    MSEHasTransportGuidByte0,
    MSEHasTransportGuidByte7,
    MSEHasTransportGuidByte4,
    MSEHasTransportGuidByte5,
    MSEHasMovementFlags,
    MSEHasMovementFlags2,
    MSEHasFallDirection,
    MSEHasGuidByte3,
    MSEGuidByte4,
    MSEGuidByte7,
    MSEGuidByte1,
    MSEGuidByte2,
    MSEGuidByte0,
    MSEGuidByte5,
    MSEBitCounterValues,
    MSEGuidByte6,
    MSETransportGuidByte6,
    MSETransportGuidByte2,
    MSETransportPositionY,
    MSETransportGuidByte5,
    MSETransportTime,
    MSETransportPositionX,
    MSETransportGuidByte1,
    MSETransportGuidByte3,
    MSETransportTime3,
    MSETransportGuidByte4,
    MSETransportPositionZ,
    MSETransportGuidByte0,
    MSETransportGuidByte7,
    MSETransportOrientation,
    MSETransportTime2,
    MSETransportSeat,
    MSEPitch,
    MSEFallTime,
    MSEFallHorizontalSpeed,
    MSEFallSinAngle,
    MSEFallCosAngle,
    MSEFallVerticalSpeed,
    MSESplineElevation,
    MSETimestamp,
    MSEUnkInt32,
    MSEOrientation,
    MSEEnd,
};

//5.4.0 17399
MovementStatusElements PlayerMoveSequence[] =
{
    MSEBit95,
    MSEHasPitch,
    MSEHasGuidByte4,
    MSEHasGuidByte2,
    MSEBitAC,
    MSEHasFallData,
    MSEHasGuidByte7,
    MSEBitCounter,
    MSEHasGuidByte5,
    MSEHasGuidByte3,
    MSEHasUnkInt32,
    MSEHasTransportData,

    MSEHasTransportGuidByte1,
    MSEHasTransportGuidByte2,
    MSEHasTransportGuidByte3,
    MSEHasTransportGuidByte4,
    MSEHasTransportGuidByte5,
    MSEHasTransportTime3,
    MSEHasTransportTime2,
    MSEHasTransportGuidByte0,
    MSEHasTransportGuidByte7,
    MSEHasTransportGuidByte6,

    MSEHasMovementFlags,
    MSEMovementFlags,

    MSEHasOrientation,
    MSEHasTimestamp,
    MSEHasFallDirection,
    MSEHasMovementFlags2,
    MSEHasGuidByte6,
    MSEHasGuidByte0,
    MSEHasGuidByte1,
    MSEHasSpline,
    MSEMovementFlags2,
    MSEHasSplineElevation,

    MSEFlushBits,

    MSEPositionX,
    MSEFallCosAngle,
    MSEFallHorizontalSpeed,
    MSEFallSinAngle,
    MSEFallTime,
    MSEFallVerticalSpeed,

    MSEGuidByte3,

    MSETransportGuidByte2,
    MSETransportGuidByte0,
    MSETransportGuidByte5,
    MSETransportSeat,
    MSETransportGuidByte4,
    MSETransportGuidByte3,
    MSETransportTime2,
    MSETransportGuidByte6,
    MSETransportGuidByte7,
    MSETransportPositionX,
    MSETransportTime3,
    MSETransportTime,
    MSETransportPositionZ,
    MSETransportGuidByte1,
    MSETransportPositionY,
    MSETransportOrientation,

    MSEGuidByte2,
    MSEGuidByte6,
    MSEBitCounterValues,
    MSEGuidByte1,
    MSEPitch,
    MSEPositionY,
    MSEPositionZ,
    MSEGuidByte4,
    MSETimestamp,
    MSESplineElevation,
    MSEUnkInt32,
    MSEGuidByte0,
    MSEGuidByte5,
    MSEGuidByte7,
    MSEOrientation,

    MSEEnd,
};

MovementStatusElements* GetMovementStatusElementsSequence(Opcodes opcode)
{
    switch (opcode)
    {
        case CMSG_CAST_SPELL:       // Cast spell has movement data part when castflags & 0x10, patched ClientSide to have same data of CMSG_PLAYER_MOVE
        case CMSG_PLAYER_MOVE:
            return MovementStartForwardSequence;
        case SMSG_MOVE_UPDATE:
            return PlayerMoveSequence;
        default:
            break;
    }

    return NULL;
}

#endif

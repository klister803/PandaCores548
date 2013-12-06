using System;
namespace OpcodeTools
{
    public abstract class FormulasBase
    {
        public abstract uint CalcCryptedFromOpcode(uint opcode);
        public abstract uint CalcSpecialFromOpcode(uint opcode);
        public abstract uint CalcAuthFromOpcode(uint opcode);
        public abstract uint CalcGuildFromOpcode(uint opcode);
        public abstract uint CalcMovementFromOpcode(uint opcode);
        public abstract uint CalcQuestFromOpcode(uint opcode);
        public abstract uint CalcSpellFromOpcode(uint opcode);

        protected abstract bool NormalCheck(uint opcode);
        protected abstract bool SpecialCheck(uint opcode);
        protected abstract bool AuthCheck(uint opcode);
        protected abstract bool SpecialSpellCheck(uint opcode);
        protected abstract bool SpecialGuildCheck(uint opcode);
        protected abstract bool SpecialMovementCheck(uint opcode);
        protected abstract bool SpecialQuestCheck(uint opcode);

        protected virtual uint BaseOffset { get { return 1376; } }
        protected virtual uint MaxOpcode { get { return 0xFFFF; } }

        public bool IsAuthOpcode(uint opcode)
        {
            return AuthCheck(opcode);
        }

        public bool IsSpecialOpcode(uint opcode)
        {
            return !IsAuthOpcode(opcode) && SpecialCheck(opcode);
        }

        public bool IsNormalOpcode(uint opcode)
        {
            return !IsSpecialOpcode(opcode) && !IsAuthOpcode(opcode) && NormalCheck(opcode);
        }

        public bool IsSpecialGuildOpcode(uint opcode)
        {
            return !IsSpecialOpcode(opcode) && SpecialGuildCheck(opcode);
        }

        public bool IsSpecialMovementOpcode(uint opcode)
        {
            return !IsSpecialOpcode(opcode) && !IsSpecialGuildOpcode(opcode) && SpecialMovementCheck(opcode);
        }

        public bool IsSpecialQuestOpcode(uint opcode)
        {
            return !IsSpecialOpcode(opcode) && !IsSpecialGuildOpcode(opcode) && !IsSpecialMovementOpcode(opcode) && SpecialQuestCheck(opcode);
        }

        public bool IsSpecialSpellOpcode(uint opcode)
        {
            return !IsSpecialOpcode(opcode) && !IsSpecialGuildOpcode(opcode) && !IsSpecialMovementOpcode(opcode) && !IsSpecialQuestOpcode(opcode) && SpecialSpellCheck(opcode);
        }

        public uint CalcOffsetFromOpcode(uint opcode)
        {
            uint crypted = CalcCryptedFromOpcode(opcode);
            return (crypted * 4) + BaseOffset;
        }

        public uint CalcOpcodeFromSpecial(uint offset)
        {
            for (uint i = 1; i < MaxOpcode; ++i)
            {
                if (IsSpecialOpcode(i))
                {
                    if (CalcSpecialFromOpcode(i) == offset)
                        return i;
                }
            }
            return 0;
        }

        public uint CalcOpcodeFromSpell(uint spell)
        {
            for (uint i = 1; i < MaxOpcode; ++i)
            {
                if (IsSpecialSpellOpcode(i))
                {
                    if (CalcSpellFromOpcode(i) == spell)
                        return i;
                }
            }

            return 0;
        }

        public uint CalcOpcodeFromMovement(uint movement)
        {
            for (uint i = 1; i < MaxOpcode; ++i)
            {
                if (IsSpecialMovementOpcode(i))
                {
                    if (CalcMovementFromOpcode(i) == movement)
                        return i;
                }
            }
            return 0;
        }

        public uint CalcOpcodeFromQuest(uint quest)
        {
            for (uint i = 1; i < MaxOpcode; ++i)
            {
                if (IsSpecialQuestOpcode(i))
                {
                    if (CalcQuestFromOpcode(i) == quest)
                        return i;
                }
            }
            return 0;
        }

        public uint CalcOpcodeFromGuild(uint guild)
        {
            for (uint i = 1; i < MaxOpcode; ++i)
            {
                if (IsSpecialGuildOpcode(i))
                {
                    if (CalcGuildFromOpcode(i) == guild)
                        return i;
                }
            }

            return 0;
        }

        public uint CalcOpcodeFromOffset(uint offset)
        {
            for (uint i = 1; i < MaxOpcode; ++i)
            {
                if (IsNormalOpcode(i))
                {
                    if (CalcOffsetFromOpcode(i) == offset)
                        return i;
                }
            }
            return 0;
        }

        public uint CalcOpcodeFromCrypted(uint val)
        {
            for (uint i = 1; i < MaxOpcode; ++i)
            {
                if (IsNormalOpcode(i))
                {
                    if (CalcCryptedFromOpcode(i) == val)
                        return i;
                }
            }

            return 0;
        }

        public uint CalcOpcodeFromAuth(uint auth)
        {
            for (uint i = 1; i < MaxOpcode; ++i)
            {
                if (IsAuthOpcode(i) &&
                    CalcAuthFromOpcode(i) == auth)
                {
                    return i;
                }
            }
            return 0;
        }
    }
}

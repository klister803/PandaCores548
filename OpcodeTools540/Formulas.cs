using System;
using System.Collections.Generic;
using System.Text;

namespace OpcodeTools
{
    public class Windows540 : FormulasBase
    {
        public override string ToString()
        {
            return "5.4.0 17399 Windows";
        }

        protected override uint MaxOpcode { get { return 0x1FFF; } }
        protected override uint BaseOffset { get { return 1360; } }

        protected override bool AuthCheck(uint opcode)
        {
            return (opcode & 0x16D6) == 66;
        }

        protected override bool SpecialCheck(uint opcode)
        {
            return (opcode & 0x1240) == 0;
        }

        protected override bool NormalCheck(uint opcode)
        {
            return (opcode & 0x1202) == 4608;
        }

        protected override bool SpecialSpellCheck(uint opcode)
        {
            return (opcode & 0x1248) == 4096;
        }

        protected override bool SpecialGuildCheck(uint opcode)
        {
            return (opcode & 0x16C2) == 1090;
        }

        protected override bool SpecialMovementCheck(uint opcode)
        {
            return (opcode & 0x1202) == 4610;
        }

        protected override bool SpecialQuestCheck(uint opcode)
        {
            return (opcode & 0x1248) == 4104;
        }

        public override uint CalcCryptedFromOpcode(uint opcode)
        {
            return opcode & 1 | ((opcode & 0x1FC | ((opcode & 0xC00 | (opcode >> 1) & 0x7000) >> 1)) >> 1);
        }

        public override uint CalcSpecialFromOpcode(uint opcode)
        {
            return (opcode & 0x180 | ((opcode & 0xC00 | (opcode >> 1) & 0x7000) >> 1)) >> 1 | opcode & 0x3F;
        }

        public override uint CalcAuthFromOpcode(uint opcode)
        {
            return opcode & 1 | ((opcode & 8 | ((opcode & 0x20 | ((opcode & 0x100 | ((opcode & 0x800 | (opcode >> 1) & 0x7000) >> 2)) >> 2)) >> 1)) >> 2);
        }

        public override uint CalcSpellFromOpcode(uint opcode)
        {
            return opcode & 7 | ((opcode & 0x30 | ((opcode & 0x180 | ((opcode & 0xC00 | (opcode >> 1) & 0x7000) >> 1)) >> 1)) >> 1);
        }

        public override uint CalcGuildFromOpcode(uint opcode)
        {
            return opcode & 1 | ((opcode & 0x3C | ((opcode & 0x100 | ((opcode & 0x800 | (opcode >> 1) & 0x7000) >> 2)) >> 2)) >> 1);
        }

        public override uint CalcMovementFromOpcode(uint opcode)
        {
            return opcode & 1 | ((opcode & 0x1FC | ((opcode & 0xC00 | (opcode >> 1) & 0x7000) >> 1)) >> 1);
        }

        public override uint CalcQuestFromOpcode(uint opcode)
        {
            return opcode & 7 | ((opcode & 0x30 | ((opcode & 0x180 | ((opcode & 0xC00 | (opcode >> 1) & 0x7000) >> 1)) >> 1)) >> 1);
        }
    }

    public class Windows541 : FormulasBase
    {
        public override string ToString()
        {
            return "5.4.1 17538 Windows";
        }

        protected override uint MaxOpcode { get { return 0x1FFF; } }
        protected override uint BaseOffset { get { return 1360; } }

        protected override bool AuthCheck(uint opcode)
        {
            return (opcode & 0x3F6) == 0x54;
        }

        protected override bool SpecialCheck(uint opcode)
        {
            return (opcode & 0x244) == 4;
        }

        protected override bool NormalCheck(uint opcode)
        {
            return (opcode & 0x244) == 0;
        }

        protected override bool SpecialSpellCheck(uint opcode)
        {
            return (opcode & 0x252) == 64;
        }

        protected override bool SpecialGuildCheck(uint opcode)
        {
            return (opcode & 0x276) == 80;
        }

        protected override bool SpecialMovementCheck(uint opcode)
        {
            return (opcode & 0x1240) == 512;
        }

        protected override bool SpecialQuestCheck(uint opcode)
        {
            return (opcode & 0x1248) == 4104;
        }

        public override uint CalcCryptedFromOpcode(uint opcode)
        {
            return opcode & 3 | ((opcode & 0x38 | ((opcode & 0x180 | (opcode >> 1) & 0x7E00) >> 1)) >> 1);
        }

        public override uint CalcSpecialFromOpcode(uint opcode)
        {
            return opcode & 3 | ((opcode & 0x38 | ((opcode & 0x180 | (opcode >> 1) & 0x7E00) >> 1)) >> 1);
        }

        public override uint CalcAuthFromOpcode(uint opcode)
        {
            return opcode & 1 | ((opcode & 8 | (opcode >> 6) & 0x3F0) >> 2);
        }

        public override uint CalcSpellFromOpcode(uint opcode)
        {
            return opcode & 1 | ((opcode & 0xC | ((opcode & 0x20 | ((opcode & 0x180 | (opcode >> 1) & 0x7E00) >> 1)) >> 1)) >> 1);
        }

        public override uint CalcGuildFromOpcode(uint opcode)
        {
            return opcode & 1 | ((opcode & 8 | ((opcode & 0x180 | (opcode >> 1) & 0x7E00) >> 3)) >> 2);
        }

        public override uint CalcMovementFromOpcode(uint opcode)
        {
            return opcode & 0x3F | ((opcode & 0x180 | ((opcode & 0xC00 | (opcode >> 1) & 0x7000) >> 1)) >> 1);
        }

        public override uint CalcQuestFromOpcode(uint opcode)
        {
            return opcode & 3 | ((opcode & 0x38 | ((opcode & 0x180 | ((opcode & 0xC00 | (opcode >> 1) & 0x7000) >> 1)) >> 1)) >> 1);
        }
    }
}

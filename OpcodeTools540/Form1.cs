using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Reflection;

namespace OpcodeTools
{
    public partial class Form1 : Form
    {
        FormulasBase f;

        public Form1()
        {
            InitializeComponent();
        }

        void updateValuesForOpcode(uint opcode)
        {
            toolStripStatusLabel.Text = "";

            SetOpcode(opcode);

            if (f.IsAuthOpcode(opcode))
            {
                uint auth = f.CalcAuthFromOpcode(opcode);
                authBox.Text = auth.ToString();
            }
            else
                authBox.Text = "";
            if (f.IsSpecialOpcode(opcode))
            {
                uint specialHandlerNum = f.CalcSpecialFromOpcode(opcode);
                specialBox.Text = String.Format("0x{0:X}", specialHandlerNum);
            }
            else
                specialBox.Text = "";
            if (f.IsNormalOpcode(opcode))
            {
                uint crypt = f.CalcCryptedFromOpcode(opcode);
                uint offset = f.CalcOffsetFromOpcode(opcode);
                cryptedBox.Text = crypt.ToString();
                offsetBox.Text = offset.ToString();
            }
            else
            {
                cryptedBox.Text = "";
                offsetBox.Text = "";
            }
            if (f.IsSpecialSpellOpcode(opcode))
            {
                uint nb = f.CalcSpellFromOpcode(opcode);
                spellBox.Text = String.Format("0x{0:X}", nb);
            }
            else
                spellBox.Text = "";
            if (f.IsSpecialGuildOpcode(opcode))
            {
                uint nb = f.CalcGuildFromOpcode(opcode);
                guildBox.Text = String.Format("0x{0:X}", nb);
            }
            else
                guildBox.Text = "";
            if (f.IsSpecialMovementOpcode(opcode))
            {
                uint nb = f.CalcMovementFromOpcode(opcode);
                movementBox.Text = nb.ToString();
            }
            else
                movementBox.Text = "";
            if (f.IsSpecialQuestOpcode(opcode))
            {
                uint nb = f.CalcQuestFromOpcode(opcode);
                questBox.Text = String.Format("0x{0:X}", nb);
            }
            else
                questBox.Text = "";
        }

        private void SetOpcode(uint opcode)
        {
            opcodeHexBox.Text = String.Format("0x{0:X}", opcode);
            opcodeDecBox.Text = opcode.ToString();
        }

        private void Versions_SelectedIndexChanged(object sender, EventArgs e)
        {
            f = (FormulasBase)Versions.SelectedItem;
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            foreach (Type t in Assembly.GetExecutingAssembly().GetTypes())
            {
                if (t.IsSubclassOf(typeof(FormulasBase)))
                {
                    object formulas = t.GetConstructor(new Type[] { }).Invoke(new Object[] { });
                    Versions.Items.Add(formulas);
                }
            }
            // choose the newest by default
            Versions.SelectedIndex = Versions.Items.Count - 1;
        }

        private void cryptedBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode != Keys.Enter)
                return;

            try
            {
                updateValuesForOpcode(f.CalcOpcodeFromCrypted(Convert.ToUInt32(cryptedBox.Text)));
            }
            catch (Exception)
            {
                toolStripStatusLabel.Text = "Failed to parse texbox value.";
            }
        }

        private void offsetBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode != Keys.Enter)
                return;

            try
            {
                updateValuesForOpcode(f.CalcOpcodeFromOffset(Convert.ToUInt32(offsetBox.Text)));
            }
            catch (Exception)
            {
                toolStripStatusLabel.Text = "Failed to parse texbox value.";
            }
        }

        private void specialBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode != Keys.Enter)
                return;

            try
            {
                updateValuesForOpcode(f.CalcOpcodeFromSpecial(Convert.ToUInt32(specialBox.Text, 16)));
            }
            catch (Exception)
            {
                toolStripStatusLabel.Text = "Failed to parse texbox value.";
            }
        }

        private void authBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode != Keys.Enter)
                return;

            try
            {
                updateValuesForOpcode(f.CalcOpcodeFromAuth(Convert.ToUInt32(authBox.Text)));
            }
            catch (Exception)
            {
                toolStripStatusLabel.Text = "Failed to parse texbox value.";
            }
        }

        private void spellBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode != Keys.Enter)
                return;

            try
            {
                updateValuesForOpcode(f.CalcOpcodeFromSpell(Convert.ToUInt32(spellBox.Text, 16)));
            }
            catch (Exception)
            {
                toolStripStatusLabel.Text = "Failed to parse texbox value.";
            }
        }

        private void guildBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode != Keys.Enter)
                return;

            try
            {
                updateValuesForOpcode(f.CalcOpcodeFromGuild(Convert.ToUInt32(guildBox.Text, 16)));
            }
            catch (Exception)
            {
                toolStripStatusLabel.Text = "Failed to parse texbox value.";
            }
        }

        private void movementBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode != Keys.Enter)
                return;

            try
            {
                updateValuesForOpcode(f.CalcOpcodeFromMovement(Convert.ToUInt32(movementBox.Text)));
            }
            catch (Exception)
            {
                toolStripStatusLabel.Text = "Failed to parse texbox value.";
            }
        }

        private void questBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode != Keys.Enter)
                return;

            try
            {
                updateValuesForOpcode(f.CalcOpcodeFromQuest(Convert.ToUInt32(questBox.Text, 16)));
            }
            catch (Exception)
            {
                toolStripStatusLabel.Text = "Failed to parse texbox value.";
            }
        }

        private void opcodeHexBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode != Keys.Enter)
                return;

            try
            {
                uint opcode = Convert.ToUInt32(opcodeHexBox.Text, 16);
                updateValuesForOpcode(opcode);
            }
            catch (Exception)
            {
                toolStripStatusLabel.Text = "Failed to parse texbox value.";
            }
        }

        private void opcodeDecBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode != Keys.Enter)
                return;

            uint opcode;
            if (!UInt32.TryParse(opcodeDecBox.Text, out opcode))
            {
                toolStripStatusLabel.Text = "Failed to parse texbox value.";
                return;
            }

            updateValuesForOpcode(opcode);
        }
    }
}

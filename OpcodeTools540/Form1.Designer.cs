namespace OpcodeTools
{
	partial class Form1
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing)
		{
			if (disposing && (components != null))
			{
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
            this.opcodeDecBox = new System.Windows.Forms.TextBox();
            this.offsetBox = new System.Windows.Forms.TextBox();
            this.opcodeHexBox = new System.Windows.Forms.TextBox();
            this.cryptedBox = new System.Windows.Forms.TextBox();
            this.opcodeCryptedLabel = new System.Windows.Forms.Label();
            this.opcodeOffsetLabel = new System.Windows.Forms.Label();
            this.opcodeDecLabel = new System.Windows.Forms.Label();
            this.opcodeHexLabel = new System.Windows.Forms.Label();
            this.opcodeSpecialLabel = new System.Windows.Forms.Label();
            this.specialBox = new System.Windows.Forms.TextBox();
            this.convLabel = new System.Windows.Forms.Label();
            this.opcodeAuthLabel = new System.Windows.Forms.Label();
            this.authBox = new System.Windows.Forms.TextBox();
            this.Versions = new System.Windows.Forms.ComboBox();
            this.opcodeSpellLabel = new System.Windows.Forms.Label();
            this.spellBox = new System.Windows.Forms.TextBox();
            this.opcodeGuildLabel = new System.Windows.Forms.Label();
            this.guildBox = new System.Windows.Forms.TextBox();
            this.opcodeMovementLabel = new System.Windows.Forms.Label();
            this.movementBox = new System.Windows.Forms.TextBox();
            this.opcodeQuestLabel = new System.Windows.Forms.Label();
            this.questBox = new System.Windows.Forms.TextBox();
            this.statusStrip = new System.Windows.Forms.StatusStrip();
            this.toolStripStatusLabel1 = new System.Windows.Forms.ToolStripStatusLabel();
            this.toolStripStatusLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this.statusStrip.SuspendLayout();
            this.SuspendLayout();
            // 
            // opcodeDecBox
            // 
            this.opcodeDecBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.opcodeDecBox.Location = new System.Drawing.Point(12, 119);
            this.opcodeDecBox.Name = "opcodeDecBox";
            this.opcodeDecBox.Size = new System.Drawing.Size(51, 20);
            this.opcodeDecBox.TabIndex = 1;
            this.opcodeDecBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.opcodeDecBox_KeyDown);
            // 
            // offsetBox
            // 
            this.offsetBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.offsetBox.Location = new System.Drawing.Point(123, 80);
            this.offsetBox.Name = "offsetBox";
            this.offsetBox.Size = new System.Drawing.Size(46, 20);
            this.offsetBox.TabIndex = 3;
            this.offsetBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.offsetBox_KeyDown);
            // 
            // opcodeHexBox
            // 
            this.opcodeHexBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.opcodeHexBox.Location = new System.Drawing.Point(13, 83);
            this.opcodeHexBox.Name = "opcodeHexBox";
            this.opcodeHexBox.Size = new System.Drawing.Size(50, 20);
            this.opcodeHexBox.TabIndex = 0;
            this.opcodeHexBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.opcodeHexBox_KeyDown);
            // 
            // cryptedBox
            // 
            this.cryptedBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.cryptedBox.Location = new System.Drawing.Point(123, 44);
            this.cryptedBox.Name = "cryptedBox";
            this.cryptedBox.Size = new System.Drawing.Size(46, 20);
            this.cryptedBox.TabIndex = 2;
            this.cryptedBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.cryptedBox_KeyDown);
            // 
            // opcodeCryptedLabel
            // 
            this.opcodeCryptedLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.opcodeCryptedLabel.AutoSize = true;
            this.opcodeCryptedLabel.Location = new System.Drawing.Point(120, 28);
            this.opcodeCryptedLabel.Name = "opcodeCryptedLabel";
            this.opcodeCryptedLabel.Size = new System.Drawing.Size(64, 13);
            this.opcodeCryptedLabel.TabIndex = 4;
            this.opcodeCryptedLabel.Text = "Crypted dec";
            // 
            // opcodeOffsetLabel
            // 
            this.opcodeOffsetLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.opcodeOffsetLabel.AutoSize = true;
            this.opcodeOffsetLabel.Location = new System.Drawing.Point(120, 64);
            this.opcodeOffsetLabel.Name = "opcodeOffsetLabel";
            this.opcodeOffsetLabel.Size = new System.Drawing.Size(56, 13);
            this.opcodeOffsetLabel.TabIndex = 5;
            this.opcodeOffsetLabel.Text = "Offset dec";
            // 
            // opcodeDecLabel
            // 
            this.opcodeDecLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.opcodeDecLabel.AutoSize = true;
            this.opcodeDecLabel.Location = new System.Drawing.Point(9, 103);
            this.opcodeDecLabel.Name = "opcodeDecLabel";
            this.opcodeDecLabel.Size = new System.Drawing.Size(66, 13);
            this.opcodeDecLabel.TabIndex = 6;
            this.opcodeDecLabel.Text = "Opcode dec";
            // 
            // opcodeHexLabel
            // 
            this.opcodeHexLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.opcodeHexLabel.AutoSize = true;
            this.opcodeHexLabel.Location = new System.Drawing.Point(10, 67);
            this.opcodeHexLabel.Name = "opcodeHexLabel";
            this.opcodeHexLabel.Size = new System.Drawing.Size(65, 13);
            this.opcodeHexLabel.TabIndex = 7;
            this.opcodeHexLabel.Text = "Opcode hex";
            // 
            // opcodeSpecialLabel
            // 
            this.opcodeSpecialLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.opcodeSpecialLabel.AutoSize = true;
            this.opcodeSpecialLabel.Location = new System.Drawing.Point(120, 101);
            this.opcodeSpecialLabel.Name = "opcodeSpecialLabel";
            this.opcodeSpecialLabel.Size = new System.Drawing.Size(87, 13);
            this.opcodeSpecialLabel.TabIndex = 9;
            this.opcodeSpecialLabel.Text = "Special (jam) hex";
            // 
            // specialBox
            // 
            this.specialBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.specialBox.Location = new System.Drawing.Point(123, 117);
            this.specialBox.Name = "specialBox";
            this.specialBox.Size = new System.Drawing.Size(46, 20);
            this.specialBox.TabIndex = 4;
            this.specialBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.specialBox_KeyDown);
            // 
            // convLabel
            // 
            this.convLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.convLabel.AutoSize = true;
            this.convLabel.Location = new System.Drawing.Point(81, 96);
            this.convLabel.Name = "convLabel";
            this.convLabel.Size = new System.Drawing.Size(25, 13);
            this.convLabel.TabIndex = 15;
            this.convLabel.Text = "<=>";
            // 
            // opcodeAuthLabel
            // 
            this.opcodeAuthLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.opcodeAuthLabel.AutoSize = true;
            this.opcodeAuthLabel.Location = new System.Drawing.Point(120, 140);
            this.opcodeAuthLabel.Name = "opcodeAuthLabel";
            this.opcodeAuthLabel.Size = new System.Drawing.Size(50, 13);
            this.opcodeAuthLabel.TabIndex = 19;
            this.opcodeAuthLabel.Text = "Auth dec";
            // 
            // authBox
            // 
            this.authBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.authBox.Location = new System.Drawing.Point(123, 156);
            this.authBox.Name = "authBox";
            this.authBox.Size = new System.Drawing.Size(46, 20);
            this.authBox.TabIndex = 5;
            this.authBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.authBox_KeyDown);
            // 
            // Versions
            // 
            this.Versions.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.Versions.FormattingEnabled = true;
            this.Versions.Location = new System.Drawing.Point(40, 4);
            this.Versions.Name = "Versions";
            this.Versions.Size = new System.Drawing.Size(142, 21);
            this.Versions.Sorted = true;
            this.Versions.TabIndex = 6;
            this.Versions.SelectedIndexChanged += new System.EventHandler(this.Versions_SelectedIndexChanged);
            // 
            // opcodeSpellLabel
            // 
            this.opcodeSpellLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.opcodeSpellLabel.AutoSize = true;
            this.opcodeSpellLabel.Location = new System.Drawing.Point(222, 28);
            this.opcodeSpellLabel.Name = "opcodeSpellLabel";
            this.opcodeSpellLabel.Size = new System.Drawing.Size(113, 13);
            this.opcodeSpellLabel.TabIndex = 21;
            this.opcodeSpellLabel.Text = "Spell Special (jam) hex";
            // 
            // spellBox
            // 
            this.spellBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.spellBox.Location = new System.Drawing.Point(225, 44);
            this.spellBox.Name = "spellBox";
            this.spellBox.Size = new System.Drawing.Size(46, 20);
            this.spellBox.TabIndex = 20;
            this.spellBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.spellBox_KeyDown);
            // 
            // opcodeGuildLabel
            // 
            this.opcodeGuildLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.opcodeGuildLabel.AutoSize = true;
            this.opcodeGuildLabel.Location = new System.Drawing.Point(222, 67);
            this.opcodeGuildLabel.Name = "opcodeGuildLabel";
            this.opcodeGuildLabel.Size = new System.Drawing.Size(114, 13);
            this.opcodeGuildLabel.TabIndex = 23;
            this.opcodeGuildLabel.Text = "Guild Special (jam) hex";
            // 
            // guildBox
            // 
            this.guildBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.guildBox.Location = new System.Drawing.Point(225, 83);
            this.guildBox.Name = "guildBox";
            this.guildBox.Size = new System.Drawing.Size(46, 20);
            this.guildBox.TabIndex = 22;
            this.guildBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.guildBox_KeyDown);
            // 
            // opcodeMovementLabel
            // 
            this.opcodeMovementLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.opcodeMovementLabel.AutoSize = true;
            this.opcodeMovementLabel.Location = new System.Drawing.Point(222, 101);
            this.opcodeMovementLabel.Name = "opcodeMovementLabel";
            this.opcodeMovementLabel.Size = new System.Drawing.Size(140, 13);
            this.opcodeMovementLabel.TabIndex = 25;
            this.opcodeMovementLabel.Text = "Movement Special (jam) hex";
            // 
            // movementBox
            // 
            this.movementBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.movementBox.Location = new System.Drawing.Point(225, 117);
            this.movementBox.Name = "movementBox";
            this.movementBox.Size = new System.Drawing.Size(46, 20);
            this.movementBox.TabIndex = 24;
            this.movementBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.movementBox_KeyDown);
            // 
            // opcodeQuestLabel
            // 
            this.opcodeQuestLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.opcodeQuestLabel.AutoSize = true;
            this.opcodeQuestLabel.Location = new System.Drawing.Point(222, 140);
            this.opcodeQuestLabel.Name = "opcodeQuestLabel";
            this.opcodeQuestLabel.Size = new System.Drawing.Size(118, 13);
            this.opcodeQuestLabel.TabIndex = 27;
            this.opcodeQuestLabel.Text = "Quest Special (jam) hex";
            // 
            // questBox
            // 
            this.questBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.questBox.Location = new System.Drawing.Point(225, 156);
            this.questBox.Name = "questBox";
            this.questBox.Size = new System.Drawing.Size(46, 20);
            this.questBox.TabIndex = 26;
            this.questBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.questBox_KeyDown);
            // 
            // statusStrip
            // 
            this.statusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripStatusLabel1,
            this.toolStripStatusLabel});
            this.statusStrip.Location = new System.Drawing.Point(0, 180);
            this.statusStrip.Name = "statusStrip";
            this.statusStrip.Size = new System.Drawing.Size(367, 22);
            this.statusStrip.TabIndex = 28;
            // 
            // toolStripStatusLabel1
            // 
            this.toolStripStatusLabel1.Name = "toolStripStatusLabel1";
            this.toolStripStatusLabel1.Size = new System.Drawing.Size(0, 17);
            // 
            // toolStripStatusLabel
            // 
            this.toolStripStatusLabel.Name = "toolStripStatusLabel";
            this.toolStripStatusLabel.Size = new System.Drawing.Size(0, 17);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(367, 202);
            this.Controls.Add(this.statusStrip);
            this.Controls.Add(this.opcodeQuestLabel);
            this.Controls.Add(this.questBox);
            this.Controls.Add(this.opcodeMovementLabel);
            this.Controls.Add(this.movementBox);
            this.Controls.Add(this.opcodeGuildLabel);
            this.Controls.Add(this.guildBox);
            this.Controls.Add(this.opcodeSpellLabel);
            this.Controls.Add(this.spellBox);
            this.Controls.Add(this.Versions);
            this.Controls.Add(this.opcodeAuthLabel);
            this.Controls.Add(this.authBox);
            this.Controls.Add(this.convLabel);
            this.Controls.Add(this.opcodeSpecialLabel);
            this.Controls.Add(this.specialBox);
            this.Controls.Add(this.opcodeHexLabel);
            this.Controls.Add(this.opcodeDecLabel);
            this.Controls.Add(this.opcodeOffsetLabel);
            this.Controls.Add(this.opcodeCryptedLabel);
            this.Controls.Add(this.cryptedBox);
            this.Controls.Add(this.opcodeHexBox);
            this.Controls.Add(this.offsetBox);
            this.Controls.Add(this.opcodeDecBox);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.Fixed3D;
            this.Name = "Form1";
            this.Text = "OpcodeTools";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.statusStrip.ResumeLayout(false);
            this.statusStrip.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.TextBox opcodeDecBox;
		private System.Windows.Forms.TextBox offsetBox;
		private System.Windows.Forms.TextBox opcodeHexBox;
		private System.Windows.Forms.TextBox cryptedBox;
		private System.Windows.Forms.Label opcodeCryptedLabel;
		private System.Windows.Forms.Label opcodeOffsetLabel;
		private System.Windows.Forms.Label opcodeDecLabel;
		private System.Windows.Forms.Label opcodeHexLabel;
		private System.Windows.Forms.Label opcodeSpecialLabel;
        private System.Windows.Forms.TextBox specialBox;
        private System.Windows.Forms.Label convLabel;
		private System.Windows.Forms.Label opcodeAuthLabel;
		private System.Windows.Forms.TextBox authBox;
        private System.Windows.Forms.ComboBox Versions;
        private System.Windows.Forms.Label opcodeSpellLabel;
        private System.Windows.Forms.TextBox spellBox;
        private System.Windows.Forms.Label opcodeGuildLabel;
        private System.Windows.Forms.TextBox guildBox;
        private System.Windows.Forms.Label opcodeMovementLabel;
        private System.Windows.Forms.TextBox movementBox;
        private System.Windows.Forms.Label opcodeQuestLabel;
        private System.Windows.Forms.TextBox questBox;
        private System.Windows.Forms.StatusStrip statusStrip;
        private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel1;
        private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel;
	}
}


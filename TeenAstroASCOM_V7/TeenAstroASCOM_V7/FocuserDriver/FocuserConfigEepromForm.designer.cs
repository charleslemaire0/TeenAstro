namespace ASCOM.TeenAstro.Focuser
{
  partial class FocuserConfigEepromForm
  {
    private System.ComponentModel.IContainer components = null;

    protected override void Dispose(bool disposing)
    {
      if (disposing && (components != null))
        components.Dispose();
      base.Dispose(disposing);
    }

    #region Windows Form Designer generated code

    private void InitializeComponent()
    {
      this.groupBoxSettings = new System.Windows.Forms.GroupBox();
      this.tableLayoutPanel = new System.Windows.Forms.TableLayoutPanel();
      this.lblParkPos = new System.Windows.Forms.Label();
      this.lblMaxPos = new System.Windows.Forms.Label();
      this.lblMinSpeed = new System.Windows.Forms.Label();
      this.lblMaxSpeed = new System.Windows.Forms.Label();
      this.lblCmdAcc = new System.Windows.Forms.Label();
      this.lblManAcc = new System.Windows.Forms.Label();
      this.lblReverse = new System.Windows.Forms.Label();
      this.lblResolution = new System.Windows.Forms.Label();
      this.lblMicro = new System.Windows.Forms.Label();
      this.lblCurrent = new System.Windows.Forms.Label();
      this.numParkPos = new System.Windows.Forms.NumericUpDown();
      this.numMaxPos = new System.Windows.Forms.NumericUpDown();
      this.numMinSpeed = new System.Windows.Forms.NumericUpDown();
      this.numMaxSpeed = new System.Windows.Forms.NumericUpDown();
      this.numCmdAcc = new System.Windows.Forms.NumericUpDown();
      this.numManAcc = new System.Windows.Forms.NumericUpDown();
      this.chkReverse = new System.Windows.Forms.CheckBox();
      this.numResolution = new System.Windows.Forms.NumericUpDown();
      this.cboMicro = new System.Windows.Forms.ComboBox();
      this.numCurrent = new System.Windows.Forms.NumericUpDown();
      this.btnWrite = new System.Windows.Forms.Button();
      this.groupBoxSettings.SuspendLayout();
      this.tableLayoutPanel.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.numParkPos)).BeginInit();
      ((System.ComponentModel.ISupportInitialize)(this.numMaxPos)).BeginInit();
      ((System.ComponentModel.ISupportInitialize)(this.numMinSpeed)).BeginInit();
      ((System.ComponentModel.ISupportInitialize)(this.numMaxSpeed)).BeginInit();
      ((System.ComponentModel.ISupportInitialize)(this.numCmdAcc)).BeginInit();
      ((System.ComponentModel.ISupportInitialize)(this.numManAcc)).BeginInit();
      ((System.ComponentModel.ISupportInitialize)(this.numResolution)).BeginInit();
      ((System.ComponentModel.ISupportInitialize)(this.numCurrent)).BeginInit();
      this.SuspendLayout();
      //
      // groupBoxSettings
      //
      this.groupBoxSettings.Controls.Add(this.tableLayoutPanel);
      this.groupBoxSettings.Location = new System.Drawing.Point(10, 5);
      this.groupBoxSettings.Name = "groupBoxSettings";
      this.groupBoxSettings.Size = new System.Drawing.Size(265, 295);
      this.groupBoxSettings.TabIndex = 0;
      this.groupBoxSettings.TabStop = false;
      this.groupBoxSettings.Text = "Settings";
      //
      // tableLayoutPanel
      //
      this.tableLayoutPanel.ColumnCount = 2;
      this.tableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
      this.tableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
      this.tableLayoutPanel.Controls.Add(this.lblParkPos, 0, 0);
      this.tableLayoutPanel.Controls.Add(this.numParkPos, 1, 0);
      this.tableLayoutPanel.Controls.Add(this.lblMaxPos, 0, 1);
      this.tableLayoutPanel.Controls.Add(this.numMaxPos, 1, 1);
      this.tableLayoutPanel.Controls.Add(this.lblMinSpeed, 0, 2);
      this.tableLayoutPanel.Controls.Add(this.numMinSpeed, 1, 2);
      this.tableLayoutPanel.Controls.Add(this.lblMaxSpeed, 0, 3);
      this.tableLayoutPanel.Controls.Add(this.numMaxSpeed, 1, 3);
      this.tableLayoutPanel.Controls.Add(this.lblCmdAcc, 0, 4);
      this.tableLayoutPanel.Controls.Add(this.numCmdAcc, 1, 4);
      this.tableLayoutPanel.Controls.Add(this.lblManAcc, 0, 5);
      this.tableLayoutPanel.Controls.Add(this.numManAcc, 1, 5);
      this.tableLayoutPanel.Controls.Add(this.lblReverse, 0, 6);
      this.tableLayoutPanel.Controls.Add(this.chkReverse, 1, 6);
      this.tableLayoutPanel.Controls.Add(this.lblResolution, 0, 7);
      this.tableLayoutPanel.Controls.Add(this.numResolution, 1, 7);
      this.tableLayoutPanel.Controls.Add(this.lblMicro, 0, 8);
      this.tableLayoutPanel.Controls.Add(this.cboMicro, 1, 8);
      this.tableLayoutPanel.Controls.Add(this.lblCurrent, 0, 9);
      this.tableLayoutPanel.Controls.Add(this.numCurrent, 1, 9);
      this.tableLayoutPanel.Location = new System.Drawing.Point(15, 20);
      this.tableLayoutPanel.Name = "tableLayoutPanel";
      this.tableLayoutPanel.RowCount = 10;
      this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tableLayoutPanel.Size = new System.Drawing.Size(241, 268);
      this.tableLayoutPanel.TabIndex = 0;
      //
      // labels
      //
      this.lblParkPos.Anchor = System.Windows.Forms.AnchorStyles.Left;
      this.lblParkPos.AutoSize = true;
      this.lblParkPos.Text = "Park Position";
      this.lblMaxPos.Anchor = System.Windows.Forms.AnchorStyles.Left;
      this.lblMaxPos.AutoSize = true;
      this.lblMaxPos.Text = "Max Position";
      this.lblMinSpeed.Anchor = System.Windows.Forms.AnchorStyles.Left;
      this.lblMinSpeed.AutoSize = true;
      this.lblMinSpeed.Text = "Manual Speed";
      this.lblMaxSpeed.Anchor = System.Windows.Forms.AnchorStyles.Left;
      this.lblMaxSpeed.AutoSize = true;
      this.lblMaxSpeed.Text = "Goto Speed";
      this.lblCmdAcc.Anchor = System.Windows.Forms.AnchorStyles.Left;
      this.lblCmdAcc.AutoSize = true;
      this.lblCmdAcc.Text = "Acceleration ASCOM";
      this.lblManAcc.Anchor = System.Windows.Forms.AnchorStyles.Left;
      this.lblManAcc.AutoSize = true;
      this.lblManAcc.Text = "Acceleration Manual";
      this.lblReverse.Anchor = System.Windows.Forms.AnchorStyles.Left;
      this.lblReverse.AutoSize = true;
      this.lblReverse.Text = "Reverse Axis";
      this.lblResolution.Anchor = System.Windows.Forms.AnchorStyles.Left;
      this.lblResolution.AutoSize = true;
      this.lblResolution.Text = "Resolution";
      this.lblMicro.Anchor = System.Windows.Forms.AnchorStyles.Left;
      this.lblMicro.AutoSize = true;
      this.lblMicro.Text = "Microstep";
      this.lblCurrent.Anchor = System.Windows.Forms.AnchorStyles.Left;
      this.lblCurrent.AutoSize = true;
      this.lblCurrent.Text = "Current in mA";
      //
      // numParkPos
      //
      this.numParkPos.Maximum = new decimal(new int[] { 65535, 0, 0, 0 });
      this.numParkPos.Size = new System.Drawing.Size(94, 20);
      //
      // numMaxPos
      //
      this.numMaxPos.Maximum = new decimal(new int[] { 65535, 0, 0, 0 });
      this.numMaxPos.Size = new System.Drawing.Size(94, 20);
      //
      // numMinSpeed
      //
      this.numMinSpeed.Maximum = new decimal(new int[] { 999, 0, 0, 0 });
      this.numMinSpeed.Minimum = new decimal(new int[] { 1, 0, 0, 0 });
      this.numMinSpeed.Value = new decimal(new int[] { 5, 0, 0, 0 });
      this.numMinSpeed.Size = new System.Drawing.Size(94, 20);
      //
      // numMaxSpeed
      //
      this.numMaxSpeed.Maximum = new decimal(new int[] { 999, 0, 0, 0 });
      this.numMaxSpeed.Minimum = new decimal(new int[] { 1, 0, 0, 0 });
      this.numMaxSpeed.Value = new decimal(new int[] { 5, 0, 0, 0 });
      this.numMaxSpeed.Size = new System.Drawing.Size(94, 20);
      //
      // numCmdAcc
      //
      this.numCmdAcc.Maximum = new decimal(new int[] { 99, 0, 0, 0 });
      this.numCmdAcc.Minimum = new decimal(new int[] { 10, 0, 0, 0 });
      this.numCmdAcc.Value = new decimal(new int[] { 10, 0, 0, 0 });
      this.numCmdAcc.Size = new System.Drawing.Size(94, 20);
      //
      // numManAcc
      //
      this.numManAcc.Maximum = new decimal(new int[] { 99, 0, 0, 0 });
      this.numManAcc.Minimum = new decimal(new int[] { 10, 0, 0, 0 });
      this.numManAcc.Value = new decimal(new int[] { 10, 0, 0, 0 });
      this.numManAcc.Size = new System.Drawing.Size(94, 20);
      //
      // chkReverse
      //
      this.chkReverse.Anchor = System.Windows.Forms.AnchorStyles.Left;
      this.chkReverse.AutoSize = true;
      //
      // numResolution
      //
      this.numResolution.Maximum = new decimal(new int[] { 512, 0, 0, 0 });
      this.numResolution.Minimum = new decimal(new int[] { 1, 0, 0, 0 });
      this.numResolution.Value = new decimal(new int[] { 10, 0, 0, 0 });
      this.numResolution.Size = new System.Drawing.Size(94, 20);
      //
      // cboMicro
      //
      this.cboMicro.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.cboMicro.FormattingEnabled = true;
      this.cboMicro.Items.AddRange(new object[] { "4", "8", "16", "32", "64", "128" });
      this.cboMicro.Size = new System.Drawing.Size(94, 21);
      //
      // numCurrent
      //
      this.numCurrent.Increment = new decimal(new int[] { 100, 0, 0, 0 });
      this.numCurrent.Maximum = new decimal(new int[] { 1600, 0, 0, 0 });
      this.numCurrent.Minimum = new decimal(new int[] { 100, 0, 0, 0 });
      this.numCurrent.Value = new decimal(new int[] { 100, 0, 0, 0 });
      this.numCurrent.Size = new System.Drawing.Size(94, 20);
      //
      // btnWrite
      //
      this.btnWrite.Location = new System.Drawing.Point(148, 306);
      this.btnWrite.Name = "btnWrite";
      this.btnWrite.Size = new System.Drawing.Size(127, 23);
      this.btnWrite.TabIndex = 1;
      this.btnWrite.Text = "Write to EEPROM";
      this.btnWrite.UseVisualStyleBackColor = true;
      this.btnWrite.Click += new System.EventHandler(this.btnWrite_Click);
      //
      // FocuserConfigEepromForm
      //
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(289, 340);
      this.Controls.Add(this.btnWrite);
      this.Controls.Add(this.groupBoxSettings);
      this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
      this.MaximizeBox = false;
      this.MinimizeBox = false;
      this.Name = "FocuserConfigEepromForm";
      this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
      this.Text = "Focuser EEPROM Configuration";
      this.groupBoxSettings.ResumeLayout(false);
      this.tableLayoutPanel.ResumeLayout(false);
      this.tableLayoutPanel.PerformLayout();
      ((System.ComponentModel.ISupportInitialize)(this.numParkPos)).EndInit();
      ((System.ComponentModel.ISupportInitialize)(this.numMaxPos)).EndInit();
      ((System.ComponentModel.ISupportInitialize)(this.numMinSpeed)).EndInit();
      ((System.ComponentModel.ISupportInitialize)(this.numMaxSpeed)).EndInit();
      ((System.ComponentModel.ISupportInitialize)(this.numCmdAcc)).EndInit();
      ((System.ComponentModel.ISupportInitialize)(this.numManAcc)).EndInit();
      ((System.ComponentModel.ISupportInitialize)(this.numResolution)).EndInit();
      ((System.ComponentModel.ISupportInitialize)(this.numCurrent)).EndInit();
      this.ResumeLayout(false);
    }

    #endregion

    private System.Windows.Forms.GroupBox groupBoxSettings;
    private System.Windows.Forms.TableLayoutPanel tableLayoutPanel;
    private System.Windows.Forms.Label lblParkPos;
    private System.Windows.Forms.Label lblMaxPos;
    private System.Windows.Forms.Label lblMinSpeed;
    private System.Windows.Forms.Label lblMaxSpeed;
    private System.Windows.Forms.Label lblCmdAcc;
    private System.Windows.Forms.Label lblManAcc;
    private System.Windows.Forms.Label lblReverse;
    private System.Windows.Forms.Label lblResolution;
    private System.Windows.Forms.Label lblMicro;
    private System.Windows.Forms.Label lblCurrent;
    private System.Windows.Forms.NumericUpDown numParkPos;
    private System.Windows.Forms.NumericUpDown numMaxPos;
    private System.Windows.Forms.NumericUpDown numMinSpeed;
    private System.Windows.Forms.NumericUpDown numMaxSpeed;
    private System.Windows.Forms.NumericUpDown numCmdAcc;
    private System.Windows.Forms.NumericUpDown numManAcc;
    private System.Windows.Forms.CheckBox chkReverse;
    private System.Windows.Forms.NumericUpDown numResolution;
    private System.Windows.Forms.ComboBox cboMicro;
    private System.Windows.Forms.NumericUpDown numCurrent;
    private System.Windows.Forms.Button btnWrite;
  }
}

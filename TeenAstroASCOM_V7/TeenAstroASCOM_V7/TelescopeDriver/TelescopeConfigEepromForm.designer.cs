namespace ASCOM.TeenAstro.Telescope
{
  partial class TelescopeConfigEepromForm
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
      this.tabControl = new System.Windows.Forms.TabControl();
      this.tabMotor1 = new System.Windows.Forms.TabPage();
      this.tabMotor2 = new System.Windows.Forms.TabPage();
      this.tabRates = new System.Windows.Forms.TabPage();
      this.tabLimits = new System.Windows.Forms.TabPage();
      this.tabEncoders = new System.Windows.Forms.TabPage();
      this.tabSiteOptions = new System.Windows.Forms.TabPage();
      this.btnWrite = new System.Windows.Forms.Button();

      // Motor Axis 1 controls
      this.tlpMotor1 = new System.Windows.Forms.TableLayoutPanel();
      this.numM1Gear = new System.Windows.Forms.NumericUpDown();
      this.numM1Steps = new System.Windows.Forms.NumericUpDown();
      this.cboM1Micro = new System.Windows.Forms.ComboBox();
      this.chkM1Reverse = new System.Windows.Forms.CheckBox();
      this.numM1LowCurr = new System.Windows.Forms.NumericUpDown();
      this.numM1HighCurr = new System.Windows.Forms.NumericUpDown();
      this.numM1Backlash = new System.Windows.Forms.NumericUpDown();
      this.numM1BacklashRate = new System.Windows.Forms.NumericUpDown();
      this.chkM1Silent = new System.Windows.Forms.CheckBox();

      // Motor Axis 2 controls
      this.tlpMotor2 = new System.Windows.Forms.TableLayoutPanel();
      this.numM2Gear = new System.Windows.Forms.NumericUpDown();
      this.numM2Steps = new System.Windows.Forms.NumericUpDown();
      this.cboM2Micro = new System.Windows.Forms.ComboBox();
      this.chkM2Reverse = new System.Windows.Forms.CheckBox();
      this.numM2LowCurr = new System.Windows.Forms.NumericUpDown();
      this.numM2HighCurr = new System.Windows.Forms.NumericUpDown();
      this.numM2Backlash = new System.Windows.Forms.NumericUpDown();
      this.numM2BacklashRate = new System.Windows.Forms.NumericUpDown();
      this.chkM2Silent = new System.Windows.Forms.CheckBox();

      // Rates controls
      this.tlpRates = new System.Windows.Forms.TableLayoutPanel();
      this.numGuideRate = new System.Windows.Forms.NumericUpDown();
      this.numRate1 = new System.Windows.Forms.NumericUpDown();
      this.numRate2 = new System.Windows.Forms.NumericUpDown();
      this.numRate3 = new System.Windows.Forms.NumericUpDown();
      this.numMaxRate = new System.Windows.Forms.NumericUpDown();
      this.numDefaultRate = new System.Windows.Forms.NumericUpDown();
      this.numDegAcc = new System.Windows.Forms.NumericUpDown();

      // Limits controls
      this.tlpLimits = new System.Windows.Forms.TableLayoutPanel();
      this.numHorizon = new System.Windows.Forms.NumericUpDown();
      this.numOverhead = new System.Windows.Forms.NumericUpDown();
      this.numAxis1Min = new System.Windows.Forms.NumericUpDown();
      this.numAxis1Max = new System.Windows.Forms.NumericUpDown();
      this.numAxis2Min = new System.Windows.Forms.NumericUpDown();
      this.numAxis2Max = new System.Windows.Forms.NumericUpDown();
      this.txtMeridianE = new System.Windows.Forms.TextBox();
      this.txtMeridianW = new System.Windows.Forms.TextBox();
      this.numUnderPole = new System.Windows.Forms.NumericUpDown();
      this.txtDistFromPole = new System.Windows.Forms.TextBox();

      // Encoder controls
      this.tlpEncoders = new System.Windows.Forms.TableLayoutPanel();
      this.numEnc1Pulse = new System.Windows.Forms.NumericUpDown();
      this.chkEnc1Reverse = new System.Windows.Forms.CheckBox();
      this.numEnc2Pulse = new System.Windows.Forms.NumericUpDown();
      this.chkEnc2Reverse = new System.Windows.Forms.CheckBox();
      this.numEncSyncMode = new System.Windows.Forms.NumericUpDown();

      // Site & Options controls
      this.tlpSiteOptions = new System.Windows.Forms.TableLayoutPanel();
      this.txtMountName = new System.Windows.Forms.TextBox();
      this.numSlewSettle = new System.Windows.Forms.NumericUpDown();
      this.chkRefrGoto = new System.Windows.Forms.CheckBox();
      this.chkRefrPole = new System.Windows.Forms.CheckBox();
      this.chkRefrTracking = new System.Windows.Forms.CheckBox();
      this.txtLatitude = new System.Windows.Forms.TextBox();
      this.txtLongitude = new System.Windows.Forms.TextBox();
      this.numElevation = new System.Windows.Forms.NumericUpDown();

      this.tabControl.SuspendLayout();
      this.SuspendLayout();

      // tabControl
      this.tabControl.Controls.Add(this.tabMotor1);
      this.tabControl.Controls.Add(this.tabMotor2);
      this.tabControl.Controls.Add(this.tabRates);
      this.tabControl.Controls.Add(this.tabLimits);
      this.tabControl.Controls.Add(this.tabEncoders);
      this.tabControl.Controls.Add(this.tabSiteOptions);
      this.tabControl.Location = new System.Drawing.Point(8, 8);
      this.tabControl.Size = new System.Drawing.Size(424, 340);
      this.tabControl.Name = "tabControl";

      // ---- Tab Motor 1 ----
      this.tabMotor1.Text = "Motor Axis 1";
      this.tabMotor1.Controls.Add(this.tlpMotor1);
      this.tabMotor1.Padding = new System.Windows.Forms.Padding(3);
      SetupMotorTable(this.tlpMotor1, this.numM1Gear, this.numM1Steps, this.cboM1Micro,
        this.chkM1Reverse, this.numM1LowCurr, this.numM1HighCurr, this.numM1Backlash,
        this.numM1BacklashRate, this.chkM1Silent);

      // ---- Tab Motor 2 ----
      this.tabMotor2.Text = "Motor Axis 2";
      this.tabMotor2.Controls.Add(this.tlpMotor2);
      this.tabMotor2.Padding = new System.Windows.Forms.Padding(3);
      SetupMotorTable(this.tlpMotor2, this.numM2Gear, this.numM2Steps, this.cboM2Micro,
        this.chkM2Reverse, this.numM2LowCurr, this.numM2HighCurr, this.numM2Backlash,
        this.numM2BacklashRate, this.chkM2Silent);

      // ---- Tab Rates ----
      this.tabRates.Text = "Rates";
      this.tabRates.Controls.Add(this.tlpRates);
      this.tabRates.Padding = new System.Windows.Forms.Padding(3);
      SetupRatesTable();

      // ---- Tab Limits ----
      this.tabLimits.Text = "Limits";
      this.tabLimits.Controls.Add(this.tlpLimits);
      this.tabLimits.Padding = new System.Windows.Forms.Padding(3);
      SetupLimitsTable();

      // ---- Tab Encoders ----
      this.tabEncoders.Text = "Encoders";
      this.tabEncoders.Controls.Add(this.tlpEncoders);
      this.tabEncoders.Padding = new System.Windows.Forms.Padding(3);
      SetupEncodersTable();

      // ---- Tab Site & Options ----
      this.tabSiteOptions.Text = "Site / Options";
      this.tabSiteOptions.Controls.Add(this.tlpSiteOptions);
      this.tabSiteOptions.Padding = new System.Windows.Forms.Padding(3);
      SetupSiteOptionsTable();

      // btnWrite
      this.btnWrite.Location = new System.Drawing.Point(300, 356);
      this.btnWrite.Size = new System.Drawing.Size(130, 26);
      this.btnWrite.Text = "Write to EEPROM";
      this.btnWrite.UseVisualStyleBackColor = true;
      this.btnWrite.Click += new System.EventHandler(this.btnWrite_Click);

      // TelescopeConfigEepromForm
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(444, 392);
      this.Controls.Add(this.tabControl);
      this.Controls.Add(this.btnWrite);
      this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
      this.MaximizeBox = false;
      this.MinimizeBox = false;
      this.Name = "TelescopeConfigEepromForm";
      this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
      this.Text = "Telescope EEPROM Configuration";
      this.tabControl.ResumeLayout(false);
      this.ResumeLayout(false);
    }

    private void SetupMotorTable(System.Windows.Forms.TableLayoutPanel tlp,
      System.Windows.Forms.NumericUpDown gear, System.Windows.Forms.NumericUpDown steps,
      System.Windows.Forms.ComboBox micro, System.Windows.Forms.CheckBox reverse,
      System.Windows.Forms.NumericUpDown lowCurr, System.Windows.Forms.NumericUpDown highCurr,
      System.Windows.Forms.NumericUpDown backlash, System.Windows.Forms.NumericUpDown backlashRate,
      System.Windows.Forms.CheckBox silent)
    {
      tlp.ColumnCount = 2;
      tlp.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
      tlp.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
      tlp.RowCount = 9;
      for (int i = 0; i < 9; i++)
        tlp.RowStyles.Add(new System.Windows.Forms.RowStyle());
      tlp.Dock = System.Windows.Forms.DockStyle.Fill;

      string[] labels = { "Gear Ratio", "Steps/Rotation", "Microstep", "Reverse", "Low Current (mA)",
        "High Current (mA)", "Backlash Amount", "Backlash Rate", "Silent/CoolStep" };
      System.Windows.Forms.Control[] controls = { gear, steps, micro, reverse, lowCurr, highCurr, backlash, backlashRate, silent };

      for (int i = 0; i < labels.Length; i++)
      {
        var lbl = new System.Windows.Forms.Label { Text = labels[i], Anchor = System.Windows.Forms.AnchorStyles.Left, AutoSize = true };
        tlp.Controls.Add(lbl, 0, i);
        tlp.Controls.Add(controls[i], 1, i);
      }

      gear.Maximum = 99999; gear.Minimum = 1; gear.Size = new System.Drawing.Size(100, 20);
      steps.Maximum = 99999; steps.Minimum = 1; steps.Size = new System.Drawing.Size(100, 20);
      micro.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      micro.Items.AddRange(new object[] { "1", "2", "4", "8", "16", "32", "64", "128", "256" });
      micro.Size = new System.Drawing.Size(100, 21);
      reverse.Anchor = System.Windows.Forms.AnchorStyles.Left; reverse.AutoSize = true;
      lowCurr.Maximum = 2500; lowCurr.Minimum = 0; lowCurr.Increment = 10; lowCurr.Size = new System.Drawing.Size(100, 20);
      highCurr.Maximum = 2500; highCurr.Minimum = 0; highCurr.Increment = 10; highCurr.Size = new System.Drawing.Size(100, 20);
      backlash.Maximum = 9999; backlash.Minimum = 0; backlash.Size = new System.Drawing.Size(100, 20);
      backlashRate.Maximum = 9999; backlashRate.Minimum = 0; backlashRate.Size = new System.Drawing.Size(100, 20);
      silent.Anchor = System.Windows.Forms.AnchorStyles.Left; silent.AutoSize = true;
    }

    private void SetupRatesTable()
    {
      this.tlpRates.ColumnCount = 2;
      this.tlpRates.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
      this.tlpRates.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
      this.tlpRates.RowCount = 7;
      for (int i = 0; i < 7; i++)
        this.tlpRates.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tlpRates.Dock = System.Windows.Forms.DockStyle.Fill;

      string[] labels = { "Guide Rate", "User Rate 1", "User Rate 2", "User Rate 3", "Max Slew Rate", "Default Rate (0-4)", "Acceleration (deg)" };
      System.Windows.Forms.NumericUpDown[] nums = { numGuideRate, numRate1, numRate2, numRate3, numMaxRate, numDefaultRate, numDegAcc };

      for (int i = 0; i < labels.Length; i++)
      {
        var lbl = new System.Windows.Forms.Label { Text = labels[i], Anchor = System.Windows.Forms.AnchorStyles.Left, AutoSize = true };
        this.tlpRates.Controls.Add(lbl, 0, i);
        nums[i].Size = new System.Drawing.Size(100, 20);
        this.tlpRates.Controls.Add(nums[i], 1, i);
      }
      numGuideRate.Maximum = 999; numRate1.Maximum = 9999; numRate2.Maximum = 9999; numRate3.Maximum = 9999;
      numMaxRate.Maximum = 9999; numDefaultRate.Maximum = 4; numDegAcc.Maximum = 999; numDegAcc.Minimum = 1;
    }

    private void SetupLimitsTable()
    {
      this.tlpLimits.ColumnCount = 2;
      this.tlpLimits.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
      this.tlpLimits.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
      this.tlpLimits.RowCount = 10;
      for (int i = 0; i < 10; i++)
        this.tlpLimits.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tlpLimits.Dock = System.Windows.Forms.DockStyle.Fill;

      string[] labels = { "Horizon Limit", "Overhead Limit", "Axis 1 Min", "Axis 1 Max", "Axis 2 Min", "Axis 2 Max",
        "Meridian East", "Meridian West", "Under-Pole", "Dist from Pole" };
      System.Windows.Forms.Control[] ctrls = { numHorizon, numOverhead, numAxis1Min, numAxis1Max, numAxis2Min, numAxis2Max,
        txtMeridianE, txtMeridianW, numUnderPole, txtDistFromPole };

      for (int i = 0; i < labels.Length; i++)
      {
        var lbl = new System.Windows.Forms.Label { Text = labels[i], Anchor = System.Windows.Forms.AnchorStyles.Left, AutoSize = true };
        this.tlpLimits.Controls.Add(lbl, 0, i);
        this.tlpLimits.Controls.Add(ctrls[i], 1, i);
      }
      numHorizon.Minimum = -30; numHorizon.Maximum = 30; numHorizon.Size = new System.Drawing.Size(100, 20);
      numOverhead.Minimum = 60; numOverhead.Maximum = 90; numOverhead.Value = 90; numOverhead.Size = new System.Drawing.Size(100, 20);
      numAxis1Min.Minimum = -9999; numAxis1Min.Maximum = 9999; numAxis1Min.Size = new System.Drawing.Size(100, 20);
      numAxis1Max.Minimum = -9999; numAxis1Max.Maximum = 9999; numAxis1Max.Size = new System.Drawing.Size(100, 20);
      numAxis2Min.Minimum = -9999; numAxis2Min.Maximum = 9999; numAxis2Min.Size = new System.Drawing.Size(100, 20);
      numAxis2Max.Minimum = -9999; numAxis2Max.Maximum = 9999; numAxis2Max.Size = new System.Drawing.Size(100, 20);
      txtMeridianE.Size = new System.Drawing.Size(100, 20);
      txtMeridianW.Size = new System.Drawing.Size(100, 20);
      numUnderPole.Maximum = 99; numUnderPole.Size = new System.Drawing.Size(100, 20);
      txtDistFromPole.Size = new System.Drawing.Size(100, 20);
    }

    private void SetupEncodersTable()
    {
      this.tlpEncoders.ColumnCount = 2;
      this.tlpEncoders.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
      this.tlpEncoders.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
      this.tlpEncoders.RowCount = 5;
      for (int i = 0; i < 5; i++)
        this.tlpEncoders.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tlpEncoders.Dock = System.Windows.Forms.DockStyle.Fill;

      string[] labels = { "Axis 1 Pulse/Deg", "Axis 1 Reverse", "Axis 2 Pulse/Deg", "Axis 2 Reverse", "Sync Mode" };
      System.Windows.Forms.Control[] ctrls = { numEnc1Pulse, chkEnc1Reverse, numEnc2Pulse, chkEnc2Reverse, numEncSyncMode };

      for (int i = 0; i < labels.Length; i++)
      {
        var lbl = new System.Windows.Forms.Label { Text = labels[i], Anchor = System.Windows.Forms.AnchorStyles.Left, AutoSize = true };
        this.tlpEncoders.Controls.Add(lbl, 0, i);
        this.tlpEncoders.Controls.Add(ctrls[i], 1, i);
      }
      numEnc1Pulse.Maximum = 99999; numEnc1Pulse.Size = new System.Drawing.Size(100, 20);
      chkEnc1Reverse.Anchor = System.Windows.Forms.AnchorStyles.Left; chkEnc1Reverse.AutoSize = true;
      numEnc2Pulse.Maximum = 99999; numEnc2Pulse.Size = new System.Drawing.Size(100, 20);
      chkEnc2Reverse.Anchor = System.Windows.Forms.AnchorStyles.Left; chkEnc2Reverse.AutoSize = true;
      numEncSyncMode.Maximum = 9; numEncSyncMode.Size = new System.Drawing.Size(100, 20);
    }

    private void SetupSiteOptionsTable()
    {
      this.tlpSiteOptions.ColumnCount = 2;
      this.tlpSiteOptions.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
      this.tlpSiteOptions.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
      this.tlpSiteOptions.RowCount = 8;
      for (int i = 0; i < 8; i++)
        this.tlpSiteOptions.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tlpSiteOptions.Dock = System.Windows.Forms.DockStyle.Fill;

      string[] labels = { "Mount Name", "Slew Settle (s)", "Refraction Goto", "Refraction Pole",
        "Refraction Tracking", "Latitude", "Longitude", "Elevation (m)" };
      System.Windows.Forms.Control[] ctrls = { txtMountName, numSlewSettle, chkRefrGoto, chkRefrPole,
        chkRefrTracking, txtLatitude, txtLongitude, numElevation };

      for (int i = 0; i < labels.Length; i++)
      {
        var lbl = new System.Windows.Forms.Label { Text = labels[i], Anchor = System.Windows.Forms.AnchorStyles.Left, AutoSize = true };
        this.tlpSiteOptions.Controls.Add(lbl, 0, i);
        this.tlpSiteOptions.Controls.Add(ctrls[i], 1, i);
      }
      txtMountName.Size = new System.Drawing.Size(140, 20);
      numSlewSettle.Maximum = 21; numSlewSettle.Size = new System.Drawing.Size(100, 20);
      chkRefrGoto.Anchor = System.Windows.Forms.AnchorStyles.Left; chkRefrGoto.AutoSize = true;
      chkRefrPole.Anchor = System.Windows.Forms.AnchorStyles.Left; chkRefrPole.AutoSize = true;
      chkRefrTracking.Anchor = System.Windows.Forms.AnchorStyles.Left; chkRefrTracking.AutoSize = true;
      txtLatitude.Size = new System.Drawing.Size(140, 20); txtLatitude.ReadOnly = true;
      txtLongitude.Size = new System.Drawing.Size(140, 20); txtLongitude.ReadOnly = true;
      numElevation.Minimum = -300; numElevation.Maximum = 10000; numElevation.Size = new System.Drawing.Size(100, 20);
    }

    #endregion

    private System.Windows.Forms.TabControl tabControl;
    private System.Windows.Forms.TabPage tabMotor1;
    private System.Windows.Forms.TabPage tabMotor2;
    private System.Windows.Forms.TabPage tabRates;
    private System.Windows.Forms.TabPage tabLimits;
    private System.Windows.Forms.TabPage tabEncoders;
    private System.Windows.Forms.TabPage tabSiteOptions;
    private System.Windows.Forms.Button btnWrite;

    private System.Windows.Forms.TableLayoutPanel tlpMotor1;
    private System.Windows.Forms.NumericUpDown numM1Gear;
    private System.Windows.Forms.NumericUpDown numM1Steps;
    private System.Windows.Forms.ComboBox cboM1Micro;
    private System.Windows.Forms.CheckBox chkM1Reverse;
    private System.Windows.Forms.NumericUpDown numM1LowCurr;
    private System.Windows.Forms.NumericUpDown numM1HighCurr;
    private System.Windows.Forms.NumericUpDown numM1Backlash;
    private System.Windows.Forms.NumericUpDown numM1BacklashRate;
    private System.Windows.Forms.CheckBox chkM1Silent;

    private System.Windows.Forms.TableLayoutPanel tlpMotor2;
    private System.Windows.Forms.NumericUpDown numM2Gear;
    private System.Windows.Forms.NumericUpDown numM2Steps;
    private System.Windows.Forms.ComboBox cboM2Micro;
    private System.Windows.Forms.CheckBox chkM2Reverse;
    private System.Windows.Forms.NumericUpDown numM2LowCurr;
    private System.Windows.Forms.NumericUpDown numM2HighCurr;
    private System.Windows.Forms.NumericUpDown numM2Backlash;
    private System.Windows.Forms.NumericUpDown numM2BacklashRate;
    private System.Windows.Forms.CheckBox chkM2Silent;

    private System.Windows.Forms.TableLayoutPanel tlpRates;
    private System.Windows.Forms.NumericUpDown numGuideRate;
    private System.Windows.Forms.NumericUpDown numRate1;
    private System.Windows.Forms.NumericUpDown numRate2;
    private System.Windows.Forms.NumericUpDown numRate3;
    private System.Windows.Forms.NumericUpDown numMaxRate;
    private System.Windows.Forms.NumericUpDown numDefaultRate;
    private System.Windows.Forms.NumericUpDown numDegAcc;

    private System.Windows.Forms.TableLayoutPanel tlpLimits;
    private System.Windows.Forms.NumericUpDown numHorizon;
    private System.Windows.Forms.NumericUpDown numOverhead;
    private System.Windows.Forms.NumericUpDown numAxis1Min;
    private System.Windows.Forms.NumericUpDown numAxis1Max;
    private System.Windows.Forms.NumericUpDown numAxis2Min;
    private System.Windows.Forms.NumericUpDown numAxis2Max;
    private System.Windows.Forms.TextBox txtMeridianE;
    private System.Windows.Forms.TextBox txtMeridianW;
    private System.Windows.Forms.NumericUpDown numUnderPole;
    private System.Windows.Forms.TextBox txtDistFromPole;

    private System.Windows.Forms.TableLayoutPanel tlpEncoders;
    private System.Windows.Forms.NumericUpDown numEnc1Pulse;
    private System.Windows.Forms.CheckBox chkEnc1Reverse;
    private System.Windows.Forms.NumericUpDown numEnc2Pulse;
    private System.Windows.Forms.CheckBox chkEnc2Reverse;
    private System.Windows.Forms.NumericUpDown numEncSyncMode;

    private System.Windows.Forms.TableLayoutPanel tlpSiteOptions;
    private System.Windows.Forms.TextBox txtMountName;
    private System.Windows.Forms.NumericUpDown numSlewSettle;
    private System.Windows.Forms.CheckBox chkRefrGoto;
    private System.Windows.Forms.CheckBox chkRefrPole;
    private System.Windows.Forms.CheckBox chkRefrTracking;
    private System.Windows.Forms.TextBox txtLatitude;
    private System.Windows.Forms.TextBox txtLongitude;
    private System.Windows.Forms.NumericUpDown numElevation;
  }
}

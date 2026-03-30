using System;
using System.Globalization;
using System.Windows.Forms;

namespace ASCOM.TeenAstro.Telescope
{
  public partial class TelescopeConfigEepromForm : Form
  {
    private TelescopeHardware.MotorAxisSettings cachedM1, cachedM2;
    private TelescopeHardware.RateSettings cachedRates;
    private TelescopeHardware.LimitSettings cachedLimits;
    private TelescopeHardware.EncoderSettings cachedEncoders;
    private TelescopeHardware.RefractionSettings cachedRefraction;
    private TelescopeHardware.SiteOptionSettings cachedSiteOptions;

    public TelescopeConfigEepromForm()
    {
      InitializeComponent();
    }

    protected override void OnLoad(EventArgs e)
    {
      base.OnLoad(e);
      try
      {
        ReadAllFromHardware();
      }
      catch (Exception ex)
      {
        MessageBox.Show("Failed to read telescope EEPROM: " + ex.Message, "Error",
          MessageBoxButtons.OK, MessageBoxIcon.Error);
        Close();
      }
    }

    private void ReadAllFromHardware()
    {
      cachedM1 = TelescopeHardware.ReadMotorSettings(1);
      cachedM2 = TelescopeHardware.ReadMotorSettings(2);
      cachedRates = TelescopeHardware.ReadRateSettings();
      cachedLimits = TelescopeHardware.ReadLimitSettings();
      cachedEncoders = TelescopeHardware.ReadEncoderSettings();
      cachedRefraction = TelescopeHardware.ReadRefractionSettings();
      cachedSiteOptions = TelescopeHardware.ReadSiteOptionSettings();
      PopulateAll();
    }

    private void PopulateAll()
    {
      PopulateMotor(cachedM1, numM1Gear, numM1Steps, cboM1Micro, chkM1Reverse,
        numM1LowCurr, numM1HighCurr, numM1Backlash, numM1BacklashRate, chkM1Silent);
      PopulateMotor(cachedM2, numM2Gear, numM2Steps, cboM2Micro, chkM2Reverse,
        numM2LowCurr, numM2HighCurr, numM2Backlash, numM2BacklashRate, chkM2Silent);
      PopulateRates();
      PopulateLimits();
      PopulateEncoders();
      PopulateRefraction();
      PopulateSiteOptions();
    }

    private static void PopulateMotor(TelescopeHardware.MotorAxisSettings m,
      NumericUpDown gear, NumericUpDown steps, ComboBox micro, CheckBox reverse,
      NumericUpDown lowCurr, NumericUpDown highCurr, NumericUpDown backlash,
      NumericUpDown backlashRate, CheckBox silent)
    {
      gear.Value = Clamp(m.GearRatio, gear.Minimum, gear.Maximum);
      steps.Value = Clamp(m.StepsPerRotation, steps.Minimum, steps.Maximum);
      // MotorAxisSettings.Microstep holds the actual microstep count (8,16,32,…). Select matching item.
      SelectMicrostep(micro, m.Microstep);
      reverse.Checked = m.Reverse;
      lowCurr.Value = Clamp(m.LowCurrent, lowCurr.Minimum, lowCurr.Maximum);
      highCurr.Value = Clamp(m.HighCurrent, highCurr.Minimum, highCurr.Maximum);
      backlash.Value = Clamp(m.BacklashAmount, backlash.Minimum, backlash.Maximum);
      backlashRate.Value = Clamp(m.BacklashRate, backlashRate.Minimum, backlashRate.Maximum);
      silent.Checked = m.Silent;
    }

    private void PopulateRates()
    {
      // Guide rate is stored as multiple of sidereal (0.01 resolution). UI shows value in "x sidereal".
      decimal g = (decimal)cachedRates.GuideRate;
      if (g < numGuideRate.Minimum) g = numGuideRate.Minimum;
      if (g > numGuideRate.Maximum) g = numGuideRate.Maximum;
      numGuideRate.Value = g;
      numRate1.Value = Clamp(cachedRates.Rate1, numRate1.Minimum, numRate1.Maximum);
      numRate2.Value = Clamp(cachedRates.Rate2, numRate2.Minimum, numRate2.Maximum);
      numRate3.Value = Clamp(cachedRates.Rate3, numRate3.Minimum, numRate3.Maximum);
      numMaxRate.Value = Clamp(cachedRates.MaxRate, numMaxRate.Minimum, numMaxRate.Maximum);
      numDefaultRate.Value = Clamp(cachedRates.DefaultRate, numDefaultRate.Minimum, numDefaultRate.Maximum);
      numDegAcc.Value = Clamp(cachedRates.DegAcc, numDegAcc.Minimum, numDegAcc.Maximum);
    }

    private void PopulateLimits()
    {
      numHorizon.Value = Clamp(cachedLimits.Horizon, numHorizon.Minimum, numHorizon.Maximum);
      numOverhead.Value = Clamp(cachedLimits.Overhead, numOverhead.Minimum, numOverhead.Maximum);
      numAxis1Min.Value = Clamp(cachedLimits.Axis1Min, numAxis1Min.Minimum, numAxis1Min.Maximum);
      numAxis1Max.Value = Clamp(cachedLimits.Axis1Max, numAxis1Max.Minimum, numAxis1Max.Maximum);
      numAxis2Min.Value = Clamp(cachedLimits.Axis2Min, numAxis2Min.Minimum, numAxis2Min.Maximum);
      numAxis2Max.Value = Clamp(cachedLimits.Axis2Max, numAxis2Max.Minimum, numAxis2Max.Maximum);
      txtMeridianE.Text = cachedLimits.MeridianE;
      txtMeridianW.Text = cachedLimits.MeridianW;
      numUnderPole.Value = Clamp(cachedLimits.UnderPole, numUnderPole.Minimum, numUnderPole.Maximum);
      txtDistFromPole.Text = cachedLimits.DistFromPole;
    }

    private void PopulateEncoders()
    {
      numEnc1Pulse.Value = Clamp(cachedEncoders.Axis1PulseDeg, numEnc1Pulse.Minimum, numEnc1Pulse.Maximum);
      chkEnc1Reverse.Checked = cachedEncoders.Axis1Reverse;
      numEnc2Pulse.Value = Clamp(cachedEncoders.Axis2PulseDeg, numEnc2Pulse.Minimum, numEnc2Pulse.Maximum);
      chkEnc2Reverse.Checked = cachedEncoders.Axis2Reverse;
      numEncSyncMode.Value = Clamp(cachedEncoders.SyncMode, numEncSyncMode.Minimum, numEncSyncMode.Maximum);
    }

    private void PopulateRefraction()
    {
      chkRefrGoto.Checked = cachedRefraction.Goto;
      chkRefrPole.Checked = cachedRefraction.Pole;
      chkRefrTracking.Checked = cachedRefraction.Tracking;
    }

    private void PopulateSiteOptions()
    {
      txtMountName.Text = cachedSiteOptions.MountName;
      numSlewSettle.Value = Clamp(cachedSiteOptions.SlewSettleDuration, numSlewSettle.Minimum, numSlewSettle.Maximum);
      txtLatitude.Text = cachedSiteOptions.Latitude;
      txtLongitude.Text = cachedSiteOptions.Longitude;
      numElevation.Value = Clamp((int)cachedSiteOptions.Elevation, numElevation.Minimum, numElevation.Maximum);
    }

    private void btnWrite_Click(object sender, EventArgs e)
    {
      try
      {
        WriteMotorChanges(1, cachedM1, numM1Gear, numM1Steps, cboM1Micro, chkM1Reverse,
          numM1LowCurr, numM1HighCurr, numM1Backlash, numM1BacklashRate, chkM1Silent);
        WriteMotorChanges(2, cachedM2, numM2Gear, numM2Steps, cboM2Micro, chkM2Reverse,
          numM2LowCurr, numM2HighCurr, numM2Backlash, numM2BacklashRate, chkM2Silent);
        WriteRateChanges();
        WriteLimitChanges();
        WriteEncoderChanges();
        WriteRefractionChanges();
        WriteSiteOptionChanges();

        ReadAllFromHardware();
      }
      catch (Exception ex)
      {
        MessageBox.Show("Write failed: " + ex.Message, "Error",
          MessageBoxButtons.OK, MessageBoxIcon.Error);
      }
    }

    private static void WriteMotorChanges(int axis, TelescopeHardware.MotorAxisSettings cached,
      NumericUpDown gear, NumericUpDown steps, ComboBox micro, CheckBox reverse,
      NumericUpDown lowCurr, NumericUpDown highCurr, NumericUpDown backlash,
      NumericUpDown backlashRate, CheckBox silent)
    {
      int uiGear = (int)gear.Value;
      if (uiGear != cached.GearRatio)
      {
        // Protocol stores gear as 1000 * ratio
        int rawGear = uiGear * 1000;
        TelescopeHardware.WriteMotorSetting(axis, "G", rawGear.ToString(CultureInfo.InvariantCulture));
      }
      if ((int)steps.Value != cached.StepsPerRotation)
        TelescopeHardware.WriteMotorSetting(axis, "S", ((int)steps.Value).ToString(CultureInfo.InvariantCulture));
      int microSteps = GetMicrostepValue(micro); // 8,16,32,...
      // Convert back to exponent 0–8 used by firmware: micro = log2(steps)
      int microExp = 0;
      if (microSteps > 0)
        microExp = (int)Math.Round(Math.Log(microSteps, 2.0), MidpointRounding.AwayFromZero);
      if (microExp != cached.Microstep)
        TelescopeHardware.WriteMotorSetting(axis, "M", microExp.ToString(CultureInfo.InvariantCulture));
      if (reverse.Checked != cached.Reverse)
        // Reverse rotation uses :GXMRR# / :GXMRD# (uppercase 'R' in MRR/MRD).
        TelescopeHardware.WriteMotorSetting(axis, "R", reverse.Checked ? "1" : "0");
      if ((int)lowCurr.Value != cached.LowCurrent)
        TelescopeHardware.WriteMotorSetting(axis, "c", ((int)lowCurr.Value).ToString(CultureInfo.InvariantCulture));
      if ((int)highCurr.Value != cached.HighCurrent)
        TelescopeHardware.WriteMotorSetting(axis, "C", ((int)highCurr.Value).ToString(CultureInfo.InvariantCulture));
      if ((int)backlash.Value != cached.BacklashAmount)
        TelescopeHardware.WriteMotorSetting(axis, "B", ((int)backlash.Value).ToString(CultureInfo.InvariantCulture));
      if ((int)backlashRate.Value != cached.BacklashRate)
        TelescopeHardware.WriteMotorSetting(axis, "b", ((int)backlashRate.Value).ToString(CultureInfo.InvariantCulture));
      if (silent.Checked != cached.Silent)
        TelescopeHardware.WriteMotorSetting(axis, "m", silent.Checked ? "1" : "0");
    }

    private void WriteRateChanges()
    {
      double uiGuideRate = (double)numGuideRate.Value;
      if (Math.Abs(uiGuideRate - cachedRates.GuideRate) > 1e-4)
      {
        // :SXR0,NNN# where NNN = guideRate * 100 (0.01 resolution)
        int encoded = (int)Math.Round(uiGuideRate * 100.0);
        TelescopeHardware.WriteRateSetting("0", encoded.ToString("000", CultureInfo.InvariantCulture));
      }
      if ((int)numRate1.Value != cachedRates.Rate1)
        TelescopeHardware.WriteRateSetting("1", ((int)numRate1.Value).ToString(CultureInfo.InvariantCulture));
      if ((int)numRate2.Value != cachedRates.Rate2)
        TelescopeHardware.WriteRateSetting("2", ((int)numRate2.Value).ToString(CultureInfo.InvariantCulture));
      if ((int)numRate3.Value != cachedRates.Rate3)
        TelescopeHardware.WriteRateSetting("3", ((int)numRate3.Value).ToString(CultureInfo.InvariantCulture));
      if ((int)numMaxRate.Value != cachedRates.MaxRate)
        TelescopeHardware.WriteRateSetting("X", ((int)numMaxRate.Value).ToString(CultureInfo.InvariantCulture));
      if ((int)numDefaultRate.Value != cachedRates.DefaultRate)
        TelescopeHardware.WriteRateSetting("D", ((int)numDefaultRate.Value).ToString(CultureInfo.InvariantCulture));
      if ((int)numDegAcc.Value != cachedRates.DegAcc)
        TelescopeHardware.WriteRateSetting("A", ((int)numDegAcc.Value).ToString(CultureInfo.InvariantCulture));
    }

    private void WriteLimitChanges()
    {
      if ((int)numHorizon.Value != cachedLimits.Horizon)
      {
        int v = (int)numHorizon.Value;
        string sv = (v >= 0 ? "+" : "") + v.ToString(CultureInfo.InvariantCulture);
        TelescopeHardware.WriteLimitSetting("H", sv);
      }
      if ((int)numOverhead.Value != cachedLimits.Overhead)
        TelescopeHardware.WriteLimitSetting("O", ((int)numOverhead.Value).ToString(CultureInfo.InvariantCulture));
      // Axis user limits are edited in degrees but stored as tenths of a degree.
      if ((int)numAxis1Min.Value != cachedLimits.Axis1Min)
        TelescopeHardware.WriteLimitSetting("A", (((int)numAxis1Min.Value) * 10).ToString(CultureInfo.InvariantCulture));
      if ((int)numAxis1Max.Value != cachedLimits.Axis1Max)
        TelescopeHardware.WriteLimitSetting("B", (((int)numAxis1Max.Value) * 10).ToString(CultureInfo.InvariantCulture));
      if ((int)numAxis2Min.Value != cachedLimits.Axis2Min)
        TelescopeHardware.WriteLimitSetting("C", (((int)numAxis2Min.Value) * 10).ToString(CultureInfo.InvariantCulture));
      if ((int)numAxis2Max.Value != cachedLimits.Axis2Max)
        TelescopeHardware.WriteLimitSetting("D", (((int)numAxis2Max.Value) * 10).ToString(CultureInfo.InvariantCulture));
      if (txtMeridianE.Text != cachedLimits.MeridianE)
      {
        // UI uses degrees of hour angle; firmware stores minutes past meridian.
        if (int.TryParse(txtMeridianE.Text, NumberStyles.Any, CultureInfo.InvariantCulture, out int degE))
        {
          int minutesE = (int)Math.Round((degE * 60.0) / 15.0);
          TelescopeHardware.WriteLimitSetting("E", minutesE.ToString(CultureInfo.InvariantCulture));
        }
      }
      if (txtMeridianW.Text != cachedLimits.MeridianW)
      {
        if (int.TryParse(txtMeridianW.Text, NumberStyles.Any, CultureInfo.InvariantCulture, out int degW))
        {
          int minutesW = (int)Math.Round((degW * 60.0) / 15.0);
          TelescopeHardware.WriteLimitSetting("W", minutesW.ToString(CultureInfo.InvariantCulture));
        }
      }
      if ((int)numUnderPole.Value != cachedLimits.UnderPole)
      {
        // Under-pole is edited in degrees but stored as ×10.
        int vU = (int)numUnderPole.Value;
        TelescopeHardware.WriteLimitSetting("U", (vU * 10).ToString(CultureInfo.InvariantCulture));
      }
      if (txtDistFromPole.Text != cachedLimits.DistFromPole)
        TelescopeHardware.WriteLimitSetting("S", txtDistFromPole.Text);
    }

    private void WriteEncoderChanges()
    {
      if ((int)numEnc1Pulse.Value != cachedEncoders.Axis1PulseDeg)
        TelescopeHardware.WriteEncoderSetting("PR", ((int)numEnc1Pulse.Value).ToString(CultureInfo.InvariantCulture));
      if (chkEnc1Reverse.Checked != cachedEncoders.Axis1Reverse)
        TelescopeHardware.WriteEncoderSetting("rR", chkEnc1Reverse.Checked ? "1" : "0");
      if ((int)numEnc2Pulse.Value != cachedEncoders.Axis2PulseDeg)
        TelescopeHardware.WriteEncoderSetting("PD", ((int)numEnc2Pulse.Value).ToString(CultureInfo.InvariantCulture));
      if (chkEnc2Reverse.Checked != cachedEncoders.Axis2Reverse)
        TelescopeHardware.WriteEncoderSetting("rD", chkEnc2Reverse.Checked ? "1" : "0");
      if ((int)numEncSyncMode.Value != cachedEncoders.SyncMode)
        TelescopeHardware.WriteEncoderSetting("O", ((int)numEncSyncMode.Value).ToString(CultureInfo.InvariantCulture));
    }

    private void WriteRefractionChanges()
    {
      if (chkRefrGoto.Checked != cachedRefraction.Goto)
        TelescopeHardware.WriteRefractionSetting("g", chkRefrGoto.Checked ? "y" : "n");
      if (chkRefrPole.Checked != cachedRefraction.Pole)
        TelescopeHardware.WriteRefractionSetting("p", chkRefrPole.Checked ? "y" : "n");
      if (chkRefrTracking.Checked != cachedRefraction.Tracking)
        TelescopeHardware.WriteRefractionSetting("t", chkRefrTracking.Checked ? "y" : "n");
    }

    private void WriteSiteOptionChanges()
    {
      if (txtMountName.Text != cachedSiteOptions.MountName)
        TelescopeHardware.WriteSiteOptionSetting("OA", txtMountName.Text);
      if ((int)numSlewSettle.Value != cachedSiteOptions.SlewSettleDuration)
        TelescopeHardware.WriteSiteOptionSetting("OS", ((int)numSlewSettle.Value).ToString(CultureInfo.InvariantCulture));
      if ((int)numElevation.Value != (int)cachedSiteOptions.Elevation)
      {
        int el = (int)numElevation.Value;
        string sg = el >= 0 ? "+" : "-";
        string cmd = "e" + sg + Math.Abs(el).ToString("00000", CultureInfo.InvariantCulture);
        TelescopeHardware.WriteSiteOptionSetting(cmd, "");
      }
    }

    private static void SelectMicrostep(ComboBox cbo, int value)
    {
      string sv = value.ToString(CultureInfo.InvariantCulture);
      for (int i = 0; i < cbo.Items.Count; i++)
      {
        if (cbo.Items[i].ToString() == sv)
        {
          cbo.SelectedIndex = i;
          return;
        }
      }
      cbo.SelectedIndex = 0;
    }

    private static int GetMicrostepValue(ComboBox cbo)
    {
      if (cbo.SelectedItem != null && int.TryParse(cbo.SelectedItem.ToString(), out int v))
        return v;
      return 1;
    }

    private static decimal Clamp(int value, decimal min, decimal max)
    {
      if (value < min) return min;
      if (value > max) return max;
      return value;
    }
  }
}

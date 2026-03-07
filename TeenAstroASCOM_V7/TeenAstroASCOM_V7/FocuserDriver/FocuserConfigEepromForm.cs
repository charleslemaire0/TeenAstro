using System;
using System.Windows.Forms;

namespace ASCOM.TeenAstro.Focuser
{
  public partial class FocuserConfigEepromForm : Form
  {
    private FocuserHardware.FocuserEepromSettings cached;

    public FocuserConfigEepromForm()
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
        MessageBox.Show("Failed to read EEPROM: " + ex.Message, "Error",
          MessageBoxButtons.OK, MessageBoxIcon.Error);
        Close();
      }
    }

    private void ReadAllFromHardware()
    {
      cached = FocuserHardware.ReadEepromSettings();
      FocuserHardware.ReadEepromMotor(cached);
      PopulateControls();
    }

    private void PopulateControls()
    {
      numParkPos.Value = cached.ParkPos;
      numMaxPos.Value = cached.MaxPos;
      numMinSpeed.Value = Math.Max(cached.MinSpeed, numMinSpeed.Minimum);
      numMaxSpeed.Value = Math.Max(cached.MaxSpeed, numMaxSpeed.Minimum);
      numCmdAcc.Value = Math.Max(cached.CmdAcc, numCmdAcc.Minimum);
      numManAcc.Value = Math.Max(cached.ManAcc, numManAcc.Minimum);
      chkReverse.Checked = cached.Reverse;
      numResolution.Value = Math.Max(cached.Resolution, numResolution.Minimum);

      int microIndex = cached.Micro - 2;
      if (microIndex >= 0 && microIndex < cboMicro.Items.Count)
        cboMicro.SelectedIndex = microIndex;
      else
        cboMicro.SelectedIndex = 0;

      numCurrent.Value = Math.Max(cached.Current * 10, numCurrent.Minimum);
    }

    private void btnWrite_Click(object sender, EventArgs e)
    {
      try
      {
        if ((ushort)numParkPos.Value != cached.ParkPos)
          FocuserHardware.WriteEepromValue("0", (ushort)numParkPos.Value);

        if ((ushort)numMaxPos.Value != cached.MaxPos)
          FocuserHardware.WriteEepromValue("1", (ushort)numMaxPos.Value);

        if ((ushort)numMinSpeed.Value != cached.MinSpeed)
          FocuserHardware.WriteEepromValue("2", (ushort)numMinSpeed.Value);

        if ((ushort)numMaxSpeed.Value != cached.MaxSpeed)
          FocuserHardware.WriteEepromValue("3", (ushort)numMaxSpeed.Value);

        if ((ushort)numCmdAcc.Value != cached.CmdAcc)
          FocuserHardware.WriteEepromValue("4", (ushort)numCmdAcc.Value);

        if ((ushort)numManAcc.Value != cached.ManAcc)
          FocuserHardware.WriteEepromValue("5", (ushort)numManAcc.Value);

        bool newReverse = chkReverse.Checked;
        if (newReverse != cached.Reverse)
          FocuserHardware.WriteEepromValue("7", (ushort)(newReverse ? 1 : 0));

        if ((ushort)numResolution.Value != cached.Resolution)
          FocuserHardware.WriteEepromValue("8", (ushort)numResolution.Value);

        ushort newCurrentDiv10 = (ushort)((int)numCurrent.Value / 10);
        if (newCurrentDiv10 != cached.Current)
          FocuserHardware.WriteEepromValue("c", newCurrentDiv10);

        int newMicroIndex = cboMicro.SelectedIndex + 2;
        if (newMicroIndex != cached.Micro)
          FocuserHardware.WriteEepromValue("m", (ushort)newMicroIndex);

        ReadAllFromHardware();
      }
      catch (Exception ex)
      {
        MessageBox.Show("Write failed: " + ex.Message, "Error",
          MessageBoxButtons.OK, MessageBoxIcon.Error);
      }
    }
  }
}

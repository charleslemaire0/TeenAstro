using ASCOM.Utilities;
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace ASCOM.TeenAstro.Focuser
{
  [ComVisible(false)]
  public partial class FocuserSetupDialogForm : Form
  {
    const string NO_PORTS_MESSAGE = "No COM ports found";
    private readonly TraceLogger tl;

    public FocuserSetupDialogForm(TraceLogger tlDriver)
    {
      InitializeComponent();
      tl = tlDriver;
      InitUI();
    }

    private void CmdCancel_Click(object sender, EventArgs e)
    {
      Close();
    }

    private void BrowseToAscom(object sender, EventArgs e)
    {
      try { Process.Start("https://ascom-standards.org/"); }
      catch (Win32Exception ex) { MessageBox.Show(ex.Message); }
      catch (Exception ex) { MessageBox.Show(ex.Message); }
    }

    private void InitUI()
    {
      var assembly = System.Reflection.Assembly.GetExecutingAssembly();
      var fvi = FileVersionInfo.GetVersionInfo(assembly.Location);
      this.Text = "TeenAstro Focuser " + fvi.FileVersion;

      chkTrace.Checked = tl.Enabled;
      TextBoxIP.Text = FocuserHardware.IP;
      TextBoxPort.Text = FocuserHardware.Port.ToString();
      RadioButtonSP.Checked = FocuserHardware.Interface == "COM";
      RadioButtonIP.Checked = FocuserHardware.Interface == "IP";

      comboBoxComPort.Items.Clear();
      using (var serial = new Serial())
        comboBoxComPort.Items.AddRange(serial.AvailableCOMPorts);
      if (comboBoxComPort.Items.Count == 0)
      {
        comboBoxComPort.Items.Add(NO_PORTS_MESSAGE);
        comboBoxComPort.SelectedItem = NO_PORTS_MESSAGE;
      }
      if (comboBoxComPort.Items.Contains(FocuserHardware.comPort))
        comboBoxComPort.SelectedItem = FocuserHardware.comPort;

      tl?.LogMessage("InitUI", $"Trace: {chkTrace.Checked}, COM: {comboBoxComPort.SelectedItem}");
    }

    private void OK_Button_Click(object sender, EventArgs e)
    {
      tl.Enabled = chkTrace.Checked;
      if (RadioButtonSP.Checked)
      {
        if (comboBoxComPort.SelectedItem != null && comboBoxComPort.SelectedItem.ToString() != NO_PORTS_MESSAGE)
        {
          FocuserHardware.comPort = comboBoxComPort.SelectedItem.ToString();
          FocuserHardware.Interface = "COM";
        }
      }
      else if (RadioButtonIP.Checked)
      {
        FocuserHardware.IP = TextBoxIP.Text;
        if (short.TryParse(TextBoxPort.Text, out short p))
          FocuserHardware.Port = p;
        FocuserHardware.Interface = "IP";
      }
      DialogResult = DialogResult.OK;
      Close();
    }

    private void Cancel_Button_Click(object sender, EventArgs e)
    {
      Close();
    }

    private void RadioButtonIP_CheckedChanged(object sender, EventArgs e)
    {
      TextBoxIP.Enabled = RadioButtonIP.Checked;
      TextBoxPort.Enabled = RadioButtonIP.Checked;
    }

    private void RadioButtonSP_CheckedChanged(object sender, EventArgs e)
    {
      comboBoxComPort.Enabled = RadioButtonSP.Checked;
    }

    private void TextBoxPort_KeyPress(object sender, KeyPressEventArgs e)
    {
      if (!char.IsDigit(e.KeyChar) && !char.IsControl(e.KeyChar))
        e.Handled = true;
    }

    private void btnOptions_Click(object sender, EventArgs e)
    {
      SaveCurrentState();
      FocuserHardware.WriteProfile();

      Guid tempId = Guid.NewGuid();
      bool wasConnected = FocuserHardware.IsConnected;
      try
      {
        if (!wasConnected)
          FocuserHardware.SetConnected(tempId, true);

        using (var form = new FocuserConfigEepromForm())
          form.ShowDialog(this);
      }
      catch (Exception ex)
      {
        MessageBox.Show("Connection failed: " + ex.Message, "Error",
          MessageBoxButtons.OK, MessageBoxIcon.Error);
      }
      finally
      {
        if (!wasConnected)
          FocuserHardware.SetConnected(tempId, false);
      }
    }

    private void SaveCurrentState()
    {
      tl.Enabled = chkTrace.Checked;
      if (RadioButtonSP.Checked)
      {
        if (comboBoxComPort.SelectedItem != null && comboBoxComPort.SelectedItem.ToString() != NO_PORTS_MESSAGE)
        {
          FocuserHardware.comPort = comboBoxComPort.SelectedItem.ToString();
          FocuserHardware.Interface = "COM";
        }
      }
      else if (RadioButtonIP.Checked)
      {
        FocuserHardware.IP = TextBoxIP.Text;
        if (short.TryParse(TextBoxPort.Text, out short p))
          FocuserHardware.Port = p;
        FocuserHardware.Interface = "IP";
      }
    }
  }
}

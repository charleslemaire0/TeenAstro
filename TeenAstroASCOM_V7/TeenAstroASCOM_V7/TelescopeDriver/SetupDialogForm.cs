using ASCOM.Utilities;
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace ASCOM.TeenAstro.Telescope
{
  [ComVisible(false)] // Form not registered for COM!
  public partial class SetupDialogForm : Form
  {
    const string NO_PORTS_MESSAGE = "No COM ports found";
    TraceLogger tl; // Holder for a reference to the driver's trace logger

    public SetupDialogForm(TraceLogger tlDriver)
    {
      InitializeComponent();

      // Save the provided trace logger for use within the setup dialogue
      tl = tlDriver;

      // Initialise current values of user settings from the ASCOM Profile
      InitUI();
    }


    private void CmdCancel_Click(object sender, EventArgs e) // Cancel button event handler
    {
      Close();
    }

    private void BrowseToAscom(object sender, EventArgs e) // Click on ASCOM logo event handler
    {
      try
      {
        System.Diagnostics.Process.Start("https://ascom-standards.org/");
      }
      catch (Win32Exception noBrowser)
      {
        if (noBrowser.ErrorCode == -2147467259)
          MessageBox.Show(noBrowser.Message);
      }
      catch (Exception other)
      {
        MessageBox.Show(other.Message);
      }
    }

    private void InitUI()
    {

      System.Reflection.Assembly assembly = System.Reflection.Assembly.GetExecutingAssembly();
      FileVersionInfo fvi = FileVersionInfo.GetVersionInfo(assembly.Location);
      String version = fvi.FileVersion;
      this.Text = "TeenAstro Telescope " + version;

      // Set the trace checkbox
      chkTrace.Checked = tl.Enabled;
      TextBoxIP.Text = TelescopeHardware.IP;
      TextBoxPort.Text = TelescopeHardware.Port.ToString();
      RadioButtonSP.Checked = TelescopeHardware.Interface == "COM";
      RadioButtonIP.Checked = TelescopeHardware.Interface == "IP";
      // set the list of COM ports to those that are currently available
      comboBoxComPort.Items.Clear(); // Clear any existing entries
      using (Serial serial = new Serial()) // User the Se5rial component to get an extended list of COM ports
      {
        comboBoxComPort.Items.AddRange(serial.AvailableCOMPorts);
      }

      // If no ports are found include a message to this effect
      if (comboBoxComPort.Items.Count == 0)
      {
        comboBoxComPort.Items.Add(NO_PORTS_MESSAGE);
        comboBoxComPort.SelectedItem = NO_PORTS_MESSAGE;
      }

      // select the current port if possible
      if (comboBoxComPort.Items.Contains(TelescopeHardware.comPort))
      {
        comboBoxComPort.SelectedItem = TelescopeHardware.comPort;
      }

      tl.LogMessage("InitUI", $"Set UI controls to Trace: {chkTrace.Checked}, COM Port: {comboBoxComPort.SelectedItem}");
    }

    private void SetupDialogForm_Load(object sender, EventArgs e)
    {
      // Bring the setup dialogue to the front of the screen
      if (WindowState == FormWindowState.Minimized)
        WindowState = FormWindowState.Normal;
      else
      {
        TopMost = true;
        Focus();
        BringToFront();
        TopMost = false;
      }
    }

    private void OK_Button_Click(object sender, EventArgs e)
    {
      // Place any validation constraint checks here and update the state variables with results from the dialogue

      tl.Enabled = chkTrace.Checked;
      if (RadioButtonSP.Checked)
      {
        // Update the COM port variable if one has been selected
        if (comboBoxComPort.SelectedItem is null) // No COM port selected
        {
          tl.LogMessage("Setup OK", $"New configuration values - COM Port: Not selected");
        }
        else if (comboBoxComPort.SelectedItem.ToString() == NO_PORTS_MESSAGE)
        {
          tl.LogMessage("Setup OK", $"New configuration values - NO COM ports detected on this PC.");
        }
        else // A valid COM port has been selected
        {
          tl.LogMessage("Setup OK", $"New configuration values - COM Port: {comboBoxComPort.SelectedItem}");
          TelescopeHardware.comPort = comboBoxComPort.SelectedItem.ToString();
          TelescopeHardware.Interface = "COM";
        }
      }
      else if (RadioButtonIP.Checked)
      {
        TelescopeHardware.IP = TextBoxIP.Text;
        TelescopeHardware.Port = Convert.ToInt16(TextBoxPort.Text);
        TelescopeHardware.Interface = "IP";
      }
      this.DialogResult = DialogResult.OK;
      this.Close();
    }

    private void PictureBoxTeenAstro_Click(object sender, EventArgs e)
    {

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
      if (!char.IsDigit(e.KeyChar) & !char.IsControl(e.KeyChar))
      {
        e.Handled = true;
      }
    }
    private void chkTrace_CheckedChanged(object sender, EventArgs e)
    {

    }

    private void Cancel_Button_Click(object sender, EventArgs e)
    {
      this.Close();
    }
  }
}
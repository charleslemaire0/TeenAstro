Imports System.Windows.Forms
Imports System.Runtime.InteropServices
Imports ASCOM.Utilities
Imports ASCOM.TeenAstro

<ComVisible(False)>
Public Class SetupDialogForm

  Private Sub OK_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles OK_Button.Click ' OK button event handler
    ' Persist new values of user settings to the ASCOM profile
    If RadioButtonSP.Checked Then
      Telescope.mcomPort = ComboBoxComPort.SelectedItem
      Telescope.mInterface = "COM"
    ElseIf RadioButtonIP.Checked Then
      Telescope.mIP = TextBoxIP.Text
      Telescope.mPort = TextBoxPort.Text
      Telescope.mInterface = "IP"

      If Not System.Net.IPAddress.TryParse(Telescope.mIP, Telescope.mobjectIP) Then
        MsgBox(Telescope.mIP + " is not a valid IP Address")
        Return
      End If
    End If
    Telescope.mtraceState = chkTrace.Checked
    Me.DialogResult = System.Windows.Forms.DialogResult.OK
    Me.Close()
  End Sub

  Private Sub Cancel_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Cancel_Button.Click 'Cancel button event handler
    Me.DialogResult = System.Windows.Forms.DialogResult.Cancel
    Me.Close()
  End Sub

  Private Sub ShowAscomWebPage(ByVal sender As System.Object, ByVal e As System.EventArgs)
    ' Click on ASCOM logo event handler
    Try
      System.Diagnostics.Process.Start("http://ascom-standards.org/")
    Catch noBrowser As System.ComponentModel.Win32Exception
      If noBrowser.ErrorCode = -2147467259 Then
        MessageBox.Show(noBrowser.Message)
      End If
    Catch other As System.Exception
      MessageBox.Show(other.Message)
    End Try
  End Sub

  Private Sub SetupDialogForm_Load(sender As System.Object, e As System.EventArgs) Handles MyBase.Load ' Form load event handler
    ' Retrieve current values of user settings from the ASCOM Profile
    InitUI()
  End Sub

  Private Sub InitUI()

    Dim assembly As System.Reflection.Assembly = System.Reflection.Assembly.GetExecutingAssembly()
    Dim fvi As FileVersionInfo = FileVersionInfo.GetVersionInfo(assembly.Location)
    Dim version As String = fvi.FileVersion
    Me.Text = "TeenAstro Telescope " + version
    chkTrace.Checked = Telescope.mtraceState
    TextBoxIP.Text = Telescope.mIP
    TextBoxPort.Text = Telescope.mPort
    RadioButtonSP.Checked = Telescope.mInterface = "COM"
    RadioButtonIP.Checked = Telescope.mInterface = "IP"
    ' set the list of com ports to those that are currently available
    ComboBoxComPort.Items.Clear()
    ComboBoxComPort.Items.AddRange(System.IO.Ports.SerialPort.GetPortNames())       ' use System.IO because it's static
    ' select the current port if possible
    If ComboBoxComPort.Items.Contains(Telescope.mcomPort) Then
      ComboBoxComPort.SelectedItem = Telescope.mcomPort
    End If
  End Sub


  Private Sub RadioButtonIP_CheckedChanged(sender As Object, e As EventArgs) Handles RadioButtonIP.CheckedChanged
    TextBoxIP.Enabled = RadioButtonIP.Checked
    TextBoxPort.Enabled = RadioButtonIP.Checked
  End Sub

  Private Sub RadioButtonSP_CheckedChanged(sender As Object, e As EventArgs) Handles RadioButtonSP.CheckedChanged
    ComboBoxComPort.Enabled = RadioButtonSP.Checked
  End Sub

  Private Sub TextBoxPort_KeyPress(sender As Object, e As KeyPressEventArgs) Handles TextBoxPort.KeyPress
    If Not Char.IsDigit(e.KeyChar) And Not Char.IsControl(e.KeyChar) Then
      e.Handled = True
    End If
  End Sub

End Class

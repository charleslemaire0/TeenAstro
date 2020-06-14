Public Class Uploader
  Private Sub ButtonUploadT_Click(sender As Object, e As EventArgs) Handles ButtonUploadT.Click
    Try
      Dim pHelp As New ProcessStartInfo
      Dim exepath As String = System.IO.Path.GetDirectoryName(Application.ExecutablePath)
      pHelp.FileName = "teensy_post_compile.exe"
      Dim pcb As String = ComboBoxPCBMainUnitT.SelectedItem()
      Dim Hexfile As String = ""
      Dim fwv As String = ComboBoxFirmwareVersion.SelectedItem
      Select Case pcb
        Case "2.2 TMC260"
          Hexfile = fwv + "\" + "Teenastro_" + fwv + "_220_TMC260"
        Case "2.3 TMC260"
          Hexfile = fwv + "\" + "Teenastro_" + fwv + "_230_TMC260"
        Case "2.4 TMC2130"
          Hexfile = fwv + "\" + "Teenastro_" + fwv + "_240_TMC2130"
        Case "2.4 TMC5160"
          Hexfile = fwv + "\" + "Teenastro_" + fwv + "_240_TMC5160"
      End Select

      If Not System.IO.File.Exists(Hexfile + ".hex") Then
        MsgBox(Hexfile + " not found!")
        Return
      End If
      Dim cmd As String = "-file=" & Hexfile & " -path=" & exepath & " -tools=" & exepath & " -board=TEENSY31"
      pHelp.Arguments = cmd
      pHelp.WindowStyle = ProcessWindowStyle.Normal
      Dim proc1 As Process = Process.Start(pHelp)
      Threading.Thread.Sleep(3000)
      cmd = cmd & " -reboot"
      pHelp.Arguments = cmd
      Dim proc2 As Process = Process.Start(pHelp)
    Catch ex As Exception
      MsgBox(ex.Message)
    End Try
  End Sub

  Private Sub Uploader_Load(sender As Object, e As EventArgs) Handles MyBase.Load
    Dim assembly As System.Reflection.Assembly = System.Reflection.Assembly.GetExecutingAssembly()
    Dim fvi As FileVersionInfo = FileVersionInfo.GetVersionInfo(assembly.Location)
    Dim version As String = fvi.FileVersion
    Me.Text = "TeenAstro Firmware Uploader " + version
    ComboBoxPCBMainUnitT.SelectedIndex = 0
    ComboBoxPCBMainUnitF.SelectedIndex = 0
    ComboBoxFirmwareVersion.SelectedIndex = 0
    ComboBoxPCBSHC.SelectedIndex = 0
    For Each sp As String In My.Computer.Ports.SerialPortNames
      ComboBoxCOMSHC.Items.Add(sp)
    Next
  End Sub

  Private Sub ButtonUploadF_Click(sender As Object, e As EventArgs) Handles ButtonUploadF.Click
    Try
      Dim pHelp As New ProcessStartInfo
      Dim exepath As String = System.IO.Path.GetDirectoryName(Application.ExecutablePath)
      pHelp.FileName = "teensy_post_compile.exe"
      Dim pcb As String = ComboBoxPCBMainUnitF.SelectedItem()
      Dim Hexfile As String = ""
      Dim fwv As String = ComboBoxFirmwareVersion.SelectedItem
      Select Case pcb
        Case "2.2 TMC2130"
          Hexfile = fwv + "\" + "TeenAstroFocuser_" + fwv + "_220_TMC2130"
        Case "2.3 TMC2130"
          Hexfile = fwv + "\" + "TeenAstroFocuser_" + fwv + "_230_TMC2130"
        Case "2.4 TMC2130"
          Hexfile = fwv + "\" + "TeenAstroFocuser_" + fwv + "_240_TMC2130"
        Case "2.4 TMC5160"
          Hexfile = fwv + "\" + "TeenAstroFocuser_" + fwv + "_240_TMC5160"
      End Select
      If Not System.IO.File.Exists(Hexfile + ".hex") Then
        MsgBox(Hexfile + " Not found!")
        Return
      End If
      Dim cmd As String = "-file=" & Hexfile & " -path=" & exepath & " -tools=" & exepath & " -board=TEENSY31"
      pHelp.Arguments = cmd
      pHelp.WindowStyle = ProcessWindowStyle.Normal
      Dim proc1 As Process = Process.Start(pHelp)
      Threading.Thread.Sleep(3000)
      cmd = cmd & " -reboot"
      pHelp.Arguments = cmd
      Dim proc2 As Process = Process.Start(pHelp)
    Catch ex As Exception
      MsgBox(ex.Message)
    End Try
  End Sub

  Private Sub ButtonUploadSHC_Click(sender As Object, e As EventArgs) Handles ButtonUploadSHC.Click
    Try
      Dim pHelp As New ProcessStartInfo
      Dim exepath As String = System.IO.Path.GetDirectoryName(Application.ExecutablePath)
      pHelp.FileName = "esptool.exe"
      Dim pcb As String = ComboBoxPCBSHC.SelectedItem()
      Dim fwv As String = ComboBoxFirmwareVersion.SelectedItem
      Dim lg As String = "_" + ComboBoxLanguage.SelectedItem
      If fwv = "1.1" Or fwv = "1.0" Then
        lg = ""
      End If
      Dim Binfile As String = fwv + "\" + "TeenAstroSHC_" + fwv + lg + ".bin"

      If Not System.IO.File.Exists(Binfile) Then
        MsgBox(Binfile + " Not found!")
        Return
      End If
      Dim comport As String = ComboBoxCOMSHC.SelectedItem
      '"-vv -cd nodemcu -cb 921600 -cp "COM8" -ca 0x00000 -cf C: \Users\Charles\AppData\Local\Temp\VMBuilds\SMARTH~1\ESP826~1\Release/SMARTH~1.BIN
      Dim cmd As String = "-vv -cd nodemcu -cb 921600 -cp " & comport & " -ca 0x00000 -cf " & Binfile
      pHelp.Arguments = cmd
      pHelp.WindowStyle = ProcessWindowStyle.Normal
      Dim proc1 As Process = Process.Start(pHelp)
    Catch ex As Exception
      MsgBox(ex.Message)
    End Try
  End Sub

  Private Sub ButtonWIFISHC_Click(sender As Object, e As EventArgs) Handles ButtonWIFISHC.Click
    Dim webAddress As String = "http://" & TextBoxIP.Text & "/update"
    Process.Start(webAddress)
  End Sub


End Class

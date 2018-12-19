﻿Public Class Uploader
  Private Sub ButtonUploadT_Click(sender As Object, e As EventArgs) Handles ButtonUploadT.Click
    Try
      Dim pHelp As New ProcessStartInfo
      Dim exepath As String = System.IO.Path.GetDirectoryName(Application.ExecutablePath)
      pHelp.FileName = "teensy_post_compile.exe"
      Dim pcb As String = ComboBoxPCBMainUnitT.SelectedItem()
      Dim Hexfile As String = ""
      Select Case pcb
        Case "2.2 TMC260"
          Hexfile = "Teenastro_1.0_220"
        Case "2.3 TMC260"
          Hexfile = "Teenastro_1.0_230"
        Case "2.4 TMC2130"
          Hexfile = "Teenastro_1.0_240_TMC2130"
        Case "2.4 TMC5160"
          Hexfile = "Teenastro_1.0_240_TMC5160"
      End Select
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
    ComboBoxPCBMainUnitT.SelectedIndex = 0
    ComboBoxPCBMainUnitF.SelectedIndex = 0
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
      Select Case pcb
        Case "2.2"
          Hexfile = "TeenAstroFocuser_1.0_220"
        Case "2.3"
          Hexfile = "TeenAstroFocuser_1.0_230"
      End Select
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
      Dim Binfile As String = "TeenAstroSHC_1.0.bin"
      Dim comport As String = ComboBoxCOMSHC.SelectedItem
      '"-vv -cd nodemcu -cb 921600 -cp "COM8" -ca 0x00000 -cf C:\Users\Charles\AppData\Local\Temp\VMBuilds\SMARTH~1\ESP826~1\Release/SMARTH~1.BIN
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

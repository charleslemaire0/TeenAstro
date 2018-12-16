Public Class Uploader
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
End Class

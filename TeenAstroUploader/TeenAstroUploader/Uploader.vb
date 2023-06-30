Imports System.IO

Public Class Uploader
  Private Sub ButtonUploadT_Click(sender As Object, e As EventArgs) Handles ButtonUploadT.Click
    Try
      Dim pHelp As New ProcessStartInfo
      Dim exepath As String = """" & System.IO.Path.GetDirectoryName(Application.ExecutablePath) & """"
      pHelp.FileName = "teensy_post_compile.exe"
      Dim pcb As String = ComboBoxPCBMainUnitT.SelectedItem()
      Dim Hexfile As String = ""
      Dim fwv As String = ComboBoxFirmwareVersion.SelectedItem
      Dim fwvdir As String = fwv
      If RadioButtonLatest.Checked Then
        fwvdir += "_latest"
      End If
      Dim HexPath = System.IO.Path.GetDirectoryName(Application.ExecutablePath) + "\" + fwvdir
      Select Case pcb
        Case "2.2 TMC260"
          Hexfile = "Teenastro_" + fwv + "_220_TMC260"
        Case "2.3 TMC260"
          Hexfile = "Teenastro_" + fwv + "_230_TMC260"
        Case "2.4 TMC2130"
          Hexfile = "Teenastro_" + fwv + "_240_TMC2130"
        Case "2.4 TMC5160"
          Hexfile = "Teenastro_" + fwv + "_240_TMC5160"
        Case "2.5 TMC2130"
          Hexfile = "Teenastro_" + fwv + "_250_TMC2130"
        Case "2.5 TMC5160"
          Hexfile = "Teenastro_" + fwv + "_250_TMC5160"
      End Select

      If Not System.IO.File.Exists(HexPath + "\" + Hexfile + ".hex") Then
        MsgBox(Hexfile + ".hex" + " not found!")
        Return
      End If
      Dim cmd As String = ""
      HexPath = """" & HexPath & """"
      Select Case pcb
        Case "2.2 TMC260", "2.3 TMC260", "2.4 TMC2130", "2.4 TMC5160"
          cmd = "-file=" & Hexfile & " -path=" & HexPath & " -tools=" & exepath & " -board=TEENSY31"
        Case "2.5 TMC2130", "2.5 TMC5160"
          cmd = "-file=" & Hexfile & " -path=" & HexPath & " -tools=" & exepath & " -board=TEENSY40"
      End Select
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
    ComboBoxLanguage.SelectedIndex = 0
    ComboBoxFirmwareVersion.SelectedIndex = 0
    ComboBoxPCBSHC.SelectedIndex = 0
  End Sub

  Private Sub ButtonUploadF_Click(sender As Object, e As EventArgs) Handles ButtonUploadF.Click
    Try
      Dim pHelp As New ProcessStartInfo
      Dim exepath As String = """" & System.IO.Path.GetDirectoryName(Application.ExecutablePath) & """"
      pHelp.FileName = "teensy_post_compile.exe"
      Dim pcb As String = ComboBoxPCBMainUnitF.SelectedItem()
      Dim Hexfile As String = ""
      Dim fwv As String = ComboBoxFirmwareVersion.SelectedItem
      Dim fwvdir As String = fwv
      If RadioButtonLatest.Checked Then
        fwvdir += "_latest"
      End If
      Dim HexPath = System.IO.Path.GetDirectoryName(Application.ExecutablePath) + "\" + fwvdir
      Select Case pcb
        Case "2.2 TMC2130"
          Hexfile = "TeenAstroFocuser_" + fwv + "_220_TMC2130"
        Case "2.3 TMC2130"
          Hexfile = "TeenAstroFocuser_" + fwv + "_230_TMC2130"
        Case "2.4 TMC2130"
          Hexfile = "TeenAstroFocuser_" + fwv + "_240_TMC2130"
        Case "2.4 TMC5160"
          Hexfile = "TeenAstroFocuser_" + fwv + "_240_TMC5160"
        Case "2.5 TMC2130"
          Hexfile = "TeenAstroFocuser_" + fwv + "_250_TMC2130"
        Case "2.5 TMC5160"
          Hexfile = "TeenAstroFocuser_" + fwv + "_250_TMC5160"
      End Select

      If Not System.IO.File.Exists(HexPath + "\" + Hexfile + ".hex") Then
        MsgBox(Hexfile + ".hex" + " not found!")
        Return
      End If
      Dim cmd As String = ""
      HexPath = """" & HexPath & """"
      Select Case pcb
        Case "2.2 TMC2130", "2.3 TMC2130", "2.4 TMC2130", "2.4 TMC5160"
          cmd = "-file=" & Hexfile & " -path=" & HexPath & " -tools=" & exepath & " -board=TEENSY31"
        Case "2.5 TMC2130", "2.5 TMC5160"
          cmd = "-file=" & Hexfile & " -path=" & HexPath & " -tools=" & exepath & " -board=TEENSY40"
      End Select
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
      Dim exepath As String = """" & System.IO.Path.GetDirectoryName(Application.ExecutablePath) & """"
      pHelp.FileName = "esptool.exe"
      Dim pcb As String = ComboBoxPCBSHC.SelectedItem()
      Dim fwv As String = ComboBoxFirmwareVersion.SelectedItem
      Dim fwvdir As String = fwv
      If RadioButtonLatest.Checked Then
        fwvdir += "_latest"
      End If
      Dim lg As String = "_" + ComboBoxLanguage.SelectedItem
      Dim Binfile As String = ".\" + fwvdir + "\" + "TeenAstroSHC_" + fwv + lg + ".bin"

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
  Private Sub downloadVersion13x(ByRef n As Integer, ByRef sum As Integer)
    Dim ver As String = "1.3"
    Dim gitRootAdress As String = "https://github.com/charleslemaire0/TeenAstro/raw/Release_" + ver + "/TeenAstroUploader/TeenAstroUploader/" + ver + "/"
    Dim Firmwares As New List(Of String)
    Firmwares.Add("TeenAstroFocuser_" + ver + "_220_TMC2130.hex")
    Firmwares.Add("TeenAstroFocuser_" + ver + "_230_TMC2130.hex")
    Firmwares.Add("TeenAstroFocuser_" + ver + "_240_TMC2130.hex")
    Firmwares.Add("TeenAstroFocuser_" + ver + "_240_TMC5160.hex")
    Firmwares.Add("TeenAstroSHC_" + ver + "_English.bin")
    Firmwares.Add("TeenAstroSHC_" + ver + "_French.bin")
    Firmwares.Add("TeenAstroSHC_" + ver + "_German.bin")
    Firmwares.Add("TeenAstro_" + ver + "_220_TMC260.hex")
    Firmwares.Add("TeenAstro_" + ver + "_230_TMC260.hex")
    Firmwares.Add("TeenAstro_" + ver + "_240_TMC2130.hex")
    Firmwares.Add("TeenAstro_" + ver + "_240_TMC5160.hex")
    Firmwares.Add("TeenAstro_" + ver + "_250_TMC2130.hex")
    Firmwares.Add("TeenAstro_" + ver + "_250_TMC5160.hex")
    If Not System.IO.Directory.Exists(".\" + ver) Then
      System.IO.Directory.CreateDirectory(".\" + ver)
    End If
    Using client As New System.Net.WebClient()
      System.Net.ServicePointManager.SecurityProtocol = System.Net.SecurityProtocolType.Tls12
      client.Headers.Add("user-agent", "Anything")
      For Each firmware In Firmwares
        Try
          client.DownloadFile(gitRootAdress + firmware, ".\" + ver + "\" + firmware)
        Catch ex As Exception
          MsgBox(ex.Message)
          Exit For
        End Try
        n = n + 1
      Next
      client.Dispose()
    End Using
    sum += Firmwares.Count.ToString
  End Sub
  Private Sub downloadVersion14x(ByRef n As Integer, ByRef sum As Integer, ByVal ext As String)
    Dim ver As String = "1.4"
    Dim verdir As String = ver + ext
    Dim gitRootAdress As String = "https://github.com/charleslemaire0/TeenAstro/raw/Release_" + ver + "/TeenAstroUploader/TeenAstroUploader/" + verdir + "/"
    Dim Firmwares As New List(Of String)
    Firmwares.Add("TeenAstroFocuser_" + ver + "_220_TMC2130.hex")
    Firmwares.Add("TeenAstroFocuser_" + ver + "_230_TMC2130.hex")
    Firmwares.Add("TeenAstroFocuser_" + ver + "_240_TMC2130.hex")
    Firmwares.Add("TeenAstroFocuser_" + ver + "_240_TMC5160.hex")
    Firmwares.Add("TeenAstroSHC_" + ver + "_English.bin")
    Firmwares.Add("TeenAstroSHC_" + ver + "_French.bin")
    Firmwares.Add("TeenAstroSHC_" + ver + "_German.bin")
    Firmwares.Add("TeenAstro_" + ver + "_220_TMC260.hex")
    Firmwares.Add("TeenAstro_" + ver + "_230_TMC260.hex")
    Firmwares.Add("TeenAstro_" + ver + "_240_TMC2130.hex")
    Firmwares.Add("TeenAstro_" + ver + "_240_TMC5160.hex")
    Firmwares.Add("TeenAstro_" + ver + "_250_TMC2130.hex")
    Firmwares.Add("TeenAstro_" + ver + "_250_TMC5160.hex")
    If Not System.IO.Directory.Exists(".\" + verdir) Then
      System.IO.Directory.CreateDirectory(".\" + verdir)
    End If
    Using client As New System.Net.WebClient()
      System.Net.ServicePointManager.SecurityProtocol = System.Net.SecurityProtocolType.Tls12
      client.Headers.Add("user-agent", "Anything")
      For Each firmware In Firmwares
        Try
          client.DownloadFile(gitRootAdress + firmware, ".\" + verdir + "\" + firmware)
        Catch ex As Exception
          MsgBox(ex.Message)
          Exit For
        End Try
        n = n + 1
      Next
      client.Dispose()
    End Using
    sum += Firmwares.Count.ToString
  End Sub

  Private Sub ButtonDownLoad_Click(sender As Object, e As EventArgs) Handles ButtonDownLoad.Click
    Dim n As Integer = 0
    Dim sum As Integer = 0
    sum = n
    downloadVersion13x(n, sum)
    downloadVersion14x(n, sum, "")
    downloadVersion14x(n, sum, "_latest")
    MsgBox(n.ToString & "of " & sum.ToString & " successfully downloaded!")
  End Sub


  Private Sub ComboBoxCOMSHC_Click(sender As Object, e As EventArgs) Handles ComboBoxCOMSHC.Click
    ComboBoxCOMSHC.Items.Clear()
    For Each sp As String In My.Computer.Ports.SerialPortNames
      ComboBoxCOMSHC.Items.Add(sp)
    Next
  End Sub
End Class

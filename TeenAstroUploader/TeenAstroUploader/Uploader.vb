Imports System.IO
Imports System.Globalization
Imports System.Threading
Imports System.Runtime.InteropServices
Imports System.Net
Imports System.Diagnostics
Public Class Uploader
  ' Firmware is stored in: C:\Users\<user>\AppData\Local\TeenAstro\Firmware
  ' (%LocalAppData%\TeenAstro\Firmware). Always writable by the current user, no admin required.
  ' If the folder does not exist, it is created automatically (including parent TeenAstro if needed).
  Private Shared Function GetFirmwareBasePath() As String
    Dim base As String = System.IO.Path.Combine(
      Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
      "TeenAstro",
      "Firmware")
    If Not Directory.Exists(base) Then Directory.CreateDirectory(base)
    Return base
  End Function

  ''' <summary>
  ''' Download a file using curl.exe (built into Windows 10+).
  ''' CrowdStrike and other endpoint security software trust system binaries,
  ''' so curl works for normal users while WebClient gets blocked.
  ''' </summary>
  ''' <summary>Returns True if downloaded, False if file not found on server (404).</summary>
  Private Shared Function DownloadFileWithCurl(url As String, destPath As String) As Boolean
    Dim curlPath As String = System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "curl.exe")
    If Not System.IO.File.Exists(curlPath) Then
      Throw New FileNotFoundException("curl.exe not found at " & curlPath & ". Windows 10 version 1803 or later is required.")
    End If
    Dim psi As New ProcessStartInfo()
    psi.FileName = curlPath
    psi.Arguments = "-L -s -f -o """ & destPath & """ """ & url & """"
    psi.UseShellExecute = False
    psi.CreateNoWindow = True
    psi.RedirectStandardError = True
    Dim proc As Process = Process.Start(psi)
    Dim stderr As String = proc.StandardError.ReadToEnd()
    proc.WaitForExit()
    If proc.ExitCode = 22 Then
      ' curl exit 22 = HTTP error (404 not found). File doesn't exist on server -- skip it.
      Return False
    End If
    If proc.ExitCode <> 0 Then
      Throw New Exception("curl failed (exit " & proc.ExitCode & "): " & stderr.Trim() & vbLf & "URL: " & url)
    End If
    Return True
  End Function

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
      Dim HexPath As String = System.IO.Path.Combine(GetFirmwareBasePath(), fwvdir)
      If Not System.IO.Directory.Exists(HexPath) Then System.IO.Directory.CreateDirectory(HexPath)
      Select Case pcb
        Case "2.2 TMC260"
          Hexfile = "TeenAstro_" + fwv + "_220_TMC260"
        Case "2.3 TMC260"
          Hexfile = "TeenAstro_" + fwv + "_230_TMC260"
        Case "2.4 TMC2130"
          Hexfile = "TeenAstro_" + fwv + "_240_TMC2130"
        Case "2.4 TMC5160"
          Hexfile = "TeenAstro_" + fwv + "_240_TMC5160"
        Case "2.5 TMC2130"
          Hexfile = "TeenAstro_" + fwv + "_250_TMC2130"
        Case "2.5 TMC5160"
          Hexfile = "TeenAstro_" + fwv + "_250_TMC5160"
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
    CultureInfo.DefaultThreadCurrentCulture = CultureInfo.InvariantCulture
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
      Dim HexPath As String = System.IO.Path.Combine(GetFirmwareBasePath(), fwvdir)
      If Not System.IO.Directory.Exists(HexPath) Then System.IO.Directory.CreateDirectory(HexPath)
      Select Case pcb
        Case "2.2 TMC2130"
          Hexfile = "TeenAstroFocuser_" + fwv + "_220_TMC2130"
        Case "2.3 TMC2130"
          Hexfile = "TeenAstroFocuser_" + fwv + "_230_TMC2130"
        Case "2.4 TMC2130"
          Hexfile = "TeenAstroFocuser_" + fwv + "_240_TMC2130"
        Case "2.4 TMC5160"
          Hexfile = "TeenAstroFocuser_" + fwv + "_240_TMC5160"
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
      Dim HexPath As String = System.IO.Path.Combine(GetFirmwareBasePath(), fwvdir)
      If Not System.IO.Directory.Exists(HexPath) Then System.IO.Directory.CreateDirectory(HexPath)
      Dim lg As String = "_" + ComboBoxLanguage.SelectedItem
      Dim Binfile As String = System.IO.Path.Combine(HexPath, "TeenAstroSHC_" + fwv + lg + ".bin")

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

  Private Shared Function GetFullExceptionMessage(ex As Exception) As String
    Dim s As String = ex.Message
    Dim inner As Exception = ex.InnerException
    While inner IsNot Nothing
      s = s & vbLf & " -> " & inner.Message
      inner = inner.InnerException
    End While
    If TypeOf ex Is WebException Then
      Dim we As WebException = CType(ex, WebException)
      If we.Response IsNot Nothing AndAlso TypeOf we.Response Is HttpWebResponse Then
        Dim resp As HttpWebResponse = CType(we.Response, HttpWebResponse)
        s = s & vbLf & "HTTP " & CInt(resp.StatusCode) & " " & resp.StatusDescription
      End If
    End If
    Return s
  End Function

  Private Sub downloadVersionx(ByRef n As Integer, ByRef sum As Integer, ByVal ext As String, ByVal ver As String)
    Dim gitRootAdress As String = ""
    Dim currentFirmware As String = ""
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
    Try
      Dim verdir As String = ver + ext
      Dim targetDir As String = System.IO.Path.Combine(GetFirmwareBasePath(), verdir)
      gitRootAdress = "https://github.com/charleslemaire0/TeenAstro/raw/Release_" + ver + "/TeenAstroUploader/TeenAstroUploader/" + verdir + "/"
      If Not System.IO.Directory.Exists(targetDir) Then
        System.IO.Directory.CreateDirectory(targetDir)
      End If
      For Each firmware In Firmwares
        currentFirmware = firmware
        Dim url As String = gitRootAdress + firmware
        Dim destPath As String = System.IO.Path.Combine(targetDir, firmware)
        If DownloadFileWithCurl(url, destPath) Then
          n = n + 1
        End If
      Next
    Catch ex As Exception
      Dim msg As String = "Download failed: " & currentFirmware & vbLf & vbLf & GetFullExceptionMessage(ex)
      If gitRootAdress <> "" AndAlso currentFirmware <> "" Then msg = msg & vbLf & vbLf & "URL: " & gitRootAdress & currentFirmware
      MsgBox(msg, MsgBoxStyle.Exclamation, "TeenAstro Firmware Download")
    End Try
    sum += Firmwares.Count
  End Sub

  Private Sub ButtonDownLoad_Click(sender As Object, e As EventArgs) Handles ButtonDownLoad.Click
    Dim n As Integer = 0
    Dim sum As Integer = 0
    Dim ver As String = ComboBoxFirmwareVersion.SelectedItem.ToString()
    downloadVersionx(n, sum, "", ver)
    downloadVersionx(n, sum, "_latest", ver)
    MsgBox(n.ToString & " of " & sum.ToString & " successfully downloaded!")
  End Sub

  Private Sub ComboBoxCOMSHC_Click(sender As Object, e As EventArgs) Handles ComboBoxCOMSHC.Click
    ComboBoxCOMSHC.Items.Clear()
    For Each sp As String In My.Computer.Ports.SerialPortNames
      ComboBoxCOMSHC.Items.Add(sp)
    Next
  End Sub

End Class

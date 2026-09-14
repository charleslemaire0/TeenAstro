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
      Dim comport As String = Nothing
      If ComboBoxCOMSHC.SelectedItem IsNot Nothing Then
        comport = ComboBoxCOMSHC.SelectedItem.ToString()
      End If
      If String.IsNullOrEmpty(comport) Then
        MsgBox("Select a ComPort first.", MsgBoxStyle.Exclamation, "Upload SHC")
        Return
      End If

      Dim fwv As String = ComboBoxFirmwareVersion.SelectedItem.ToString()
      Dim fwvdir As String = fwv
      If RadioButtonLatest.Checked Then
        fwvdir += "_latest"
      End If
      Dim HexPath As String = System.IO.Path.Combine(GetFirmwareBasePath(), fwvdir)
      If Not System.IO.Directory.Exists(HexPath) Then System.IO.Directory.CreateDirectory(HexPath)
      Dim lg As String = ComboBoxLanguage.SelectedItem.ToString()
      Dim isS3 As Boolean = IsShcEsp32S3()
      Dim Binfile As String = GetShcFirmwarePath(HexPath, fwv, lg, isS3)

      If Not System.IO.File.Exists(Binfile) Then
        MsgBox(Binfile + " Not found!" & vbLf & vbLf &
               If(isS3,
                  "Expected a merged ESP32-S3 image (TeenAstroSHC_" & fwv & "_S3_" & lg & ".bin)." & vbLf &
                  "Build with: pio run -d TeenAstroSHC -e esp32s3" & vbLf &
                  "then copy TeenAstroSHC_*_esp32s3_merged.bin into the firmware folder.",
                  "Download firmware first, or place the .bin in the firmware folder."),
               MsgBoxStyle.Exclamation, "Upload SHC")
        Return
      End If

      Dim pHelp As New ProcessStartInfo
      pHelp.WorkingDirectory = System.IO.Path.GetDirectoryName(Application.ExecutablePath)
      pHelp.WindowStyle = ProcessWindowStyle.Normal

      If isS3 Then
        Dim espTool As String = FindEspressifEsptool()
        If String.IsNullOrEmpty(espTool) Then
          MsgBox("ESP32 esptool not found." & vbLf & vbLf &
                 "Install Arduino ESP32 board support, or PlatformIO, then retry." & vbLf &
                 "Looked for esptool_esp32.exe next to this app and Arduino15/PlatformIO esptool.",
                 MsgBoxStyle.Exclamation, "Upload SHC")
          Return
        End If
        ApplyEspressifEsptool(pHelp, espTool,
          "--chip esp32s3 --port " & comport &
          " --baud 921600 --before default_reset --after hard_reset write_flash -z 0x0 """ & Binfile & """")
      Else
        pHelp.FileName = "esptool.exe"
        pHelp.Arguments = "-vv -cd nodemcu -cb 921600 -cp " & comport & " -ca 0x00000 -cf """ & Binfile & """"
      End If
      Process.Start(pHelp)
    Catch ex As Exception
      MsgBox(ex.Message)
    End Try
  End Sub

  Private Sub ButtonEraseSHC_Click(sender As Object, e As EventArgs) Handles ButtonEraseSHC.Click
    Try
      Dim comport As String = Nothing
      If ComboBoxCOMSHC.SelectedItem IsNot Nothing Then
        comport = ComboBoxCOMSHC.SelectedItem.ToString()
      End If
      If String.IsNullOrEmpty(comport) Then
        MsgBox("Select a ComPort first.", MsgBoxStyle.Exclamation, "Erase Flash")
        Return
      End If

      Dim isS3 As Boolean = IsShcEsp32S3()
      Dim chipName As String = If(isS3, "ESP32-S3", "ESP8266")
      Dim confirm As MsgBoxResult = MsgBox(
        "This erases the entire " & chipName & " flash (firmware and settings)." & vbLf & vbLf &
        "You must Upload over COM afterwards to restore the Hand Controller." & vbLf & vbLf &
        "Continue on " & comport & "?",
        MsgBoxStyle.YesNo Or MsgBoxStyle.Exclamation Or MsgBoxStyle.DefaultButton2,
        "Erase Flash")
      If confirm <> MsgBoxResult.Yes Then Return

      Dim pHelp As New ProcessStartInfo
      pHelp.WorkingDirectory = System.IO.Path.GetDirectoryName(Application.ExecutablePath)
      pHelp.WindowStyle = ProcessWindowStyle.Normal

      If isS3 Then
        Dim espTool As String = FindEspressifEsptool()
        If String.IsNullOrEmpty(espTool) Then
          MsgBox("ESP32 esptool not found." & vbLf & vbLf &
                 "Install Arduino ESP32 board support, or PlatformIO, then retry.",
                 MsgBoxStyle.Exclamation, "Erase Flash")
          Return
        End If
        ApplyEspressifEsptool(pHelp, espTool, "--chip esp32s3 --port " & comport & " erase_flash")
      Else
        pHelp.FileName = "esptool.exe"
        ' esptool-ck: -ce erases the whole flash (same tool as Upload over COM)
        pHelp.Arguments = "-vv -cd nodemcu -cb 921600 -cp " & comport & " -ce"
      End If
      Process.Start(pHelp)
    Catch ex As Exception
      MsgBox(ex.Message)
    End Try
  End Sub

  Private Function IsShcEsp32S3() As Boolean
    Dim pcb As String = ""
    If ComboBoxPCBSHC.SelectedItem IsNot Nothing Then
      pcb = ComboBoxPCBSHC.SelectedItem.ToString()
    End If
    Return pcb.IndexOf("S3", StringComparison.OrdinalIgnoreCase) >= 0
  End Function

  Private Shared Function GetShcFirmwarePath(hexPath As String, fwv As String, language As String, isS3 As Boolean) As String
    If isS3 Then
      ' Preferred packaged name for TeenAstroUploader
      Dim preferred As String = System.IO.Path.Combine(hexPath, "TeenAstroSHC_" & fwv & "_S3_" & language & ".bin")
      If System.IO.File.Exists(preferred) Then Return preferred
      ' PlatformIO merge output from rename_shc.py (English env = esp32s3)
      Dim envSuffix As String = "esp32s3"
      If language.Equals("French", StringComparison.OrdinalIgnoreCase) Then
        envSuffix = "esp32s3_FRENCH"
      ElseIf language.Equals("German", StringComparison.OrdinalIgnoreCase) Then
        envSuffix = "esp32s3_GERMAN"
      End If
      Dim merged As String = System.IO.Path.Combine(hexPath, "TeenAstroSHC_166_" & envSuffix & "_merged.bin")
      If System.IO.File.Exists(merged) Then Return merged
      Return preferred
    End If
    Return System.IO.Path.Combine(hexPath, "TeenAstroSHC_" & fwv & "_" & language & ".bin")
  End Function

  ''' <summary>
  ''' Locate Espressif esptool (ESP32-S3). The bundled esptool.exe is esptool-ck (ESP8266 only).
  ''' </summary>
  Private Shared Function FindEspressifEsptool() As String
    Dim appDir As String = System.IO.Path.GetDirectoryName(Application.ExecutablePath)
    Dim candidates As New List(Of String)
    candidates.Add(System.IO.Path.Combine(appDir, "esptool_esp32.exe"))

    Dim arduinoTools As String = System.IO.Path.Combine(
      Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
      "Arduino15", "packages", "esp32", "tools", "esptool_py")
    If System.IO.Directory.Exists(arduinoTools) Then
      Dim dirs() As String = System.IO.Directory.GetDirectories(arduinoTools)
      Array.Sort(dirs)
      Array.Reverse(dirs)
      For Each verDir As String In dirs
        candidates.Add(System.IO.Path.Combine(verDir, "esptool.exe"))
      Next
    End If

    For Each c As String In candidates
      If System.IO.File.Exists(c) Then Return c
    Next

    Dim pioEsptool As String = System.IO.Path.Combine(
      Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),
      ".platformio", "packages", "tool-esptoolpy", "esptool.py")
    Dim pioPython As String = System.IO.Path.Combine(
      Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),
      ".platformio", "penv", "Scripts", "python.exe")
    If System.IO.File.Exists(pioEsptool) AndAlso System.IO.File.Exists(pioPython) Then
      Return "PIO:" & pioPython & "|" & pioEsptool
    End If

    Return Nothing
  End Function

  Private Shared Sub ApplyEspressifEsptool(ByRef pHelp As ProcessStartInfo, espTool As String, args As String)
    If espTool.StartsWith("PIO:") Then
      Dim parts = espTool.Substring(4).Split("|"c)
      pHelp.FileName = parts(0)
      pHelp.Arguments = """" & parts(1) & """ " & args
    Else
      pHelp.FileName = espTool
      pHelp.Arguments = args
    End If
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
    Firmwares.Add("TeenAstroSHC_" + ver + "_S3_English.bin")
    Firmwares.Add("TeenAstroSHC_" + ver + "_S3_French.bin")
    Firmwares.Add("TeenAstroSHC_" + ver + "_S3_German.bin")
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

  Private Sub ButtonOpenFirmwareFolder_Click(sender As Object, e As EventArgs) Handles ButtonOpenFirmwareFolder.Click
    Try
      Dim folder As String = GetFirmwareBasePath()
      Process.Start(New ProcessStartInfo(folder) With {.UseShellExecute = True})
    Catch ex As Exception
      MsgBox(ex.Message, MsgBoxStyle.Exclamation, "TeenAstro Firmware Uploader")
    End Try
  End Sub

  Private Sub ComboBoxCOMSHC_Click(sender As Object, e As EventArgs) Handles ComboBoxCOMSHC.Click
    ComboBoxCOMSHC.Items.Clear()
    For Each sp As String In My.Computer.Ports.SerialPortNames
      ComboBoxCOMSHC.Items.Add(sp)
    Next
  End Sub

End Class

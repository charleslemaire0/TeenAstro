Imports System.IO
Imports System.IO.Ports
Imports System.Globalization
Imports System.Threading
Imports System.Runtime.InteropServices
Imports System.Net
Imports System.Diagnostics
Imports System.ComponentModel
Public Class Uploader
  Private Class DownloadResult
    Public SuccessCount As Integer
    Public TotalCount As Integer
    Public ErrorMessage As String
  End Class

  Private Class DownloadProgress
    Public Current As Integer
    Public Total As Integer
    Public FileName As String
  End Class

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

  Private Class DetectedMainUnit
    Public PortName As String
    Public Firmware As String
    Public Board As Integer
    Public Driver As Integer
    Public Pcb As String
  End Class

  ' :GVB# is the PCB code (220, 230, 240, 250). :GVb# is AxisDriver
  ' (1 = TOS100, filed as TMC260 on 2.2/2.3; 2 = TMC2130; 3 = TMC5160).
  Private Shared Function PcbFromBoard(board As Integer, driver As Integer) As String
    Select Case board
      Case 220
        If driver = 1 Then Return "2.2 TMC260"
      Case 230
        If driver = 1 Then Return "2.3 TMC260"
      Case 240
        If driver = 2 Then Return "2.4 TMC2130"
        If driver = 3 Then Return "2.4 TMC5160"
      Case 250
        If driver = 2 Then Return "2.5 TMC2130"
        If driver = 3 Then Return "2.5 TMC5160"
    End Select
    Return Nothing
  End Function

  Private Shared Function Lx200Query(port As SerialPort, cmd As String) As String
    port.DiscardInBuffer()
    port.Write(cmd)
    Dim buf As String = ""
    Dim deadline As DateTime = DateTime.UtcNow.AddMilliseconds(350)
    While DateTime.UtcNow < deadline
      If port.BytesToRead > 0 Then
        buf &= port.ReadExisting()
        If buf.Contains("#") Then Exit While
      Else
        Thread.Sleep(15)
      End If
    End While
    Dim hash As Integer = buf.IndexOf("#"c)
    If hash < 0 Then Return Nothing
    Return buf.Substring(0, hash).Trim()
  End Function

  Private Shared Function TryReadMainUnit(portName As String, baud As Integer) As DetectedMainUnit
    Dim port As SerialPort = Nothing
    Try
      port = New SerialPort(portName, baud)
      port.ReadTimeout = 300
      port.WriteTimeout = 300
      port.DtrEnable = False
      port.RtsEnable = False
      port.Open()
      Thread.Sleep(120)
      Dim product As String = Lx200Query(port, ":GVP#")
      If product <> "TeenAstro" Then Return Nothing
      Dim fw As String = Lx200Query(port, ":GVN#")
      Dim boardText As String = Lx200Query(port, ":GVB#")
      Dim driverText As String = Lx200Query(port, ":GVb#")
      Dim board As Integer
      Dim driver As Integer
      If Not Integer.TryParse(boardText, board) Then Return Nothing
      If Not Integer.TryParse(driverText, driver) Then Return Nothing
      Dim found As New DetectedMainUnit()
      found.PortName = portName
      found.Firmware = If(fw, "?")
      found.Board = board
      found.Driver = driver
      found.Pcb = PcbFromBoard(board, driver)
      Return found
    Catch
      Return Nothing
    Finally
      If port IsNot Nothing Then
        Try
          If port.IsOpen Then port.Close()
        Catch
        End Try
        port.Dispose()
      End If
    End Try
  End Function

  Private Shared Function FindMainUnits() As List(Of DetectedMainUnit)
    Dim found As New List(Of DetectedMainUnit)
    For Each portName As String In My.Computer.Ports.SerialPortNames
      Dim unit As DetectedMainUnit = TryReadMainUnit(portName, 57600)
      If unit Is Nothing Then unit = TryReadMainUnit(portName, 115200)
      If unit IsNot Nothing Then found.Add(unit)
    Next
    Return found
  End Function

  Private Sub UploadTelescope(pcb As String)
    Dim pHelp As New ProcessStartInfo
    Dim exepath As String = """" & System.IO.Path.GetDirectoryName(Application.ExecutablePath) & """"
    pHelp.FileName = "teensy_post_compile.exe"
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

    If Hexfile = "" Then
      MsgBox("No firmware file for this board.")
      Return
    End If
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
  End Sub

  Private Sub ButtonUploadT_Click(sender As Object, e As EventArgs) Handles ButtonUploadT.Click
    Try
      If ComboBoxPCBMainUnitT.SelectedItem Is Nothing Then
        MsgBox("Select a PCB board.")
        Return
      End If
      UploadTelescope(ComboBoxPCBMainUnitT.SelectedItem.ToString())
    Catch ex As Exception
      MsgBox(ex.Message)
    End Try
  End Sub

  Private Sub ButtonAutoT_Click(sender As Object, e As EventArgs) Handles ButtonAutoT.Click
    Try
      Cursor = Cursors.WaitCursor
      ButtonAutoT.Enabled = False
      Dim units As List(Of DetectedMainUnit) = FindMainUnits()
      Cursor = Cursors.Default
      ButtonAutoT.Enabled = True
      If units.Count = 0 Then
        MsgBox("No TeenAstro MainUnit found on a COM port.")
        Return
      End If
      If units.Count > 1 Then
        Dim lines As String = ""
        For Each unit As DetectedMainUnit In units
          lines &= unit.PortName & "  PCB " & unit.Board & "  driver " & unit.Driver & "  (" & unit.Pcb & ")" & vbLf
        Next
        MsgBox("Several MainUnits are connected. Unplug the others and press Auto again." & vbLf & vbLf & lines)
        Return
      End If
      Dim one As DetectedMainUnit = units(0)
      If one.Pcb Is Nothing Then
        MsgBox("MainUnit on " & one.PortName & " is PCB " & one.Board & ", driver " & one.Driver & "." & vbLf & "This uploader has no firmware for that board.")
        Return
      End If
      ComboBoxPCBMainUnitT.SelectedItem = one.Pcb
      Dim fwv As String = ComboBoxFirmwareVersion.SelectedItem.ToString()
      If RadioButtonLatest.Checked Then fwv &= " latest"
      Dim ask As String = "MainUnit on " & one.PortName & vbLf &
        "PCB " & one.Board & ", driver " & one.Driver & " (" & one.Pcb & ")" & vbLf &
        "Firmware now: " & one.Firmware & vbLf & vbLf &
        "Upload " & fwv & "?"
      If MsgBox(ask, MsgBoxStyle.YesNo Or MsgBoxStyle.Question, "Auto") <> MsgBoxResult.Yes Then Return
      UploadTelescope(one.Pcb)
    Catch ex As Exception
      Cursor = Cursors.Default
      ButtonAutoT.Enabled = True
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

  Private Class DetectedFocuser
    Public PortName As String
    Public Firmware As String
    Public BoardText As String
    Public Driver As Integer
    Public Pcb As String
  End Class

  ' :FV# replies "$ TeenAstro Focuser 2.4.0 1.6.2#" and, on current firmware,
  ' a trailing AxisDriver (2 = TMC2130, 3 = TMC5160). PCB 2.2 and 2.3 are always TMC2130.
  Private Shared Function PcbFromFocuser(boardText As String, driver As Integer) As String
    Dim board As String = boardText
    If board.EndsWith(".0") Then board = board.Substring(0, board.Length - 2)
    Select Case board
      Case "2.2"
        Return "2.2 TMC2130"
      Case "2.3"
        Return "2.3 TMC2130"
      Case "2.4"
        If driver = 3 Then Return "2.4 TMC5160"
        If driver = 2 Then Return "2.4 TMC2130"
    End Select
    Return Nothing
  End Function

  Private Shared Function TryReadFocuser(portName As String) As DetectedFocuser
    Dim port As SerialPort = Nothing
    Try
      port = New SerialPort(portName, 9600)
      port.ReadTimeout = 300
      port.WriteTimeout = 300
      port.DtrEnable = False
      port.RtsEnable = False
      port.Open()
      Thread.Sleep(120)
      Dim reply As String = Lx200Query(port, ":FV#")
      If reply Is Nothing OrElse Not reply.Contains("TeenAstro Focuser") Then Return Nothing
      Dim parts() As String = reply.Split(New Char() {" "c}, StringSplitOptions.RemoveEmptyEntries)
      Dim boardText As String = Nothing
      Dim firmware As String = "?"
      Dim driver As Integer = 0
      For i As Integer = 0 To parts.Length - 1
        If parts(i) = "Focuser" AndAlso i + 1 < parts.Length Then
          boardText = parts(i + 1)
          If i + 2 < parts.Length Then firmware = parts(i + 2)
          If i + 3 < parts.Length Then Integer.TryParse(parts(i + 3), driver)
          Exit For
        End If
      Next
      If boardText Is Nothing Then Return Nothing
      Dim found As New DetectedFocuser()
      found.PortName = portName
      found.Firmware = firmware
      found.BoardText = boardText
      found.Driver = driver
      found.Pcb = PcbFromFocuser(boardText, driver)
      Return found
    Catch
      Return Nothing
    Finally
      If port IsNot Nothing Then
        Try
          If port.IsOpen Then port.Close()
        Catch
        End Try
        port.Dispose()
      End If
    End Try
  End Function

  Private Shared Function FindFocusers() As List(Of DetectedFocuser)
    Dim found As New List(Of DetectedFocuser)
    For Each portName As String In My.Computer.Ports.SerialPortNames
      Dim unit As DetectedFocuser = TryReadFocuser(portName)
      If unit IsNot Nothing Then found.Add(unit)
    Next
    Return found
  End Function

  Private Sub UploadFocuser(pcb As String)
    Dim pHelp As New ProcessStartInfo
    Dim exepath As String = """" & System.IO.Path.GetDirectoryName(Application.ExecutablePath) & """"
    pHelp.FileName = "teensy_post_compile.exe"
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

    If Hexfile = "" Then
      MsgBox("No firmware file for this board.")
      Return
    End If
    If Not System.IO.File.Exists(HexPath + "\" + Hexfile + ".hex") Then
      MsgBox(Hexfile + ".hex" + " not found!")
      Return
    End If
    Dim cmd As String = "-file=" & Hexfile & " -path=" & """" & HexPath & """" & " -tools=" & exepath & " -board=TEENSY31"
    pHelp.Arguments = cmd
    pHelp.WindowStyle = ProcessWindowStyle.Normal
    Dim proc1 As Process = Process.Start(pHelp)
    Threading.Thread.Sleep(3000)
    cmd = cmd & " -reboot"
    pHelp.Arguments = cmd
    Dim proc2 As Process = Process.Start(pHelp)
  End Sub

  Private Sub ButtonUploadF_Click(sender As Object, e As EventArgs) Handles ButtonUploadF.Click
    Try
      If ComboBoxPCBMainUnitF.SelectedItem Is Nothing Then
        MsgBox("Select a PCB board.")
        Return
      End If
      UploadFocuser(ComboBoxPCBMainUnitF.SelectedItem.ToString())
    Catch ex As Exception
      MsgBox(ex.Message)
    End Try
  End Sub

  Private Sub ButtonAutoF_Click(sender As Object, e As EventArgs) Handles ButtonAutoF.Click
    Try
      Cursor = Cursors.WaitCursor
      ButtonAutoF.Enabled = False
      Dim units As List(Of DetectedFocuser) = FindFocusers()
      Cursor = Cursors.Default
      ButtonAutoF.Enabled = True
      If units.Count = 0 Then
        MsgBox("No TeenAstro Focuser found on a COM port.")
        Return
      End If
      If units.Count > 1 Then
        Dim lines As String = ""
        For Each unit As DetectedFocuser In units
          lines &= unit.PortName & "  PCB " & unit.BoardText & vbLf
        Next
        MsgBox("Several Focusers are connected. Unplug the others and press Auto again." & vbLf & vbLf & lines)
        Return
      End If
      Dim one As DetectedFocuser = units(0)
      If one.Pcb Is Nothing AndAlso one.BoardText.StartsWith("2.4") Then
        Dim pick As MsgBoxResult = MsgBox(
          "Focuser on " & one.PortName & " is PCB " & one.BoardText & "." & vbLf &
          "It did not report the stepper driver." & vbLf & vbLf &
          "Yes = TMC5160" & vbLf & "No = TMC2130",
          MsgBoxStyle.YesNoCancel Or MsgBoxStyle.Question, "Auto")
        If pick = MsgBoxResult.Cancel Then Return
        one.Driver = If(pick = MsgBoxResult.Yes, 3, 2)
        one.Pcb = PcbFromFocuser(one.BoardText, one.Driver)
      End If
      If one.Pcb Is Nothing Then
        MsgBox("Focuser on " & one.PortName & " is PCB " & one.BoardText & "." & vbLf & "This uploader has no firmware for that board.")
        Return
      End If
      ComboBoxPCBMainUnitF.SelectedItem = one.Pcb
      Dim fwv As String = ComboBoxFirmwareVersion.SelectedItem.ToString()
      If RadioButtonLatest.Checked Then fwv &= " latest"
      Dim driverNote As String = If(one.Driver = 0, "", ", driver " & one.Driver.ToString())
      Dim ask As String = "Focuser on " & one.PortName & vbLf &
        "PCB " & one.BoardText & driverNote & " (" & one.Pcb & ")" & vbLf &
        "Firmware now: " & one.Firmware & vbLf & vbLf &
        "Upload " & fwv & "?"
      If MsgBox(ask, MsgBoxStyle.YesNo Or MsgBoxStyle.Question, "Auto") <> MsgBoxResult.Yes Then Return
      UploadFocuser(one.Pcb)
    Catch ex As Exception
      Cursor = Cursors.Default
      ButtonAutoF.Enabled = True
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

  Private Shared Function GetFirmwareFileList(ver As String) As List(Of String)
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
    Return Firmwares
  End Function

  Private Sub downloadVersionx(ByRef n As Integer, ByRef done As Integer, ByVal ext As String, ByVal ver As String,
                               ByVal worker As BackgroundWorker, ByVal totalFiles As Integer, ByRef errorMessage As String)
    Dim gitRootAdress As String = ""
    Dim currentFirmware As String = ""
    Dim Firmwares As List(Of String) = GetFirmwareFileList(ver)
    Try
      Dim verdir As String = ver + ext
      Dim targetDir As String = System.IO.Path.Combine(GetFirmwareBasePath(), verdir)
      gitRootAdress = "https://github.com/charleslemaire0/TeenAstro/raw/Release_" + ver + "/TeenAstroUploader/TeenAstroUploader/" + verdir + "/"
      If Not System.IO.Directory.Exists(targetDir) Then
        System.IO.Directory.CreateDirectory(targetDir)
      End If
      For Each firmware In Firmwares
        currentFirmware = firmware
        done = done + 1
        Dim progress As New DownloadProgress With {
          .Current = done,
          .Total = totalFiles,
          .FileName = firmware
        }
        worker.ReportProgress(CInt(100.0 * done / totalFiles), progress)
        Dim url As String = gitRootAdress + firmware
        Dim destPath As String = System.IO.Path.Combine(targetDir, firmware)
        If DownloadFileWithCurl(url, destPath) Then
          n = n + 1
        End If
      Next
    Catch ex As Exception
      Dim msg As String = "Download failed: " & currentFirmware & vbLf & vbLf & GetFullExceptionMessage(ex)
      If gitRootAdress <> "" AndAlso currentFirmware <> "" Then msg = msg & vbLf & vbLf & "URL: " & gitRootAdress & currentFirmware
      If errorMessage = "" Then
        errorMessage = msg
      Else
        errorMessage = errorMessage & vbLf & vbLf & msg
      End If
    End Try
  End Sub

  Private Sub ButtonDownLoad_Click(sender As Object, e As EventArgs) Handles ButtonDownLoad.Click
    If BackgroundWorkerDownload.IsBusy Then Return
    If ComboBoxFirmwareVersion.SelectedItem Is Nothing Then
      MsgBox("Select a firmware version.")
      Return
    End If
    Dim ver As String = ComboBoxFirmwareVersion.SelectedItem.ToString()
    ButtonDownLoad.Enabled = False
    ProgressBarDownload.Value = 0
    LabelDownloadStatus.Text = "Starting download..."
    BackgroundWorkerDownload.RunWorkerAsync(ver)
  End Sub

  Private Sub BackgroundWorkerDownload_DoWork(sender As Object, e As DoWorkEventArgs) Handles BackgroundWorkerDownload.DoWork
    Dim worker As BackgroundWorker = CType(sender, BackgroundWorker)
    Dim ver As String = CStr(e.Argument)
    Dim result As New DownloadResult()
    Dim n As Integer = 0
    Dim done As Integer = 0
    Dim errorMessage As String = ""
    Dim totalFiles As Integer = GetFirmwareFileList(ver).Count * 2
    downloadVersionx(n, done, "", ver, worker, totalFiles, errorMessage)
    downloadVersionx(n, done, "_latest", ver, worker, totalFiles, errorMessage)
    result.SuccessCount = n
    result.TotalCount = totalFiles
    result.ErrorMessage = errorMessage
    e.Result = result
  End Sub

  Private Sub BackgroundWorkerDownload_ProgressChanged(sender As Object, e As ProgressChangedEventArgs) Handles BackgroundWorkerDownload.ProgressChanged
    Dim progress As DownloadProgress = TryCast(e.UserState, DownloadProgress)
    ProgressBarDownload.Value = Math.Max(0, Math.Min(100, e.ProgressPercentage))
    If progress IsNot Nothing Then
      LabelDownloadStatus.Text = "Downloading " & progress.Current.ToString() & " of " & progress.Total.ToString() & ": " & progress.FileName
    End If
  End Sub

  Private Sub BackgroundWorkerDownload_RunWorkerCompleted(sender As Object, e As RunWorkerCompletedEventArgs) Handles BackgroundWorkerDownload.RunWorkerCompleted
    ButtonDownLoad.Enabled = True
    If e.Error IsNot Nothing Then
      ProgressBarDownload.Value = 0
      LabelDownloadStatus.Text = "Download failed."
      MsgBox(GetFullExceptionMessage(e.Error), MsgBoxStyle.Exclamation, "TeenAstro Firmware Download")
      Return
    End If
    Dim result As DownloadResult = TryCast(e.Result, DownloadResult)
    If result Is Nothing Then
      LabelDownloadStatus.Text = ""
      Return
    End If
    ProgressBarDownload.Value = 100
    LabelDownloadStatus.Text = result.SuccessCount.ToString() & " of " & result.TotalCount.ToString() & " downloaded"
    If result.ErrorMessage <> "" Then
      MsgBox(result.ErrorMessage, MsgBoxStyle.Exclamation, "TeenAstro Firmware Download")
    End If
    MsgBox(result.SuccessCount.ToString() & " of " & result.TotalCount.ToString() & " successfully downloaded!")
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

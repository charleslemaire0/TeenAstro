'tabs=4
' --------------------------------------------------------------------------------
' TODO fill in this information for your driver, then remove this line!
'
' ASCOM Telescope driver for TeenAstro
'
' Description:	Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam 
'				nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam 
'				erat, sed diam voluptua. At vero eos et accusam et justo duo 
'				dolores et ea rebum. Stet clita kasd gubergren, no sea takimata 
'				sanctus est Lorem ipsum dolor sit amet.
'
' Implements:	ASCOM Telescope interface version: 1.0
' Author:		(XXX) Your N. Here <your@email.here>
'
' Edit Log:
'
' Date			Who	Vers	Description
' -----------	---	-----	-------------------------------------------------------
' dd-mmm-yyyy	XXX	1.0.0	Initial edit, from Telescope template
' ---------------------------------------------------------------------------------
'
'
' Your driver's ID is ASCOM.TeenAstro.Telescope
'
' The Guid attribute sets the CLSID for ASCOM.DeviceName.Telescope
' The ClassInterface/None addribute prevents an empty interface called
' _Telescope from being created and used as the [default] interface
'

' This definition is used to select code that's only applicable for one device type
#Const Device = "Telescope"

Imports ASCOM
Imports ASCOM.Astrometry
Imports ASCOM.Astrometry.AstroUtils
Imports ASCOM.DeviceInterface
Imports ASCOM.Utilities

Imports System
Imports System.Collections
Imports System.Collections.Generic
Imports System.Globalization
Imports System.Reflection
Imports System.Runtime.InteropServices
Imports System.Text
Imports System.Text.RegularExpressions
Imports System.Windows.Forms.AxHost

<Guid("e3db1df1-f25d-42ce-ba54-5c035ac6f94c")>
<ClassInterface(ClassInterfaceType.None)>
Public Class Telescope

  ' The Guid attribute sets the CLSID for ASCOM.TeenAstro.Telescope
  ' The ClassInterface/None addribute prevents an empty interface called
  ' _TeenAstro from being created and used as the [default] interface

  ' TODO Replace the not implemented exceptions with code to implement the function or
  ' throw the appropriate ASCOM exception.
  '
  Implements ITelescopeV3

  '
  ' Driver ID and descriptive string that shows in the Chooser
  '
  Friend Shared driverID As String = "ASCOM.TeenAstro.Telescope"
  Private Shared driverDescription As String = "TeenAstro Telescope"

  Friend Shared mcomPortProfileName As String = "COM Port" 'Constants used for Profile persistence
  Friend Shared mcomPortDefault As String = "COM1"
  Friend Shared mIPProfileName As String = "IP Adress"
  Friend Shared mIPDefault As String = "192.168.0.1"
  Friend Shared mPortProfileName As String = "Port"
  Friend Shared mPortDefault As String = "9999"
  Friend Shared mInterfaceProfileName As String = "Interface"
  Friend Shared mInterfaceDefault As String = "COM"
  Friend Shared mtraceStateProfileName As String = "Trace Level"
  Friend Shared mtraceStateDefault As String = "False"

  ' Variables to hold the currrent device configuration
  Friend Shared mcomPort As String
  Friend Shared mIP As String
  Friend Shared mPort As Integer
  Friend Shared mInterface As String
  Friend Shared mtraceState As Boolean = False
  Friend Shared mobjectIP As Net.IPAddress
  Private mobjectSerial As ASCOM.Utilities.Serial
  Private mconnectedState As Boolean = False ' Private variable to hold the connected state
  Private mbLongFormat As Boolean

  Private mTelStatus As String


  Private mConnectionStatusDate As Date

  Private mtgtRa As Double = -999
  Private mtgtDec As Double = -999



  Private mutilities As Util ' Private variable to hold an ASCOM Utilities object
  Private mastroUtilities As AstroUtils ' Private variable to hold an AstroUtils object to provide the Range method
  Private mTL As TraceLogger ' Private variable to hold the trace logger object (creates a diagnostic log file with information that you specify)

  ' Slew speeds for speed settings 0-4 as defined in the main unit's Global.h in the array guideRates[].
  ' Values are given as multiples of sidereal speed. all these values are overwritten by EEPROM at runtime.
  Private mSlewSpeeds As Double = 0
  Private mSlewSpeed As String = "" ' Last slew speed set via R command
  Private mSiderealRate As Double = 15.04106858 / 3600 ' Sidereal rate in degrees per second

  'Private Shared mformcontrol As FormControl
  '
  ' Constructor - Must be public for COM registration!
  '
  Public Sub New()

    ReadProfile() ' Read device configuration from the ASCOM Profile store
    mTL = New TraceLogger("", "TeenAstro")
    mTL.Enabled = mtraceState
    mTL.LogMessage("Telescope", "Starting initialisation")
    mutilities = New Util() ' Initialise util object

    Dim assembly As System.Reflection.Assembly = System.Reflection.Assembly.GetExecutingAssembly()
    Dim fvi As FileVersionInfo = FileVersionInfo.GetVersionInfo(assembly.Location)
    Dim version As String = fvi.FileVersion
    mutilities.SerialTraceFile = mTL.LogFilePath + "\TeenAstro_Serial_" + version + ".txt"
    mutilities.SerialTrace = mtraceState
    mastroUtilities = New AstroUtils 'Initialise new astro utiliites object

    'TODO: Implement your additional construction here

  End Sub

  '
  ' PUBLIC COM INTERFACE ITelescopeV3 IMPLEMENTATION
  '

#Region "Common properties and methods"
  ''' <summary>
  ''' Displays the Setup Dialog form.
  ''' If the user clicks the OK button to dismiss the form, then
  ''' the new settings are saved, otherwise the old values are reloaded.
  ''' THIS IS THE ONLY PLACE WHERE SHOWING USER INTERFACE IS ALLOWED!
  ''' </summary>
  Public Sub SetupDialog() Implements ITelescopeV3.SetupDialog
    ' consider only showing the setup dialog if not connected
    ' or call a different dialog if connected
    If IsConnected Then
      System.Windows.Forms.MessageBox.Show("Already connected, just press OK")
      Exit Sub
    End If

    Using F As SetupDialogForm = New SetupDialogForm()
      Dim result As System.Windows.Forms.DialogResult = F.ShowDialog()
      If result = DialogResult.OK Then
        WriteProfile() ' Persist device configuration values to the ASCOM Profile store
      End If
    End Using
  End Sub

  Public ReadOnly Property SupportedActions() As ArrayList Implements ITelescopeV3.SupportedActions
    Get
      mTL.LogMessage("Get SupportedActions", "Returning arraylisof supported actions") 'empty arraylist")
      Dim suppActions As ArrayList = New ArrayList
      suppActions.Add("AutoAlign")
      Return suppActions 'New ArrayList()
    End Get
  End Property

  Public Function Action(ByVal ActionName As String, ByVal ActionParameters As String) As String Implements ITelescopeV3.Action
    If ActionName = "AutoAlign" Then
      If Not Connected Then
        Throw New ASCOM.NotConnectedException
      End If
      If Not CommandBoolSingleChar("AA") Then
        Throw New ASCOM.DriverException("AutoAlign command failure")
      End If
    Else
      Throw New ActionNotImplementedException("Action " & ActionName & " is not supported by this driver")
    End If
  End Function

  Private Function GenericCommand(ByVal Command As String, ByVal Raw As Boolean,
                                ByVal Mode As Integer, ByRef buf As String) As Boolean
    If Not Raw Then
      Command = ":" & Command & "#"
    End If
    If mInterface = "COM" Then
      Return GetSerial(Command, Mode, buf, 3)
    ElseIf mInterface = "IP" Then
      Return getStream(Command, Mode, buf, 3)
    End If
    Return False
  End Function

  Public Sub CommandBlind(ByVal Command As String, Optional ByVal Raw As Boolean = False) Implements ITelescopeV3.CommandBlind
    Dim buf As String = ""
    If Not GenericCommand(Command, Raw, 0, buf) Then
      Throw New ASCOM.NotConnectedException("CommandBlind " + Command + " has failed")
    End If
    mConnectionStatusDate = Date.UtcNow
  End Sub

  Public Function CommandBool(ByVal Command As String, Optional ByVal Raw As Boolean = False) As Boolean _
    Implements ITelescopeV3.CommandBool
    Dim buf As String = ""
    If Not GenericCommand(Command, Raw, 1, buf) Then
      Throw New ASCOM.NotConnectedException("CommandBool " + Command + " has failed")
    End If
    mConnectionStatusDate = Date.UtcNow
    Return buf = "1"
  End Function

  Private Function CommandBoolString(ByVal Command As String, Optional ByVal Raw As Boolean = False) As Boolean
    Dim buf As String = CommandString(Command, Raw)
    If buf.Length > 0 Then
      If buf = "1" Then
        Return True
      ElseIf buf = "0" Then
        Return False
      End If
      Throw New ASCOM.DriverException("CommandBoolString " + Command + " returned invalid value: " & buf)
    End If
    Return False
  End Function
  Private Function CommandBoolSingleChar(ByVal Command As String, Optional ByVal Raw As Boolean = False) As Boolean
    Dim buf As String = CommandSingleChar(Command, Raw)
    If buf.Length > 0 Then
      If buf = "1" Then
        Return True
      ElseIf buf = "0" Then
        Return False
      End If
      Throw New ASCOM.DriverException("CommandBoolSingleChar " + Command + " returned invalid value: " & buf)
    End If
    Return False
  End Function

  Private Function CommandSingleChar(ByVal Command As String, Optional ByVal Raw As Boolean = False) As String
    Dim buf As String = ""
    If Not GenericCommand(Command, Raw, 1, buf) Then
      Throw New ASCOM.NotConnectedException("CommandSingleChar " + Command + " has failed")
    End If
    mConnectionStatusDate = Date.UtcNow
    Return buf
  End Function

  Public Function CommandString(ByVal Command As String, Optional ByVal Raw As Boolean = False) As String _
    Implements ITelescopeV3.CommandString
    Dim buf As String = ""
    If Not GenericCommand(Command, Raw, 2, buf) Then
      Throw New ASCOM.NotConnectedException("CommandString " + Command + " has failed")
    End If
    mConnectionStatusDate = Date.UtcNow
    Return buf
  End Function

  Private Function MyDevice() As Boolean
    MyDevice = False
    '        mConnectionStatusDate = Date.UtcNow
    Try
      MyDevice = CommandString("GVP") = "TeenAstro"
    Catch ex As Exception
      Throw New ASCOM.DriverException(ex.Message)
    End Try
  End Function

  Private Sub ConnectSerial(value As Boolean)
    If value Then
      mobjectSerial = New ASCOM.Utilities.Serial
      mTL.LogMessage("Connected Set", "Connecting to port " + mcomPort)
      Try
        Dim c As String = Replace(mcomPort, "COM", "")
        mobjectSerial.Port = CInt(c)
      Catch ex As Exception
        Throw New ASCOM.DriverException(ex.Message)
        mconnectedState = False
        Return
      End Try
      mobjectSerial.Speed = 57600
      Try
        mobjectSerial.Connected = True
      Catch ex As Exception
        mconnectedState = False
        mobjectSerial.Dispose()
        Throw New ASCOM.DriverException(ex.Message)
      End Try
      mobjectSerial.ReceiveTimeoutMs = 5000
      If Not MyDevice() Then
        mobjectSerial.Connected = False
        mconnectedState = False
        mobjectSerial.Dispose()
        Return
      Else
        Dim timeTelescope As DateTime = UTCDate()
        Dim time As Date = DateTime.UtcNow()
        mconnectedState = True
        If Math.Abs((timeTelescope - time).TotalSeconds) > 2 Then
          Me.UTCDate = DateTime.UtcNow()
          mTL.LogMessage("Connected Set", "Synced with computer time")
        End If
        If Not mconnectedState Then
            Throw New ASCOM.NotConnectedException("Connection has failed!")
          End If
        End If
        Else
        mobjectSerial.Connected = False
      mobjectSerial.Dispose()
      mconnectedState = False
      mTL.LogMessage("Connected Set", "Disconnecting from port " + mcomPort)
    End If
  End Sub

  Private Sub ConnectIP(value As Boolean)
    If value Then
      If Not System.Net.IPAddress.TryParse(mIP, mobjectIP) Then
        MsgBox(mIP + " is Not AddressOf valid IP Address")
        Return
      End If
      mconnectedState = MyDevice()
      'mformcontrol = New FormControl(Me)
      'If My.Settings.ShowII Then
      '    mformcontrol.Show()
      'End If
      If Not mconnectedState Then
        Throw New ASCOM.InvalidValueException("Connection has failed!")
      Else
        Dim timeTelescope As DateTime = UTCDate()
        Dim time As Date = DateTime.UtcNow()
        If Math.Abs((timeTelescope - time).TotalSeconds) > 2 Then
          mTL.LogMessage("Connected Set", "Synced with computer time")
          Me.UTCDate = DateTime.UtcNow()
        End If
      End If
    Else
      mconnectedState = False
      mTL.LogMessage("Connected Set", "Disconnecting from IP " + mIP)
    End If
  End Sub

  Public Property Connected() As Boolean Implements ITelescopeV3.Connected
    Get
      mTL.LogMessage("Get Connected", IsConnected.ToString())
      Return IsConnected
    End Get
    Set(value As Boolean)
      mTL.LogMessage("Set Connected", value.ToString())
      If value = IsConnected Then
        Return
      End If
      If mInterface = "COM" Then
        ConnectSerial(value)
      ElseIf mInterface = "IP" Then
        ConnectIP(value)
      End If
      If value Then
        If Not checkCompatibility() Then

          If mInterface = "COM" Then
            ConnectSerial(False)
          ElseIf mInterface = "IP" Then
            ConnectIP(False)
          End If
        End If
      End If

    End Set
  End Property
  Private Function getStream(ByVal Command As String, ByVal Mode As Integer, ByRef buf As String, ByVal retry As Integer) As Boolean
    For k = 0 To retry
      If getStream(Command, Mode, buf) Then
        Return True
      End If
    Next
    Return False
  End Function
  Private Function getStream(ByVal Command As String, ByVal Mode As Integer, ByRef buf As String) As Boolean

    Dim ClientSocket As System.Net.Sockets.TcpClient = New Net.Sockets.TcpClient
    Dim result As IAsyncResult = ClientSocket.BeginConnect(mobjectIP, mPort, Nothing, Nothing)
    Dim online = result.AsyncWaitHandle.WaitOne(2000, True)
    If Not online Then
      ClientSocket.Close()
      mconnectedState = False
      Return False
    End If
    Try
      Dim ServerStream As Net.Sockets.NetworkStream = ClientSocket.GetStream()
      Dim outStream As Byte() = System.Text.Encoding.ASCII.GetBytes(Command)
      ServerStream.Write(outStream, 0, outStream.Length)
      ServerStream.Flush()
      buf = ""
      Select Case Mode
        Case 0
          If ServerStream.CanRead Then
            Dim myReadBuffer(ClientSocket.ReceiveBufferSize) As Byte
            ServerStream.Read(myReadBuffer, 0, myReadBuffer.Length)
          End If
          getStream = True
        Case 1 To 2
          If ServerStream.CanRead Then
            Dim myReadBuffer(ClientSocket.ReceiveBufferSize) As Byte
            Dim myCompleteMessage As StringBuilder = New StringBuilder()
            Dim numberOfBytesRead As Integer = 0
            ' Incoming message may be larger than the buffer size.
            Do
              numberOfBytesRead = ServerStream.Read(myReadBuffer, 0, myReadBuffer.Length)
              myCompleteMessage.AppendFormat("{0}", Encoding.ASCII.GetString(myReadBuffer, 0, numberOfBytesRead))
            Loop While ServerStream.DataAvailable
            buf = myCompleteMessage.ToString()
            If Mode = 1 And buf <> "" Then
              buf = buf.Substring(0, 1)
            ElseIf Mode = 2 And buf <> "" Then
              buf = buf.Split("#")(0)
            End If
            getStream = buf <> ""
          Else
            getStream = False
          End If

      End Select
    Catch ex As Exception
      mTL.LogMessage("Network error", ex.Message)
      getStream = False
    End Try
    ClientSocket.Close()
  End Function
  Private Function GetSerial(ByVal Command As String, ByVal Mode As Integer, ByRef buf As String, ByVal retry As Integer) As Boolean
    For k = 0 To retry
      If GetSerial(Command, Mode, buf) Then
        Return True
      End If
    Next
    Return False
  End Function
  Private Function GetSerial(ByVal Command As String, ByVal Mode As Integer, ByRef buf As String) As Boolean
    mobjectSerial.ClearBuffers()
    Try
      mobjectSerial.Transmit(Command)
      Select Case Mode
        Case 0
          GetSerial = True
        Case 1
          buf = mobjectSerial.ReceiveCounted(1)
          GetSerial = buf <> ""
        Case 2
          buf = mobjectSerial.ReceiveTerminated("#").TrimEnd("#")
          GetSerial = buf <> ""
      End Select
    Catch ex As Exception
      mTL.LogMessage("Serial connection", ex.Message)
    End Try
  End Function

  Public ReadOnly Property Description As String Implements ITelescopeV3.Description
    Get
      ' this pattern seems to be needed to allow a public property to return a private field
      Dim d As String = driverDescription
      mTL.LogMessage("Get Description", d)
      Return d
    End Get
  End Property

  Public ReadOnly Property DriverInfo As String Implements ITelescopeV3.DriverInfo
    Get
      Dim m_version As Version = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version
      Dim s_driverInfo As String = "Version: " + m_version.Major.ToString() + "." + m_version.Minor.ToString()
      mTL.LogMessage("Get DriverInfo", s_driverInfo)
      Return s_driverInfo
    End Get
  End Property

  Public ReadOnly Property DriverVersion() As String Implements ITelescopeV3.DriverVersion
    Get
      ' Get our own assembly and report its version number
      mTL.LogMessage("Get DriverVersion", Reflection.Assembly.GetExecutingAssembly.GetName.Version.ToString(2))
      Return Reflection.Assembly.GetExecutingAssembly.GetName.Version.ToString(2)
    End Get
  End Property

  Public ReadOnly Property InterfaceVersion() As Short Implements ITelescopeV3.InterfaceVersion
    Get
      mTL.LogMessage("Get InterfaceVersion", "3")
      Return 3
    End Get
  End Property

  Public ReadOnly Property Name As String Implements ITelescopeV3.Name
    Get
      Dim s_name As String = "TeenAstro"
      mTL.LogMessage("Get Name", s_name)
      Return s_name
    End Get
  End Property

  Public Sub Dispose() Implements ITelescopeV3.Dispose
    ' Clean up the tracelogger and util objects
    mTL.Enabled = False
    mTL.Dispose()
    mTL = Nothing
    mutilities.Dispose()
    mutilities = Nothing
    mastroUtilities.Dispose()
    mastroUtilities = Nothing
  End Sub

#End Region

#Region "ITelescope Implementation"
  Public Sub AbortSlew() Implements ITelescopeV3.AbortSlew
    If Me.AtPark Then
      Throw New ASCOM.ParkedException
    End If
    CommandBlind("Q")
    mTL.LogMessage("AbortSlew", "done")
  End Sub

  Public ReadOnly Property AlignmentMode() As AlignmentModes Implements ITelescopeV3.AlignmentMode
    Get
      updateTelStatus()
      Dim m As String = mTelStatus.Substring(12, 1)
      mTL.LogMessage("AlignmentMode", m)
      If m = "E" Then
        Return AlignmentModes.algGermanPolar
      ElseIf m = "K" Then
        Return AlignmentModes.algPolar
      ElseIf m = "k" Or m = "A" Then
        Return AlignmentModes.algAltAz
      End If
    End Get
  End Property

  Public ReadOnly Property Altitude() As Double Implements ITelescopeV3.Altitude
    Get
      Dim Alt As Double = mutilities.DMSToDegrees(Me.CommandString("GA"))
      mTL.LogMessage("Get Altitude", Alt.ToString("0.0000000"))
      Return Alt
    End Get
  End Property

  Public ReadOnly Property ApertureArea() As Double Implements ITelescopeV3.ApertureArea
    Get
      mTL.LogMessage("Get ApertureArea", "Not implemented")
      Throw New ASCOM.PropertyNotImplementedException("ApertureArea", False)
    End Get
  End Property

  Public ReadOnly Property ApertureDiameter() As Double Implements ITelescopeV3.ApertureDiameter
    Get
      mTL.LogMessage("Get ApertureDiameter", "Not implemented")
      Throw New ASCOM.PropertyNotImplementedException("ApertureDiameter", False)
    End Get
  End Property

  Public ReadOnly Property AtHome() As Boolean Implements ITelescopeV3.AtHome
    Get
      updateTelStatus()
      Dim isAtHome As Boolean = mTelStatus.Substring(3, 1) = "H"
      mTL.LogMessage("Get AtHome", isAtHome.ToString())
      Return isAtHome
    End Get
  End Property

  Public ReadOnly Property AtPark() As Boolean Implements ITelescopeV3.AtPark
    Get
      updateTelStatus()
      Dim isAtPark As Boolean = mTelStatus.Substring(2, 1) = "P"
      mTL.LogMessage("Get AtPark", isAtPark.ToString())
      Return isAtPark
    End Get
  End Property

  Public Function AxisRates(Axis As TelescopeAxes) As IAxisRates Implements ITelescopeV3.AxisRates
    ' Read maxSpeed from TeenAstro main unit and assign top two speed settings on this basis
    Dim Speed As Double, response As String
    'MAX_SPEED
    response = Me.CommandString("GXRX")
    mTL.LogMessage("Get AxisRates", "for " & Axis.ToString() & "Max value:  " & response)
    If Not (Double.TryParse(response, Speed)) Then
      Throw New ASCOM.InvalidValueException("Retrieve MAX_SPEED via :GXRX# has failed: '" & response & "'")
    End If
    mSlewSpeeds = Speed
    Return New AxisRates(Axis, mSlewSpeeds, mSiderealRate)
  End Function

  Public ReadOnly Property Azimuth() As Double Implements ITelescopeV3.Azimuth
    Get
      Dim AZ As Double = mutilities.DMSToDegrees(Me.CommandString("GZ"))
      mTL.LogMessage("Get Azimuth", AZ.ToString("0.0000000"))
      Return AZ
    End Get
  End Property

  Public ReadOnly Property CanFindHome() As Boolean Implements ITelescopeV3.CanFindHome
    Get
      Return True
    End Get
  End Property

  Public Function CanMoveAxis(Axis As TelescopeAxes) As Boolean Implements ITelescopeV3.CanMoveAxis
    mTL.LogMessage("Get CanMoveAxis", Axis.ToString())
    Select Case Axis
      Case TelescopeAxes.axisPrimary
        Return True
      Case TelescopeAxes.axisSecondary
        Return True
      Case TelescopeAxes.axisTertiary
        Return False
      Case Else
        Throw New InvalidValueException("CanMoveAxis", Axis.ToString(), "0 To 2")
    End Select
  End Function

  Public ReadOnly Property CanPark() As Boolean Implements ITelescopeV3.CanPark
    Get
      Dim value As Boolean = True
      mTL.LogMessage("Get CanPark", value.ToString())
      Return value
    End Get
  End Property

  Public ReadOnly Property CanPulseGuide() As Boolean Implements ITelescopeV3.CanPulseGuide
    Get
      mTL.LogMessage("Get CanPulseGuide", True.ToString())
      Return True
    End Get
  End Property

  Public ReadOnly Property CanSetDeclinationRate() As Boolean Implements ITelescopeV3.CanSetDeclinationRate
    Get
      mTL.LogMessage("Get CanSetDeclinationRate", True.ToString())
      Return True
    End Get
  End Property

  Public ReadOnly Property CanSetGuideRates() As Boolean Implements ITelescopeV3.CanSetGuideRates
    Get
      mTL.LogMessage("Get CanSetGuideRates", True.ToString())
      Return True
    End Get
  End Property

  Public ReadOnly Property CanSetPark() As Boolean Implements ITelescopeV3.CanSetPark
    Get
      Dim value As Boolean = True
      mTL.LogMessage("Get CanSetPark", value.ToString())
      Return value
    End Get
  End Property

  Public ReadOnly Property CanSetPierSide() As Boolean Implements ITelescopeV3.CanSetPierSide
    Get
      Dim can As Boolean = (AlignmentMode = AlignmentModes.algGermanPolar)
      mTL.LogMessage("Get CanSetPierSide", can.ToString())
      Return can
    End Get
  End Property

  Public ReadOnly Property CanSetRightAscensionRate() As Boolean Implements ITelescopeV3.CanSetRightAscensionRate
    Get
      mTL.LogMessage("Get CanSetRightAscensionRate", True.ToString())
      Return True
    End Get
  End Property

  Public ReadOnly Property CanSetTracking() As Boolean Implements ITelescopeV3.CanSetTracking
    Get
      Dim value As Boolean = True
      mTL.LogMessage("Get CanSetTracking", value.ToString())
      Return value
    End Get
  End Property

  Public ReadOnly Property CanSlew() As Boolean Implements ITelescopeV3.CanSlew
    Get
      mTL.LogMessage("Get CanSlew", True.ToString())
      Return True
    End Get
  End Property

  Public ReadOnly Property CanSlewAltAz() As Boolean Implements ITelescopeV3.CanSlewAltAz
    Get
      mTL.LogMessage("Get CanSlewAltAz", True.ToString())
      Return True
    End Get
  End Property

  Public ReadOnly Property CanSlewAltAzAsync() As Boolean Implements ITelescopeV3.CanSlewAltAzAsync
    Get
      mTL.LogMessage("Get CanSlewAltAzAsync", True.ToString())
      Return True
    End Get
  End Property

  Public ReadOnly Property CanSlewAsync() As Boolean Implements ITelescopeV3.CanSlewAsync
    Get
      mTL.LogMessage("Get CanSlewAsync", True.ToString())
      Return True
    End Get
  End Property

  Public ReadOnly Property CanSync() As Boolean Implements ITelescopeV3.CanSync
    Get
      mTL.LogMessage("Get CanSync", True.ToString())
      Return True
    End Get
  End Property

  Public ReadOnly Property CanSyncAltAz() As Boolean Implements ITelescopeV3.CanSyncAltAz
    Get
      mTL.LogMessage("Get CanSyncAltAz", True.ToString())
      Return True
    End Get
  End Property

  Public ReadOnly Property CanUnpark() As Boolean Implements ITelescopeV3.CanUnpark
    Get
      mTL.LogMessage("Get CanUnpark", True.ToString())
      Return True
    End Get
  End Property

  Public ReadOnly Property Declination() As Double Implements ITelescopeV3.Declination
    Get
      Dim Dec As Double = mutilities.DMSToDegrees(Me.CommandString("GD"))
      mTL.LogMessage("Get Declination", mutilities.DegreesToDMS(Dec))
      Return Dec
    End Get
  End Property

  Public Property DeclinationRate() As Double Implements ITelescopeV3.DeclinationRate
    Get
      Dim rate As Double
      Dim response As String = Me.CommandString("GXRd")
      mTL.LogMessage("Get DeclinationRate", "value: " & response)
      If Not (Double.TryParse(response, rate)) Then
        Throw New ASCOM.InvalidValueException("Retrieve DeclinationRate via :GXRr# has failed: '" & response & "'")
      End If
      Return (rate) / 10000
    End Get
    Set(value As Double)
      Dim rate As Integer = value * 10000
      Dim cmd As String = "SXRd," & rate.ToString()
      mTL.LogMessage("Set DeclinationRate", "value: " & cmd)
      If Not CommandBoolSingleChar(cmd) Then
        Throw New ASCOM.InvalidValueException("Set DeclinationRate via :" & cmd & " has failed")
      End If
    End Set
  End Property

  Public Function DestinationSideOfPier(RightAscension As Double, Declination As Double) As PierSide Implements ITelescopeV3.DestinationSideOfPier
    Dim RaString As String = RaToString(RightAscension)
    Dim DecString As String = DecToString(Declination)
    Dim state As String = Me.CommandSingleChar("M?" + RaString + DecString)
    If state = "E" Then
      DestinationSideOfPier = PierSide.pierEast
    ElseIf state = "W" Then
      DestinationSideOfPier = PierSide.pierWest
    ElseIf state = "?" Then
      Throw New ASCOM.InvalidValueException("Destination cannot be reached")
      Return PierSide.pierUnknown
    Else
      Throw New ASCOM.InvalidValueException("DestinationSideOfPier has failed")
      Return PierSide.pierUnknown
    End If
    mTL.LogMessage("Get DestinationSideOfPier", DestinationSideOfPier.ToString)
    Return DestinationSideOfPier
  End Function

  Public Property DoesRefraction() As Boolean Implements ITelescopeV3.DoesRefraction
    Get
      Dim str1 As String = CommandString("GXrg")
      Dim str2 As String = CommandString("GXrp")
      Dim str3 As String = CommandString("GXrt")

      If (str1.Length = 1 And str2.Length = 1 And str3.Length = 1) Then
        Return str1.Substring(0, 1) = "y" And str2.Substring(0, 1) = "y" And str3.Substring(0, 1) = "y"
      End If
      Throw New ASCOM.InvalidOperationException("Get refraction failed")
    End Get
    Set(value As Boolean)
      Dim ok As Boolean = True
      If value Then
        ok = CommandBoolSingleChar("SXrg,y")
        ok = ok And CommandBoolSingleChar("SXrp,y")
        ok = ok And CommandBoolSingleChar("SXrt,y")
      Else
        ok = CommandBoolSingleChar("SXrg,n")
        ok = ok And CommandBoolSingleChar("SXrp,n")
        ok = ok And CommandBoolSingleChar("SXrt,n")
      End If
      If Not ok Then
        If (value) Then
          Throw New ASCOM.DriverException("turn refraction on failed")
        Else
          Throw New ASCOM.DriverException("turn refraction off failed")
        End If
      End If
    End Set
  End Property

  Public ReadOnly Property EquatorialSystem() As EquatorialCoordinateType Implements ITelescopeV3.EquatorialSystem
    Get
      Dim equatorialSystem__1 As EquatorialCoordinateType = EquatorialCoordinateType.equTopocentric
      mTL.LogMessage("Get DeclinationRate", equatorialSystem__1.ToString())
      Return equatorialSystem__1
    End Get
  End Property

  Public Sub FindHome() Implements ITelescopeV3.FindHome
    If AtPark Then
      Throw New ASCOM.InvalidOperationException("Telescope is parked")
    End If
    If Not AtHome Then
      If CommandBoolSingleChar("hC") Then
        Threading.Thread.Sleep(1000)
        While Me.Slewing
          Threading.Thread.Sleep(1000)
        End While
      Else
        Throw New ASCOM.DriverException("Homing failed")
      End If
      If Not AtHome Then
        Throw New ASCOM.DriverException("Homing failed")
      End If
    End If
  End Sub

  Public ReadOnly Property FocalLength() As Double Implements ITelescopeV3.FocalLength
    Get
      mTL.LogMessage("Get FocalLength", "Not implemented")
      Throw New ASCOM.PropertyNotImplementedException("FocalLength", False)
    End Get
  End Property

  Private Function GetGuideRate() As Double
    'GUIDE_SPEED
    Dim response As String = Me.CommandString("GXR0")
    Dim speed As Double
    mTL.LogMessage("Get AxisRates", "value: " & response)
    If Not (Double.TryParse(response, speed)) Then
      Throw New ASCOM.InvalidValueException("Retrieve GetGuideRate via :GXR0# has failed: '" & response & "'")
    End If
    Return speed * mSiderealRate
  End Function
  Private Sub SetGuideRate(Val As Double)
    Dim speed As Integer = Val / mSiderealRate * 100
    Dim cmd As String = "SXR0," & speed.ToString("000")
    mTL.LogMessage("Set AxisRates", "value: " & Val)
    If Not Me.CommandBoolSingleChar(cmd) Then
      Throw New ASCOM.InvalidValueException("Set SetGuideRate via :" & cmd & " has failed")
    End If
  End Sub

  Public Property GuideRateDeclination() As Double Implements ITelescopeV3.GuideRateDeclination
    Get
      Return GetGuideRate()
    End Get
    Set(value As Double)
      SetGuideRate(value)
    End Set
  End Property
  Public Property GuideRateRightAscension() As Double Implements ITelescopeV3.GuideRateRightAscension
    Get
      Return GetGuideRate()
    End Get
    Set(value As Double)
      SetGuideRate(value)
    End Set
  End Property

  Public ReadOnly Property IsPulseGuiding() As Boolean Implements ITelescopeV3.IsPulseGuiding
    Get
      IsPulseGuiding = CommandBoolString("GXJP")
      mTL.LogMessage("Get IsPulseGuiding", IsPulseGuiding.ToString)
      Return IsPulseGuiding
    End Get
  End Property

  Public Sub MoveAxis(Axis As TelescopeAxes, Rate As Double) Implements ITelescopeV3.MoveAxis
    mTL.LogMessage("Set MoveAxis", Axis.ToString() & ":" & Rate.ToString())
    Dim cmd As String
    Dim waitStopM1 As Boolean = False
    Dim waitStopM2 As Boolean = False
    If Me.AtPark Then
      Throw New ASCOM.ParkedException
    End If
    Rate = Rate / mSiderealRate
    If (Axis = TelescopeAxes.axisPrimary) Then
      cmd = "M1" & Rate.ToString("+0.0000000;-0.0000000")
      If Rate = 0 Then
        waitStopM1 = True
      End If
    ElseIf (Axis = TelescopeAxes.axisSecondary) Then
      cmd = "M2" & Rate.ToString("+0.0000000;-0.0000000")
      If Rate = 0 Then
        waitStopM2 = True
      End If
    Else
      Throw New ASCOM.InvalidValueException("MoveAxis", Axis.ToString(), "0 To 1")
    End If
    Dim ret As String = Me.CommandSingleChar(cmd)
    If ret = "0" Or ret = "i" Then
      Throw New ASCOM.InvalidValueException("MoveAxis via :" & cmd & " has failed")
    ElseIf ret = "e" Then
      Throw New ASCOM.DriverException("MoveAxis is ignored, the telescop has already an error")
    ElseIf ret = "h" Then
      Throw New ASCOM.InvalidValueException("MoveAxis via :" & cmd & " has failed, the requested rate is not supported")
    ElseIf ret = "s" Then
      Throw New ASCOM.DriverException("MoveAxis is ignored, the telescop is slewing")
    ElseIf ret = "g" Then
      Throw New ASCOM.DriverException("MoveAxis is ignored, the telescop is guiding")
    End If
    If waitStopM1 Then
      While CommandBoolString("GXJM1")
        Threading.Thread.Sleep(500)
      End While
    End If
    If waitStopM2 Then
      While CommandBoolString("GXJM2")
        Threading.Thread.Sleep(500)
      End While
    End If

  End Sub

  Public Sub Park() Implements ITelescopeV3.Park
    Dim cmd As String = "hP"
    If CommandBoolSingleChar(cmd) Then
      mTL.LogMessage("Park", "done")
      Threading.Thread.Sleep(3000)
    Else
      mTL.LogMessage("Park", "failed")
      Throw New ASCOM.DriverException("Park has failed")
    End If
  End Sub

  Public Sub PulseGuide(Direction As GuideDirections, Duration As Integer) Implements ITelescopeV3.PulseGuide

    Dim ok As Boolean = Not AtPark And Not Slewing
    If ok Then
      Dim dir As String = ""
      Select Case Direction
        Case GuideDirections.guideNorth
          dir = "Mgn"
        Case GuideDirections.guideSouth
          dir = "Mgs"
        Case GuideDirections.guideEast
          dir = "Mge"
        Case GuideDirections.guideWest
          dir = "Mgw"
      End Select
      CommandBlind(dir & Duration)

      mTL.LogMessage("PulseGuide", dir & Duration & " done ")
    Else
      Throw New ASCOM.DriverException("Pulse guiding failed")
      mTL.LogMessage("PulseGuide", dir & Duration & " has failed ")
    End If

  End Sub

  Public ReadOnly Property RightAscension() As Double Implements ITelescopeV3.RightAscension
    Get
      Dim Ra As Double = mutilities.HMSToHours(Me.CommandString("GR"))
      mTL.LogMessage("Get RightAscension", mutilities.HoursToHMS(Ra))
      Return Ra
    End Get
  End Property

  Public Property RightAscensionRate() As Double Implements ITelescopeV3.RightAscensionRate
    Get
      Dim rate As Double
      Dim response As String = Me.CommandString("GXRr")
      mTL.LogMessage("Get RightAscensionRate", "value: " & response)
      If Not (Double.TryParse(response, rate)) Then
        Throw New ASCOM.InvalidValueException("Retrieve RightAscensionRate via :GXRr# has failed: '" & response & "'")
      End If
      Return rate / 10000
    End Get
    Set(value As Double)
      Dim rate As Integer = value * 10000
      Dim cmd As String = "SXRr," & rate.ToString()
      mTL.LogMessage("Set RightAscensionRate", "value: " & cmd)
      If Not CommandBoolSingleChar(cmd) Then
        Throw New ASCOM.InvalidValueException("Set RightAscensionRate via :" & cmd & " has failed")
      End If
    End Set
  End Property

  Public Sub SetPark() Implements ITelescopeV3.SetPark
    Dim ok As Boolean = CommandBoolSingleChar("hQ")
    If ok Then
      mTL.LogMessage("SetPark", "done")
    Else
      mTL.LogMessage("SetPark", "failed")
      Throw New ASCOM.DriverException("Set Park failed")
    End If
  End Sub

  Public Property SideOfPier() As PierSide Implements ITelescopeV3.SideOfPier
    Get
      updateTelStatus()
      mTL.LogMessage("Get SideOfPier", mTelStatus(13))
      If (mTelStatus(13) = " ") Then
        mTL.LogMessage("Get SideOfPier", "Unknown")
        Return PierSide.pierUnknown
      ElseIf (mTelStatus(13) = "E") Then
        mTL.LogMessage("Get SideOfPier", "East")
        Return PierSide.pierEast
      ElseIf (mTelStatus(13) = "W") Then
        mTL.LogMessage("Get SideOfPier", "West")
        Return PierSide.pierWest
      Else
        mTL.LogMessage("Get SideOfPier", "failed")
        Throw New ASCOM.InvalidValueException("Get SideOfPier failed")
      End If
    End Get
    Set(value As PierSide)
      Dim currentSide As PierSide = Me.SideOfPier()
      If CanSetPierSide Then
        If value = PierSide.pierUnknown Then
          Throw New ASCOM.InvalidValueException("German mount cannot be set with pierUnknow")
        ElseIf currentSide <> value Then
          checkFlip()
          Dim state = Me.CommandSingleChar("MF")
          If state.Length = 0 Then
            Throw New ASCOM.InvalidOperationException("Telescope is not replying")
          ElseIf Not state.Length = 1 Then
            Throw New ASCOM.InvalidOperationException("Telescope reply is corrupt")
          End If
          If state = "0" Then
            mTL.LogMessage("Slew to target", "Started")
          Else
            ReportState(state)
          End If

        End If
      Else
        Throw New ASCOM.InvalidOperationException("Change Side of Pier is only supported for german mount")
      End If
    End Set
  End Property

  Public ReadOnly Property SiderealTime() As Double Implements ITelescopeV3.SiderealTime
    Get
      Dim lst As Double = mutilities.DMSToDegrees(Me.CommandString("GS"))
      mTL.LogMessage("Get SiderealTime", lst.ToString("+0.000000"))
      Return lst
    End Get
  End Property

  Public Property SiteElevation() As Double Implements ITelescopeV3.SiteElevation
    Get
      'todo
      Dim elev As Double = Me.CommandString("Ge")
      mTL.LogMessage("Get SiteElevation", elev.ToString("0.0"))
      Return elev
    End Get
    Set(value As Double)
      If value > 10000 Or value < -300 Then
        Throw New ASCOM.InvalidValueException
      End If
      value = Int(value)
      mTL.LogMessage("Set SiteElevation", value.ToString("0.0"))
      Dim sg As String = ""
      If value >= 0 Then
        sg = "+"
      Else
        sg = "-"
      End If
      Dim cmd As String = "Se" + sg + Math.Abs(value).ToString("00000")
      CommandBoolSingleChar(cmd)
    End Set
  End Property

  Public Property SiteLatitude() As Double Implements ITelescopeV3.SiteLatitude
    Get
      Dim lati As Double = mutilities.DMSToDegrees(Me.CommandString("Gtf"))
      mTL.LogMessage("Get SiteLatitude", lati.ToString("0.000000"))
      Return lati
    End Get
    Set(value As Double)
      If value > 90 Or value < -90 Then
        Throw New ASCOM.InvalidValueException
      End If
      Dim sg As String = ""
      If value >= 0 Then
        sg = "+"
      Else
        sg = "-"
        value = -value
      End If
      Dim cmd As String = "St" + sg + DegtoDDMMSS(value)
      If Not CommandBoolSingleChar(cmd) Then
        Throw New ASCOM.InvalidOperationException
      End If
      mTL.LogMessage("SiteLatitude Set", value)
    End Set
  End Property

  Public Property SiteLongitude() As Double Implements ITelescopeV3.SiteLongitude
    'Opposite sign notation!
    Get
      Dim longi As Double = mutilities.DMSToDegrees(Me.CommandString("Ggf")) * -1
      mTL.LogMessage("Get SiteLongitude", longi.ToString("0.000000"))
      Return longi
    End Get
    Set(value As Double)
      If value > 180 Or value < -180 Then
        Throw New ASCOM.InvalidValueException
      End If
      Dim sg As String = ""
      If value >= 0 Then
        sg = "-"
      Else
        sg = "+"
        value = -value
      End If
      Dim cmd As String = "Sg" + sg + DegtoDDDMMSS(value)
      If Not CommandBoolSingleChar(cmd) Then
        Throw New ASCOM.InvalidOperationException
      End If
      mTL.LogMessage("Set SiteLongitude", value)
    End Set
  End Property

  Public Property SlewSettleTime() As Short Implements ITelescopeV3.SlewSettleTime
    Get
      mTL.LogMessage("Get SlewSettleTime", "Not implemented")
      Throw New ASCOM.PropertyNotImplementedException("SlewSettleTime", False)
    End Get
    Set(value As Short)
      mTL.LogMessage("Set SlewSettleTime", "Not implemented")
      Throw New ASCOM.PropertyNotImplementedException("SlewSettleTime", True)
    End Set
  End Property

  Public Sub SlewToAltAz(Azimuth As Double, Altitude As Double) Implements ITelescopeV3.SlewToAltAz
    setAzalt(Azimuth, Altitude)
    checkslewALTAZ()
    doslew(False, True)
    mTL.LogMessage("SlewToAltAzAz", "done")
  End Sub

  Public Sub SlewToAltAzAsync(Azimuth As Double, Altitude As Double) Implements ITelescopeV3.SlewToAltAzAsync
    setAzalt(Azimuth, Altitude)
    checkslewALTAZ()
    doslew(True, True)
    mTL.LogMessage("SlewToAltAzAsync", "done")
  End Sub

  Public Sub SlewToCoordinates(RightAscension As Double, Declination As Double) Implements ITelescopeV3.SlewToCoordinates
    mTL.LogMessage("SlewToCoordinates", "done")
    Me.TargetDeclination = Declination
    Me.TargetRightAscension = RightAscension
    Me.SlewToTarget()
  End Sub

  Public Sub SlewToCoordinatesAsync(RightAscension As Double, Declination As Double) Implements ITelescopeV3.SlewToCoordinatesAsync
    mTL.LogMessage("SlewToCoordinatesAsync", "done")
    Me.TargetDeclination = Declination
    Me.TargetRightAscension = RightAscension
    Me.SlewToTargetAsync()
  End Sub

  Public Sub SlewToTarget() Implements ITelescopeV3.SlewToTarget
    checkslewRADEC()
    doslew(False)
    mTL.LogMessage("SlewToTarget", "done")
  End Sub

  Public Sub SlewToTargetAsync() Implements ITelescopeV3.SlewToTargetAsync
    checkslewRADEC()
    doslew(True)
    mTL.LogMessage("SlewToTarget", "done")
  End Sub

  Public ReadOnly Property Slewing() As Boolean Implements ITelescopeV3.Slewing
    Get
      Slewing = CommandBoolString("GXJS")
      mTL.LogMessage("slewing", Slewing)
      Return Slewing
    End Get
  End Property

  Public Sub SyncToAltAz(Azimuth As Double, Altitude As Double) Implements ITelescopeV3.SyncToAltAz
    setAzalt(Azimuth, Altitude)
    CommandString("CA")
    mTL.LogMessage("SyncToAltAz", "done")
  End Sub

  Public Sub SyncToCoordinates(RightAscension As Double, Declination As Double) Implements ITelescopeV3.SyncToCoordinates
    Me.TargetDeclination = Declination
    Me.TargetRightAscension = RightAscension
    SyncToTarget()
    mTL.LogMessage("SyncToCoordinates", "done")
  End Sub

  Public Sub SyncToPark()
    If CommandBoolSingleChar("hO") Then
      mTL.LogMessage("SyncToPark", "done")
    Else
      Throw New ASCOM.InvalidOperationException("SyncToPark has failed")
      mTL.LogMessage("SyncToPark", "Failed")
    End If
  End Sub

  Public Sub SyncToTarget() Implements ITelescopeV3.SyncToTarget
    If Me.AtPark Then
      Throw New ASCOM.InvalidOperationException(ErrorCodes.InvalidWhileParked)
    End If
    If Not Me.Tracking Then
      Throw New ASCOM.InvalidOperationException
    End If
    If CommandString("CM") = "N/A" Then
      mTL.LogMessage("SyncToTarget", "done")
    Else
      Throw New ASCOM.InvalidOperationException("SyncToTarget has failed")
      mTL.LogMessage("SyncToTarget", "Failed")
    End If
  End Sub

  Public Property TargetDeclination() As Double Implements ITelescopeV3.TargetDeclination
    Get
      If mtgtDec = -999 Then
        Throw New ASCOM.ValueNotSetException
      End If
      mTL.LogMessage("Get TargetDeclination", mtgtDec.ToString)
      Return mtgtDec
    End Get
    Set(value As Double)
      mTL.LogMessage("Set TargetDeclination", value.ToString)
      Dim sexa As String = DecToString(value)
      Dim cmd As String = "Sd" & sexa
      If Not CommandBoolSingleChar(cmd) Then
        Throw New ASCOM.InvalidOperationException("Set Target Declination " & cmd & " has failed")
      End If

      mtgtDec = value
    End Set
  End Property



  Private Function DecToString(value As Double) As String
    If value < -90 Or value > 90 Then
      Throw New ASCOM.InvalidValueException
    End If
    Dim sexa As String = mutilities.DegreesToDMS(value, ":", ":", "") ' Long format, whole seconds
    If Strings.Left(sexa, 1) <> "-" Then
      sexa = "+" & sexa         ' Both need leading '+'
    End If
    Return sexa
  End Function

  Private Function AltToString(value As Double) As String
    If value < -30 Or value > 90 Then
      Throw New ASCOM.InvalidValueException
    End If
    Dim sexa As String = mutilities.DegreesToDMS(value, ":", ":", "") ' Long format, whole seconds
    If Strings.Left(sexa, 1) <> "-" Then
      sexa = "+" & sexa         ' Both need leading '+'
    End If
    Return sexa
  End Function

  Private Function AzToString(value As Double) As String
    If value < 0 Or value > 360 Then
      Throw New ASCOM.InvalidValueException
    End If
    Dim sexa As String = DegtoDDDMMSS(value)
    Return sexa
  End Function

  Public Property TargetRightAscension() As Double Implements ITelescopeV3.TargetRightAscension
    Get
      If mtgtRa = -999 Then
        Throw New ASCOM.ValueNotSetException
      End If
      mTL.LogMessage("Get TargetRightAscension", mtgtRa.ToString)
      Return mtgtRa
    End Get
    Set(value As Double)
      mTL.LogMessage("Set TargetRightAscension", value.ToString)
      Dim sexa As String = RaToString(value)   ' Long format, whole seconds
      Dim cmd As String = "Sr" & sexa
      If Not CommandBoolSingleChar(cmd) Then
        Throw New ASCOM.InvalidOperationException("Set Target RightAscension " & cmd & " has failed")
      End If
      mtgtRa = value
    End Set
  End Property

  Private Function RaToString(value As Double) As String
    If value < 0 Or value > 24 Then
      Throw New ASCOM.InvalidValueException

    End If
    Return mutilities.HoursToHMS(value, ":", ":", "")   ' Long format, whole seconds

  End Function

  Public Property Tracking() As Boolean Implements ITelescopeV3.Tracking
    Get
      Dim trk As Boolean = CommandBoolString("GXJT")
      mTL.LogMessage("Get Tracking", trk.ToString())
      Return trk
    End Get
    Set(value As Boolean)
      If value Then
        If Not CommandBoolSingleChar("Te") Then
          Throw New ASCOM.InvalidValueException
        End If
      Else
        If Not CommandBoolSingleChar("Td") Then
          Throw New ASCOM.InvalidOperationException
        End If
      End If
      mTL.LogMessage("Set Tracking", value.ToString)
    End Set
  End Property

  Public Property TrackingRate() As DriveRates Implements ITelescopeV3.TrackingRate
    Get
      Dim trackingRate__1 As DriveRates ' = DriveRates.driveSidereal
      updateTelStatus()
      Select Case mTelStatus.Substring(1, 1)
        Case "0"
          trackingRate__1 = DriveRates.driveSidereal
        Case "1"
          trackingRate__1 = DriveRates.driveSolar
        Case "2"
          trackingRate__1 = DriveRates.driveLunar
      End Select
      Return trackingRate__1
    End Get
    Set(value As DriveRates)
      Select Case value
        Case DriveRates.driveSidereal
          CommandBlind("TQ")
        Case DriveRates.driveSolar
          CommandBlind("TS")
        Case DriveRates.driveLunar
          CommandBlind("TL")
        Case Else
          Throw New InvalidValueException("Unsupported TrackingRate")
          'Case DriveRates.driveKing
          'CommandBlind("TK")
      End Select
    End Set
  End Property

  Public ReadOnly Property TrackingRates() As ITrackingRates Implements ITelescopeV3.TrackingRates
    Get
      Dim trackingRates__1 As ITrackingRates = New TrackingRates()
      For Each driveRate As DriveRates In trackingRates__1
        mTL.LogMessage("Get TrackingRates", driveRate.ToString())
      Next
      Return trackingRates__1
    End Get
  End Property

  Public Property UTCDate() As DateTime Implements ITelescopeV3.UTCDate
    Get
      Try
        Dim secs As Double = CDbl(CommandString("GXT2"))
        Dim utcDate__1 As DateTime = New DateTime(1970, 1, 1, 0, 0, 0).AddSeconds(secs)
        mTL.LogMessage("Get UTCDate", String.Format("Get - {0}", utcDate__1))
        Return utcDate__1
      Catch ex As Exception
        mTL.LogMessage("Get UTCDate", "failed")
        Throw New ASCOM.InvalidOperationException()
      End Try
    End Get
    Set(value As DateTime)
      Dim s As Long = (value - New DateTime(1970, 1, 1, 0, 0, 0)).TotalSeconds
      If (CommandBoolSingleChar("SXT2," & s)) Then
        mTL.LogMessage("Set UTCDate", "done")
      Else
        mTL.LogMessage("Set UTCDate", "failed")
        Throw New ASCOM.InvalidOperationException()
      End If
    End Set
  End Property

  Public Sub Unpark() Implements ITelescopeV3.Unpark
    If CommandBoolSingleChar("hR") Then
      mTL.LogMessage("Unpark", "done")
    Else
      mTL.LogMessage("Unpark", "failed")
      Throw New ASCOM.InvalidOperationException()
    End If
  End Sub

  Private Sub doslew(ByRef async As Boolean, Optional ByRef altaz As Boolean = False)

    Dim state As String
    If altaz Then
      state = Me.CommandSingleChar("MA", False)
    Else
      state = Me.CommandSingleChar("MS", False)
    End If
    If state.Length = 0 Then
      Throw New ASCOM.DriverException("Telescope is not replying")
    ElseIf Not state.Length = 1 Then
      Throw New ASCOM.DriverException("Telescope reply is corrupt")
    End If
    If state = "0" Then
      mTL.LogMessage("Slew to target", "Started")
      If (Not async) Then
        While (Slewing)
          Threading.Thread.Sleep(1000)
        End While
      End If
    Else
      ReportState(state)
    End If
  End Sub

  Private Sub ReportState(ByVal state As String)
    Dim val As Integer = Asc(state(0)) - Asc("0"c)
    Select Case val
      Case 1
        Throw New ASCOM.InvalidOperationException("Object below min altitude")
      Case 2
        Throw New ASCOM.ValueNotSetException
      Case 3
        Throw New ASCOM.InvalidOperationException("Mount Cannot Flip here")
      Case 4
        Throw New ASCOM.ParkedException
      Case 5
        Throw New ASCOM.InvalidOperationException("Telescope is Slewing")
      Case 6
        Throw New ASCOM.InvalidOperationException("Object is outside limits")
      Case 7
        Throw New ASCOM.InvalidOperationException("Telescope is Guiding")
      Case 8
        Throw New ASCOM.InvalidOperationException("Object above max altitude")
      Case 9
        Throw New ASCOM.InvalidOperationException("Motor is fault")
      Case 11
        Throw New ASCOM.InvalidOperationException("Motor is fault")
      Case 12
        Throw New ASCOM.InvalidOperationException("Telescope is below horizon limit")
      Case 13
        Throw New ASCOM.InvalidOperationException("Limit Sensor")
      Case 14
        Throw New ASCOM.InvalidOperationException("Telescope is outside Axis 1 limit")
      Case 15
        Throw New ASCOM.InvalidOperationException("Telescope is outside Axis 2 limit")
      Case 16
        Throw New ASCOM.InvalidOperationException("Telescope is above overhead limit")
      Case 17
        Throw New ASCOM.InvalidOperationException("Telescope is outside meridian limit")
    End Select
  End Sub

  Private Function checkCompatibility() As Boolean
    Dim asm As System.Reflection.Assembly = System.Reflection.Assembly.GetExecutingAssembly()
    Dim fvi As FileVersionInfo = FileVersionInfo.GetVersionInfo(asm.Location)
    Dim versionFW() As String = Me.CommandString("GVN").Split(".")
    Dim versionASCOM() As String = fvi.FileVersion.Split(".")
    If versionFW(0) = versionASCOM(0) And versionFW(1) = versionASCOM(1) Then
      Return True
    End If
    MsgBox(
  "Connection has failed!" & vbCrLf &
  "TeenAstro version is " & versionFW(0) & "." & versionFW(1) & "." & versionFW(2) &
  ", TeenAstro driver version is " & fvi.FileVersion)
    Return False
  End Function

  Private Sub updateTelStatus()
    mTelStatus = Me.CommandString("GXI")
  End Sub


  Private Sub DegtoDMS(ByVal degf As Double, ByRef degi As Integer, ByRef mini As Integer, ByRef seci As Integer)
    Dim tts As Integer = Math.Floor(degf * 3600 + 0.5)
    degi = tts \ 3600
    mini = (tts - degi * 3600) \ 60
    seci = tts Mod 60
  End Sub
  Private Function DegtoDDDMMSS(value As Double) As String
    Dim d, m, s As Integer
    DegtoDMS(value, d, m, s)
    Return d.ToString("000") + ":" + m.ToString("00") + ":" + s.ToString("00")
  End Function
  Private Function DegtoDDMMSS(value As Double) As String
    Dim d, m, s As Integer
    DegtoDMS(value, d, m, s)
    Return d.ToString("00") + ":" + m.ToString("00") + ":" + s.ToString("00")
  End Function

  Private Sub checkslewRADEC()
    If mtgtDec = -999 Or mtgtRa = -999 Then
      Throw New ASCOM.ValueNotSetException
    End If
    If Me.AtPark Then
      Throw New ASCOM.ParkedException
    End If
    If Not Me.Tracking Then
      Throw New ASCOM.InvalidOperationException
    End If
    While Me.Slewing
      Me.AbortSlew()
      Threading.Thread.Sleep(200)
    End While
    While Me.IsPulseGuiding
      Me.AbortSlew()
      Threading.Thread.Sleep(200)
    End While
  End Sub

  Private Sub checkFlip()
    If Me.AtPark Then
      Throw New ASCOM.ParkedException
    End If
    If Not Me.Tracking Then
      Throw New ASCOM.InvalidOperationException
    End If
    While Me.Slewing
      Me.AbortSlew()
      Threading.Thread.Sleep(100)
    End While
    While Me.IsPulseGuiding
      Me.AbortSlew()
      Threading.Thread.Sleep(100)
    End While
  End Sub


  Private Sub checkslewALTAZ()

    If Me.AtPark Then
      Throw New ASCOM.ParkedException
    End If


    While Me.Slewing
      Me.AbortSlew()
      Threading.Thread.Sleep(100)
    End While
  End Sub

  Private Sub setAzalt(Azimuth As Double, Altitude As Double)
    Dim sexa As String = AzToString(Azimuth)
    If Not CommandBoolSingleChar("Sz" & sexa) Then
      Throw New ASCOM.InvalidOperationException
    End If
    sexa = AltToString(Altitude)
    If Not CommandBoolSingleChar("Sa" & sexa) Then
      Throw New ASCOM.InvalidOperationException
    End If
  End Sub



#End Region

#Region "Private properties and methods"
  ' here are some useful properties and methods that can be used as required
  ' to help with

#Region "ASCOM Registration"

  Private Shared Sub RegUnregASCOM(ByVal bRegister As Boolean)

    Using P As New Profile() With {.DeviceType = "Telescope"}
      If bRegister Then
        P.Register(driverID, driverDescription)
      Else
        P.Unregister(driverID)
      End If
    End Using

  End Sub

  <ComRegisterFunction()>
  Public Shared Sub RegisterASCOM(ByVal T As Type)

    RegUnregASCOM(True)

  End Sub

  <ComUnregisterFunction()>
  Public Shared Sub UnregisterASCOM(ByVal T As Type)

    RegUnregASCOM(False)

  End Sub

#End Region

  ''' <summary>
  ''' Returns true if there is a valid connection to the driver hardware
  ''' </summary>
  Private ReadOnly Property IsConnected As Boolean
    Get
      ' TODO check that the driver hardware connection exists and is connected to the hardware
      Dim s1 As Double = (Date.UtcNow - mConnectionStatusDate).TotalMilliseconds
      ' TODO check that the driver hardware connection exists and is connected to the hardware

      Try
        If (s1 > 1000 And mconnectedState) Then
          mconnectedState = False
          Dim k As Integer = 0
          While Not mconnectedState And k < 10
            mconnectedState = CommandBoolString("GXJC")
            Threading.Thread.Sleep(200)
            k += 1
          End While
          mconnectedState = k < 10
        End If
      Catch ex As Exception
        'Throw New ASCOM.DriverException(ex.Message)
        mconnectedState = False
      End Try
      Return mconnectedState
    End Get
  End Property

  ''' <summary>
  ''' Use this function to throw an exception if we aren't connected to the hardware
  ''' </summary>
  ''' <param name="message"></param>
  Private Sub CheckConnected(ByVal message As String)
    If Not IsConnected Then
      Throw New NotConnectedException(message)
    End If
  End Sub



  ''' <summary>
  ''' Read the device configuration from the ASCOM Profile store
  ''' </summary>
  Friend Sub ReadProfile()
    Using driverProfile As New Profile()
      driverProfile.DeviceType = "Telescope"
      mtraceState = Convert.ToBoolean(driverProfile.GetValue(driverID, mtraceStateProfileName, String.Empty, mtraceStateDefault))
      mcomPort = driverProfile.GetValue(driverID, mcomPortProfileName, String.Empty, mcomPortDefault)
      mIP = driverProfile.GetValue(driverID, mIPProfileName, String.Empty, mIPDefault)
      mPort = driverProfile.GetValue(driverID, mPortProfileName, String.Empty, mPortDefault)
      mInterface = driverProfile.GetValue(driverID, mInterfaceProfileName, String.Empty, mInterfaceDefault)
    End Using
  End Sub


  ''' <summary>
  ''' Write the device configuration to the  ASCOM  Profile store
  ''' </summary>
  Friend Sub WriteProfile()
    Using driverProfile As New Profile()
      driverProfile.DeviceType = "Telescope"
      driverProfile.WriteValue(driverID, mtraceStateProfileName, mtraceState.ToString())
      driverProfile.WriteValue(driverID, mcomPortProfileName, mcomPort.ToString())
      driverProfile.WriteValue(driverID, mIPProfileName, mIP.ToString())
      driverProfile.WriteValue(driverID, mPortProfileName, mPort.ToString())
      driverProfile.WriteValue(driverID, mInterfaceProfileName, mInterface.ToString())
    End Using
  End Sub
#End Region



End Class

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
Imports System.Runtime.InteropServices
Imports System.Text
Imports System.Text.RegularExpressions

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

  Private mupdateRate As Double = -1
  Private mRa As Double
  Private mRadate As Date = Date.UtcNow
  Private mDec As Double
  Private mDecdate As Date = Date.UtcNow
  Private mAlt As Double
  Private mAltdate As Date = Date.UtcNow
  Private mAZ As Double
  Private mAZdate As Date = Date.UtcNow
  Private mTelStatus As String
  Private mTelStatusDate As Date = Date.UtcNow

  Private mtgtRa As Double = -999
  Private mtgtDec As Double = -999



  Private mutilities As Util ' Private variable to hold an ASCOM Utilities object
  Private mastroUtilities As AstroUtils ' Private variable to hold an AstroUtils object to provide the Range method
  Private mTL As TraceLogger ' Private variable to hold the trace logger object (creates a diagnostic log file with information that you specify)

  ' Slew speeds for speed settings 0-4 as defined in the main unit's Global.h in the array guideRates[].
  ' Values are given as multiples of sidereal speed. all these values are overwritten by EEPROM at runtime.
  Private mSlewSpeeds() As Double = {1, 4, 16, 64, 64}
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
      mTL.LogMessage("SupportedActions Get", "Returning empty arraylist")
      Return New ArrayList()
    End Get
  End Property

  Public Function Action(ByVal ActionName As String, ByVal ActionParameters As String) As String Implements ITelescopeV3.Action
    Throw New ActionNotImplementedException("Action " & ActionName & " is not supported by this driver")
  End Function

  Private Function GenericCommand(ByVal Command As String, ByVal Raw As Boolean,
                                  ByVal Mode As Integer, ByRef buf As String) As Boolean
    If Not Raw Then
      Command = ":" & Command & "#"
    End If
    If mInterface = "COM" Then
      Return GetSerial(Command, Mode, buf)
    ElseIf mInterface = "IP" Then
      Return getStreamIter(Command, Mode, buf)
    End If
    Return False
  End Function

  Public Sub CommandBlind(ByVal Command As String, Optional ByVal Raw As Boolean = False) Implements ITelescopeV3.CommandBlind
    Dim buf As String = ""
    If Not GenericCommand(Command, Raw, 0, buf) Then
      Throw New ASCOM.InvalidValueException("CommandBlind " + Command + " has failed")
    End If
  End Sub

  Public Function CommandBool(ByVal Command As String, Optional ByVal Raw As Boolean = False) As Boolean _
      Implements ITelescopeV3.CommandBool
    Dim buf As String = ""
    If Not GenericCommand(Command, Raw, 1, buf) Then
      Throw New ASCOM.InvalidValueException("CommandBool " + Command + " has failed")
    End If
    Return buf = "1"
  End Function

  Private Function CommandSingleChar(ByVal Command As String, Optional ByVal Raw As Boolean = False) As String
    Dim buf As String = ""
    If Not GenericCommand(Command, Raw, 1, buf) Then
      Throw New ASCOM.InvalidValueException("CommandSingleChar " + Command + " has failed")
    End If
    Return buf
  End Function


  Public Function CommandString(ByVal Command As String, Optional ByVal Raw As Boolean = False) As String _
      Implements ITelescopeV3.CommandString
    Dim buf As String = ""
    If Not GenericCommand(Command, Raw, 2, buf) Then
      Throw New ASCOM.InvalidValueException("CommandString " + Command + " has failed")
    End If
    Return buf
  End Function

  Private Function MyDevice() As Boolean
    MyDevice = False
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
      mupdateRate = -1
      Try
        mobjectSerial.Connected = True
      Catch ex As Exception
        mconnectedState = False
        mobjectSerial.Dispose()
        Throw New ASCOM.DriverException(ex.Message)
      End Try
      mobjectSerial.ReceiveTimeout = 1
      If Not MyDevice() Then
        mobjectSerial.Connected = False
        mconnectedState = False
        mobjectSerial.Dispose()
        Return
      Else
        mconnectedState = True
      End If
      If Not mconnectedState Then
        Throw New ASCOM.InvalidValueException("Connection has failed!")
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
      mupdateRate = -1
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
      End If
    Else
      mconnectedState = False
      mTL.LogMessage("Connected Set", "Disconnecting from IP " + mIP)
    End If
  End Sub

  Public Property Connected() As Boolean Implements ITelescopeV3.Connected
    Get
      mTL.LogMessage("Connected Get", IsConnected.ToString())
      Return IsConnected
    End Get
    Set(value As Boolean)
      mTL.LogMessage("Connected Set", value.ToString())
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

  Private Function getStreamIter(ByVal Command As String, ByVal Mode As Integer, ByRef buf As String) As Boolean
    Dim k As Integer = 0
    While k < 3
      buf = ""
      If getStream(Command, Mode, buf) Then
        Return True
      Else
        k += 1
      End If
    End While
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
      getStream = False
    End Try
    ClientSocket.Close()
  End Function

  Private Function GetSerial(ByVal Command As String, ByVal Mode As Integer, ByRef buf As String) As Boolean
    mobjectSerial.ClearBuffers()
    mobjectSerial.ReceiveTimeout = 100
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
  End Function

  Public ReadOnly Property Description As String Implements ITelescopeV3.Description
    Get
      ' this pattern seems to be needed to allow a public property to return a private field
      Dim d As String = driverDescription
      mTL.LogMessage("Description Get", d)
      Return d
    End Get
  End Property

  Public ReadOnly Property DriverInfo As String Implements ITelescopeV3.DriverInfo
    Get
      Dim m_version As Version = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version
      ' TODO customise this driver description
      Dim s_driverInfo As String = "Information about the driver itself. Version: " + m_version.Major.ToString() + "." + m_version.Minor.ToString()
      mTL.LogMessage("DriverInfo Get", s_driverInfo)
      Return s_driverInfo
    End Get
  End Property

  Public ReadOnly Property DriverVersion() As String Implements ITelescopeV3.DriverVersion
    Get
      ' Get our own assembly and report its version number
      mTL.LogMessage("DriverVersion Get", Reflection.Assembly.GetExecutingAssembly.GetName.Version.ToString(2))
      Return Reflection.Assembly.GetExecutingAssembly.GetName.Version.ToString(2)
    End Get
  End Property

  Public ReadOnly Property InterfaceVersion() As Short Implements ITelescopeV3.InterfaceVersion
    Get
      mTL.LogMessage("InterfaceVersion Get", "3")
      Return 3
    End Get
  End Property

  Public ReadOnly Property Name As String Implements ITelescopeV3.Name
    Get
      Dim s_name As String = "TeenAstro"
      mTL.LogMessage("Name Get", s_name)
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
      ElseIf m = "k" Or m = "K" Then
        Return AlignmentModes.algAltAz
      End If

    End Get
  End Property

  Public ReadOnly Property Altitude() As Double Implements ITelescopeV3.Altitude
    Get
      Dim s1 As Double = (Date.UtcNow - mAltdate).TotalMilliseconds
      If s1 > mupdateRate Then
        mAlt = mutilities.DMSToDegrees(Me.CommandString("GA"))
        mAltdate = Date.UtcNow
      End If
      mTL.LogMessage("Altitude", mAlt.ToString("0.0000000"))
      Return mAlt
    End Get
  End Property

  Public ReadOnly Property ApertureArea() As Double Implements ITelescopeV3.ApertureArea
    Get
      mTL.LogMessage("ApertureArea Get", "Not implemented")
      Throw New ASCOM.PropertyNotImplementedException("ApertureArea", False)
    End Get
  End Property

  Public ReadOnly Property ApertureDiameter() As Double Implements ITelescopeV3.ApertureDiameter
    Get
      mTL.LogMessage("ApertureDiameter Get", "Not implemented")
      Throw New ASCOM.PropertyNotImplementedException("ApertureDiameter", False)
    End Get
  End Property

  Public ReadOnly Property AtHome() As Boolean Implements ITelescopeV3.AtHome
    Get
      updateTelStatus()
      Dim isAtHome As Boolean = mTelStatus.Substring(3, 1) = "H"
      mTL.LogMessage("AtHome", "Get - " & isAtHome.ToString())
      Return isAtHome
    End Get
  End Property

  Public ReadOnly Property AtPark() As Boolean Implements ITelescopeV3.AtPark
    Get
      updateTelStatus()
      Dim isAtPark As Boolean = mTelStatus.Substring(2, 1) = "P"
      mTL.LogMessage("AtPark", "Get - " & isAtPark.ToString())
      Return isAtPark
    End Get
  End Property

  Public Function AxisRates(Axis As TelescopeAxes) As IAxisRates Implements ITelescopeV3.AxisRates
    mTL.LogMessage("AxisRates", "Get - " & Axis.ToString())
    ' Read maxSpeed from TeenAstro main unit and assign top two speed settings on this basis
    Dim Speed As Double, response As String
    'MAX_SPEED
    response = Me.CommandString("GXRX")
    mTL.LogMessage("AxisRates", "Get value: " & response)
    If Not (Double.TryParse(response, Speed)) Then
      Throw New ASCOM.InvalidValueException("Retrieve MAX_SPEED via :GXRX# has failed: '" & response & "'")
    End If
    mSlewSpeeds(mSlewSpeeds.GetUpperBound(0)) = Speed
    'FAST_SPEED
    response = Me.CommandString("GXR3")
    mTL.LogMessage("AxisRates", "Get value: " & response)
    If Not (Double.TryParse(response, Speed)) Then
      Throw New ASCOM.InvalidValueException("Retrieve FAST_SPEED via :GXR3# has failed: '" & response & "'")
    End If
    mSlewSpeeds(mSlewSpeeds.GetUpperBound(0) - 1) = Speed
    'MEDIUM_SPEED
    response = Me.CommandString("GXR2")
    mTL.LogMessage("AxisRates", "Get value: " & response)
    If Not (Double.TryParse(response, Speed)) Then
      Throw New ASCOM.InvalidValueException("Retrieve MEDIUM_SPEED via :GXR2# has failed: '" & response & "'")
    End If
    mSlewSpeeds(mSlewSpeeds.GetUpperBound(0) - 2) = Speed
    'SLOW_SPEED
    response = Me.CommandString("GXR1")
    mTL.LogMessage("AxisRates", "Get value: " & response)
    If Not (Double.TryParse(response, Speed)) Then
      Throw New ASCOM.InvalidValueException("Retrieve MEDIUM_SLOW via :GXR1# has failed: '" & response & "'")
    End If
    'GUIDE_SPEED
    mSlewSpeeds(mSlewSpeeds.GetUpperBound(0) - 3) = Speed
    response = Me.CommandString("GXR0")
    mTL.LogMessage("AxisRates", "Get value: " & response)
    If Not (Double.TryParse(response, Speed)) Then
      Throw New ASCOM.InvalidValueException("Retrieve GUIDE_SPEED via :GXR0# has failed: '" & response & "'")
    End If
    mSlewSpeeds(mSlewSpeeds.GetUpperBound(0) - 4) = Speed / 100
    Return New AxisRates(Axis, mSlewSpeeds, mSiderealRate)
  End Function

  Public ReadOnly Property Azimuth() As Double Implements ITelescopeV3.Azimuth
    Get
      Dim s1 As Double = (Date.UtcNow - mAZdate).TotalMilliseconds
      If s1 > mupdateRate Then
        mAZ = mutilities.DMSToDegrees(Me.CommandString("GZ"))
        mAZdate = Date.UtcNow
      End If
      mTL.LogMessage("Azimuth", mAZ.ToString("0.0000000"))
      Return mAZ
    End Get
  End Property

  Public ReadOnly Property CanFindHome() As Boolean Implements ITelescopeV3.CanFindHome
    Get
      mTL.LogMessage("CanFindHome", "Get - " & False.ToString())
      Return False
    End Get
  End Property

  Public Function CanMoveAxis(Axis As TelescopeAxes) As Boolean Implements ITelescopeV3.CanMoveAxis
    mTL.LogMessage("CanMoveAxis", "Get - " & Axis.ToString())
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
      mTL.LogMessage("CanPark", "Get - " & value.ToString())
      Return value
    End Get
  End Property

  Public ReadOnly Property CanPulseGuide() As Boolean Implements ITelescopeV3.CanPulseGuide
    Get
      mTL.LogMessage("CanPulseGuide", "Get - " & True.ToString())
      Return True
    End Get
  End Property

  Public ReadOnly Property CanSetDeclinationRate() As Boolean Implements ITelescopeV3.CanSetDeclinationRate
    Get
      mTL.LogMessage("CanSetDeclinationRate", "Get - " & False.ToString())
      Return False
    End Get
  End Property

  Public ReadOnly Property CanSetGuideRates() As Boolean Implements ITelescopeV3.CanSetGuideRates
    Get
      mTL.LogMessage("CanSetGuideRates", "Get - " & False.ToString())
      Return False
    End Get
  End Property

  Public ReadOnly Property CanSetPark() As Boolean Implements ITelescopeV3.CanSetPark
    Get
      Dim value As Boolean = True
      mTL.LogMessage("CanSetPark", "Get - " & value.ToString())
      Return value
    End Get
  End Property

  Public ReadOnly Property CanSetPierSide() As Boolean Implements ITelescopeV3.CanSetPierSide
    Get
      Dim can As Boolean = (AlignmentMode = AlignmentModes.algGermanPolar)
      mTL.LogMessage("CanSetPierSide", "Get - " & can.ToString())
      Return can
    End Get
  End Property

  Public ReadOnly Property CanSetRightAscensionRate() As Boolean Implements ITelescopeV3.CanSetRightAscensionRate
    Get
      mTL.LogMessage("CanSetRightAscensionRate", "Get - " & False.ToString())
      Return False
    End Get
  End Property

  Public ReadOnly Property CanSetTracking() As Boolean Implements ITelescopeV3.CanSetTracking
    Get
      Dim value As Boolean = True
      mTL.LogMessage("CanSetTracking", "Get - " & value.ToString())
      Return value
    End Get
  End Property

  Public ReadOnly Property CanSlew() As Boolean Implements ITelescopeV3.CanSlew
    Get
      mTL.LogMessage("CanSlew", "Get - " & True.ToString())
      Return True
    End Get
  End Property

  Public ReadOnly Property CanSlewAltAz() As Boolean Implements ITelescopeV3.CanSlewAltAz
    Get
      mTL.LogMessage("CanSlewAltAz", "Get - " & False.ToString())
      Return False
    End Get
  End Property

  Public ReadOnly Property CanSlewAltAzAsync() As Boolean Implements ITelescopeV3.CanSlewAltAzAsync
    Get
      mTL.LogMessage("CanSlewAltAzAsync", "Get - " & False.ToString())
      Return False
    End Get
  End Property

  Public ReadOnly Property CanSlewAsync() As Boolean Implements ITelescopeV3.CanSlewAsync
    Get
      mTL.LogMessage("CanSlewAsync", "Get - " & True.ToString())
      Return True
    End Get
  End Property

  Public ReadOnly Property CanSync() As Boolean Implements ITelescopeV3.CanSync
    Get
      mTL.LogMessage("CanSync", "Get - " & True.ToString())
      Return True
    End Get
  End Property

  Public ReadOnly Property CanSyncAltAz() As Boolean Implements ITelescopeV3.CanSyncAltAz
    Get
      mTL.LogMessage("CanSyncAltAz", "Get - " & False.ToString())
      Return False
    End Get
  End Property

  Public ReadOnly Property CanUnpark() As Boolean Implements ITelescopeV3.CanUnpark
    Get
      mTL.LogMessage("CanUnpark", "Get - " & True.ToString())
      Return True
    End Get
  End Property

  Public ReadOnly Property Declination() As Double Implements ITelescopeV3.Declination
    Get
      Dim s1 As Double = (Date.UtcNow - mDecdate).TotalMilliseconds
      If s1 > mupdateRate Then
        mDec = mutilities.DMSToDegrees(Me.CommandString("GD"))
        mDecdate = Date.UtcNow
      End If
      mTL.LogMessage("Declination", "Get - " & mutilities.DegreesToDMS(mDec))
      Return mDec
    End Get
  End Property

  Public Property DeclinationRate() As Double Implements ITelescopeV3.DeclinationRate
    Get
      Return 0
    End Get
    Set(value As Double)
      mTL.LogMessage("DeclinationRate Set", "Not implemented")
      Throw New ASCOM.PropertyNotImplementedException("DeclinationRate", True)
    End Set
  End Property

  Public Function DestinationSideOfPier(RightAscension As Double, Declination As Double) As PierSide Implements ITelescopeV3.DestinationSideOfPier
    Dim RaString As String = RaToString(RightAscension)
    Dim DecString As String = DecToString(Declination)
    Dim state As String = Me.CommandString("M?" + RaString + DecString)
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
    mTL.LogMessage("DestinationSideOfPier Get", DestinationSideOfPier.ToString)
    Return DestinationSideOfPier
  End Function

  Public Property DoesRefraction() As Boolean Implements ITelescopeV3.DoesRefraction
    Get
      mTL.LogMessage("DoesRefraction Get", False.ToString)
      Return False
    End Get
    Set(value As Boolean)
      mTL.LogMessage("DoesRefraction Set", "Not implemented")
      Throw New ASCOM.PropertyNotImplementedException("DoesRefraction", True)
    End Set
  End Property

  Public ReadOnly Property EquatorialSystem() As EquatorialCoordinateType Implements ITelescopeV3.EquatorialSystem
    Get
      Dim equatorialSystem__1 As EquatorialCoordinateType = EquatorialCoordinateType.equTopocentric
      mTL.LogMessage("DeclinationRate", "Get - " & equatorialSystem__1.ToString())
      Return equatorialSystem__1
    End Get
  End Property

  Public Sub FindHome() Implements ITelescopeV3.FindHome
    mTL.LogMessage("FindHome", "Not implemented")
    Throw New ASCOM.MethodNotImplementedException("FindHome")
  End Sub

  Public ReadOnly Property FocalLength() As Double Implements ITelescopeV3.FocalLength
    Get
      mTL.LogMessage("FocalLength Get", "Not implemented")
      Throw New ASCOM.PropertyNotImplementedException("FocalLength", False)
    End Get
  End Property

  Public Property GuideRateDeclination() As Double Implements ITelescopeV3.GuideRateDeclination
    Get
      mTL.LogMessage("GuideRateDeclination Get", "Not implemented")
      Throw New ASCOM.PropertyNotImplementedException("GuideRateDeclination", False)
    End Get
    Set(value As Double)
      mTL.LogMessage("GuideRateDeclination Set", "Not implemented")
      Throw New ASCOM.PropertyNotImplementedException("GuideRateDeclination", True)
    End Set
  End Property

  Public Property GuideRateRightAscension() As Double Implements ITelescopeV3.GuideRateRightAscension
    Get
      mTL.LogMessage("GuideRateRightAscension Get", "Not implemented")
      Throw New ASCOM.PropertyNotImplementedException("GuideRateRightAscension", False)
    End Get
    Set(value As Double)
      mTL.LogMessage("GuideRateRightAscension Set", "Not implemented")
      Throw New ASCOM.PropertyNotImplementedException("GuideRateRightAscension", True)
    End Set
  End Property

  Public ReadOnly Property IsPulseGuiding() As Boolean Implements ITelescopeV3.IsPulseGuiding
    Get
      updateTelStatus()
      IsPulseGuiding = mTelStatus.Substring(6, 1) = "*"
      mTL.LogMessage("IsPulseGuiding Get", IsPulseGuiding.ToString)
      Return IsPulseGuiding
    End Get
  End Property

  Public Sub MoveAxis(Axis As TelescopeAxes, Rate As Double) Implements ITelescopeV3.MoveAxis
    mTL.LogMessage("MoveAxis", Axis.ToString() & ":" & Rate.ToString())

    ' Set slew speed to given rate, if different from currently set value.
    Dim absRate As Double = Math.Abs(Rate)
    Dim slewSpeed As String = ""
    For i = mSlewSpeeds.GetUpperBound(0) To 0 Step -1
      If (absRate >= 0.99 * mSlewSpeeds(i) * mSiderealRate) Then
        slewSpeed = "R" & i.ToString()
        Exit For
      End If
    Next
    If (slewSpeed <> "") And (slewSpeed <> mSlewSpeed) Then
      CommandBlind(slewSpeed)
      mSlewSpeed = slewSpeed
    End If

    ' Issue movement command
    Dim dir As String = ""
    If (Axis = TelescopeAxes.axisPrimary) Then
      If (Rate > 0) Then
        dir = "Me"
      ElseIf (Rate = 0) Then
        dir = "Qe"  ' TeenAstro main unit implementation halts both east- and westward slews
      Else
        dir = "Mw"
      End If
    ElseIf (Axis = TelescopeAxes.axisSecondary) Then
      If (Rate > 0) Then
        dir = "Mn"
      ElseIf (Rate = 0) Then
        dir = "Qn"  ' TeenAstro main unit implementation halts both north- and southward slews
      Else
        dir = "Ms"
      End If
    Else
      Throw New ASCOM.InvalidValueException("MoveAxis", Axis.ToString(), "0 To 1")
    End If
    CommandBlind(dir)
  End Sub

  Public Sub Park() Implements ITelescopeV3.Park
    If CommandBool("hP") Then
      mTL.LogMessage("Park", "done")
    Else
      mTL.LogMessage("Park", "failed")
      Throw New ASCOM.InvalidValueException("The park has failed")
    End If
  End Sub

  Public Sub PulseGuide(Direction As GuideDirections, Duration As Integer) Implements ITelescopeV3.PulseGuide

    Dim ok As Boolean = False
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
    'If ok Then
    '    mTL.LogMessage("PulseGuide", dir & Duration & " done ")
    'Else
    '    Throw New ASCOM.InvalidValueException("Pulse guiding failed")
    '    mTL.LogMessage("PulseGuide", dir & Duration & " has failed ")
    'End If

  End Sub

  Public ReadOnly Property RightAscension() As Double Implements ITelescopeV3.RightAscension
    Get
      Dim s1 As Double = (Date.UtcNow - mRadate).TotalMilliseconds()
      If s1 > mupdateRate Then
        mRa = mutilities.HMSToHours(Me.CommandString("GR"))
        mRadate = Date.UtcNow
      End If
      mTL.LogMessage("RightAscension", "Get - " & mutilities.HoursToHMS(mRa))
      Return mRa
    End Get
  End Property

  Public Property RightAscensionRate() As Double Implements ITelescopeV3.RightAscensionRate
    Get
      Return 0
    End Get
    Set(value As Double)
      mTL.LogMessage("RightAscensionRate Set", "Not implemented")
      Throw New ASCOM.PropertyNotImplementedException("RightAscensionRate", True)
    End Set
  End Property

  Public Sub SetPark() Implements ITelescopeV3.SetPark
    Dim ok As Boolean = CommandBool("hQ")
    If ok Then
      mTL.LogMessage("SetPark", "done")
    Else
      mTL.LogMessage("SetPark", "failed")
      Throw New ASCOM.InvalidValueException("Set Park failed")
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
      If CanSetPierSide Then
        If value = PierSide.pierUnknown Then
          Throw New ASCOM.InvalidValueException("German mount cannot be set with pierUnknow")
        ElseIf Not Me.SideOfPier = value Then
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
      Return lst
    End Get
  End Property

  Public Property SiteElevation() As Double Implements ITelescopeV3.SiteElevation
    Get
      'todo
      Dim elev As Double = Me.CommandString("Ge")
      mTL.LogMessage("SiteElevation Get", elev.ToString("0.0"))
      Return elev
    End Get
    Set(value As Double)
      If value > 10000 Or value < -300 Then
        Throw New ASCOM.InvalidValueException
      End If
      value = Int(value)
      mTL.LogMessage("SiteElevation Set", value.ToString("0"))
      Dim sg As String = ""
      If value >= 0 Then
        sg = "+"
      Else
        sg = "-"
      End If
      Dim cmd As String = "Se" + sg + Math.Abs(value).ToString("00000")
      Me.CommandBool(cmd)
    End Set
  End Property

  Public Property SiteLatitude() As Double Implements ITelescopeV3.SiteLatitude
    Get
      Dim lati As Double = mutilities.DMSToDegrees(Me.CommandString("Gt"))
      mTL.LogMessage("SiteLatitude Get", lati.ToString("0.000000"))
      Return lati
    End Get
    Set(value As Double)
      If value > 90 Or value < -90 Then
        Throw New ASCOM.InvalidValueException
      End If
      Dim deg = Math.Abs(Math.Truncate(value))
      Dim min = 60 * (Math.Abs(value) - deg)
      Dim sg As String = ""
      If value >= 0 Then
        sg = "+"
      Else
        sg = "-"
      End If
      Dim cmd As String = "St" + sg + deg.ToString("00") + "*" + min.ToString("00")
      Me.CommandBool(cmd)
      mTL.LogMessage("SiteLatitude Set", value)
    End Set
  End Property

  Public Property SiteLongitude() As Double Implements ITelescopeV3.SiteLongitude
    Get
      Dim longi As Double = mutilities.DMSToDegrees(Me.CommandString("Gg")) * -1
      mTL.LogMessage("SiteLongitude Get", longi.ToString("0.000000"))
      Return longi
    End Get
    Set(value As Double)
      If value > 180 Or value < -180 Then
        Throw New ASCOM.InvalidValueException
      End If
      value = -1 * value
      Dim deg = Math.Abs(Math.Truncate(value))
      Dim min = 60 * (Math.Abs(value) - deg)
      Dim sg As String = ""
      If value >= 0 Then
        sg = "+"
      Else
        sg = "-"
      End If
      Dim cmd As String = "Sg" + sg + deg.ToString("000") + "*" + min.ToString("00")
      Me.CommandBool(cmd)
      mTL.LogMessage("SiteLongitude Set", value)
    End Set
  End Property

  Public Property SlewSettleTime() As Short Implements ITelescopeV3.SlewSettleTime
    Get
      mTL.LogMessage("SlewSettleTime Get", "Not implemented")
      Throw New ASCOM.PropertyNotImplementedException("SlewSettleTime", False)
    End Get
    Set(value As Short)
      mTL.LogMessage("SlewSettleTime Set", "Not implemented")
      Throw New ASCOM.PropertyNotImplementedException("SlewSettleTime", True)
    End Set
  End Property

  Public Sub SlewToAltAz(Azimuth As Double, Altitude As Double) Implements ITelescopeV3.SlewToAltAz
    setAzalt(Azimuth, Altitude)
    checkslew()
    doslew(False, True)
    mTL.LogMessage("SlewToAltAzAz", "done")
  End Sub

  Public Sub SlewToAltAzAsync(Azimuth As Double, Altitude As Double) Implements ITelescopeV3.SlewToAltAzAsync
    setAzalt(Azimuth, Altitude)
    checkslew()
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
    checkslew()
    doslew(False)
    mTL.LogMessage("SlewToTarget", "done")
  End Sub

  Public Sub SlewToTargetAsync() Implements ITelescopeV3.SlewToTargetAsync
    checkslew()
    doslew(True)
    mTL.LogMessage("SlewToTarget", "done")
  End Sub

  Public ReadOnly Property Slewing() As Boolean Implements ITelescopeV3.Slewing
    Get
      updateTelStatus()
      If mTelStatus.Substring(0, 1) = "2" Or mTelStatus.Substring(0, 1) = "3" Or mTelStatus.Substring(6, 1) = "+" Then
        Slewing = True
      Else
        Slewing = False
      End If
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
    If CommandBool("hO") Then
      mTL.LogMessage("SyncToPark", "done")
    Else
      Throw New ASCOM.InvalidOperationException("SyncToPark has failed")
      mTL.LogMessage("SyncToPark", "Failed")
    End If
  End Sub

  Public Sub SyncToTarget() Implements ITelescopeV3.SyncToTarget
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
      mTL.LogMessage("TargetDeclination Get", mtgtDec.ToString)
      Return mtgtDec
    End Get
    Set(value As Double)
      mTL.LogMessage("TargetDeclination Set", value.ToString)
      Dim sexa As String = DecToString(value)
      If Not Me.CommandBool("Sd" & sexa) Then
        Throw New ASCOM.InvalidOperationException
      End If

      mtgtDec = value
    End Set
  End Property

  Private Function DecToString(value As Double) As String
    If value < -90 Or value > 90 Then
      Throw New ASCOM.InvalidValueException
    End If
    Dim sexa As String = mutilities.DegreesToDMS(value, ":", ":", "") ' Long format, whole seconds
    If Left$(sexa, 1) <> "-" Then
      sexa = "+" & sexa         ' Both need leading '+'
    End If
    Return sexa
  End Function

  Private Function AzToString(value As Double) As String
    value = value Mod 360
    If value < 0 Then
      value += 360
    End If
    Dim sexa As String = mutilities.DegreesToDMS(value, ":", ":", "") ' Long format, whole seconds
    Return sexa
  End Function

  Public Property TargetRightAscension() As Double Implements ITelescopeV3.TargetRightAscension
    Get
      If mtgtRa = -999 Then
        Throw New ASCOM.ValueNotSetException
      End If
      mTL.LogMessage("TargetRightAscension Get", mtgtRa.ToString)
      Return mtgtRa
    End Get
    Set(value As Double)
      mTL.LogMessage("TargetRightAscension Set", value.ToString)
      Dim sexa As String = RaToString(value)   ' Long format, whole seconds
      If Not Me.CommandBool("Sr" & sexa) Then
        Throw New ASCOM.InvalidOperationException
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
      Dim trk As Boolean = True
      updateTelStatus()
      If (mTelStatus.Substring(0, 1) = "1" Or mTelStatus.Substring(0, 1) = "3") Then
        trk = True
      ElseIf (mTelStatus.Substring(0, 1) = "0" Or mTelStatus.Substring(0, 1) = "2") Then
        trk = False
      End If
      mTL.LogMessage("Tracking", "Get - " & trk.ToString())
      Return trk
    End Get
    Set(value As Boolean)
      If value Then
        If Not Me.CommandBool("Te") Then
          Throw New ASCOM.InvalidValueException
        End If
      Else
        If Not Me.CommandBool("Td") Then
          Throw New ASCOM.InvalidOperationException
        End If
      End If
      mTL.LogMessage("Tracking Set", value.ToString)
    End Set
  End Property

  Public Property TrackingRate() As DriveRates Implements ITelescopeV3.TrackingRate
    Get
      Dim trackingRate__1 As DriveRates = DriveRates.driveSidereal
      mTL.LogMessage("TrackingRate Get", "done")
      Return trackingRate__1
    End Get
    Set(value As DriveRates)
      mTL.LogMessage("TrackingRate Set", "Not implemented")
      Throw New ASCOM.PropertyNotImplementedException("TrackingRate", False)
    End Set
  End Property

  Public ReadOnly Property TrackingRates() As ITrackingRates Implements ITelescopeV3.TrackingRates
    Get
      Dim trackingRates__1 As ITrackingRates = New TrackingRates()
      mTL.LogMessage("TrackingRates", "Get - ")
      For Each driveRate As DriveRates In trackingRates__1
        mTL.LogMessage("TrackingRates", "Get - " & driveRate.ToString())
      Next
      Return trackingRates__1
    End Get
  End Property

  Public Property UTCDate() As DateTime Implements ITelescopeV3.UTCDate
    Get
      Try
        Dim secs As Double = CDbl(CommandString("GXT2"))
        Dim utcDate__1 As DateTime = New DateTime(1970, 1, 1, 0, 0, 0).AddSeconds(secs)
        mTL.LogMessage("UTCDate", String.Format("Get - {0}", utcDate__1))
        Return utcDate__1
      Catch ex As Exception
        mTL.LogMessage("UTCDate", "failed")
        Throw New ASCOM.InvalidOperationException()
      End Try
    End Get
    Set(value As DateTime)
      Dim s As Long = (Date.UtcNow() - New DateTime(1970, 1, 1, 0, 0, 0)).TotalSeconds
      If (CommandBool("SXT2_" & s)) Then
        mTL.LogMessage("Set UTCDate", "done")
      Else
        mTL.LogMessage("Set UTCDate", "failed")
        Throw New ASCOM.InvalidOperationException()
      End If
    End Set
  End Property

  Public Sub Unpark() Implements ITelescopeV3.Unpark
    If Me.CommandBool("hR") Then
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
      Throw New ASCOM.InvalidOperationException("Telescope is not replying")
    ElseIf Not state.Length = 1 Then
      Throw New ASCOM.InvalidOperationException("Telescope reply is corrupt")
    End If
    If state = "0" Then
      mTL.LogMessage("Slew to target", "Started")
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
      Case 4
        Throw New ASCOM.ParkedException
      Case 5
        Throw New ASCOM.InvalidOperationException("Telescope is busy")
      Case 7
        Throw New ASCOM.InvalidOperationException("Telescope is busy")
      Case 6
        Throw New ASCOM.InvalidOperationException("Object is outside limits")
      Case 8
        Throw New ASCOM.InvalidOperationException("Object above max altitude")
      Case 11
        Throw New ASCOM.InvalidOperationException("Motor is fault")
      Case 12
        Throw New ASCOM.InvalidOperationException("Telescope is below horizon limit")
      Case 13
        Throw New ASCOM.InvalidOperationException("Limit Sensor")
      Case 14
        Throw New ASCOM.InvalidOperationException("Telescope is above max declination limit")
      Case 15
        Throw New ASCOM.InvalidOperationException("Telescope is outside azimunth limit")
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
      "TeenAstro version is " & versionFW(1) & "." & versionFW(1) &
      ", TeenAstro driver version is " & fvi.FileVersion)
    Return False
  End Function

  Private Sub updateTelStatus()
    Dim s1 As Double = (Date.UtcNow - mTelStatusDate).TotalMilliseconds
    If s1 > mupdateRate Or mTelStatus = "" Then
      mTelStatus = Me.CommandString("GXI")
      If (mTelStatus <> "") Then
        mTelStatusDate = Date.UtcNow
      End If
    End If
  End Sub

  Private Sub checkslew()
    If mtgtDec = -999 Or mtgtRa = -999 Then
      Throw New ASCOM.ValueNotSetException
    End If
    If Me.AtPark Then
      Throw New ASCOM.ParkedException
    End If
    While Me.Slewing
      Me.AbortSlew()
      Threading.Thread.Sleep(100)
    End While
  End Sub

  Sub setAzalt(Azimuth As Double, Altitude As Double)
    Dim sexa As String = AzToString(Azimuth)
    If Not Me.CommandBool("Sz" & sexa) Then
      Throw New ASCOM.InvalidOperationException
    End If
    sexa = DecToString(Altitude)
    If Not Me.CommandBool("Sz" & sexa) Then
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

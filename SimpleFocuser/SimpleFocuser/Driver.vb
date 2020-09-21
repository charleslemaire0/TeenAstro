'tabs=4
' --------------------------------------------------------------------------------
' TODO fill in this information for your driver, then remove this line!
'
' ASCOM Focuser driver for TeenAstroFocuser
'
' Description:	Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam 
'				nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam 
'				erat, sed diam voluptua. At vero eos et accusam et justo duo 
'				dolores et ea rebum. Stet clita kasd gubergren, no sea takimata 
'				sanctus est Lorem ipsum dolor sit amet.
'
' Implements:	ASCOM Focuser interface version: 1.0
' Author:		(XXX) Your N. Here <your@email.here>
'
' Edit Log:
'
' Date			Who	Vers	Description
' -----------	---	-----	-------------------------------------------------------
' dd-mmm-yyyy	XXX	1.0.0	Initial edit, from Focuser template
' ---------------------------------------------------------------------------------
'
'
' Your driver's ID is ASCOM.TeenAstro.Focuser
'
' The Guid attribute sets the CLSID for ASCOM.DeviceName.Focuser
' The ClassInterface/None addribute prevents an empty interface called
' _Focuser from being created and used as the [default] interface
'

' This definition is used to select code that's only applicable for one device type
#Const Device = "Focuser"

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

<Guid("ffec8045-8303-4c89-aa32-19d4b2a3211e")>
<ClassInterface(ClassInterfaceType.None)>
Public Class Focuser

  ' The Guid attribute sets the CLSID for ASCOM.TeenAstro.Focuser
  ' The ClassInterface/None addribute prevents an empty interface called
  ' _SimpleFocuser from being created and used as the [default] interface

  ' TODO Replace the not implemented exceptions with code to implement the function or
  ' throw the appropriate ASCOM exception.
  '
  Implements IFocuserV2

  '
  ' Driver ID and descriptive string that shows in the Chooser
  '
  Friend Shared driverID As String = "ASCOM.TeenAstro.Focuser"
  Private Shared driverDescription As String = "TeenAstroFocuser"

  Friend Shared mcomPortProfileName As String = "COM Port" 'Constants used for Profile persistence
  Friend Shared mcomPortDefault As String = "COM1"
  Friend Shared mIPProfileName As String = "IP Adress"
  Friend Shared mIPDefault As String = "192.168.0.1"
  Friend Shared mPortProfileName As String = "Port"
  Friend Shared mPortDefault As String = "9999"
  Friend Shared mInterfaceProfileName As String = "Interface"
  Friend Shared mInterfaceDefault As String = "COM"
  Friend Shared mStepSizeProfileName As String = "StepSize" 'Constants used for Profile persistence
  Friend Shared mStepSizeDefault As String = "10"
  Friend Shared mtraceStateProfileName As String = "Trace Level"
  Friend Shared mtraceStateDefault As String = "False"

  ' Variables to hold the currrent device configuration
  Friend Shared mStepSize As Integer
  Friend Shared mcomPort As String
  Friend Shared mIP As String
  Friend Shared mPort As Integer
  Friend Shared mInterface As String
  Friend Shared mtraceState As Boolean = False
  Friend Shared mobjectIP As Net.IPAddress
  Private mobjectSerial As ASCOM.Utilities.Serial

  Private mconnectedState As Boolean = False ' Private variable to hold the connected state

  Private utilities As Util ' Private variable to hold an ASCOM Utilities object
  Private astroUtilities As AstroUtils ' Private variable to hold an AstroUtils object to provide the Range method
  Private TL As TraceLogger ' Private variable to hold the trace logger object (creates a diagnostic log file with information that you specify)

  '
  ' Constructor - Must be public for COM registration!
  '
  Public Sub New()

    ReadProfile() ' Read device configuration from the ASCOM Profile store
    TL = New TraceLogger("", "TeenAstroFocuser")
    TL.Enabled = mtraceState
    TL.LogMessage("Focuser", "Starting initialisation")

    mconnectedState = False ' Initialise connected to false
    utilities = New Util() ' Initialise util object
    astroUtilities = New AstroUtils 'Initialise new astro utiliites object

    'TODO: Implement your additional construction here

    TL.LogMessage("Focuser", "Completed initialisation")
  End Sub

  '
  ' PUBLIC COM INTERFACE IFocuserV2 IMPLEMENTATION
  '

#Region "Common properties and methods"
  ''' <summary>
  ''' Displays the Setup Dialog form.
  ''' If the user clicks the OK button to dismiss the form, then
  ''' the new settings are saved, otherwise the old values are reloaded.
  ''' THIS IS THE ONLY PLACE WHERE SHOWING USER INTERFACE IS ALLOWED!
  ''' </summary>
  Public Sub SetupDialog() Implements IFocuserV2.SetupDialog
    ' consider only showing the setup dialog if not connected
    ' or call a different dialog if connected
    If IsConnected Then
      System.Windows.Forms.MessageBox.Show("Already connected, just press OK")
    End If

    Using F As SetupDialogForm = New SetupDialogForm()
      Dim result As System.Windows.Forms.DialogResult = F.ShowDialog()
      If result = DialogResult.OK Then
        WriteProfile() ' Persist device configuration values to the ASCOM Profile store
      End If
    End Using
  End Sub

  Public ReadOnly Property SupportedActions() As ArrayList Implements IFocuserV2.SupportedActions
    Get
      TL.LogMessage("SupportedActions Get", "Returning empty arraylist")
      Return New ArrayList()
    End Get
  End Property

  Public Function Action(ByVal ActionName As String, ByVal ActionParameters As String) As String Implements IFocuserV2.Action
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

  Public Sub CommandBlind(ByVal Command As String, Optional ByVal Raw As Boolean = False) Implements IFocuserV2.CommandBlind
    'CheckConnected("CommandBlind")
    Dim buf As String = ""
    If Not GenericCommand(Command, Raw, 0, buf) Then
      Throw New ASCOM.InvalidValueException("CommandBlind " + Command + " has failed")
    End If

  End Sub

  Public Function CommandBool(ByVal Command As String, Optional ByVal Raw As Boolean = False) As Boolean _
      Implements IFocuserV2.CommandBool
    'CheckConnected("CommandBool")
    Dim buf As String = ""
    If Not GenericCommand(Command, Raw, 1, buf) Then
      Throw New ASCOM.InvalidValueException("CommandBool " + Command + " has failed")
    End If
    Return buf = "1"
  End Function

  Public Function CommandString(ByVal Command As String, Optional ByVal Raw As Boolean = False) As String _
      Implements IFocuserV2.CommandString
    'CheckConnected("CommandString")
    Dim buf As String = ""
    If Not GenericCommand(Command, Raw, 2, buf) Then
      Throw New ASCOM.InvalidValueException("CommandString " + Command + " has failed")
    End If
    Return buf
  End Function

  Private Function MyDevice() As Boolean
    MyDevice = False
    Try
      Dim s As String = CommandString("FV")
      MyDevice = s.StartsWith("$ TeenAstro Focuser")
    Catch ex As Exception
      Throw New ASCOM.DriverException(ex.Message)
    End Try
  End Function

  Private Sub ConnectSerial(value As Boolean)
    If value Then
      mobjectSerial = New ASCOM.Utilities.Serial
      TL.LogMessage("Connected Set", "Connecting to port " + mcomPort)
      Try
        Dim c As String = Replace(mcomPort, "COM", "")
        mobjectSerial.Port = CInt(c)
      Catch ex As Exception
        Throw New ASCOM.DriverException(ex.Message)
        mconnectedState = False
        Return
      End Try
      mobjectSerial.Speed = 9600
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
      TL.LogMessage("Connected Set", "Disconnecting from port " + mcomPort)
    End If
  End Sub

  Private Sub ConnectIP(value As Boolean)
    If value Then
      mconnectedState = False
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
      TL.LogMessage("Connected Set", "Disconnecting from IP " + mIP)
    End If
  End Sub

  Public Property Connected() As Boolean Implements IFocuserV2.Connected
    Get
      TL.LogMessage("Connected Get", IsConnected.ToString())
      Return IsConnected
    End Get
    Set(value As Boolean)
      TL.LogMessage("Connected Set", value.ToString())
      If value = IsConnected Then
        Return
      End If
      If mInterface = "COM" Then
        ConnectSerial(value)
      ElseIf mInterface = "IP" Then
        ConnectIP(value)
      End If
    End Set
  End Property

  Public ReadOnly Property Description As String Implements IFocuserV2.Description
    Get
      ' this pattern seems to be needed to allow a public property to return a private field
      Dim d As String = driverDescription
      TL.LogMessage("Description Get", d)
      Return d
    End Get
  End Property

  Public ReadOnly Property DriverInfo As String Implements IFocuserV2.DriverInfo
    Get
      Dim m_version As Version = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version
      ' TODO customise this driver description
      Dim s_driverInfo As String = " TeenAstro Focuser Version: " + m_version.Major.ToString() + "." + m_version.Minor.ToString()
      TL.LogMessage("DriverInfo Get", s_driverInfo)
      Return s_driverInfo
    End Get
  End Property

  Public ReadOnly Property DriverVersion() As String Implements IFocuserV2.DriverVersion
    Get
      ' Get our own assembly and report its version number
      TL.LogMessage("DriverVersion Get", Reflection.Assembly.GetExecutingAssembly.GetName.Version.ToString(2))
      Return Reflection.Assembly.GetExecutingAssembly.GetName.Version.ToString(2)
    End Get
  End Property

  Public ReadOnly Property InterfaceVersion() As Short Implements IFocuserV2.InterfaceVersion
    Get
      TL.LogMessage("InterfaceVersion Get", "2")
      Return 2
    End Get
  End Property

  Public ReadOnly Property Name As String Implements IFocuserV2.Name
    Get
      Dim s_name As String = "TeenAstro Focuser"
      TL.LogMessage("Name Get", s_name)
      Return s_name
    End Get
  End Property

  Public Sub Dispose() Implements IFocuserV2.Dispose
    ' Clean up the tracelogger and util objects
    TL.Enabled = False
    TL.Dispose()
    TL = Nothing
    utilities.Dispose()
    utilities = Nothing
    astroUtilities.Dispose()
    astroUtilities = Nothing
  End Sub

#End Region

#Region "IFocuser Implementation"

  Private focuserPosition As Integer = 0 ' Class level variable to hold the current focuser position
  Private focuserSpeed As Integer = 0
  Private focuserTemperature As Double = -99.99
  Private m_ismoving As Boolean = False
  Private m_maxPos As UShort


  Public ReadOnly Property Absolute() As Boolean Implements IFocuserV2.Absolute
    Get
      TL.LogMessage("Absolute Get", True.ToString())
      Return True ' This is an absolute focuser
    End Get
  End Property

  Public Sub Halt() Implements IFocuserV2.Halt
    CommandBlind("FX")
  End Sub

  Public ReadOnly Property IsMoving() As Boolean Implements IFocuserV2.IsMoving
    Get
      UpdateStatus()
      TL.LogMessage("IsMoving Get", m_ismoving.ToString())
      Return m_ismoving ' This focuser always moves instantaneously so no need for IsMoving ever to be True
    End Get
  End Property

  Public Property Link() As Boolean Implements IFocuserV2.Link
    Get
      TL.LogMessage("Link Get", Me.Connected.ToString())
      Return Me.Connected ' Direct function to the connected method, the Link method is just here for backwards compatibility
    End Get
    Set(value As Boolean)
      TL.LogMessage("Link Set", value.ToString())
      Me.Connected = value ' Direct function to the connected method, the Link method is just here for backwards compatibility
    End Set
  End Property

  Public ReadOnly Property MaxIncrement() As Integer Implements IFocuserV2.MaxIncrement
    Get
      UpdateConfig()
      TL.LogMessage("MaxIncrement Get", m_maxPos.ToString())
      Return m_maxPos ' Maximum change in one move
    End Get
  End Property

  Public ReadOnly Property MaxStep() As Integer Implements IFocuserV2.MaxStep
    Get
      UpdateConfig()
      TL.LogMessage("MaxStep Get", m_maxPos.ToString())
      Return m_maxPos ' Maximum extent of the focuser, so position range is 0 to 10,000
    End Get
  End Property

  Public Sub Move(Position As Integer) Implements IFocuserV2.Move
    TL.LogMessage("Move", Position.ToString())
    If Position < m_maxPos Then
      CommandBlind("FG," & Position)
    End If
  End Sub

  Public ReadOnly Property Position() As Integer Implements IFocuserV2.Position
    Get
      UpdateStatus()
      Return focuserPosition ' Return the focuser position
    End Get
  End Property

  Public ReadOnly Property StepSize() As Double Implements IFocuserV2.StepSize
    Get
      TL.LogMessage("StepSize Get", mStepSize.ToString)
      Return mStepSize
    End Get
  End Property

  Public Property TempComp() As Boolean Implements IFocuserV2.TempComp
    Get
      TL.LogMessage("TempComp Get", False.ToString())
      Return False
    End Get
    Set(value As Boolean)
      TL.LogMessage("TempComp Set", "Not implemented")
      Throw New ASCOM.PropertyNotImplementedException("TempComp", False)
    End Set
  End Property

  Public ReadOnly Property TempCompAvailable() As Boolean Implements IFocuserV2.TempCompAvailable
    Get
      TL.LogMessage("TempCompAvailable Get", False.ToString())
      Return False ' Temperature compensation is not available in this driver
    End Get
  End Property

  Public ReadOnly Property Temperature() As Double Implements IFocuserV2.Temperature
    Get
      UpdateStatus()
      TL.LogMessage("Temperature Get", focuserTemperature.ToString)
      Return focuserTemperature
    End Get
  End Property

  Public Sub force()
    WriteProfile()
  End Sub

#End Region

#Region "Private properties and methods"
  ' here are some useful properties and methods that can be used as required
  ' to help with

#Region "ASCOM Registration"

  Private Shared Sub RegUnregASCOM(ByVal bRegister As Boolean)

    Using P As New Profile() With {.DeviceType = "Focuser"}
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
      driverProfile.DeviceType = "Focuser"
      mtraceState = Convert.ToBoolean(driverProfile.GetValue(driverID, mtraceStateProfileName, String.Empty, mtraceStateDefault))
      mcomPort = driverProfile.GetValue(driverID, mcomPortProfileName, String.Empty, mcomPortDefault)
      mIP = driverProfile.GetValue(driverID, mIPProfileName, String.Empty, mIPDefault)
      mPort = driverProfile.GetValue(driverID, mPortProfileName, String.Empty, mPortDefault)
      mInterface = driverProfile.GetValue(driverID, mInterfaceProfileName, String.Empty, mInterfaceDefault)
      mStepSize = driverProfile.GetValue(driverID, mStepSizeProfileName, String.Empty, mStepSizeDefault)
    End Using
  End Sub

  ''' <summary>
  ''' Write the device configuration to the  ASCOM  Profile store
  ''' </summary>
  Public Shared Sub WriteProfile()
    Using driverProfile As New Profile()
      driverProfile.DeviceType = "Focuser"
      driverProfile.WriteValue(driverID, mtraceStateProfileName, mtraceState.ToString())
      driverProfile.WriteValue(driverID, mcomPortProfileName, mcomPort.ToString())
      driverProfile.WriteValue(driverID, mIPProfileName, mIP.ToString())
      driverProfile.WriteValue(driverID, mPortProfileName, mPort.ToString())
      driverProfile.WriteValue(driverID, mInterfaceProfileName, mInterface.ToString())
      driverProfile.WriteValue(driverID, mStepSizeProfileName, mStepSize.ToString())
    End Using

  End Sub


#End Region

  Private Sub UpdateStatus()
    Try
      Dim s As String = CommandString("F?").Trim("?")
      Dim elments As String() = s.Split(" ")
      focuserPosition = CInt(elments(0))
      focuserSpeed = CInt(elments(1))
      focuserTemperature = Double.Parse(elments(2), CultureInfo.InvariantCulture)
      If focuserSpeed > 0 Then
        m_ismoving = True
      Else
        m_ismoving = False
      End If
    Catch ex As Exception

    End Try
  End Sub

  Private Sub UpdateConfig()
    Dim k As Integer = 0
    Dim s As String = ""
    While Not s.StartsWith("~")
      s = CommandString("F~")
    End While
    s = s.TrimStart("~")
    Dim info As String() = s.Split(" ")
    Try
      m_maxPos = info(1)
    Catch ex As Exception

    End Try


  End Sub

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
            Threading.Thread.Sleep(50)
            ' Incoming message may be larger than the buffer size.
            While ServerStream.DataAvailable
              numberOfBytesRead = ServerStream.Read(myReadBuffer, 0, myReadBuffer.Length)
              myCompleteMessage.AppendFormat("{0}", Encoding.ASCII.GetString(myReadBuffer, 0, numberOfBytesRead))
            End While

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

End Class

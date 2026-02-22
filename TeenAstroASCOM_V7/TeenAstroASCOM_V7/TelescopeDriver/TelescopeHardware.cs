// TODO fill in this information for your driver, then remove this line!
//
// ASCOM Telescope hardware class for TeenAstro
//
// Description:	 <To be completed by driver developer>
//
// Implements:	ASCOM Telescope interface version: <To be completed by driver developer>
// Author:		(XXX) Your N. Here <your@email.here>

// TODO: Customise the SetConnected and InitialiseHardware methods as needed for your hardware

using ASCOM.Astrometry.AstroUtils;
using ASCOM.DeviceInterface;
using ASCOM.Utilities;
using Microsoft.VisualBasic;
using Microsoft.VisualBasic.CompilerServices;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

namespace ASCOM.TeenAstro.Telescope
{
  //
  // TODO Customise the InitialiseHardware() method with code to set up a communication path to your hardware and validate that the hardware exists
  //
  // TODO Customise the SetConnected() method with code to connect to and disconnect from your hardware
  // NOTE You should not need to customise the code in the Connecting, Connect() and Disconnect() members as these are already fully implemented and call SetConnected() when appropriate.
  //
  // TODO Replace the not implemented exceptions with code to implement the functions or throw the appropriate ASCOM exceptions.
  //

  /// <summary>
  /// ASCOM Telescope hardware class for TeenAstro.
  /// </summary>
  [HardwareClass()] // Class attribute flag this as a device hardware class that needs to be disposed by the local server when it exits.
  internal static class TelescopeHardware
  {
    // Constants used for Profile persistence
    internal const string comPortProfileName = "COM Port";
    internal const string comPortDefault = "COM1";
    internal const string IPProfileName = "IP Adress";
    internal const string IPDefault = "192.168.0.1";
    internal const string PortProfileName = "Port";
    internal const string PortDefault = "9999";
    internal const string InterfaceProfileName = "Interface";
    internal const string InterfaceDefault = "COM";
    internal const string traceStateProfileName = "Trace Level";
    internal const string traceStateDefault = "true";

    private static string DriverProgId = ""; // ASCOM DeviceID (COM ProgID) for this driver, the value is set by the driver's class initialiser.
    private static string DriverDescription = ""; // The value is set by the driver's class initialiser.
    internal static string comPort; // COM port name (if required)
    internal static string IP;      // IP adress
    internal static Int16 Port;     // Port for IP
    internal static string Interface;
    internal static System.Net.IPAddress objectIP;
    internal static ASCOM.Utilities.Serial objectSerial;
    private static bool connectedState; // Local server's connected state
    private static bool runOnce = false; // Flag to enable "one-off" activities only to run once.
    internal static Util utilities; // ASCOM Utilities object for use as required
    internal static AstroUtils astroUtilities; // ASCOM AstroUtilities object for use as required
    internal static TraceLogger tl; // Local server's trace logger object for diagnostic log with information that you specify

    private static List<Guid> uniqueIds = new List<Guid>(); // List of driver instance unique IDs

    private static string TelStatus;
    private static string DECstringFormat = "+00.00000;-00.00000";
    private static string RAstringFormat = "000.00000";
    private static DateTime ConnectionStatusDate;
    private static Stopwatch TelstatusStopWatch = new Stopwatch();

    private static double tgtRa = -999;
    private static double tgtDec = -999;
    private static bool HasMotors = true;
    // Slew speeds for speed settings 0-4 as defined in the main unit's Global.h in the array guideRates[].
    // Values are given as multiples of sidereal speed. all these values are overwritten by EEPROM at runtime.
    private static double SlewSpeeds = 0d;
    private static string SlewSpeed = ""; // Last slew speed set via R command
    private static double SiderealRate = 15.04106858d / 3600d; // Sidereal rate in degrees per second




    /// <summary>
    /// Initializes a new instance of the device Hardware class.
    /// </summary>
    static TelescopeHardware()
    {
      try
      {
        // Create the hardware trace logger in the static initialiser.
        // All other initialisation should go in the InitialiseHardware method.
        tl = new TraceLogger("", "TeenAstro.Hardware");

        // DriverProgId has to be set here because it used by ReadProfile to get the TraceState flag.
        DriverProgId = Telescope.DriverProgId; // Get this device's ProgID so that it can be used to read the Profile configuration values

        // ReadProfile has to go here before anything is written to the log because it loads the TraceLogger enable / disable state.
        ReadProfile(); // Read device configuration from the ASCOM Profile store, including the trace state

        LogMessage("TelescopeHardware", $"Static initialiser completed.");
      }
      catch (Exception ex)
      {
        try { LogMessage("TelescopeHardware", $"Initialisation exception: {ex}"); } catch { }
        MessageBox.Show($"TelescopeHardware - {ex.Message}\r\n{ex}", $"Exception creating {Telescope.DriverProgId}", MessageBoxButtons.OK, MessageBoxIcon.Error);
        throw;
      }
    }

    /// <summary>
    /// Place device initialisation code here
    /// </summary>
    /// <remarks>Called every time a new instance of the driver is created.</remarks>
    internal static void InitialiseHardware()
    {
      // This method will be called every time a new ASCOM client loads your driver
      LogMessage("InitialiseHardware", $"Start.");

      // Add any code that you want to run every time a client connects to your driver here

      // Add any code that you only want to run when the first client connects in the if (runOnce == false) block below
      if (runOnce == false)
      {
        LogMessage("InitialiseHardware", $"Starting one-off initialisation.");

        DriverDescription = Telescope.DriverDescription; // Get this device's Chooser description

        LogMessage("InitialiseHardware", $"ProgID: {DriverProgId}, Description: {DriverDescription}");

        connectedState = false; // Initialise connected to false
        utilities = new Util(); //Initialise ASCOM Utilities object
        astroUtilities = new AstroUtils(); // Initialise ASCOM Astronomy Utilities object

        LogMessage("InitialiseHardware", "Completed basic initialisation");

        // Add your own "one off" device initialisation here e.g. validating existence of hardware and setting up communications
        // If you are using a serial COM port you will find the COM port name selected by the user through the setup dialogue in the comPort variable.

        LogMessage("InitialiseHardware", $"One-off initialisation complete.");
        runOnce = true; // Set the flag to ensure that this code is not run again
      }
    }

    // PUBLIC COM INTERFACE ITelescopeV4 IMPLEMENTATION

    #region Common properties and methods.

    /// <summary>
    /// Displays the Setup Dialogue form.
    /// If the user clicks the OK button to dismiss the form, then
    /// the new settings are saved, otherwise the old values are reloaded.
    /// THIS IS THE ONLY PLACE WHERE SHOWING USER INTERFACE IS ALLOWED!
    /// </summary>
    public static void SetupDialog()
    {
      // Don't permit the setup dialogue if already connected
      if (IsConnected)
      {
        MessageBox.Show("Already connected, just press OK");
        return; // Exit the method if already connected
      }

      using (SetupDialogForm F = new SetupDialogForm(tl))
      {
        var result = F.ShowDialog();
        if (result == DialogResult.OK)
        {
          WriteProfile(); // Persist device configuration values to the ASCOM Profile store
        }
      }
    }

    /// <summary>Returns the list of custom action names supported by this driver.</summary>
    /// <value>An ArrayList of strings (SafeArray collection) containing the names of supported actions.</value>
    public static ArrayList SupportedActions
    {
      get
      {
        LogMessage("SupportedActions Get", "Returning empty ArrayList");
        ArrayList SuppActions = new ArrayList();
        SuppActions.Add("AutoAlign");
        return SuppActions;
      }
    }

    /// <summary>Invokes the specified device-specific custom action.</summary>
    /// <param name="ActionName">A well known name agreed by interested parties that represents the action to be carried out.</param>
    /// <param name="ActionParameters">List of required parameters or an <see cref="String.Empty">Empty String</see> if none are required.</param>
    /// <returns>A string response. The meaning of returned strings is set by the driver author.
    /// <para>Suppose filter wheels start to appear with automatic wheel changers; new actions could be <c>QueryWheels</c> and <c>SelectWheel</c>. The former returning a formatted list
    /// of wheel names and the second taking a wheel name and making the change, returning appropriate values to indicate success or failure.</para>
    /// </returns>
    public static string Action(string actionName, string actionParameters)
    {

      if (actionName == "AutoAlign")
      {
        if (!IsConnected)
        {
          throw new ASCOM.NotConnectedException();
        }
        if (!CommandBoolSingleChar("AA"))
        {
          throw new ASCOM.DriverException("AutoAlign command failure");
        }
        return "AutoAlign Started";
      }
      else
      {
        LogMessage("Action", $"Action {actionName}, parameters {actionParameters} is not implemented");
        throw new ActionNotImplementedException("Action " + actionName + " is not implemented by this driver");
      }

    }

    private static bool GenericCommand(string Command, bool Raw, int Mode, ref string buf)
    {
      if (!Raw)
      {
        Command = ":" + Command + "#";
      }
      if (Interface == "COM")
      {
        return GetSerial(Command, Mode, ref buf, 3);
      }
      else if (Interface == "IP")
      {
        return getStream(Command, Mode, ref buf, 3);
      }
      return false;
    }

    /// <summary>
    /// Transmits an arbitrary string to the device and does not wait for a response.
    /// Optionally, protocol framing characters may be added to the string before transmission.
    /// </summary>
    /// <param name="Command">The literal command string to be transmitted.</param>
    /// <param name="Raw">
    /// if set to <c>true</c> the string is transmitted 'as-is'.
    /// If set to <c>false</c> then protocol framing characters may be added prior to transmission.
    /// </param>
    public static void CommandBlind(string command, bool raw)
    {
      String buf = "";
      //CheckConnected("CommandBlind");
      if (!GenericCommand(command,raw,0,ref buf))
        throw new ASCOM.NotConnectedException("CommandBlind " + command + " has failed");
      ConnectionStatusDate = DateTime.UtcNow;

      // TODO The optional CommandBlind method should either be implemented OR throw a MethodNotImplementedException
      // If implemented, CommandBlind must send the supplied command to the mount and return immediately without waiting for a response

    }

    /// <summary>
    /// Transmits an arbitrary string to the device and waits for a boolean response.
    /// Optionally, protocol framing characters may be added to the string before transmission.
    /// </summary>
    /// <param name="Command">The literal command string to be transmitted.</param>
    /// <param name="Raw">
    /// if set to <c>true</c> the string is transmitted 'as-is'.
    /// If set to <c>false</c> then protocol framing characters may be added prior to transmission.
    /// </param>
    /// <returns>
    /// Returns the interpreted boolean response received from the device.
    /// </returns>
    public static bool CommandBool(string Command, bool Raw)
    {
      //CheckConnected("CommandBool");
      string buf = "";
      if (!GenericCommand(Command, Raw, 1, ref buf))
      {
        throw new ASCOM.NotConnectedException("CommandBool " + Command + " has failed");
      }
      ConnectionStatusDate = DateTime.UtcNow;
      return buf == "1";
    }

    private static bool CommandBoolString(string Command, bool Raw = false)
    {
      string buf = CommandString(Command, Raw);
      if (buf.Length > 0)
      {
        if (buf == "1")
        {
          return true;
        }
        else if (buf == "0")
        {
          return false;
        }
        throw new ASCOM.DriverException("CommandBoolString " + Command + " returned invalid value: " + buf);
      }
      return false;
    }

    private static void GetDouble(string GetName, String command, ref double val)
    {
      string response = CommandString(command, false);
      LogMessage("Get "+ GetName, response);
      if (!double.TryParse(response, NumberStyles.Any, CultureInfo.InvariantCulture, out val))
        throw new ASCOM.DriverException("get " + GetName + " has failed");
    }
    private static bool CommandBoolSingleChar(string Command, bool Raw = false)
    {
      string buf = CommandSingleChar(Command, Raw);
      if (buf.Length > 0)
      {
        if (buf == "1")
        {
          return true;
        }
        else if (buf == "0")
        {
          return false;
        }
        throw new ASCOM.DriverException("CommandBoolSingleChar " + Command + " returned invalid value: " + buf);
      }
      return false;
    }
    private static string CommandSingleChar(string Command, bool Raw = false)
    {
      string buf = "";
      if (!GenericCommand(Command, Raw, 1, ref buf))
      {
        throw new ASCOM.NotConnectedException("CommandSingleChar " + Command + " has failed");
      }
      ConnectionStatusDate = DateTime.UtcNow;
      return buf;
    }

    /// <summary>
    /// Transmits an arbitrary string to the device and waits for a string response.
    /// Optionally, protocol framing characters may be added to the string before transmission.
    /// </summary>
    /// <param name="Command">The literal command string to be transmitted.</param>
    /// <param name="Raw">
    /// if set to <c>true</c> the string is transmitted 'as-is'.
    /// If set to <c>false</c> then protocol framing characters may be added prior to transmission.
    /// </param>
    /// <returns>
    /// Returns the string response received from the device.
    /// </returns>
    public static string CommandString(string Command, bool raw)
    {
      //CheckConnected("CommandString");

      string buf = "";
      if (!GenericCommand(Command, raw, 2, ref buf))
      {
        throw new ASCOM.NotConnectedException("CommandString " + Command + " has failed");
      }
      ConnectionStatusDate = DateTime.UtcNow;
      return buf;
    }
    private static bool MyDevice()
    {
      bool MyDeviceRet;
      try
      {
        MyDeviceRet = CommandString("GVP",false) == "TeenAstro";
      }
      catch (Exception ex)
      {
        throw new ASCOM.DriverException(ex.Message);
      }
      return MyDeviceRet;
    }


    private static void CloseAndDisposeSerial()
    {
      connectedState = false;
      if (objectSerial != null)
      {
        if (objectSerial.Connected)
        {
          try
          {
            objectSerial.Connected = false;
          }
          catch
          {

          }
        }
        objectSerial.Dispose();
        objectSerial = null;
      }
    }
    private static void ConnectSerial(bool value)
    {
      if (value)
      {
        objectSerial = new ASCOM.Utilities.Serial();
        LogMessage("Connected Set", "Connecting to port " + comPort);
        try
        {          
          string c = Strings.Replace(comPort, "COM", "");
          objectSerial.Port = Conversions.ToInteger(c);
        }
        catch (Exception ex)
        {
          CloseAndDisposeSerial();
          throw new ASCOM.DriverException(ex.Message);
       
        }
        objectSerial.Speed = SerialSpeed.ps57600;
        try
        {
          objectSerial.Connected = true;
        }
        catch (Exception ex)
        {
          CloseAndDisposeSerial();
          throw new ASCOM.DriverException(ex.Message);
        }
        connectedState = true;
        ConnectionStatusDate = DateTime.UtcNow;
        objectSerial.ReceiveTimeoutMs = 5000;
        //if (!MyDevice())
        if (false)
        { 
          CloseAndDisposeSerial();
          return;
        }
        else
        {
          DateTime timeTelescope = UTCDate;
          DateTime time = DateTime.UtcNow;
          if (Math.Abs((timeTelescope - time).TotalSeconds) > 2d)
          {
            UTCDate = DateTime.UtcNow;
            LogMessage("Connected Set", "Synced with computer time");
          }
          try
          {
            HasMotors = CommandBoolString("GXJm");
          }
          catch (Exception ex)
          {
            CloseAndDisposeSerial();
            throw new ASCOM.DriverException(ex.Message);
          }
          if (!connectedState)
          {
            throw new ASCOM.NotConnectedException("Connection has failed!");
          }
        }
      }
      else
      {
        CloseAndDisposeSerial();
        LogMessage("Connected Set", "Disconnecting from port " + comPort);
      }
    }

    private static void ReConnectSerial()
    {
      LogMessage("Serial connection", "port has been closed, unexpectedly closed try to reopen");
      try
      {
        ConnectSerial(false);
        ConnectSerial(true);
      }
      catch (Exception ex)
      {
        LogMessage("Serial connection", ex.Message);
      }
    }
    private static void ConnectIP(bool value)
    {
      if (value)
      {

        if (System.Net.IPAddress.TryParse(IP, out objectIP))
        {
          connectedState = MyDevice();
          try
          {
            HasMotors = CommandBoolString("GXJm");
          }
          catch (Exception ex)
          {
            connectedState = false;
            throw new ASCOM.DriverException(ex.Message);
          }
          // mformcontrol = New FormControl(Me)
          // If My.Settings.ShowII Then
          // mformcontrol.Show()
          // End If
          if (!connectedState)
          {
            throw new ASCOM.InvalidValueException("Connection has failed!");
          }
          else
          {
            DateTime timeTelescope = UTCDate;
            DateTime time = DateTime.UtcNow;
            if (Math.Abs((timeTelescope - time).TotalSeconds) > 2d)
            {
              LogMessage("Connected Set", "Synced with computer time");
              UTCDate = DateTime.UtcNow;
            }
          }
        }
        else
        {
          Interaction.MsgBox(IP + " is Not AddressOf valid IP Address");
          return;
        }
      }
      else
      {
        connectedState = false;
        LogMessage("Connected Set", "Disconnecting from IP " + IP);
      }
    }
    private static bool getStream(string Command, int Mode, ref string buf, int retry)
    {
      for (int k = 0, loopTo = retry; k <= loopTo; k++)
      {
        if (getStream(Command, Mode, ref buf))
        {
          return true;
        }
      }
      return false;
    }
    private static bool getStream(string Command, int Mode, ref string buf)
    {
      bool getStreamRet = default;

      var ClientSocket = new System.Net.Sockets.TcpClient();
      var result = ClientSocket.BeginConnect(objectIP.ToString(), Port, default, default);
      bool online = result.AsyncWaitHandle.WaitOne(2000, true);
      if (!online)
      {
        ClientSocket.Close();
        connectedState = false;
        return false;
      }
      try
      {
        var ServerStream = ClientSocket.GetStream();
        byte[] outStream = Encoding.ASCII.GetBytes(Command);
        ServerStream.Write(outStream, 0, outStream.Length);
        ServerStream.Flush();
        buf = "";
        switch (Mode)
        {
          case 0:
            {
              if (ServerStream.CanRead)
              {
                var myReadBuffer = new byte[ClientSocket.ReceiveBufferSize + 1];
                ServerStream.Read(myReadBuffer, 0, myReadBuffer.Length);
              }
              getStreamRet = true;
              break;
            }
          case var @case when 1 <= @case && @case <= 2:
            {
              if (ServerStream.CanRead)
              {
                var myReadBuffer = new byte[ClientSocket.ReceiveBufferSize + 1];
                var myCompleteMessage = new StringBuilder();
                int numberOfBytesRead = 0;
                // Incoming message may be larger than the buffer size.
                do
                {
                  numberOfBytesRead = ServerStream.Read(myReadBuffer, 0, myReadBuffer.Length);
                  myCompleteMessage.AppendFormat("{0}", Encoding.ASCII.GetString(myReadBuffer, 0, numberOfBytesRead));
                }
                while (ServerStream.DataAvailable);
                buf = myCompleteMessage.ToString();
                if (Mode == 1 && !string.IsNullOrEmpty(buf))
                {
                  buf = buf.Substring(0, 1);
                }
                else if (Mode == 2 && !string.IsNullOrEmpty(buf))
                {
                  buf = buf.Split('#')[0];
                }
                getStreamRet = !string.IsNullOrEmpty(buf);
              }
              else
              {
                getStreamRet = false;
              }

              break;
            }

        }
      }
      catch (Exception ex)
      {
        LogMessage("Network error", ex.Message);
        getStreamRet = false;
      }
      ClientSocket.Close();
      return getStreamRet;
    }
    private static bool GetSerial(string Command, int Mode, ref string buf, int retry)
    {
      for (int k = 0, loopTo = retry; k <= loopTo; k++)
      {
        if (GetSerial(Command, Mode, ref buf))
        {
          return true;
        }
        else if (!connectedState)
        {
          break;
        }
      }
      return false;
    }
    private static bool GetSerial(string Command, int Mode, ref string buf)
    {
      bool GetSerialRet = false;
      if (!connectedState || objectSerial==null)
      {
        ReConnectSerial();
      }
      if (!connectedState || objectSerial == null)
      {
        LogMessage("Serial connection", "unable to reconnect");
        return GetSerialRet;
      }
      objectSerial.ClearBuffers();
      try
      {
        objectSerial.Transmit(Command);
      }
      catch (Exception ex) when (ex.HResult == -2146233079)
      {
        LogMessage("Serial connection", ex.Message);
        ReConnectSerial();
        return GetSerialRet;
      }
      catch (Exception ex) when (ex.HResult == -2147220478)
      {
        LogMessage("Serial connection", ex.Message);
        return GetSerialRet;
      }
      catch (Exception ex)
      {
        LogMessage("Serial connection", ex.Message);
        LogMessage("Serial connection error code ", ex.HResult.ToString());
        return GetSerialRet;
      }

      try
      {
        switch (Mode)
        {
          case 0:
            {
              GetSerialRet = true;
              break;
            }
          case 1:
            {
              buf = objectSerial.ReceiveCounted(1);
              GetSerialRet = !string.IsNullOrEmpty(buf);
              break;
            }
          case 2:
            {
              buf = objectSerial.ReceiveTerminated("#").TrimEnd('#');
              GetSerialRet = !string.IsNullOrEmpty(buf);
              break;
            }
        }
      }
      catch (Exception ex)
      {
        LogMessage("Serial connection", ex.Message);
        LogMessage("Serial connection error code ", ex.HResult.ToString());
      }
      return GetSerialRet;
    }



    /// <summary>
    /// Deterministically release both managed and unmanaged resources that are used by this class.
    /// </summary>
    /// <remarks>
    /// TODO: Release any managed or unmanaged resources that are used in this class.
    /// 
    /// Do not call this method from the Dispose method in your driver class.
    ///
    /// This is because this hardware class is decorated with the <see cref="HardwareClassAttribute"/> attribute and this Dispose() method will be called 
    /// automatically by the  local server executable when it is irretrievably shutting down. This gives you the opportunity to release managed and unmanaged 
    /// resources in a timely fashion and avoid any time delay between local server close down and garbage collection by the .NET runtime.
    ///
    /// For the same reason, do not call the SharedResources.Dispose() method from this method. Any resources used in the static shared resources class
    /// itself should be released in the SharedResources.Dispose() method as usual. The SharedResources.Dispose() method will be called automatically 
    /// by the local server just before it shuts down.
    /// 
    /// </remarks>
    public static void Dispose()
    {
      try { LogMessage("Dispose", $"Disposing of assets and closing down."); } catch { }

      try
      {
        // Clean up the trace logger and utility objects
        tl.Enabled = false;
        tl.Dispose();
        tl = null;
      }
      catch { }

      try
      {
        utilities.Dispose();
        utilities = null;
      }
      catch { }

      try
      {
        astroUtilities.Dispose();
        astroUtilities = null;
      }
      catch { }
    }

    /// <summary>
    /// Synchronously connects to or disconnects from the hardware
    /// </summary>
    /// <param name="uniqueId">Driver's unique ID</param>
    /// <param name="newState">New state: Connected or Disconnected</param>
    public static void SetConnected(Guid uniqueId, bool newState)
    {
      // Check whether we are connecting or disconnecting
      if (newState) // We are connecting
      {
        // Check whether this driver instance has already connected
        if (uniqueIds.Contains(uniqueId)) // Instance already connected
        {
          // Ignore the request, the unique ID is already in the list
          LogMessage("SetConnected", $"Ignoring request to connect because the device is already connected.");
        }
        else // Instance not already connected, so connect it
        {
          // Check whether this is the first connection to the hardware
          if (uniqueIds.Count == 0) // This is the first connection to the hardware so initiate the hardware connection
          {
            if (Interface == "COM")
            {
              ConnectSerial(true);
            }
            else if (Interface == "IP")
            {
              ConnectIP(true);
            }
            if (!checkCompatibility())
            {
              if (Interface == "COM")
              {
                ConnectSerial(false);
              }
              else if (Interface == "IP")
              {
                ConnectIP(false);
              }
            }
            LogMessage("SetConnected", $"Connecting to hardware.");
          }
          else // Other device instances are connected so the hardware is already connected
          {
            // Since the hardware is already connected no action is required
            LogMessage("SetConnected", $"Hardware already connected.");
          }

          // The hardware either "already was" or "is now" connected, so add the driver unique ID to the connected list
          uniqueIds.Add(uniqueId);
          LogMessage("SetConnected", $"Unique id {uniqueId} added to the connection list.");
        }
      }
      else // We are disconnecting
      {
        // Check whether this driver instance has already disconnected
        if (!uniqueIds.Contains(uniqueId)) // Instance not connected so ignore request
        {
          // Ignore the request, the unique ID is not in the list
          LogMessage("SetConnected", $"Ignoring request to disconnect because the device is already disconnected.");
        }
        else // Instance currently connected so disconnect it
        {
          // Remove the driver unique ID to the connected list
          uniqueIds.Remove(uniqueId);
          LogMessage("SetConnected", $"Unique id {uniqueId} removed from the connection list.");

          // Check whether there are now any connected driver instances 
          if (uniqueIds.Count == 0) // There are no connected driver instances so disconnect from the hardware
          {
            //
            if (Interface == "COM")
            {
              ConnectSerial(false);
            }
            else if (Interface == "IP")
            {
              ConnectIP(false);
            }
            //
          }
          else // Other device instances are connected so do not disconnect the hardware
          {
            // No action is required
            LogMessage("SetConnected", $"Hardware already connected.");
          }
        }
      }

      // Log the current connected state
      LogMessage("SetConnected", $"Currently connected driver ids:");
      foreach (Guid id in uniqueIds)
      {
        LogMessage("SetConnected", $" ID {id} is connected");
      }
    }

    /// <summary>
    /// Returns a description of the device, such as manufacturer and model number. Any ASCII characters may be used.
    /// </summary>
    /// <value>The description.</value>
    public static string Description
    {
      // TODO customise this device description if required
      get
      {
        LogMessage("Description Get", DriverDescription);
        return DriverDescription;
      }
    }

    /// <summary>
    /// Descriptive and version information about this ASCOM driver.
    /// </summary>
    public static string DriverInfo
    {
      get
      {
        Version version = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version;
        // TODO customise this driver description if required
        string driverInfo = $"Information about the driver itself. Version: {version.Major}.{version.Minor}";
        LogMessage("DriverInfo Get", driverInfo);
        return driverInfo;
      }
    }

    /// <summary>
    /// A string containing only the major and minor version of the driver formatted as 'm.n'.
    /// </summary>
    public static string DriverVersion
    {
      get
      {
        Version version = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version;
        string driverVersion = $"{version.Major}.{version.Minor}";
        LogMessage("DriverVersion Get", driverVersion);
        return driverVersion;
      }
    }

    /// <summary>
    /// The interface version number that this device supports.
    /// </summary>
    public static short InterfaceVersion
    {
      // set by the driver wizard
      get
      {
        LogMessage("InterfaceVersion Get", "4");
        return Convert.ToInt16("4");
      }
    }

    /// <summary>
    /// The short name of the driver, for display purposes
    /// </summary>
    public static string Name
    {
      // TODO customise this device name as required
      get
      {
        string name = "TeenAstro";
        LogMessage("Name Get", name);
        return name;
      }
    }

    #endregion

    #region ITelescope Implementation

    /// <summary>
    /// Stops a slew in progress.
    /// </summary>
    internal static void AbortSlew()
    {
      if (CanSlew)
      {
        if (AtPark)
        {
          throw new ASCOM.ParkedException();
        }
        CommandBlind("Q",false);
        LogMessage("AbortSlew", "done");
      }
      else
      {
        throw new ASCOM.ActionNotImplementedException();
      }
    }

    /// <summary>
    /// The alignment mode of the mount (Alt/Az, Polar, German Polar).
    /// </summary>
    internal static AlignmentModes AlignmentMode
    {
      get
      {
        if (!updateTelStatus())
        {
          throw new ASCOM.InvalidOperationException("Get AlignmentMode failed");
        }
        string m = TelStatus.Substring(12, 1);
        LogMessage("AlignmentMode", m);
        if (m == "E")
        {
          return AlignmentModes.algGermanPolar;
        }
        if (m == "K")
        {
          return AlignmentModes.algPolar;
        }
        else if (m == "k" | m == "A")
        {
          return AlignmentModes.algAltAz;
        }
        throw new ASCOM.InvalidValueException(" get AlignmentMode has failed");
      }
    }

    /// <summary>
    /// The Altitude above the local horizon of the telescope's current position (degrees, positive up)
    /// </summary>
    internal static double Altitude
    {
      get
      {
        double Alt = utilities.DMSToDegrees(CommandString("GA", false));
        LogMessage("Get Altitude", Alt.ToString("0.0000000", CultureInfo.InvariantCulture));
        return Alt;
      }
    }

    /// <summary>
    /// The area of the telescope's aperture, taking into account any obstructions (square meters)
    /// </summary>
    internal static double ApertureArea
    {
      get
      {
        LogMessage("ApertureArea Get", "Not implemented");
        throw new PropertyNotImplementedException("ApertureArea", false);
      }
    }

    /// <summary>
    /// The telescope's effective aperture diameter (meters)
    /// </summary>
    internal static double ApertureDiameter
    {
      get
      {
        LogMessage("ApertureDiameter Get", "Not implemented");
        throw new PropertyNotImplementedException("ApertureDiameter", false);
      }
    }

    /// <summary>
    /// True if the telescope is stopped in the Home position. Set only following a <see cref="FindHome"></see> operation,
    /// and reset with any slew operation. This property must be False if the telescope does not support homing.
    /// </summary>
    internal static bool AtHome
    {
      get
      {
        if (!updateTelStatus())
        {
          throw new ASCOM.InvalidOperationException("Get atHome has failed");
        }
        bool isAtPark = TelStatus.Substring(2, 1) == "H";
        LogMessage("Get AtHome", isAtPark.ToString());
        return isAtPark;
      }
    }

    /// <summary>
    /// True if the telescope has been put into the parked state by the seee <see cref="Park" /> method. Set False by calling the Unpark() method.
    /// </summary>
    internal static bool AtPark
    {
      get
      {
        if (!updateTelStatus())
        {
          throw new ASCOM.InvalidOperationException("Get AtPark has failed");
        }
        bool isAtPark = TelStatus.Substring(2, 1) == "P";
        LogMessage("Get AtPark", isAtPark.ToString());
        return isAtPark;
      }
    }

    /// <summary>
    /// Determine the rates at which the telescope may be moved about the specified axis by the <see cref="MoveAxis" /> method.
    /// </summary>
    /// <param name="Axis">The axis about which rate information is desired (TelescopeAxes value)</param>
    /// <returns>Collection of <see cref="IRate" /> rate objects</returns>
    internal static IAxisRates AxisRates(TelescopeAxes Axis)
    {
      // Read maxSpeed from TeenAstro main unit and assign top two speed settings on this basis
      double Speed=0;
      GetDouble("AxisRates for " + Axis.ToString(), "GXRX", ref Speed);
      SlewSpeeds = Speed;
      return new AxisRates(Axis, SlewSpeeds, SiderealRate);
    }

    /// <summary>
    /// The azimuth at the local horizon of the telescope's current position (degrees, North-referenced, positive East/clockwise).
    /// </summary>
    internal static double Azimuth
    {
      get
      {        
        string response = CommandString("GZ", false);
        LogMessage("Azimuth", response);
        try
        {
          double AZ = utilities.DMSToDegrees(CommandString("GZ", false));
          LogMessage("Get Azimuth", AZ.ToString("0.0000000", CultureInfo.InvariantCulture));
          return AZ;
        }
        catch
        {
          throw new ASCOM.DriverException("get Az has failed");
        }
      }
    }

    /// <summary>
    /// True if this telescope is capable of programmed finding its home position (<see cref="FindHome" /> method).
    /// </summary>
    internal static bool CanFindHome
    {
      get
      {
        LogMessage("CanFindHome", "Get - " + false.ToString());
        return false;
      }
    }

    /// <summary>
    /// True if this telescope can move the requested axis
    /// </summary>
    internal static bool CanMoveAxis(TelescopeAxes Axis)
    {
      LogMessage("CanMoveAxis", "Get - " + Axis.ToString());
      switch (Axis)
      {
        case TelescopeAxes.axisPrimary: return HasMotors;
        case TelescopeAxes.axisSecondary: return HasMotors;
        case TelescopeAxes.axisTertiary: return false;
        default: throw new InvalidValueException("CanMoveAxis", Axis.ToString(), "0 to 2");
      }
    }

    /// <summary>
    /// True if this telescope is capable of programmed parking (<see cref="Park" />method)
    /// </summary>
    internal static bool CanPark
    {
      get
      {
        bool value = CommandBool("hS", false) && HasMotors;
        LogMessage("Get CanPark", value.ToString());
        return value;
      }
    }

    /// <summary>
    /// True if this telescope is capable of software-pulsed guiding (via the <see cref="PulseGuide" /> method)
    /// </summary>
    internal static bool CanPulseGuide
    {
      get
      {
        LogMessage("CanPulseGuide", "Get - " + HasMotors.ToString());
        return true;
      }
    }

    /// <summary>
    /// True if the <see cref="DeclinationRate" /> property can be changed to provide offset tracking in the declination axis.
    /// </summary>
    internal static bool CanSetDeclinationRate
    {
      get
      {
        LogMessage("CanSetDeclinationRate", "Get - " + HasMotors.ToString());
        return HasMotors && TrackingRate == DriveRates.driveSidereal;
      }
    }

    /// <summary>
    /// True if the guide rate properties used for <see cref="PulseGuide" /> can ba adjusted.
    /// </summary>
    internal static bool CanSetGuideRates
    {
      get
      {
        LogMessage("CanSetGuideRates", "Get - " + HasMotors.ToString());
        return HasMotors;
      }
    }

    /// <summary>
    /// True if this telescope is capable of programmed setting of its park position (<see cref="SetPark" /> method)
    /// </summary>
    internal static bool CanSetPark
    {
      get
      {
        LogMessage("CanSetPark", "Get - " + HasMotors.ToString());
        return HasMotors;
      }
    }

    /// <summary>
    /// True if the <see cref="SideOfPier" /> property can be set, meaning that the mount can be forced to flip.
    /// </summary>
    internal static bool CanSetPierSide
    {
      get
      {
        LogMessage("CanSetPierSide", "Get - " + true.ToString());
        return true;
      }
    }

    /// <summary>
    /// True if the <see cref="RightAscensionRate" /> property can be changed to provide offset tracking in the right ascension axis.
    /// </summary>
    internal static bool CanSetRightAscensionRate
    {
      get
      {
        LogMessage("CanSetRightAscensionRate", "Get - " + HasMotors.ToString());
        return HasMotors && TrackingRate == DriveRates.driveSidereal;
      }
    }

    /// <summary>
    /// True if the <see cref="Tracking" /> property can be changed, turning telescope sidereal tracking on and off.
    /// </summary>
    internal static bool CanSetTracking
    {
      get
      {
        LogMessage("CanSetTracking", "Get - " + HasMotors.ToString());
        return HasMotors;
      }
    }

    /// <summary>
    /// True if this telescope is capable of programmed slewing (synchronous or asynchronous) to equatorial coordinates
    /// </summary>
    internal static bool CanSlew
    {
      get
      {
        LogMessage("CanSlew", "Get - " + true.ToString());
        return true;
      }
    }

    /// <summary>
    /// True if this telescope is capable of programmed slewing (synchronous or asynchronous) to local horizontal coordinates
    /// </summary>
    internal static bool CanSlewAltAz
    {
      get
      {
        LogMessage("CanSlewAltAz", "Get - " + HasMotors.ToString());
        return HasMotors;
      }
    }

    /// <summary>
    /// True if this telescope is capable of programmed asynchronous slewing to local horizontal coordinates
    /// </summary>
    internal static bool CanSlewAltAzAsync
    {
      get
      {
        LogMessage("CanSlewAltAzAsync", "Get - " + HasMotors.ToString());
        return HasMotors;
      }
    }

    /// <summary>
    /// True if this telescope is capable of programmed asynchronous slewing to equatorial coordinates.
    /// </summary>
    internal static bool CanSlewAsync
    {
      get
      {
        LogMessage("CanSlewAsync", "Get - " + HasMotors.ToString());
        return HasMotors;
      }
    }

    /// <summary>
    /// True if this telescope is capable of programmed syncing to equatorial coordinates.
    /// </summary>
    internal static bool CanSync
    {
      get
      {
        LogMessage("CanSync", "Get - " + HasMotors.ToString());
        return HasMotors;
      }
    }

    /// <summary>
    /// True if this telescope is capable of programmed syncing to local horizontal coordinates
    /// </summary>
    internal static bool CanSyncAltAz
    {
      get
      {
        LogMessage("CanSyncAltAz", "Get - " + HasMotors.ToString());
        return HasMotors;
      }
    }

    /// <summary>
    /// True if this telescope is capable of programmed unparking (<see cref="Unpark" /> method).
    /// </summary>
    internal static bool CanUnpark
    {
      get
      {
        bool value = CommandBool("hS", false) && HasMotors;
        LogMessage("Get CanUnpark", value.ToString());
        return value;
      }
    }

    /// <summary>
    /// The declination (degrees) of the telescope's current equatorial coordinates, in the coordinate system given by the <see cref="EquatorialSystem" /> property.
    /// Reading the property will raise an error if the value is unavailable.
    /// </summary>
    internal static double Declination
    {
      get
      {
        double DEC=0;
        GetDouble("Declination", "GDL", ref DEC);
        LogMessage("Get Declination", utilities.DegreesToDMS(DEC));
        return DEC;
      }
    }

    /// <summary>
    /// The declination tracking rate (arcseconds per SI second, default = 0.0)
    /// </summary>
    internal static double DeclinationRate
    {
      get
      {
        double rate = 0;
        if (TrackingRate == DriveRates.driveSidereal)
        {
          GetDouble("DeclinationRate", "GXRd", ref rate);
        }
        return rate / 10000d;
      }
      set
      {
        if (CanSetDeclinationRate)
        {
          int rate = (int)(value * 10000);
          string cmd = "SXRd," + rate.ToString(CultureInfo.InvariantCulture);
          LogMessage("Set DeclinationRate", "value: " + cmd);
          if (!CommandBoolSingleChar(cmd))
          {
            throw new ASCOM.InvalidValueException("Set DeclinationRate via :" + cmd + " has failed");
          }
        }
        else
        {
          throw new ASCOM.InvalidOperationException();
        }
      }
    }

    /// <summary>
    /// Predict side of pier for German equatorial mounts at the provided coordinates
    /// </summary>
    internal static PierSide DestinationSideOfPier(double RightAscension, double Declination)
    {
      PierSide dsop;
      string RaString = RaToString(RightAscension);
      string DecString = DecToString(Declination);
      string state = CommandSingleChar("M?" + RaString + DecString);
      if (state == "E")
      {
        dsop = PierSide.pierEast;
      }
      else if (state == "W")
      {
        dsop = PierSide.pierWest;
      }
      else if (state == "?")
      {
        throw new ASCOM.InvalidValueException("Destination cannot be reached");
      }
      else
      {
        throw new ASCOM.InvalidValueException("DestinationSideOfPier has failed");
      }
      LogMessage("Get DestinationSideOfPier", dsop.ToString());
      return dsop;
    }

    /// <summary>
    /// True if the telescope or driver applies atmospheric refraction to coordinates.
    /// </summary>
    internal static bool DoesRefraction
    {
      get
      {
        string str1 = CommandString("GXrg", false);
        string str2 = CommandString("GXrp", false);
        string str3 = CommandString("GXrt", false);
        if (str1.Length == 1 && str2.Length == 1 && str3.Length == 1)
        {
          return str1.Substring(0, 1) == "y" && str2.Substring(0, 1) == "y" && str3.Substring(0, 1) == "y";
        }
        throw new ASCOM.InvalidOperationException("Get refraction failed");
      }
      set
      {
        bool ok;
        if (value)
        {
          ok = CommandBoolSingleChar("SXrg,y");
          ok &= CommandBoolSingleChar("SXrp,y");
          ok &= CommandBoolSingleChar("SXrt,y");
        }
        else
        {
          ok = CommandBoolSingleChar("SXrg,n");
          ok &= CommandBoolSingleChar("SXrp,n");
          ok &= CommandBoolSingleChar("SXrt,n");
        }
        if (!ok)
        {
          if (DoesRefraction)
          {
            throw new ASCOM.DriverException("turn refraction on failed");
          }
          else
          {
            throw new ASCOM.DriverException("turn refraction off failed");
          }
        }
      }
    }

    /// <summary>
    /// Equatorial coordinate system used by this telescope (e.g. Topocentric or J2000).
    /// </summary>
    internal static EquatorialCoordinateType EquatorialSystem
    {
      get
      {
        EquatorialCoordinateType equatorialSystem = EquatorialCoordinateType.equTopocentric;
        LogMessage("DeclinationRate", "Get - " + equatorialSystem.ToString());
        return equatorialSystem;
      }
    }

    /// <summary>
    /// Locates the telescope's "home" position (synchronous)
    /// </summary>
    internal static void FindHome()
    {
      LogMessage("FindHome", "Not implemented");
      throw new MethodNotImplementedException("FindHome");
    }

    /// <summary>
    /// The telescope's focal length, meters
    /// </summary>
    internal static double FocalLength
    {
      get
      {
        LogMessage("FocalLength Get", "Not implemented");
        throw new PropertyNotImplementedException("FocalLength", false);
      }
    }


    private static double GetGuideRate()
    {
      // GUIDE_SPEED
      double speed = 0;
      GetDouble("GuideRate", "GXR0", ref speed);
      return speed * SiderealRate;
    }
    private static void SetGuideRate(double Val)
    {
      int speed = (int)(Val / SiderealRate * 100);
      string cmd = "SXR0," + speed.ToString("000", CultureInfo.InvariantCulture);
      LogMessage("Set AxisRates", "value: " + Val);
      if (!CommandBoolSingleChar(cmd))
      {
        throw new ASCOM.InvalidValueException("Set SetGuideRate via :" + cmd + " has failed");
      }
    }
    /// <summary>
    /// The current Declination movement rate offset for telescope guiding (degrees/sec)
    /// </summary>
    internal static double GuideRateDeclination
    {
      get
      {
        double r = GetGuideRate();
        LogMessage("GuideRateDeclination Get", r.ToString(CultureInfo.InvariantCulture));
        return r;
      }
      set
      {
        if (CanSetGuideRates)
        {
          LogMessage("GuideRateDeclination Set", value.ToString(CultureInfo.InvariantCulture));
          SetGuideRate(value);
        }
        else
        {
          LogMessage("GuideRateDeclination Set", "Not implemented");
          throw new PropertyNotImplementedException("GuideRateDeclination", true);
        }
      }
    }

    /// <summary>
    /// The current Right Ascension movement rate offset for telescope guiding (degrees/sec)
    /// </summary>
    internal static double GuideRateRightAscension
    {
      get
      {
        double r = GetGuideRate();
        LogMessage("GuideRateRightAscension Get", r.ToString(CultureInfo.InvariantCulture));
        return r;
      }
      set
      {
        if (CanSetGuideRates)
        {
          LogMessage("GuideRateRightAscension Set", value.ToString(CultureInfo.InvariantCulture));
          SetGuideRate(value);
        }
        else
        {
          LogMessage("GuideRateRightAscension Set", "Not implemented");
          throw new PropertyNotImplementedException("GuideRateRightAscension", true);
        }
        
      }
    }

    /// <summary>
    /// True if a <see cref="PulseGuide" /> command is in progress, False otherwise
    /// </summary>
    internal static bool IsPulseGuiding
    {
      get
      {
        if (CanPulseGuide)
        {
          bool ipg = CommandBoolString("GXJP");
          LogMessage("Get IsPulseGuiding", ipg.ToString(CultureInfo.InvariantCulture));
          return ipg;
        }
        else
        {
          LogMessage("IsPulseGuiding Get", "Not implemented");
          throw new PropertyNotImplementedException("IsPulseGuiding", false);
        }
      }
    }

    /// <summary>
    /// Move the telescope in one axis at the given rate.
    /// </summary>
    /// <param name="Axis">The physical axis about which movement is desired</param>
    /// <param name="Rate">The rate of motion (deg/sec) about the specified axis</param>
    internal static void MoveAxis(TelescopeAxes Axis, double Rate)
    {
      if (CanMoveAxis(Axis))
      {
        LogMessage("Set MoveAxis", Axis.ToString() + ":" + Rate.ToString(CultureInfo.InvariantCulture));
        string cmd;
        if (AtPark)
        {
          throw new ASCOM.ParkedException();
        }
        Rate = Rate / SiderealRate;
        if (Axis == TelescopeAxes.axisPrimary)
        {
          cmd = "M1" + Rate.ToString("+0.0000000;-0.0000000", CultureInfo.InvariantCulture);
        }
        else if (Axis == TelescopeAxes.axisSecondary)
        {
          cmd = "M2" + Rate.ToString("+0.0000000;-0.0000000", CultureInfo.InvariantCulture);
        }
        else
        {
          throw new ASCOM.InvalidValueException("MoveAxis", Axis.ToString(), "0 To 1");
        }
        string ret = CommandSingleChar(cmd);
        if (ret == "0" | ret == "i")
        {
          throw new ASCOM.InvalidValueException("MoveAxis via :" + cmd + " has failed");
        }
        else if (ret == "e")
        {
          throw new ASCOM.DriverException("MoveAxis is ignored, the telescop has already an error");
        }
        else if (ret == "h")
        {
          throw new ASCOM.InvalidValueException("MoveAxis via :" + cmd + " has failed, the requested rate is not supported");
        }
        else if (ret == "s")
        {
          throw new ASCOM.DriverException("MoveAxis is ignored, the telescop is slewing");
        }
        else if (ret == "g")
        {
          throw new ASCOM.DriverException("MoveAxis is ignored, the telescop is guiding");
        }
      }
      else
      {
        throw new ASCOM.MethodNotImplementedException("MoveAxis");
      }
    }


    /// <summary>
    /// Move the telescope to its park position, stop all motion (or restrict to a small safe range), and set <see cref="AtPark" /> to True.
    /// </summary>
    internal static void Park()
    {
      if (CanPark)
      {
        string cmd = "hP";
        if (CommandBoolSingleChar(cmd))
        {
          LogMessage("Park", "Started");
        }
        else
        {
          LogMessage("Park", "failed");
          throw new ASCOM.DriverException("Park has failed");
        }
      }
      else
      {
        throw new ASCOM.MethodNotImplementedException("Park");
      }
    }

    /// <summary>
    /// Moves the scope in the given direction for the given interval or time at
    /// the rate given by the corresponding guide rate property
    /// </summary>
    /// <param name="Direction">The direction in which the guide-rate motion is to be made</param>
    /// <param name="Duration">The duration of the guide-rate motion (milliseconds)</param>
    internal static void PulseGuide(GuideDirections Direction, int Duration)
    {
      if (CanPulseGuide)
      {
        bool ok = !AtPark && !Slewing;
        if (ok)
        {
          string dir = "";
          switch (Direction)
          {
            case GuideDirections.guideNorth:
              {
                dir = "Mgn";
                break;
              }
            case GuideDirections.guideSouth:
              {
                dir = "Mgs";
                break;
              }
            case GuideDirections.guideEast:
              {
                dir = "Mge";
                break;
              }
            case GuideDirections.guideWest:
              {
                dir = "Mgw";
                break;
              }
          }
          CommandBlind(dir + Duration, false);
          LogMessage("PulseGuide", dir + Duration + " done ");
        }
        else
        {
          LogMessage("PulseGuide" , Direction + Duration + " has failed ");
          throw new ASCOM.DriverException("Pulse guiding failed");
        }
      }
      else
      {
        throw new ASCOM.MethodNotImplementedException("PulseGuide");
      }
    }

    /// <summary>
    /// The right ascension (hours) of the telescope's current equatorial coordinates,
    /// in the coordinate system given by the EquatorialSystem property
    /// </summary>
    internal static double RightAscension
    {
      get
      {
        double RA = 0;
        GetDouble("RightAscension", "GRL", ref RA);
        LogMessage("Get RightAscension", utilities.HoursToHMS(RA/15));
        return RA/15;
      }
    }

    /// <summary>
    /// The right ascension tracking rate offset from sidereal (seconds per sidereal second, default = 0.0)
    /// </summary>
    internal static double RightAscensionRate
    {
      get
      {
        double rate = 0;
        if ( TrackingRate == DriveRates.driveSidereal)
        {
          GetDouble("RightAscensionRate", "GXRr", ref rate);
        }
        return rate / 10000d;
      }
      set
      {
        if (CanSetRightAscensionRate)
        {
          int rate = (int)(value * 10000);
          string cmd = "SXRr," + rate.ToString(CultureInfo.InvariantCulture);
          LogMessage("Set RightAscensionRate", "value: " + cmd);
          if (!CommandBoolSingleChar(cmd))
          {
            throw new ASCOM.InvalidValueException("Set RightAscensionRate via :" + cmd + " has failed");
          }
        }
        else
        {
          throw new ASCOM.InvalidOperationException("RightAscensionRate can not be set!");
        }
      }
    }

    /// <summary>
    /// Sets the telescope's park position to be its current position.
    /// </summary>
    internal static void SetPark()
    {
      if (CommandBoolSingleChar("hQ"))
      {
        LogMessage("SetPark", "done");
      }
      else
      {
        LogMessage("SetPark", "failed");
        throw new ASCOM.DriverException("Set Park failed");
      }
    }

    /// <summary>
    /// Indicates the pointing state of the mount. Read the articles installed with the ASCOM Developer
    /// Components for more detailed information.
    /// </summary>
    internal static PierSide SideOfPier
    {
      get
      {
        if (!updateTelStatus())
        {
          throw new ASCOM.InvalidOperationException("Get SideOfPier has failed");
        }
        LogMessage("Get SideOfPier", TelStatus[13].ToString(CultureInfo.InvariantCulture));
        if (TelStatus[13] == ' ')
        {
          LogMessage("Get SideOfPier", "Unknown");
          return PierSide.pierUnknown;
        }
        else if (TelStatus[13] == 'E')
        {
          LogMessage("Get SideOfPier", "East");
          return PierSide.pierEast;
        }
        else if (TelStatus[13] == 'W')
        {
          LogMessage("Get SideOfPier", "West");
          return PierSide.pierWest;
        }
        else
        {
          LogMessage("Get SideOfPier", "failed");
          throw new ASCOM.InvalidOperationException("Get SideOfPier failed");
        }
      }
      set
      {
        PierSide currentSide = SideOfPier;
        if (CanSetPierSide)
        {
          if (value == PierSide.pierUnknown)
          {
            throw new ASCOM.InvalidValueException("German mount cannot be set with pierUnknow");
          }
          else if (currentSide != value)
          {
            checkFlip();
            var state = CommandSingleChar("MF");
            if (state.Length == 0)
            {
              throw new ASCOM.InvalidOperationException("Telescope is not replying");
            }
            else if (!(state.Length == 1))
            {
              throw new ASCOM.InvalidOperationException("Telescope reply is corrupt");
            }
            if (state == "0")
            {
              LogMessage("Slew to target", "Started");
            }
            else
            {
              ReportState(state);
            }
          }
        }
        else
        {
          throw new ASCOM.InvalidOperationException("Change Side of Pier is only supported for german mount");
        }
      }
    }

    /// <summary>
    /// The local apparent sidereal time from the telescope's internal clock (hours, sidereal)
    /// </summary>
    internal static double SiderealTime
    {
      get
      {
        double lst = 0;
        GetDouble("SiderealTime", "GSL", ref lst);
        string response = CommandString("GSL", false);
        return lst;
      }
    }

    /// <summary>
    /// The elevation above mean sea level (meters) of the site at which the telescope is located
    /// </summary>
    internal static double SiteElevation
    {
      get
      {
        string response = CommandString("Ge", false);
        double el = 0;
        GetDouble("SiteElevation", "Ge", ref el);
        return el;
      }
      set
      {
        double el = value;
        if (el <=  -300 || el >= 10000)
        {
          throw new ASCOM.InvalidValueException();
        }
        int el_int = (int)el;
        LogMessage("Set SiteElevation", el_int.ToString("0.0", CultureInfo.InvariantCulture));
        string sg = (el_int >= 0) ?  "+" :  "-";
        string cmd = "Se" + sg + Math.Abs(el_int).ToString("00000", CultureInfo.InvariantCulture);
        CommandBoolSingleChar(cmd);
      }
    }

    /// <summary>
    /// The geodetic(map) latitude (degrees, positive North, WGS84) of the site at which the telescope is located.
    /// </summary>
    internal static double SiteLatitude
    {
      get
      {
        double lt = utilities.DMSToDegrees(CommandString("Gtf", false));
        LogMessage("Get SiteLatitude", lt.ToString("0.000000", CultureInfo.InvariantCulture));
        return lt;
      }
      set
      {
        double lt = value;
        if (lt > 90.0 || lt < -90.0)
        {
          throw new ASCOM.InvalidValueException();
        }
        string sg = (lt >= 0) ? "+" : "-";
        string cmd = "St" + sg + DegtoDDMMSS(Math.Abs(lt));
        if (!CommandBoolSingleChar(cmd))
        {
          throw new ASCOM.InvalidOperationException();
        }
        LogMessage("SiteLatitude Set", lt.ToString(CultureInfo.InvariantCulture));
      }
    }

    /// <summary>
    /// The longitude (degrees, positive East, WGS84) of the site at which the telescope is located.
    /// </summary>
    internal static double SiteLongitude
    {
      get
      {
        double lg = utilities.DMSToDegrees(CommandString("Ggf", false)) * -1;
        LogMessage("Get SiteLongitude", lg.ToString("0.000000"));
        return lg;
      }
      set
      {
        double lg = value;
        if (lg > 180 || lg < -180)
        {
          throw new ASCOM.InvalidValueException();
        }
        string sg = (lg >= 0) ? "+" : "-";
        string cmd = "Sg" + sg + DegtoDDDMMSS(Math.Abs(lg));
        if (!CommandBoolSingleChar(cmd))
        {
          throw new ASCOM.InvalidOperationException();
        }
        LogMessage("Set SiteLongitude", lg.ToString(CultureInfo.InvariantCulture));
      }
    }

    /// <summary>
    /// Specifies a post-slew settling time (sec.).
    /// </summary>
    internal static short SlewSettleTime
    {
      get
      {
        short time = Convert.ToInt16(CommandString("GXOS",false));
        LogMessage("Get SlewSettleTime", time.ToString(CultureInfo.InvariantCulture));
        return time;
      }
      set
      {
        if (value > 21 || value < 0)
        {
          throw new ASCOM.InvalidValueException();
        }
        string cmd = "SXOS," + SlewSettleTime.ToString(CultureInfo.InvariantCulture);
        LogMessage("Set SlewSettleTime", SlewSettleTime.ToString(CultureInfo.InvariantCulture));
        if (!CommandBoolSingleChar(cmd))
        {
          throw new ASCOM.InvalidOperationException();
        }
      }
    }

    /// <summary>
    /// Move the telescope to the given local horizontal coordinates, return when slew is complete
    /// </summary>
    internal static void SlewToAltAz(double Azimuth, double Altitude)
    {
      if (CanSlewAltAz)
      {
        setAzalt(Azimuth, Altitude);
        checkslewALTAZ();
        doslew(true, true);
        LogMessage("SlewToAltAzAsync", "done");
      }
      else
      {
        throw new ASCOM.MethodNotImplementedException();
      }
    }


    /// <summary>
    /// This Method must be implemented if <see cref="CanSlewAltAzAsync" /> returns True.
    /// It returns immediately, with Slewing set to True
    /// </summary>
    /// <param name="Azimuth">Azimuth to which to move</param>
    /// <param name="Altitude">Altitude to which to move to</param>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Style", "VSTHRD200:Use \"Async\" suffix for async methods", Justification = "internal static method name used for many years.")]
    internal static void SlewToAltAzAsync(double Azimuth, double Altitude)
    {
      if (CanSlewAltAzAsync)
      {
        setAzalt(Azimuth, Altitude);
        checkslewALTAZ();
        doslew(false, true);
        LogMessage("SlewToAltAzAsync", "done");
      }
      else
      {
        throw new ASCOM.MethodNotImplementedException();
      }
    }

    /// <summary>
    /// This Method must be implemented if <see cref="CanSlewAltAzAsync" /> returns True.
    /// It does not return to the caller until the slew is complete.
    /// </summary>
    internal static void SlewToCoordinates(double RightAscension, double Declination)
    {
      if (CanSlew)
      {
        TargetDeclination = Declination;
        TargetRightAscension = RightAscension;
        doslew(false);
        LogMessage("SlewToCoordinates", "done");
      }
      else
      {
        throw new ASCOM.MethodNotImplementedException();
      }
    }

    /// <summary>
    /// Move the telescope to the given equatorial coordinates, return with Slewing set to True immediately after starting the slew.
    /// </summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Style", "VSTHRD200:Use \"Async\" suffix for async methods", Justification = "internal static method name used for many years.")]
    internal static void SlewToCoordinatesAsync(double RightAscension, double Declination)
    {
      if (CanSlewAsync)
      {
        TargetDeclination = Declination;
        TargetRightAscension = RightAscension;
        doslew(true);
        LogMessage("SlewToCoordinatesAsync", "done");
      }
      else
      {
        throw new ASCOM.MethodNotImplementedException();
      }
    }

    /// <summary>
    /// Move the telescope to the <see cref="TargetRightAscension" /> and <see cref="TargetDeclination" /> coordinates, return when slew complete.
    /// </summary>
    internal static void SlewToTarget()
    {
      if (CanSlew)
      {
        checkslewRADEC();
        doslew(false);
        LogMessage("SlewToTarget", "done");
      }
      else
      {
        throw new ASCOM.MethodNotImplementedException();
      }
    }

    /// <summary>
    /// Move the telescope to the <see cref="TargetRightAscension" /> and <see cref="TargetDeclination" />  coordinates,
    /// returns immediately after starting the slew with Slewing set to True.
    /// </summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Style", "VSTHRD200:Use \"Async\" suffix for async methods", Justification = "internal static method name used for many years.")]
    internal static void SlewToTargetAsync()
    {
      if (CanSlewAsync)
      {
        checkslewRADEC();
        doslew(true);
        LogMessage("SlewToTargetAsync", "done");
      }
      else
      {
        throw new ASCOM.MethodNotImplementedException();
      }
    }

    /// <summary>
    /// True if telescope is in the process of moving in response to one of the
    /// Slew methods or the <see cref="MoveAxis" /> method, False at all other times.
    /// </summary>
    internal static bool Slewing
    {
      get
      {
        bool slewing = CommandBoolString("GXJS");
        LogMessage("slewing", slewing.ToString());
        return slewing;
      }
    }

    /// <summary>
    /// Matches the scope's local horizontal coordinates to the given local horizontal coordinates.
    /// </summary>
    internal static void SyncToAltAz(double Azimuth, double Altitude)
    {
      setAzalt(Azimuth, Altitude);
      CommandString("CA", false);
      LogMessage("SyncToAltAz", "done");
    }

    /// <summary>
    /// Matches the scope's equatorial coordinates to the given equatorial coordinates.
    /// </summary>
    internal static void SyncToCoordinates(double RightAscension, double Declination)
    {
      TargetRightAscension = RightAscension;
      TargetDeclination =Declination;
      SyncToTarget();
      LogMessage("SyncToCoordinates", "done");
    }

    /// <summary>
    /// Matches the scope's equatorial coordinates to the target equatorial coordinates.
    /// </summary>
    internal static void SyncToTarget()
    {
      if (AtPark)
      {
        throw new ASCOM.ParkedException();
      }
      if (CanSetTracking)
      {
        if (!Tracking)
        {
          throw new ASCOM.InvalidOperationException();
        }
      }
      if (CommandString("CM", false) == "N/A")
      {
        LogMessage("SyncToTarget", "done");
      }
      else
      {
        LogMessage("SyncToTarget", "Failed");
        throw new ASCOM.InvalidOperationException("SyncToTarget has failed");
      }
    }


    private static string DecToString(double value)
    {
      if (value < -90 | value > 90d)
      {
        throw new ASCOM.InvalidValueException();
      }
      string sexa = utilities.DegreesToDMS(value, ":", ":", ""); // Long format, whole seconds
      if (Strings.Left(sexa, 1) != "-")
      {
        sexa = "+" + sexa;         // Both need leading '+'
      }
      return sexa;
    }

    private static string RaToString(double value)
    {
      if (value < 0d | value > 24d)
      {
        throw new ASCOM.InvalidValueException();
      }
      return utilities.HoursToHMS(value, ":", ":", "");   // Long format, whole seconds
    }

    private static string AltToString(double value)
    {
      if (value < -30 | value > 90d)
      {
        throw new ASCOM.InvalidValueException();
      }
      string sexa = utilities.DegreesToDMS(value, ":", ":", ""); // Long format, whole seconds
      if (Strings.Left(sexa, 1) != "-")
      {
        sexa = "+" + sexa;         // Both need leading '+'
      }
      return sexa;
    }

    private static string AzToString(double value)
    {
      if (value < 0d | value > 360d)
      {
        throw new ASCOM.InvalidValueException();
      }
      string sexa = DegtoDDDMMSS(value);
      return sexa;
    }

    /// <summary>
    /// The declination (degrees, positive North) for the target of an equatorial slew or sync operation
    /// </summary>
    internal static double TargetDeclination
    {
      get
      {
        if (tgtDec == -999)
        {
          throw new ASCOM.ValueNotSetException();
        }
        try
        {
          GetDouble("TargetDeclination", "GdL", ref tgtDec);
        }
        catch (Exception ex)
        {
          LogMessage("Get TargetDeclination", ex.Message);
          throw new ASCOM.DriverException("get TargetDeclination has failed");
        }
        LogMessage("Get TargetDeclination", tgtDec.ToString(CultureInfo.InvariantCulture));
        return tgtDec;
      }
      set
      {
        //Thread.CurrentThread.CurrentCulture = CultureInfo.InvariantCulture;
        LogMessage("Set TargetDeclination", value.ToString(CultureInfo.InvariantCulture));
        if (value < -90 | value > 90)
        {
          throw new ASCOM.InvalidValueException();
        }
        string cmd = "SdL" + value.ToString(DECstringFormat, CultureInfo.InvariantCulture);
        if (!CommandBoolSingleChar(cmd))
        {
          throw new ASCOM.InvalidOperationException("Set Target Declination " + cmd + " has failed");
        }
        tgtDec = value;
      }
    }

    /// <summary>
    /// The right ascension (hours) for the target of an equatorial slew or sync operation
    /// </summary>
    internal static double TargetRightAscension
    {
      get
      {
        if (tgtRa == -999)
        {
          throw new ASCOM.ValueNotSetException();
        }
        try
        {
          GetDouble("TargetRightAscension", "GrL", ref tgtRa);
          tgtRa /= 15.0;
        }
        catch (Exception ex)
        {
          LogMessage("Get TargetRightAscension", ex.Message);
          throw new ASCOM.DriverException("get TargetRightAscension has failed");
        }
        LogMessage("Get TargetRightAscension", tgtRa.ToString(CultureInfo.InvariantCulture));
        return tgtRa;
      }
      set
      {
        LogMessage("Set TargetRightAscension", value.ToString(CultureInfo.InvariantCulture));
        if (value < 0 | value > 24)
        {
          throw new ASCOM.InvalidValueException();
        }
        string cmd = "SrL" + (15.0 * value).ToString(RAstringFormat, CultureInfo.InvariantCulture);
        LogMessage("Set TargetRightAscension cmd", cmd);
        if (!CommandBoolSingleChar(cmd))
        {
          throw new ASCOM.InvalidOperationException("Set Target RightAscension " + cmd + " has failed");
        }
        tgtRa = value;
      }
    }

    /// <summary>
    /// The state of the telescope's sidereal tracking drive.
    /// </summary>
    internal static bool Tracking
    {
      get
      {
        bool trk = CommandBoolString("GXJT");
        LogMessage("Get Tracking", trk.ToString(CultureInfo.InvariantCulture));
        return trk;
      }
      set
      {
        if (CanSetTracking)
        {
          if (value)
          {
            if (!CommandBoolSingleChar("Te"))
            {
              throw new ASCOM.InvalidValueException();
            }
          }
          else if (!CommandBoolSingleChar("Td"))
          {
            throw new ASCOM.InvalidOperationException();
          }
          LogMessage("Set Tracking", Tracking.ToString());
        }
        else
        {
          throw new ASCOM.MethodNotImplementedException();
        }
      }
    }

    /// <summary>
    /// The current tracking rate of the telescope's sidereal drive
    /// </summary>
    internal static DriveRates TrackingRate
    {
      get
      {
        DriveRates r = default(DriveRates); // = DriveRates.driveSidereal
        if (!updateTelStatus())
        {
          throw new ASCOM.InvalidOperationException("Get TrackingRate has failed");
        }
        switch (TelStatus.Substring(1, 1))
        {
          case "0":
            {
              r = DriveRates.driveSidereal;
              break;
            }
          case "1":
            {
              r = DriveRates.driveSolar;
              break;
            }
          case "2":
            {
              r = DriveRates.driveLunar;
              break;
            }
        }
        return r;
      }
      set
      {
        TelstatusStopWatch.Stop();
        switch (value)
        {
          case DriveRates.driveSidereal:
            {
              CommandBlind("TQ", false);
              break;
            }
          case DriveRates.driveSolar:
            {
              CommandBlind("TS", false);
              break;
            }
          case DriveRates.driveLunar:
            {
              CommandBlind("TL", false);
              break;
            }

          default:
            {

              throw new InvalidValueException("Unsupported TrackingRate");
            }
          
            // Case DriveRates.driveKing
            // CommandBlind("TK")
        }
      }
    }

    /// <summary>
    /// Returns a collection of supported <see cref="DriveRates" /> values that describe the permissible
    /// values of the <see cref="TrackingRate" /> property for this telescope type.
    /// </summary>
    internal static ITrackingRates TrackingRates
    {
      get
      {
        ITrackingRates trackingRates = new TrackingRates();
        LogMessage("TrackingRates", "Get - ");
        foreach (DriveRates driveRate in trackingRates)
        {
          LogMessage("TrackingRates", "Get - " + driveRate.ToString(CultureInfo.InvariantCulture));
        }
        return trackingRates;
      }
    }

    /// <summary>
    /// The UTC date/time of the telescope's internal clock
    /// </summary>
    internal static DateTime UTCDate
    {
      get
      {
        try
        {
          double secs = double.Parse(CommandString("GXT2", false),CultureInfo.InvariantCulture);
          var utcDate__1 = new DateTime(1970, 1, 1, 0, 0, 0).AddSeconds(secs);
          LogMessage("Get UTCDate", string.Format("Get - {0}", utcDate__1));
          return utcDate__1;
        }
        catch (Exception ex)
        {
          LogMessage("Get UTCDate", "failed: " + ex.Message);
          throw new ASCOM.DriverException(ex.Message);
        }
      }
      set
      {
        long s = (long)(value - new DateTime(1970, 1, 1, 0, 0, 0)).TotalSeconds;
        if (CommandBoolSingleChar("SXT2," + s))
        {
          LogMessage("Set UTCDate", "done");
        }
        else
        {
          LogMessage("Set UTCDate", "failed");
          throw new ASCOM.InvalidOperationException();
        }
      }
    }

    /// <summary>
    /// Takes telescope out of the Parked state.
    /// </summary>
    internal static void Unpark()
    {
      if (CanPark)
      {
        if (CommandBoolSingleChar("hR"))
        {
          LogMessage("Unpark", "done");
        }
        else
        {
          LogMessage("Unpark", "failed");
          throw new ASCOM.InvalidOperationException();
        }
      }
      else
      {
        throw new ASCOM.MethodNotImplementedException();
      }
    }

    private static void doslew(bool @async, [Optional, DefaultParameterValue(false)] bool altaz)
    {

      string state;
      if (altaz)
      {
        state = CommandSingleChar("MA", false);
      }
      else
      {
        state = CommandSingleChar("MS", false);
      }
      if (state.Length == 0)
      {
        throw new ASCOM.DriverException("Telescope is not replying");
      }
      else if (!(state.Length == 1))
      {
        throw new ASCOM.DriverException("Telescope reply is corrupt");
      }
      if (state == "0")
      {
        LogMessage("Slew to target", "Started");
        if (!async)
        {
          while (Slewing)
            System.Threading.Thread.Sleep(1000);
        }
      }
      else
      {
        ReportState(state);
      }
    }

    private static void ReportState(string state)
    {
      int val = Strings.Asc(state[0]) - Strings.Asc('0');
      switch (val)
      {
        case 1:
          {
            throw new ASCOM.InvalidOperationException("Object below min altitude");
          }
        case 2:
          {
            throw new ASCOM.ValueNotSetException();
          }
        case 3:
          {
            throw new ASCOM.InvalidOperationException("Mount Cannot Flip here");
          }
        case 4:
          {
            throw new ASCOM.ParkedException();
          }
        case 5:
          {
            throw new ASCOM.InvalidOperationException("Telescope is Slewing");
          }
        case 6:
          {
            throw new ASCOM.InvalidOperationException("Object is outside limits");
          }
        case 7:
          {
            throw new ASCOM.InvalidOperationException("Telescope is Guiding");
          }
        case 8:
          {
            throw new ASCOM.InvalidOperationException("Object above max altitude");
          }
        case 9:
          {
            throw new ASCOM.InvalidOperationException("Motor is fault");
          }
        case 11:
          {
            throw new ASCOM.InvalidOperationException("Motor is fault");
          }
        case 12:
          {
            throw new ASCOM.InvalidOperationException("Telescope is below horizon limit");
          }
        case 13:
          {
            throw new ASCOM.InvalidOperationException("Limit Sensor");
          }
        case 14:
          {
            throw new ASCOM.InvalidOperationException("Telescope is outside Axis 1 limit");
          }
        case 15:
          {
            throw new ASCOM.InvalidOperationException("Telescope is outside Axis 2 limit");
          }
        case 16:
          {
            throw new ASCOM.InvalidOperationException("Telescope is above overhead limit");
          }
        case 17:
          {
            throw new ASCOM.InvalidOperationException("Telescope is outside meridian limit");
          }
      }
    }

    private static bool checkCompatibility()
    {
      var asm = Assembly.GetExecutingAssembly();
      var fvi = FileVersionInfo.GetVersionInfo(asm.Location);
      string[] versionFW = CommandString("GVN",false).Split('.');
      string[] versionASCOM = fvi.FileVersion.Split('.');
      return true;
      if ((versionFW[0] ?? "") == (versionASCOM[0] ?? "") && (versionFW[1] ?? "") == (versionASCOM[1] ?? ""))
      {
        return true;
      }
      Interaction.MsgBox("Connection has failed!" + Constants.vbCrLf + "TeenAstro version is " + versionFW[0] + "." + versionFW[1] + "." + versionFW[2] + ", TeenAstro driver version is " + fvi.FileVersion);
      return false;
    }

    private static bool updateTelStatus()
    {
      if (!TelstatusStopWatch.IsRunning || TelstatusStopWatch.ElapsedMilliseconds > 100)
      {
        TelStatus = CommandString("GXI", false);
        int l = TelStatus.Length;
        if (TelStatus.Length!=17)
        {
          LogMessage("updateTelStatus Failed", TelStatus);
          TelStatus = "";
          TelstatusStopWatch.Stop();
          return false;
        }
        TelstatusStopWatch.Restart();
      }
      return true;
    }


    private static void DegtoDMS(double degf, ref int degi, ref int mini, ref int seci)
    {
      int tts = (int)Math.Round(Math.Floor(degf * 3600d + 0.5d));
      degi = tts / 3600;
      mini = (tts - degi * 3600) / 60;
      seci = tts % 60;
    }

    private static string DegtoDDDMMSS(double value)
    {
      int d = default, m = default, s = default;
      DegtoDMS(value, ref d, ref m, ref s);
      return d.ToString("000") + ":" + m.ToString("00") + ":" + s.ToString("00",CultureInfo.InvariantCulture);
    }

    private static string DegtoDDMMSS(double value)
    {
      int d = default, m = default, s = default;
      DegtoDMS(value, ref d, ref m, ref s);
      return d.ToString("00") + ":" + m.ToString("00") + ":" + s.ToString("00", CultureInfo.InvariantCulture);
    }

    private static void checkslewRADEC()
    {
      if (tgtDec == -999 | tgtRa == -999)
      {
        throw new ASCOM.ValueNotSetException();
      }
      if (AtPark)
      {
        throw new ASCOM.ParkedException();
      }
      if (!Tracking)
      {
        throw new ASCOM.InvalidOperationException();
      }
      while (Slewing)
      {
        AbortSlew();
        System.Threading.Thread.Sleep(200);
      }
      while (IsPulseGuiding)
      {
        AbortSlew();
        System.Threading.Thread.Sleep(200);
      }
    }

    private static void checkFlip()
    {
      if (AtPark)
      {
        throw new ASCOM.ParkedException();
      }
      if (!Tracking)
      {
        throw new ASCOM.InvalidOperationException();
      }
      while (Slewing)
      {
        AbortSlew();
        System.Threading.Thread.Sleep(100);
      }
      while (IsPulseGuiding)
      {
        AbortSlew();
        System.Threading.Thread.Sleep(100);
      }
    }


    private static void checkslewALTAZ()
    {

      if (AtPark)
      {
        throw new ASCOM.ParkedException();
      }
      while (Slewing)
      {
        AbortSlew();
        System.Threading.Thread.Sleep(100);
      }
    }

    private static void setAzalt(double Azimuth, double Altitude)
    {
      string sexa = AzToString(Azimuth);
      if (!CommandBoolSingleChar("Sz" + sexa))
      {
        throw new ASCOM.InvalidOperationException();
      }
      sexa = AltToString(Altitude);
      if (!CommandBoolSingleChar("Sa" + sexa))
      {
        throw new ASCOM.InvalidOperationException();
      }
    }



    #endregion

    #region Private properties and methods
    // Useful methods that can be used as required to help with driver development

    /// <summary>
    /// Returns true if there is a valid connection to the driver hardware
    /// </summary>
    private static bool IsConnected
    {
      get
      {
        bool ok = false;
        double s1 = (DateTime.UtcNow - ConnectionStatusDate).TotalMilliseconds;
        try
        {
          if (s1 > 1000000000d && connectedState)
          {
            int k = 0;
            while ( k < 10)
            {
              if (CommandBoolString("GXJC"))
              {
                ok = true;
                break;
              }
              System.Threading.Thread.Sleep(200);
              k += 1;
            }
          }
        }
        catch (Exception ex)
        {
          throw new ASCOM.DriverException(ex.Message);
        }
        return ok;
      }
    }

    /// <summary>
    /// Use this function to throw an exception if we aren't connected to the hardware
    /// </summary>
    /// <param name="message"></param>
    private static void CheckConnected(string message)
    {
      if (!IsConnected)
      {
        throw new NotConnectedException(message);
      }
    }

    /// <summary>
    /// Read the device configuration from the ASCOM Profile store
    /// </summary>
    internal static void ReadProfile()
    {
      using (Profile driverProfile = new Profile())
      {
        driverProfile.DeviceType = "Telescope";
        tl.Enabled = Convert.ToBoolean(driverProfile.GetValue(DriverProgId, traceStateProfileName, string.Empty, traceStateDefault));
        comPort = driverProfile.GetValue(DriverProgId, comPortProfileName, string.Empty, comPortDefault);
        IP = driverProfile.GetValue(DriverProgId, IPProfileName, string.Empty, IPDefault);
        Port = Convert.ToInt16(driverProfile.GetValue(DriverProgId, PortProfileName, string.Empty, PortDefault.ToString()));
        Interface = driverProfile.GetValue(DriverProgId, InterfaceProfileName, string.Empty, InterfaceDefault);
      }
    }

    /// <summary>
    /// Write the device configuration to the  ASCOM  Profile store
    /// </summary>
    internal static void WriteProfile()
    {
      using (Profile driverProfile = new Profile())
      {
        driverProfile.DeviceType = "Telescope";
        driverProfile.WriteValue(DriverProgId, traceStateProfileName, tl.Enabled.ToString());
        driverProfile.WriteValue(DriverProgId, comPortProfileName, comPort.ToString());
        driverProfile.WriteValue(DriverProgId, IPProfileName, IP.ToString());
        driverProfile.WriteValue(DriverProgId, PortProfileName, Port.ToString());
        driverProfile.WriteValue(DriverProgId, InterfaceProfileName, Interface.ToString());
      }
    }

    /// <summary>
    /// Log helper function that takes identifier and message strings
    /// </summary>
    /// <param name="identifier"></param>
    /// <param name="message"></param>
    internal static void LogMessage(string identifier, string message)
    {
      tl.LogMessageCrLf(identifier, message);
    }

    /// <summary>
    /// Log helper function that takes formatted strings and arguments
    /// </summary>
    /// <param name="identifier"></param>
    /// <param name="message"></param>
    /// <param name="args"></param>
    internal static void LogMessage(string identifier, string message, params object[] args)
    {
      var msg = string.Format(message, args);
      LogMessage(identifier, msg);
    }
    #endregion
  }
}


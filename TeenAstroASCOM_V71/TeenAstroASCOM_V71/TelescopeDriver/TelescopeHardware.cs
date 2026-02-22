// TODO fill in this information for your driver, then remove this line!
//
// ASCOM Telescope hardware class for TeenAstro
//
// Description:	 <To be completed by driver developer>
//
// Implements:	ASCOM Telescope interface version: <To be completed by driver developer>
// Author:		(XXX) Your N. Here <your@email.here>

// TODO: Customise the SetConnected and InitialiseHardware methods as needed for your hardware

using ASCOM;
using ASCOM.Astrometry;
using ASCOM.Astrometry.AstroUtils;
using ASCOM.Astrometry.NOVAS;
using ASCOM.DeviceInterface;
using ASCOM.Utilities;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.Threading.Tasks;
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
        internal const string IPProfileName = "IP Address";
        internal const string IPDefault = "192.168.0.1";
        internal const string PortProfileName = "Port";
        internal const string PortDefault = "9999";
        internal const string InterfaceProfileName = "Interface";
        internal const string InterfaceDefault = "COM";
        internal const string InterfaceCOM = "COM";
        internal const string InterfaceIP = "IP";
        internal const string traceStateProfileName = "Trace Level";
        internal const string traceStateDefault = "true";

        // Message constants
        private const string MsgNotImplemented = "Not implemented";
        private const string MsgGetPrefix = "Get - ";
        private const string MsgInvalidIP = " is not a valid IP Address";
        private const string MsgConnectionFailed = "Connection has failed!";
        private const string MsgDone = "done";
        private const string DriverName = "TeenAstro";
        private const string DmsSeparator = ":";

        private static string DriverProgId = "";
        private static string DriverDescription = "";
        internal static string comPort;
        internal static string IP;
        internal static short Port;
        internal static string Interface;
        internal static System.Net.IPAddress objectIP;
        private static TeenAstroConnection _connection;
        private static bool connectedState;
        private static bool runOnce = false;
        internal static Util utilities;
        internal static AstroUtils astroUtilities;
        internal static TraceLogger tl;
        private static List<Guid> uniqueIds = new List<Guid>();

        private static GXASStateWrapper _gxasState;
        private static System.Diagnostics.Stopwatch _gxasStopwatch = new System.Diagnostics.Stopwatch();
        private static DateTime ConnectionStatusDate;
        private static double tgtRa = -999;
        private static double tgtDec = -999;
        private static bool HasMotors = true;

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
                // Do not show UI from a static initializer. Log and rethrow to allow caller to handle the error.
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
                return new ArrayList();
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
            LogMessage("Action", $"Action {actionName}, parameters {actionParameters} is not implemented");
            throw new ActionNotImplementedException("Action " + actionName + " is not implemented by this driver");
        }

        #region Communication (Native DLL)

        private static bool NativeCommandBlind(string command, bool raw)
        {
            return _connection.CommandBlind(command, raw);
        }

        private static bool NativeCommandBool(string command, bool raw)
        {
            return _connection.CommandBool(command, raw);
        }

        private static string NativeCommandString(string command, bool raw)
        {
            return _connection.CommandString(command, raw);
        }

        private static void CloseNative()
        {
            connectedState = false;
            if (_connection != null)
            {
                try { _connection.Disconnect(); } catch { }
                _connection = null;
            }
        }

        private static void ConnectSerial(bool value)
        {
            if (value)
            {
                LogMessage("Connected Set", "Connecting to port " + comPort);
                _connection = new TeenAstroConnection();
                if (!_connection.ConnectSerial(comPort))
                {
                    _connection = null;
                    throw new DriverException("Failed to connect to " + comPort);
                }
                connectedState = true;
                ConnectionStatusDate = DateTime.UtcNow;
                try { HasMotors = _connection.HasMotors(); }
                catch (Exception ex) { CloseNative(); throw new DriverException(ex.Message); }
            }
            else
            {
                CloseNative();
                LogMessage("Connected Set", "Disconnecting from port " + comPort);
            }
        }

        private static void ConnectIP(bool value)
        {
            if (value)
            {
                if (!System.Net.IPAddress.TryParse(IP, out objectIP))
                {
                    LogMessage("ConnectIP", IP + MsgInvalidIP);
                    throw new InvalidValueException(IP + MsgInvalidIP);
                }
                LogMessage("Connected Set", "Connecting to " + IP + ":" + Port);
                _connection = new TeenAstroConnection();
                if (!_connection.ConnectTcp(IP, Port))
                {
                    _connection = null;
                    throw new InvalidValueException(MsgConnectionFailed);
                }
                connectedState = true;
                ConnectionStatusDate = DateTime.UtcNow;
                try { HasMotors = _connection.HasMotors(); }
                catch (Exception ex) { connectedState = false; throw new DriverException(ex.Message); }
                DateTime timeTelescope = _gxasState != null && _gxasState.Valid ? _gxasState.GetUTCDate() : DateTime.UtcNow;
                if (Math.Abs((timeTelescope - DateTime.UtcNow).TotalSeconds) > 2d)
                    LogMessage("Connected Set", "Synced with computer time");
            }
            else
            {
                CloseNative();
                LogMessage("Connected Set", "Disconnecting from IP " + IP);
            }
        }

        private static bool UpdateGXASState()
        {
            if (_connection == null || !_connection.IsConnected) return false;
            if (!_gxasStopwatch.IsRunning || _gxasStopwatch.ElapsedMilliseconds > 100)
            {
                if (_connection.GetMountState(out GXASState state) && state.Valid != 0)
                {
                    _gxasState = new GXASStateWrapper(state);
                    _gxasStopwatch.Restart();
                    return true;
                }
                _gxasState = null;
                _gxasStopwatch.Stop();
                return false;
            }
            return _gxasState != null && _gxasState.Valid;
        }

        #endregion

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
            CheckConnected("CommandBlind");
            if (!NativeCommandBlind(command, raw))
                throw new NotConnectedException("CommandBlind " + command + " has failed");
            ConnectionStatusDate = DateTime.UtcNow;
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
        public static bool CommandBool(string command, bool raw)
        {
            CheckConnected("CommandBool");
            bool r = _connection.CommandBool(command, raw);
            ConnectionStatusDate = DateTime.UtcNow;
            return r;
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
        public static string CommandString(string command, bool raw)
        {
            CheckConnected("CommandString");
            string buf = NativeCommandString(command, raw);
            if (buf == null)
                throw new NotConnectedException("CommandString " + command + " has failed");
            ConnectionStatusDate = DateTime.UtcNow;
            return buf;
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
                        if (Interface == InterfaceCOM)
                            ConnectSerial(true);
                        else if (Interface == InterfaceIP)
                        {
                            if (System.Net.IPAddress.TryParse(IP, out objectIP))
                            {
                                ConnectIP(true);
                            }
                            else
                            {
                                // Do not show UI from driver code. Log and throw so caller can handle the error.
                                LogMessage("SetConnected", IP + MsgInvalidIP);
                                throw new InvalidValueException(IP + MsgInvalidIP);
                            }
                        }
                        if (!connectedState)
                            throw new InvalidValueException(MsgConnectionFailed);
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
                        if (Interface == InterfaceCOM)
                            ConnectSerial(false);
                        else if (Interface == InterfaceIP)
                            ConnectIP(false);
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
            get
            {
                LogMessage("Name Get", DriverName);
                return DriverName;
            }
        }

        #endregion

        #region ITelescope Implementation

        /// <summary>
        /// Stops a slew in progress.
        /// </summary>
        internal static void AbortSlew()
        {
            if (!_connection.AbortSlew())
                throw new NotConnectedException("AbortSlew has failed");
            ConnectionStatusDate = DateTime.UtcNow;
            LogMessage("AbortSlew", MsgDone);
        }

        /// <summary>
        /// The alignment mode of the mount (Alt/Az, Polar, German Polar).
        /// </summary>
        internal static AlignmentModes AlignmentMode
        {
            get
            {
                ThrowPropertyNotImplemented("AlignmentMode", false);
                return default(AlignmentModes);
            }
        }

        /// <summary>
        /// The Altitude above the local horizon of the telescope's current position (degrees, positive up)
        /// </summary>
        internal static double Altitude
        {
            get
            {
                if (!UpdateGXASState())
                    throw new InvalidOperationException("Altitude unavailable");
                double alt = _gxasState.AltitudeDegrees;
                LogMessage("Altitude", utilities.DegreesToDMS(alt, DmsSeparator, DmsSeparator));
                return alt;
            }
        }

        /// <summary>
        /// The area of the telescope's aperture, taking into account any obstructions (square meters)
        /// </summary>
        internal static double ApertureArea
        {
            get
            {
                ThrowPropertyNotImplemented("ApertureArea", false);
                return 0.0;
            }
        }

        /// <summary>
        /// The telescope's effective aperture diameter (meters)
        /// </summary>
        internal static double ApertureDiameter
        {
            get
            {
                ThrowPropertyNotImplemented("ApertureDiameter", false);
                return 0.0;
            }
        }

        /// <summary>
        /// The azimuth at the local horizon of the telescope's current position (degrees, North-referenced, positive East/clockwise).
        /// </summary>
        internal static double Azimuth
        {
            get
            {
                if (!UpdateGXASState())
                    throw new InvalidOperationException("Azimuth unavailable");
                double az = _gxasState.AzimuthDegrees;
                LogMessage("Azimuth", utilities.DegreesToDMS(az, DmsSeparator, DmsSeparator));
                return az;
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
                if (!UpdateGXASState())
                    return false;
                bool v = _gxasState.AtHome;
                LogMessage("AtHome", MsgGetPrefix + v.ToString());
                return v;
            }
        }

        /// <summary>
        /// True if the telescope has been put into the parked state by the seee <see cref="Park" /> method. Set False by calling the Unpark() method.
        /// </summary>
        internal static bool AtPark
        {
            get
            {
                if (!UpdateGXASState())
                    return false;
                bool v = _gxasState.ParkState == 2;
                LogMessage("AtPark", MsgGetPrefix + v.ToString());
                return v;
            }
        }

        /// <summary>
        /// Determine the rates at which the telescope may be moved about the specified axis by the <see cref="MoveAxis" /> method.
        /// </summary>
        /// <param name="Axis">The axis about which rate information is desired (TelescopeAxes value)</param>
        /// <returns>Collection of <see cref="IRate" /> rate objects</returns>
        internal static IAxisRates AxisRates(TelescopeAxes Axis)
        {
            LogMessage("AxisRates", MsgGetPrefix + Axis.ToString());
            return new AxisRates(Axis);
        }

        /// <summary>
        /// True if this telescope is capable of programmed finding its home position (<see cref="FindHome" /> method).
        /// </summary>
        internal static bool CanFindHome
        {
            get { return _connection != null && _connection.IsConnected && _connection.HasSite() && HasMotors; }
        }

        /// <summary>
        /// True if this telescope can move the requested axis
        /// </summary>
        internal static bool CanMoveAxis(TelescopeAxes Axis)
        {
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
            get { return _connection != null && _connection.IsConnected && _connection.HasSite() && HasMotors; }
        }

        /// <summary>
        /// True if this telescope is capable of software-pulsed guiding (via the <see cref="PulseGuide" /> method)
        /// </summary>
        internal static bool CanPulseGuide
        {
            get { return HasMotors; }
        }

        /// <summary>
        /// True if the <see cref="DeclinationRate" /> property can be changed to provide offset tracking in the declination axis.
        /// </summary>
        internal static bool CanSetDeclinationRate
        {
            get { return HasMotors; }
        }

        /// <summary>
        /// True if the guide rate properties used for <see cref="PulseGuide" /> can be adjusted.
        /// </summary>
        internal static bool CanSetGuideRates
        {
            get { return HasMotors; }
        }

        /// <summary>
        /// True if this telescope is capable of programmed setting of its park position (<see cref="SetPark" /> method)
        /// </summary>
        internal static bool CanSetPark
        {
            get { return _connection != null && _connection.IsConnected && _connection.HasSite() && HasMotors; }
        }

        /// <summary>
        /// True if the <see cref="SideOfPier" /> property can be set, meaning that the mount can be forced to flip.
        /// </summary>
        internal static bool CanSetPierSide
        {
            get { return false; }
        }

        /// <summary>
        /// True if the <see cref="RightAscensionRate" /> property can be changed to provide offset tracking in the right ascension axis.
        /// </summary>
        internal static bool CanSetRightAscensionRate
        {
            get { return HasMotors; }
        }

        /// <summary>
        /// True if the <see cref="Tracking" /> property can be changed, turning telescope sidereal tracking on and off.
        /// </summary>
        internal static bool CanSetTracking
        {
            get { return HasMotors; }
        }

        /// <summary>
        /// True if this telescope is capable of programmed slewing (synchronous or asynchronous) to equatorial coordinates
        /// </summary>
        internal static bool CanSlew
        {
            get { return HasMotors; }
        }

        /// <summary>
        /// True if this telescope is capable of programmed slewing (synchronous or asynchronous) to local horizontal coordinates
        /// </summary>
        internal static bool CanSlewAltAz
        {
            get { return HasMotors; }
        }

        /// <summary>
        /// True if this telescope is capable of programmed asynchronous slewing to local horizontal coordinates
        /// </summary>
        internal static bool CanSlewAltAzAsync
        {
            get { return HasMotors; }
        }

        /// <summary>
        /// True if this telescope is capable of programmed asynchronous slewing to equatorial coordinates.
        /// </summary>
        internal static bool CanSlewAsync
        {
            get { return HasMotors; }
        }

        /// <summary>
        /// True if this telescope is capable of programmed syncing to equatorial coordinates.
        /// </summary>
        internal static bool CanSync
        {
            get { return HasMotors; }
        }

        /// <summary>
        /// True if this telescope is capable of programmed syncing to local horizontal coordinates
        /// </summary>
        internal static bool CanSyncAltAz
        {
            get { return HasMotors; }
        }

        /// <summary>
        /// True if this telescope is capable of programmed unparking (<see cref="Unpark" /> method).
        /// </summary>
        internal static bool CanUnpark
        {
            get { return _connection != null && _connection.IsConnected && _connection.HasSite() && HasMotors; }
        }

        /// <summary>
        /// The declination (degrees) of the telescope's current equatorial coordinates, in the coordinate system given by the <see cref="EquatorialSystem" /> property.
        /// Reading the property will raise an error if the value is unavailable.
        /// </summary>
        internal static double Declination
        {
            get
            {
                if (!UpdateGXASState())
                    throw new InvalidOperationException("Declination unavailable");
                double dec = _gxasState.DeclinationDegrees;
                LogMessage("Declination", MsgGetPrefix + utilities.DegreesToDMS(dec, DmsSeparator, DmsSeparator));
                return dec;
            }
        }

        /// <summary>
        /// The declination tracking rate (arcseconds per SI second, default = 0.0)
        /// </summary>
        internal static double DeclinationRate
        {
            get
            {
                double declination = 0.0;
                LogMessage("DeclinationRate", MsgGetPrefix + declination.ToString());
                return declination;
            }
            set
            {
                ThrowPropertyNotImplemented("DeclinationRate", true);
            }
        }

        /// <summary>
        /// Predict side of pier for German equatorial mounts at the provided coordinates
        /// </summary>
        internal static PierSide DestinationSideOfPier(double RightAscension, double Declination)
        {
            ThrowPropertyNotImplemented("DestinationSideOfPier", false);
            return default(PierSide);
        }

        /// <summary>
        /// True if the telescope or driver applies atmospheric refraction to coordinates.
        /// </summary>
        internal static bool DoesRefraction
        {
            get
            {
                ThrowPropertyNotImplemented("DoesRefraction", false);
                return false;
            }
            set
            {
                ThrowPropertyNotImplemented("DoesRefraction", true);
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
                LogMessage("DeclinationRate", MsgGetPrefix + equatorialSystem.ToString());
                return equatorialSystem;
            }
        }

        /// <summary>
        /// Locates the telescope's "home" position (synchronous)
        /// </summary>
        internal static void FindHome()
        {
            ThrowMethodNotImplemented("FindHome");
        }

        /// <summary>
        /// The telescope's focal length, meters
        /// </summary>
        internal static double FocalLength
        {
            get
            {
                ThrowPropertyNotImplemented("FocalLength", false);
                return 0.0;
            }
        }

        /// <summary>
        /// The current Declination movement rate offset for telescope guiding (degrees/sec)
        /// </summary>
        internal static double GuideRateDeclination
        {
            get
            {
                ThrowPropertyNotImplemented("GuideRateDeclination", false);
                return 0.0;
            }
            set
            {
                ThrowPropertyNotImplemented("GuideRateDeclination", true);
            }
        }

        /// <summary>
        /// The current Right Ascension movement rate offset for telescope guiding (degrees/sec)
        /// </summary>
        internal static double GuideRateRightAscension
        {
            get
            {
                ThrowPropertyNotImplemented("GuideRateRightAscension", false);
                return 0.0;
            }
            set
            {
                ThrowPropertyNotImplemented("GuideRateRightAscension", true);
            }
        }

        /// <summary>
        /// True if a <see cref="PulseGuide" /> command is in progress, False otherwise
        /// </summary>
        internal static bool IsPulseGuiding
        {
            get
            {
                if (!UpdateGXASState())
                    return false;
                return _gxasState.IsPulseGuiding;
            }
        }

        /// <summary>
        /// Move the telescope in one axis at the given rate.
        /// </summary>
        /// <param name="Axis">The physical axis about which movement is desired</param>
        /// <param name="Rate">The rate of motion (deg/sec) about the specified axis</param>
        internal static void MoveAxis(TelescopeAxes Axis, double Rate)
        {
            if (Math.Abs(Rate) < 1e-6)
            {
                _connection.AbortSlew();
                ConnectionStatusDate = DateTime.UtcNow;
                return;
            }
            int axis = (Axis == TelescopeAxes.axisPrimary) ? 0 : 1;
            double rateArcsec = Rate * 3600.0; // deg/sec to arcsec/sec
            if (!_connection.MoveAxis(axis, rateArcsec))
                throw new NotConnectedException("MoveAxis has failed");
            ConnectionStatusDate = DateTime.UtcNow;
            LogMessage("MoveAxis", Axis + " " + Rate.ToString(CultureInfo.InvariantCulture));
        }


        /// <summary>
        /// Move the telescope to its park position, stop all motion (or restrict to a small safe range), and set <see cref="AtPark" /> to True.
        /// </summary>
        internal static void Park()
        {
            if (!_connection.Park())
                throw new InvalidOperationException("Park has failed");
            ConnectionStatusDate = DateTime.UtcNow;
            LogMessage("Park", MsgDone);
        }

        /// <summary>
        /// Moves the scope in the given direction for the given interval or time at
        /// the rate given by the corresponding guide rate property
        /// </summary>
        /// <param name="Direction">The direction in which the guide-rate motion is to be made</param>
        /// <param name="Duration">The duration of the guide-rate motion (milliseconds)</param>
        internal static void PulseGuide(GuideDirections Direction, int Duration)
        {
            int dir;
            switch (Direction)
            {
                case GuideDirections.guideNorth: dir = 0; break;
                case GuideDirections.guideSouth: dir = 1; break;
                case GuideDirections.guideEast: dir = 2; break;
                case GuideDirections.guideWest: dir = 3; break;
                default: throw new InvalidValueException("PulseGuide", Direction.ToString(), "Invalid direction");
            }
            if (!_connection.PulseGuide(dir, Duration))
                throw new NotConnectedException("PulseGuide has failed");
            ConnectionStatusDate = DateTime.UtcNow;
            LogMessage("PulseGuide", Direction + " " + (Duration / 1000.0).ToString("0.000", CultureInfo.InvariantCulture) + "s");
        }

        /// <summary>
        /// The right ascension (hours) of the telescope's current equatorial coordinates,
        /// in the coordinate system given by the EquatorialSystem property
        /// </summary>
        internal static double RightAscension
        {
            get
            {
                if (!UpdateGXASState())
                    throw new InvalidOperationException("RightAscension unavailable");
                double ra = _gxasState.RightAscensionHours;
                LogMessage("RightAscension", MsgGetPrefix + utilities.HoursToHMS(ra));
                return ra;
            }
        }

        /// <summary>
        /// The right ascension tracking rate offset from sidereal (seconds per sidereal second, default = 0.0)
        /// </summary>
        internal static double RightAscensionRate
        {
            get
            {
                double rightAscensionRate = 0.0;
                LogMessage("RightAscensionRate", MsgGetPrefix + rightAscensionRate.ToString());
                return rightAscensionRate;
            }
            set
            {
                ThrowPropertyNotImplemented("RightAscensionRate", true);
            }
        }

        /// <summary>
        /// Sets the telescope's park position to be its current position.
        /// </summary>
        internal static void SetPark()
        {
            if (!_connection.SetPark())
                throw new InvalidOperationException("SetPark has failed");
            ConnectionStatusDate = DateTime.UtcNow;
            LogMessage("SetPark", MsgDone);
        }

        /// <summary>
        /// Indicates the pointing state of the mount. Read the articles installed with the ASCOM Developer
        /// Components for more detailed information.
        /// </summary>
        internal static PierSide SideOfPier
        {
            get
            {
                if (!UpdateGXASState())
                    throw new InvalidOperationException("SideOfPier unavailable");
                return _gxasState.PierSideWest ? DeviceInterface.PierSide.pierWest : DeviceInterface.PierSide.pierEast;
            }
            set { throw new PropertyNotImplementedException("SideOfPier", true); }
        }

        /// <summary>
        /// The local apparent sidereal time from the telescope's internal clock (hours, sidereal)
        /// </summary>
        internal static double SiderealTime
        {
            get
            {
                if (UpdateGXASState() && _gxasState.Valid)
                {
                    double lst = _gxasState.SiderealTimeHours;
                    lst = astroUtilities.ConditionRA(lst);
                    LogMessage("SiderealTime", MsgGetPrefix + lst.ToString(CultureInfo.InvariantCulture));
                    return lst;
                }
                double siderealTime = 0.0;
                using (var novas = new NOVAS31())
                {
                    double julianDate = utilities.DateUTCToJulian(DateTime.UtcNow);
                    novas.SiderealTime(julianDate, 0, novas.DeltaT(julianDate), GstType.GreenwichApparentSiderealTime, Method.EquinoxBased, Accuracy.Full, ref siderealTime);
                }
                try { siderealTime += SiteLongitude / 360.0 * 24.0; }
                catch (PropertyNotImplementedException) { }
                catch (Exception) { throw; }
                siderealTime = astroUtilities.ConditionRA(siderealTime);
                LogMessage("SiderealTime", MsgGetPrefix + siderealTime.ToString(CultureInfo.InvariantCulture));
                return siderealTime;
            }
        }

        /// <summary>
        /// The elevation above mean sea level (meters) of the site at which the telescope is located
        /// </summary>
        internal static double SiteElevation
        {
            get
            {
                ThrowPropertyNotImplemented("SiteElevation", false);
                return 0.0;
            }
            set
            {
                ThrowPropertyNotImplemented("SiteElevation", true);
            }
        }

        /// <summary>
        /// The geodetic(map) latitude (degrees, positive North, WGS84) of the site at which the telescope is located.
        /// </summary>
        internal static double SiteLatitude
        {
            get
            {
                if (!_connection.GetSiteLatitude(out double lat))
                    throw new DriverException("get SiteLatitude has failed");
                return lat;
            }
            set
            {
                string sg = value >= 0 ? "+" : "-";
                string latStr = sg + DegtoDDMMSS(Math.Abs(value));
                if (!_connection.SetSiteLatitude(latStr))
                    throw new InvalidOperationException("Set SiteLatitude failed");
            }
        }

        /// <summary>
        /// The longitude (degrees, positive East, WGS84) of the site at which the telescope is located.
        /// </summary>
        internal static double SiteLongitude
        {
            get
            {
                if (!_connection.GetSiteLongitude(out double lg))
                    throw new DriverException("get SiteLongitude has failed");
                return lg * -1;
            }
            set
            {
                string sg = value >= 0 ? "+" : "-";
                string lonStr = sg + DegtoDDDMMSS(Math.Abs(value));
                if (!_connection.SetSiteLongitude(lonStr))
                    throw new InvalidOperationException("Set SiteLongitude failed");
            }
        }

        /// <summary>
        /// Specifies a post-slew settling time (sec.).
        /// </summary>
        internal static short SlewSettleTime
        {
            get
            {
                ThrowPropertyNotImplemented("SlewSettleTime", false);
                return 0;
            }
            set
            {
                ThrowPropertyNotImplemented("SlewSettleTime", true);
            }
        }

        /// <summary>
        /// Move the telescope to the given local horizontal coordinates, return when slew is complete
        /// </summary>
        internal static void SlewToAltAz(double Azimuth, double Altitude)
        {
            SetAzAlt(Azimuth, Altitude);
            CheckSlewAltAz();
            DoSlew(false, true);
            LogMessage("SlewToAltAz", MsgDone);
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
            SetAzAlt(Azimuth, Altitude);
            CheckSlewAltAz();
            DoSlew(true, true);
            LogMessage("SlewToAltAzAsync", MsgDone);
        }

        /// <summary>
        /// This Method must be implemented if <see cref="CanSlewAltAzAsync" /> returns True.
        /// It does not return to the caller until the slew is complete.
        /// </summary>
        internal static void SlewToCoordinates(double RightAscension, double Declination)
        {
            TargetRightAscension = RightAscension;
            TargetDeclination = Declination;
            CheckSlewRADEC();
            DoSlew(false, false);
            LogMessage("SlewToCoordinates", MsgDone);
        }

        /// <summary>
        /// Move the telescope to the given equatorial coordinates, return with Slewing set to True immediately after starting the slew.
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Style", "VSTHRD200:Use \"Async\" suffix for async methods", Justification = "internal static method name used for many years.")]
        internal static void SlewToCoordinatesAsync(double RightAscension, double Declination)
        {
            TargetRightAscension = RightAscension;
            TargetDeclination = Declination;
            CheckSlewRADEC();
            DoSlew(true, false);
            LogMessage("SlewToCoordinatesAsync", MsgDone);
        }

        /// <summary>
        /// Move the telescope to the <see cref="TargetRightAscension" /> and <see cref="TargetDeclination" /> coordinates, return when slew complete.
        /// </summary>
        internal static void SlewToTarget()
        {
            CheckSlewRADEC();
            DoSlew(false, false);
            LogMessage("SlewToTarget", MsgDone);
        }

        /// <summary>
        /// Move the telescope to the <see cref="TargetRightAscension" /> and <see cref="TargetDeclination" />  coordinates,
        /// returns immediately after starting the slew with Slewing set to True.
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Style", "VSTHRD200:Use \"Async\" suffix for async methods", Justification = "internal static method name used for many years.")]
        internal static void SlewToTargetAsync()
        {
            CheckSlewRADEC();
            DoSlew(true, false);
            LogMessage("SlewToTargetAsync", MsgDone);
        }

        /// <summary>
        /// True if telescope is in the process of moving in response to one of the
        /// Slew methods or the <see cref="MoveAxis" /> method, False at all other times.
        /// </summary>
        internal static bool Slewing
        {
            get
            {
                if (!UpdateGXASState())
                    return false;
                return _gxasState.Slewing;
            }
        }

        /// <summary>
        /// Matches the scope's local horizontal coordinates to the given local horizontal coordinates.
        /// </summary>
        internal static void SyncToAltAz(double Azimuth, double Altitude)
        {
            SetAzAlt(Azimuth, Altitude);
            if (!_connection.SyncToAltAz())
                throw new InvalidOperationException("SyncToAltAz has failed");
            ConnectionStatusDate = DateTime.UtcNow;
            LogMessage("SyncToAltAz", MsgDone);
        }

        /// <summary>
        /// Matches the scope's equatorial coordinates to the given equatorial coordinates.
        /// </summary>
        internal static void SyncToCoordinates(double RightAscension, double Declination)
        {
            TargetRightAscension = RightAscension;
            TargetDeclination = Declination;
            SyncToTarget();
            LogMessage("SyncToCoordinates", MsgDone);
        }

        /// <summary>
        /// Matches the scope's equatorial coordinates to the target equatorial coordinates.
        /// </summary>
        internal static void SyncToTarget()
        {
            if (AtPark)
                throw new ParkedException();
            if (CanSetTracking && !Tracking)
                throw new InvalidOperationException("Tracking must be on to sync");
            if (!_connection.SyncToEquatorial())
                throw new InvalidOperationException("SyncToTarget has failed");
            ConnectionStatusDate = DateTime.UtcNow;
            LogMessage("SyncToTarget", MsgDone);
        }

        /// <summary>
        /// The declination (degrees, positive North) for the target of an equatorial slew or sync operation
        /// </summary>
        internal static double TargetDeclination
        {
            get
            {
                if (tgtDec == -999)
                    throw new ValueNotSetException();
                if (!UpdateGXASState())
                    return tgtDec;
                tgtDec = _gxasState.TargetDecDegrees;
                return tgtDec;
            }
            set
            {
                LogMessage("Set TargetDeclination", value.ToString(CultureInfo.InvariantCulture));
                if (value < -90 || value > 90)
                    throw new InvalidValueException("TargetDeclination", value.ToString(), "-90 to 90");
                if (!_connection.SetTargetDec(value))
                    throw new InvalidOperationException("Set Target Declination has failed");
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
                    throw new ValueNotSetException();
                if (!UpdateGXASState())
                    return tgtRa;
                tgtRa = _gxasState.TargetRAHours;
                return tgtRa;
            }
            set
            {
                LogMessage("Set TargetRightAscension", value.ToString(CultureInfo.InvariantCulture));
                if (value < 0 || value > 24)
                    throw new InvalidValueException("TargetRightAscension", value.ToString(), "0 to 24");
                if (!_connection.SetTargetRA(value))
                    throw new InvalidOperationException("Set Target RightAscension has failed");
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
                if (!UpdateGXASState())
                    return true;
                bool trk = _gxasState.Tracking;
                LogMessage("Get Tracking", trk.ToString());
                return trk;
            }
            set
            {
                if (!_connection.EnableTracking(value))
                {
                    if (value)
                        throw new InvalidValueException("Could not enable tracking");
                    throw new InvalidOperationException("Could not disable tracking");
                }
                ConnectionStatusDate = DateTime.UtcNow;
                LogMessage("Set Tracking", value.ToString());
            }
        }

        /// <summary>
        /// The current tracking rate of the telescope's sidereal drive
        /// </summary>
        internal static DriveRates TrackingRate
        {
            get
            {
                const DriveRates DEFAULT_DRIVERATE = DriveRates.driveSidereal;
                LogMessage("TrackingRate Get", $"{DEFAULT_DRIVERATE}");
                return DEFAULT_DRIVERATE;
            }
            set
            {
                ThrowPropertyNotImplemented("TrackingRate", true);
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
                LogMessage("TrackingRates", MsgGetPrefix);
                foreach (DriveRates driveRate in trackingRates)
                {
                    LogMessage("TrackingRates", MsgGetPrefix + driveRate.ToString());
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
                if (UpdateGXASState() && _gxasState.Valid)
                    return _gxasState.GetUTCDate();
                if (!_connection.GetUTCTimestamp(out double secs))
                    throw new DriverException("Get UTCDate has failed");
                return new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc).AddSeconds(secs);
            }
            set
            {
                long s = (long)(value - new DateTime(1970, 1, 1, 0, 0, 0)).TotalSeconds;
                if (!_connection.SetUTCTimestamp(s))
                    throw new InvalidOperationException("Set UTCDate has failed");
                ConnectionStatusDate = DateTime.UtcNow;
                LogMessage("Set UTCDate", MsgDone);
            }
        }

        /// <summary>
        /// Takes telescope out of the Parked state.
        /// </summary>
        internal static void Unpark()
        {
            if (!_connection.Unpark())
                throw new InvalidOperationException("Unpark has failed");
            ConnectionStatusDate = DateTime.UtcNow;
            LogMessage("Unpark", MsgDone);
        }

        #endregion

        #region Helper methods for slew/sync
        private static void DegtoDMS(double degf, ref int degi, ref int mini, ref int seci)
        {
            int tts = (int)Math.Round(Math.Floor(degf * 3600d + 0.5d));
            degi = tts / 3600;
            mini = (tts - degi * 3600) / 60;
            seci = tts % 60;
        }

        private static string DegtoDDDMMSS(double value)
        {
            int d = 0, m = 0, s = 0;
            DegtoDMS(value, ref d, ref m, ref s);
            return d.ToString("000") + ":" + m.ToString("00") + ":" + s.ToString("00", CultureInfo.InvariantCulture);
        }

        private static string DegtoDDMMSS(double value)
        {
            int d = 0, m = 0, s = 0;
            DegtoDMS(value, ref d, ref m, ref s);
            return d.ToString("00") + ":" + m.ToString("00") + ":" + s.ToString("00", CultureInfo.InvariantCulture);
        }

        private static string DecToString(double value)
        {
            if (value < -90 || value > 90)
                throw new InvalidValueException("DecToString", value.ToString(), "-90 to 90");
            string sexa = utilities.DegreesToDMS(value, DmsSeparator, DmsSeparator, "");
            if (!sexa.StartsWith("-"))
                sexa = "+" + sexa;
            return sexa;
        }

        private static string RaToString(double value)
        {
            if (value < 0 || value > 24)
                throw new InvalidValueException("RaToString", value.ToString(), "0 to 24");
            return utilities.HoursToHMS(value, DmsSeparator, DmsSeparator, "");
        }

        private static string AltToString(double value)
        {
            if (value < -30 || value > 90)
                throw new InvalidValueException("AltToString", value.ToString(), "-30 to 90");
            string sexa = utilities.DegreesToDMS(value, DmsSeparator, DmsSeparator, "");
            if (!sexa.StartsWith("-"))
                sexa = "+" + sexa;
            return sexa;
        }

        private static string AzToString(double value)
        {
            if (value < 0 || value > 360)
                throw new InvalidValueException("AzToString", value.ToString(), "0 to 360");
            return DegtoDDDMMSS(value);
        }

        private static void SetAzAlt(double Azimuth, double Altitude)
        {
            if (!_connection.SetTargetAz(AzToString(Azimuth)))
                throw new InvalidOperationException("Set azimuth failed");
            if (!_connection.SetTargetAlt(AltToString(Altitude)))
                throw new InvalidOperationException("Set altitude failed");
        }

        private static void CheckSlewRADEC()
        {
            if (tgtDec == -999 || tgtRa == -999)
                throw new ValueNotSetException();
            if (AtPark)
                throw new ParkedException();
            if (!Tracking)
                throw new InvalidOperationException("Tracking must be on to slew");
            while (Slewing) { AbortSlew(); System.Threading.Thread.Sleep(200); }
            while (IsPulseGuiding) { AbortSlew(); System.Threading.Thread.Sleep(200); }
        }

        private static void CheckSlewAltAz()
        {
            if (AtPark)
                throw new ParkedException();
            while (Slewing) { AbortSlew(); System.Threading.Thread.Sleep(100); }
        }

        private static void DoSlew(bool async, bool altAz)
        {
            var buf = new byte[8];
            bool ok = altAz
                ? _connection.SlewToAltAz(buf, buf.Length)
                : _connection.SlewToEquatorial(buf, buf.Length);
            if (!ok)
                throw new DriverException("Telescope is not replying");
            int len = Array.IndexOf(buf, (byte)0);
            if (len < 0) len = buf.Length;
            string state = System.Text.Encoding.ASCII.GetString(buf, 0, len);
            if (string.IsNullOrEmpty(state))
                throw new DriverException("Telescope reply is corrupt");
            if (state.Length >= 1 && state[0] == '0')
            {
                ConnectionStatusDate = DateTime.UtcNow;
                LogMessage("Slew", "Started");
                if (!async)
                {
                    while (Slewing)
                        System.Threading.Thread.Sleep(1000);
                }
            }
            else if (state.Length >= 1)
            {
                ReportState(state);
            }
        }

        private static void ReportState(string state)
        {
            int val = (int)state[0] - (int)'0';
            switch (val)
            {
                case 1: throw new InvalidOperationException("Object below min altitude");
                case 2: throw new ValueNotSetException();
                case 3: throw new InvalidOperationException("Mount Cannot Flip here");
                case 4: throw new ParkedException();
                case 5: throw new InvalidOperationException("Telescope is Slewing");
                case 6: throw new InvalidOperationException("Object is outside limits");
                case 7: throw new InvalidOperationException("Telescope is Guiding");
                case 8: throw new InvalidOperationException("Object above max altitude");
                case 9:
                case 11: throw new InvalidOperationException("Motor is fault");
                case 12: throw new InvalidOperationException("Telescope is below horizon limit");
                case 13: throw new InvalidOperationException("Limit Sensor");
                case 14: throw new InvalidOperationException("Telescope is outside Axis 1 limit");
                case 15: throw new InvalidOperationException("Telescope is outside Axis 2 limit");
                case 16: throw new InvalidOperationException("Telescope is above overhead limit");
                case 17: throw new InvalidOperationException("Telescope is outside meridian limit");
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
                // TODO check that the driver hardware connection exists and is connected to the hardware
                return connectedState;
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
                Port = Convert.ToInt16(driverProfile.GetValue(DriverProgId, PortProfileName, string.Empty, PortDefault));
                Interface = driverProfile.GetValue(DriverProgId, InterfaceProfileName, string.Empty, InterfaceDefault);
            }
        }

        /// <summary>
        /// Write the device configuration to the ASCOM Profile store
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

        private static void ThrowPropertyNotImplemented(string propertyName, bool isWrite)
        {
            string context = isWrite ? " Set" : " Get";
            LogMessage(propertyName + context, MsgNotImplemented);
            throw new PropertyNotImplementedException(propertyName, isWrite);
        }

        private static void ThrowMethodNotImplemented(string methodName)
        {
            LogMessage(methodName, MsgNotImplemented);
            throw new MethodNotImplementedException(methodName);
        }
        #endregion
    }
}


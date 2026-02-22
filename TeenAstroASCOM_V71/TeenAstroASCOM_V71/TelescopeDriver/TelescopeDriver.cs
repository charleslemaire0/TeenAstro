// TODO fill in this information for your driver, then remove this line!
//
// ASCOM Telescope driver for TeenAstro
//
// Description:	 <To be completed by driver developer>
//
// Implements:	ASCOM Telescope interface version: <To be completed by driver developer>
// Author:		(XXX) Your N. Here <your@email.here>
//

using ASCOM;
using ASCOM.DeviceInterface;
using ASCOM.LocalServer;
using ASCOM.Utilities;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace ASCOM.TeenAstro.Telescope
{
    //
    // This code is mostly a presentation layer for the functionality in the TelescopeHardware class. You should not need to change the contents of this file very much, if at all.
    // Most customisation will be in the TelescopeHardware class, which is shared by all instances of the driver, and which must handle all aspects of communicating with your device.
    //
    // Your driver's DeviceID is ASCOM.TeenAstro.Telescope
    //
    // The COM Guid attribute sets the CLSID for ASCOM.TeenAstro.Telescope
    // The COM ClassInterface/None attribute prevents an empty interface called _TeenAstro from being created and used as the [default] interface
    //

    /// <summary>
    /// ASCOM Telescope Driver for TeenAstro.
    /// </summary>
    [ComVisible(true)]
    [Guid("1f594a69-71cb-42af-ad6c-1626922c9c88")]
    [ProgId("ASCOM.TeenAstro.Telescope")]
    [ServedClassName("ASCOM Telescope Driver for TeenAstro")] // Driver description that appears in the Chooser, customise as required
    [ClassInterface(ClassInterfaceType.None)]
    public class Telescope : ReferenceCountedObjectBase, ITelescopeV4, IDisposable
    {
        internal static string DriverProgId; // ASCOM DeviceID (COM ProgID) for this driver, the value is retrieved from the ServedClassName attribute in the class initialiser.
        internal static string DriverDescription; // The value is retrieved from the ServedClassName attribute in the class initialiser.

        // connectedState and connectingState holds the states from this driver instance's perspective, as opposed to the local server's perspective, which may be different because of other client connections.
        internal bool connectedState; // The connected state from this driver's perspective)
        internal bool connectingState; // The connecting state from this driver's perspective)
        internal Exception connectionException = null; // Record any exception thrown if the driver encounters an error when connecting to the hardware using Connect() or Disconnect

        internal TraceLogger tl; // Trace logger object to hold diagnostic information just for this instance of the driver, as opposed to the local server's log, which includes activity from all driver instances.
        private bool disposedValue;

        private Guid uniqueId; // A unique ID for this instance of the driver

        #region Initialisation and Dispose

        /// <summary>
        /// Initializes a new instance of the <see cref="TeenAstro"/> class. Must be public to successfully register for COM.
        /// </summary>
        public Telescope()
        {
            try
            {
                // Pull the ProgID from the ProgID class attribute.
                Attribute attr = Attribute.GetCustomAttribute(this.GetType(), typeof(ProgIdAttribute));
                DriverProgId = ((ProgIdAttribute)attr).Value ?? "PROGID NOT SET!";  // Get the driver ProgIDfrom the ProgID attribute.

                // Pull the display name from the ServedClassName class attribute.
                attr = Attribute.GetCustomAttribute(this.GetType(), typeof(ServedClassNameAttribute));
                DriverDescription = ((ServedClassNameAttribute)attr).DisplayName ?? "DISPLAY NAME NOT SET!";  // Get the driver description that displays in the ASCOM Chooser from the ServedClassName attribute.

                // LOGGING CONFIGURATION
                // By default all driver logging will appear in Hardware log file
                // If you would like each instance of the driver to have its own log file as well, uncomment the lines below

                tl = new TraceLogger("", "TeenAstro.Driver"); // Remove the leading ASCOM. from the ProgId because this will be added back by TraceLogger.
                SetTraceState();

                // Initialise the hardware if required
                TelescopeHardware.InitialiseHardware();

                LogMessage("Telescope", "Starting driver initialisation");
                LogMessage("Telescope", $"ProgID: {DriverProgId}, Description: {DriverDescription}");

                connectedState = false; // Initialise connected to false

                // Create a unique ID to identify this driver instance
                uniqueId = Guid.NewGuid();

                LogMessage("Telescope", "Completed initialisation");
            }
            catch (Exception ex)
            {
                LogMessage("Telescope", $"Initialisation exception: {ex}");
                MessageBox.Show($"{ex.Message}", "Exception creating ASCOM.TeenAstro.Telescope", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        /// <summary>
        /// Class destructor called automatically by the .NET runtime when the object is finalised in order to release resources that are NOT managed by the .NET runtime.
        /// </summary>
        /// <remarks>See the Dispose(bool disposing) remarks for further information.</remarks>
        ~Telescope()
        {
            // Please do not change this code.
            // The Dispose(false) method is called here just to release unmanaged resources. Managed resources will be dealt with automatically by the .NET runtime.

            Dispose(false);
        }

        /// <summary>
        /// Deterministically dispose of any managed and unmanaged resources used in this instance of the driver.
        /// </summary>
        /// <remarks>
        /// Do not dispose of items in this method, put clean-up code in the 'Dispose(bool disposing)' method instead.
        /// </remarks>
        public void Dispose()
        {
            // Please do not change the code in this method.

            // Release resources now.
            Dispose(disposing: true);

            // Do not add GC.SuppressFinalize(this); here because it breaks the ReferenceCountedObjectBase COM connection counting mechanic
        }

        /// <summary>
        /// Dispose of large or scarce resources created or used within this driver file
        /// </summary>
        /// <remarks>
        /// The purpose of this method is to enable you to release finite system resources back to the operating system as soon as possible, so that other applications work as effectively as possible.
        ///
        /// NOTES
        /// 1) Do not call the TelescopeHardware.Dispose() method from this method. Any resources used in the static TelescopeHardware class itself, 
        ///    which is shared between all instances of the driver, should be released in the TelescopeHardware.Dispose() method as usual. 
        ///    The TelescopeHardware.Dispose() method will be called automatically by the local server just before it shuts down.
        /// 2) You do not need to release every .NET resource you use in your driver because the .NET runtime is very effective at reclaiming these resources. 
        /// 3) Strong candidates for release here are:
        ///     a) Objects that have a large memory footprint (> 1Mb) such as images
        ///     b) Objects that consume finite OS resources such as file handles, synchronisation object handles, memory allocations requested directly from the operating system (NativeMemory methods) etc.
        /// 4) Please ensure that you do not return exceptions from this method
        /// 5) Be aware that Dispose() can be called more than once:
        ///     a) By the client application
        ///     b) Automatically, by the .NET runtime during finalisation
        /// 6) Because of 5) above, you should make sure that your code is tolerant of multiple calls.    
        /// </remarks>
        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    try
                    {
                        // Dispose of managed objects here

                        // Clean up the trace logger object
                        if (!(tl is null))
                        {
                            tl.Enabled = false;
                            tl.Dispose();
                            tl = null;
                        }
                    }
                    catch (Exception)
                    {
                        // Any exception is not re-thrown because Microsoft's best practice says not to return exceptions from the Dispose method. 
                    }
                }

                try
                {
                    // Dispose of unmanaged objects, if any, here (OS handles etc.)
                }
                catch (Exception)
                {
                    // Any exception is not re-thrown because Microsoft's best practice says not to return exceptions from the Dispose method. 
                }

                // Flag that Dispose() has already run and disposed of all resources
                disposedValue = true;
            }
        }

        #endregion

        // PUBLIC COM INTERFACE ITelescopeV4 IMPLEMENTATION

        #region Common properties and methods.

        /// <summary>
        /// Displays the Setup Dialogue form.
        /// If the user clicks the OK button to dismiss the form, then
        /// the new settings are saved, otherwise the old values are reloaded.
        /// THIS IS THE ONLY PLACE WHERE SHOWING USER INTERFACE IS ALLOWED!
        /// </summary>
        public void SetupDialog()
        {
            try
            {
                if (connectedState) // Don't show if already connected
                {
                    MessageBox.Show("Already connected, just press OK");
                }
                else // Show dialogue
                {
                    LogMessage("SetupDialog", $"Calling SetupDialog.");
                    TelescopeHardware.SetupDialog();
                    LogMessage("SetupDialog", $"Completed.");
                }
            }
            catch (Exception ex)
            {
                LogMessage("SetupDialog", $"Threw an exception: \r\n{ex}");
                throw;
            }
        }

        /// <summary>Returns the list of custom action names supported by this driver.</summary>
        /// <value>An ArrayList of strings (SafeArray collection) containing the names of supported actions.</value>
        public ArrayList SupportedActions
        {
            get
            {
                try
                {
                    CheckConnected($"SupportedActions");
                    ArrayList actions = TelescopeHardware.SupportedActions;
                    LogMessage("SupportedActions", $"Returning {actions.Count} actions.");
                    return actions;
                }
                catch (Exception ex)
                {
                    LogMessage("SupportedActions", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>Invokes the specified device-specific custom action.</summary>
        /// <param name="ActionName">A well known name agreed by interested parties that represents the action to be carried out.</param>
        /// <param name="ActionParameters">List of required parameters or an <see cref="String.Empty">Empty String</see> if none are required.</param>
        /// <returns>A string response. The meaning of returned strings is set by the driver author.
        /// <para>Suppose filter wheels start to appear with automatic wheel changers; new actions could be <c>QueryWheels</c> and <c>SelectWheel</c>. The former returning a formatted list
        /// of wheel names and the second taking a wheel name and making the change, returning appropriate values to indicate success or failure.</para>
        /// </returns>
        public string Action(string actionName, string actionParameters)
        {
            try
            {
                CheckConnected($"Action {actionName} - {actionParameters}");
                LogMessage("", $"Calling Action: {actionName} with parameters: {actionParameters}");
                string actionResponse = TelescopeHardware.Action(actionName, actionParameters);
                LogMessage("Action", $"Completed.");
                return actionResponse;
            }
            catch (Exception ex)
            {
                LogMessage("Action", $"Threw an exception: \r\n{ex}");
                throw;
            }
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
        public void CommandBlind(string command, bool raw)
        {
            try
            {
                CheckConnected($"CommandBlind: {command}, Raw: {raw}");
                LogMessage("CommandBlind", $"Calling method - Command: {command}, Raw: {raw}");
                TelescopeHardware.CommandBlind(command, raw);
                LogMessage("CommandBlind", $"Completed.");
            }
            catch (Exception ex)
            {
                LogMessage("CommandBlind", $"Command: {command}, Raw: {raw} threw an exception: \r\n{ex}");
                throw;
            }
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
        public bool CommandBool(string command, bool raw)
        {
            try
            {
                CheckConnected($"CommandBool: {command}, Raw: {raw}");
                LogMessage("CommandBlind", $"Calling method - Command: {command}, Raw: {raw}");
                bool commandBoolResponse = TelescopeHardware.CommandBool(command, raw);
                LogMessage("CommandBlind", $"Returning: {commandBoolResponse}.");
                return commandBoolResponse;
            }
            catch (Exception ex)
            {
                LogMessage("CommandBool", $"Command: {command}, Raw: {raw} threw an exception: \r\n{ex}");
                throw;
            }
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
        public string CommandString(string command, bool raw)
        {
            try
            {
                CheckConnected($"CommandString: {command}, Raw: {raw}");
                LogMessage("CommandString", $"Calling method - Command: {command}, Raw: {raw}");
                string commandStringResponse = TelescopeHardware.CommandString(command, raw);
                LogMessage("CommandString", $"Returning: {commandStringResponse}.");
                return commandStringResponse;
            }
            catch (Exception ex)
            {
                LogMessage("CommandString", $"Command: {command}, Raw: {raw} threw an exception: \r\n{ex}");
                throw;
            }
        }

        /// <summary>
        /// Connect to the device asynchronously using Connecting as the completion variable
        /// </summary>
        public void Connect()
        {
            try
            {
                if (connectedState)
                {
                    LogMessage("Connect", "Device already connected, ignoring method");
                    return;
                }

                // Initialise connection variables
                connectionException = null; // Clear any previous exception
                connectingState = true;

                // Start a task to connect to the hardware and then set the connected state to true
                _ = Task.Run(() =>
                {
                    try
                    {
                        LogMessage("Connect Task", "Starting connection");
                        TelescopeHardware.SetConnected(uniqueId, true);
                        connectedState = true;
                        LogMessage("Connect Task", "Connection completed");
                    }
                    catch (Exception ex)
                    {
                        // Something went wrong so save the returned exception to return through Connecting and log the event.
                        connectionException = ex;
                        LogMessage("Connect Task", $"The connect task threw an exception: {ex.Message}\r\n{ex}");
                    }
                    finally
                    {
                        connectingState = false;
                    }
                });
            }
            catch (Exception ex)
            {
                LogMessage("Connect", $"Threw an exception: \r\n{ex}");
                throw;
            }
            LogMessage("Connect", $"Connect completed OK");
        }

        /// <summary>
        /// Set True to connect to the device hardware. Set False to disconnect from the device hardware.
        /// You can also read the property to check whether it is connected. This reports the current hardware state.
        /// </summary>
        /// <value><c>true</c> if connected to the hardware; otherwise, <c>false</c>.</value>
        public bool Connected
        {
            get
            {
                try
                {
                    // Returns the driver's connection state rather than the local server's connected state, which could be different because there may be other client connections still active.
                    LogMessage("Connected Get", connectedState.ToString());
                    return connectedState;
                }
                catch (Exception ex)
                {
                    LogMessage("Connected Get", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
            set
            {
                try
                {
                    if (value == connectedState)
                    {
                        LogMessage("Connected Set", "Device already connected, ignoring Connected Set = true");
                        return;
                    }

                    if (value)
                    {
                        LogMessage("Connected Set", "Connecting to device...");
                        TelescopeHardware.SetConnected(uniqueId, true);
                        LogMessage("Connected Set", "Connected OK");
                        connectedState = true;
                    }
                    else
                    {
                        connectedState = false;
                        LogMessage("Connected Set", "Disconnecting from device...");
                        TelescopeHardware.SetConnected(uniqueId, false);
                        LogMessage("Connected Set", "Disconnected OK");
                    }
                }
                catch (Exception ex)
                {
                    LogMessage("Connected Set", $"Threw an exception: {ex.Message}\r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// Completion variable for the asynchronous Connect() and Disconnect()  methods
        /// </summary>
        public bool Connecting
        {
            get
            {
                // Return any exception returned by the Connect() or Disconnect() methods
                if (!(connectionException is null))
                    throw connectionException;

                // Otherwise return the current connecting state
                return connectingState;
            }
        }

        /// <summary>
        /// Disconnect from the device asynchronously using Connecting as the completion variable
        /// </summary>
        public void Disconnect()
        {
            try
            {
                if (!connectedState)
                {
                    LogMessage("Disconnect", "Device already disconnected, ignoring method");
                    return;
                }

                // Initialise connection variables
                connectionException = null; // Clear any previous exception
                connectingState = true;

                // Start a task to connect to the hardware and then set the connected state to true
                _ = Task.Run(() =>
                {
                    try
                    {
                        LogMessage("Disconnect Task", "Calling Connected");
                        TelescopeHardware.SetConnected(uniqueId, false);
                        connectedState = false;
                        LogMessage("Disconnect Task", "Disconnection completed");
                    }
                    catch (Exception ex)
                    {
                        // Something went wrong so save the returned exception to return through Connecting and log the event.
                        connectionException = ex;
                        LogMessage("Disconnect Task", $"The disconnect task threw an exception: {ex.Message}\r\n{ex}");
                    }
                    finally
                    {
                        connectingState = false;
                    }
                });
            }
            catch (Exception ex)
            {
                LogMessage("Disconnect", $"Threw an exception: {ex.Message}\r\n{ex}");
                throw;
            }

            LogMessage("Disconnect", $"Disconnect completed OK");
        }

        /// <summary>
        /// Returns a description of the device, such as manufacturer and model number. Any ASCII characters may be used.
        /// </summary>
        /// <value>The description.</value>
        public string Description
        {
            get
            {
                try
                {
                    CheckConnected($"Description");
                    string description = TelescopeHardware.Description;
                    LogMessage("Description", description);
                    return description;
                }
                catch (Exception ex)
                {
                    LogMessage("Description", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// Descriptive and version information about this ASCOM driver.
        /// </summary>
        public string DriverInfo
        {
            get
            {
                try
                {
                    // This should work regardless of whether or not the driver is Connected, hence no CheckConnected method.
                    string driverInfo = TelescopeHardware.DriverInfo;
                    LogMessage("DriverInfo", driverInfo);
                    return driverInfo;
                }
                catch (Exception ex)
                {
                    LogMessage("DriverInfo", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// A string containing only the major and minor version of the driver formatted as 'm.n'.
        /// </summary>
        public string DriverVersion
        {
            get
            {
                try
                {
                    // This should work regardless of whether or not the driver is Connected, hence no CheckConnected method.
                    string driverVersion = TelescopeHardware.DriverVersion;
                    LogMessage("DriverVersion", driverVersion);
                    return driverVersion;
                }
                catch (Exception ex)
                {
                    LogMessage("DriverVersion", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// The interface version number that this device supports.
        /// </summary>
        public short InterfaceVersion
        {
            get
            {
                try
                {
                    // This should work regardless of whether or not the driver is Connected, hence no CheckConnected method.
                    short interfaceVersion = TelescopeHardware.InterfaceVersion;
                    LogMessage("InterfaceVersion", interfaceVersion.ToString());
                    return interfaceVersion;
                }
                catch (Exception ex)
                {
                    LogMessage("InterfaceVersion", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// The short name of the driver, for display purposes
        /// </summary>
        public string Name
        {
            get
            {
                try
                {
                    // This should work regardless of whether or not the driver is Connected, hence no CheckConnected method.
                    string name = TelescopeHardware.Name;
                    LogMessage("Name Get", name);
                    return name;
                }
                catch (Exception ex)
                {
                    LogMessage("Name", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        #endregion

        #region ITelescope Implementation

        /// <summary>
        /// Stops a slew in progress.
        /// </summary>
        public void AbortSlew()
        {
            try
            {
                CheckConnected("AbortSlew");
                LogMessage("AbortSlew", $"Calling method.");
                TelescopeHardware.AbortSlew();
                LogMessage("AbortSlew", $"Completed.");
            }
            catch (Exception ex)
            {
                LogMessage("AbortSlew", $"Threw an exception: \r\n{ex}");
                throw;
            }
        }

        /// <summary>
        /// The alignment mode of the mount (Alt/Az, Polar, German Polar).
        /// </summary>
        public AlignmentModes AlignmentMode
        {
            get
            {
                try
                {
                    CheckConnected("AlignmentMode");
                    AlignmentModes alignmentMode = TelescopeHardware.AlignmentMode;
                    LogMessage("AlignmentMode", alignmentMode.ToString());
                    return alignmentMode;
                }
                catch (Exception ex)
                {
                    LogMessage("AlignmentMode", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// The Altitude above the local horizon of the telescope's current position (degrees, positive up)
        /// </summary>
        public double Altitude
        {
            get
            {
                try
                {
                    CheckConnected("Altitude");
                    double altitude = TelescopeHardware.Altitude;
                    LogMessage("Altitude", altitude.ToString());
                    return altitude;
                }
                catch (Exception ex)
                {
                    LogMessage("Altitude", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// The area of the telescope's aperture, taking into account any obstructions (square meters)
        /// </summary>
        public double ApertureArea
        {
            get
            {
                try
                {
                    CheckConnected("ApertureArea");
                    double apertureArea = TelescopeHardware.ApertureArea;
                    LogMessage("ApertureArea", apertureArea.ToString());
                    return apertureArea;
                }
                catch (Exception ex)
                {
                    LogMessage("ApertureArea", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// The telescope's effective aperture diameter (meters)
        /// </summary>
        public double ApertureDiameter
        {
            get
            {
                try
                {
                    CheckConnected("ApertureDiameter");
                    double apertureArea = TelescopeHardware.ApertureDiameter;
                    LogMessage("ApertureDiameter", apertureArea.ToString());
                    return apertureArea;
                }
                catch (Exception ex)
                {
                    LogMessage("ApertureDiameter", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// True if the telescope is stopped in the Home position. Set only following a <see cref="FindHome"></see> operation,
        /// and reset with any slew operation. This property must be False if the telescope does not support homing.
        /// </summary>
        public bool AtHome
        {
            get
            {
                try
                {
                    CheckConnected("AtHome");
                    bool atHome = TelescopeHardware.AtHome;
                    LogMessage("AtHome", atHome.ToString());
                    return atHome;
                }
                catch (Exception ex)
                {
                    LogMessage("AtHome", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// True if the telescope has been put into the parked state by the <see cref="Park" /> method. Set False by calling the Unpark() method.
        /// </summary>
        public bool AtPark
        {
            get
            {
                try
                {
                    CheckConnected("AtPark");
                    bool atPark = TelescopeHardware.AtPark;
                    LogMessage("AtPark", atPark.ToString());
                    return atPark;
                }
                catch (Exception ex)
                {
                    LogMessage("AtPark", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// Determine the rates at which the telescope may be moved about the specified axis by the <see cref="MoveAxis" /> method.
        /// </summary>
        /// <param name="axis">The axis about which rate information is desired (TelescopeAxes value)</param>
        /// <returns>Collection of <see cref="IRate" /> rate objects</returns>
        public IAxisRates AxisRates(TelescopeAxes axis)
        {
            try
            {
                CheckConnected("AxisRates");
                IAxisRates axisRates = TelescopeHardware.AxisRates(axis);
                LogMessage("AxisRates", $"Axis: {axis} returned {axisRates.Count} rates.");
                return axisRates;
            }
            catch (Exception ex)
            {
                LogMessage("AxisRates", $"Threw an exception: \r\n{ex}");
                throw;
            }
        }

        /// <summary>
        /// The azimuth at the local horizon of the telescope's current position (degrees, North-referenced, positive East/clockwise).
        /// </summary>
        public double Azimuth
        {
            get
            {
                try
                {
                    CheckConnected("Azimuth");
                    double azimuth = TelescopeHardware.Azimuth;
                    LogMessage("Azimuth", azimuth.ToString());
                    return azimuth;
                }
                catch (Exception ex)
                {
                    LogMessage("Azimuth", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// True if this telescope is capable of programmed finding its home position (<see cref="FindHome" /> method).
        /// </summary>
        public bool CanFindHome
        {
            get
            {
                try
                {
                    CheckConnected("CanFindHome");
                    bool canFindHome = TelescopeHardware.CanFindHome;
                    LogMessage("CanFindHome", canFindHome.ToString());
                    return canFindHome;
                }
                catch (Exception ex)
                {
                    LogMessage("CanFindHome", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// True if this telescope can move the requested axis
        /// </summary>
        public bool CanMoveAxis(TelescopeAxes axis)
        {
            try
            {
                CheckConnected("CanMoveAxis");
                bool canMoveAxis = TelescopeHardware.CanMoveAxis(axis);
                LogMessage("CanMoveAxis", $"Axis: {axis} returned {canMoveAxis}.");
                return canMoveAxis;
            }
            catch (Exception ex)
            {
                LogMessage("CanMoveAxis", $"Threw an exception: \r\n{ex}");
                throw;
            }
        }

        /// <summary>
        /// True if this telescope is capable of programmed parking (<see cref="Park" />method)
        /// </summary>
        public bool CanPark
        {
            get
            {
                try
                {
                    CheckConnected("CanPark");
                    bool canPark = TelescopeHardware.CanPark;
                    LogMessage("CanPark", canPark.ToString());
                    return canPark;
                }
                catch (Exception ex)
                {
                    LogMessage("CanPark", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// True if this telescope is capable of software-pulsed guiding (via the <see cref="PulseGuide" /> method)
        /// </summary>
        public bool CanPulseGuide
        {
            get
            {
                try
                {
                    CheckConnected("CanPulseGuide");
                    bool canPulseGuide = TelescopeHardware.CanPulseGuide;
                    LogMessage("CanPulseGuide", canPulseGuide.ToString());
                    return canPulseGuide;
                }
                catch (Exception ex)
                {
                    LogMessage("CanPulseGuide", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// True if the <see cref="DeclinationRate" /> property can be changed to provide offset tracking in the declination axis.
        /// </summary>
        public bool CanSetDeclinationRate
        {
            get
            {
                try
                {
                    CheckConnected("CanSetDeclinationRate");
                    bool canSetDeclinationRate = TelescopeHardware.CanSetDeclinationRate;
                    LogMessage("CanSetDeclinationRate", canSetDeclinationRate.ToString());
                    return canSetDeclinationRate;
                }
                catch (Exception ex)
                {
                    LogMessage("CanSetDeclinationRate", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// True if the guide rate properties used for <see cref="PulseGuide" /> can be adjusted.
        /// </summary>
        public bool CanSetGuideRates
        {
            get
            {
                try
                {
                    CheckConnected("CanSetGuideRates");
                    bool canSetGuideRates = TelescopeHardware.CanSetGuideRates;
                    LogMessage("CanSetGuideRates", canSetGuideRates.ToString());
                    return canSetGuideRates;
                }
                catch (Exception ex)
                {
                    LogMessage("CanSetGuideRates", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// True if this telescope is capable of programmed setting of its park position (<see cref="SetPark" /> method)
        /// </summary>
        public bool CanSetPark
        {
            get
            {
                try
                {
                    CheckConnected("CanSetPark");
                    bool canSetPark = TelescopeHardware.CanSetPark;
                    LogMessage("CanSetPark", canSetPark.ToString());
                    return canSetPark;
                }
                catch (Exception ex)
                {
                    LogMessage("CanSetPark", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// True if the <see cref="SideOfPier" /> property can be set, meaning that the mount can be forced to flip.
        /// </summary>
        public bool CanSetPierSide
        {
            get
            {
                try
                {
                    CheckConnected("CanSetPierSide");
                    bool canSetPierSide = TelescopeHardware.CanSetPierSide;
                    LogMessage("CanSetPierSide", canSetPierSide.ToString());
                    return canSetPierSide;
                }
                catch (Exception ex)
                {
                    LogMessage("CanSetPierSide", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// True if the <see cref="RightAscensionRate" /> property can be changed to provide offset tracking in the right ascension axis.
        /// </summary>
        public bool CanSetRightAscensionRate
        {
            get
            {
                try
                {
                    CheckConnected("CanSetRightAscensionRate");
                    bool canSetRightAscensionRate = TelescopeHardware.CanSetRightAscensionRate;
                    LogMessage("CanSetRightAscensionRate", canSetRightAscensionRate.ToString());
                    return canSetRightAscensionRate;
                }
                catch (Exception ex)
                {
                    LogMessage("CanSetRightAscensionRate", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// True if the <see cref="Tracking" /> property can be changed, turning telescope sidereal tracking on and off.
        /// </summary>
        public bool CanSetTracking
        {
            get
            {
                try
                {
                    CheckConnected("CanSetTracking");
                    bool canSetTracking = TelescopeHardware.CanSetTracking;
                    LogMessage("CanSetTracking", canSetTracking.ToString());
                    return canSetTracking;
                }
                catch (Exception ex)
                {
                    LogMessage("CanSetTracking", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// True if this telescope is capable of programmed slewing (synchronous or asynchronous) to equatorial coordinates
        /// </summary>
        public bool CanSlew
        {
            get
            {
                try
                {
                    CheckConnected("CanSlew");
                    bool canSlew = TelescopeHardware.CanSlew;
                    LogMessage("CanSlew", canSlew.ToString());
                    return canSlew;
                }
                catch (Exception ex)
                {
                    LogMessage("CanSlew", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// True if this telescope is capable of programmed slewing (synchronous or asynchronous) to local horizontal coordinates
        /// </summary>
        public bool CanSlewAltAz
        {
            get
            {
                try
                {
                    CheckConnected("CanSlewAltAz");
                    bool canSlewAltAz = TelescopeHardware.CanSlewAltAz;
                    LogMessage("CanSlewAltAz", canSlewAltAz.ToString());
                    return canSlewAltAz;
                }
                catch (Exception ex)
                {
                    LogMessage("CanSlewAltAz", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// True if this telescope is capable of programmed asynchronous slewing to local horizontal coordinates
        /// </summary>
        public bool CanSlewAltAzAsync
        {
            get
            {
                try
                {
                    CheckConnected("CanSlewAltAzAsync");
                    bool canSlewAltAzAsync = TelescopeHardware.CanSlewAltAzAsync;
                    LogMessage("CanSlewAltAzAsync", canSlewAltAzAsync.ToString());
                    return canSlewAltAzAsync;
                }
                catch (Exception ex)
                {
                    LogMessage("CanSlewAltAzAsync", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// True if this telescope is capable of programmed asynchronous slewing to equatorial coordinates.
        /// </summary>
        public bool CanSlewAsync
        {
            get
            {
                try
                {
                    CheckConnected("CanSlewAsync");
                    bool canSlewAsync = TelescopeHardware.CanSlewAsync;
                    LogMessage("CanSlewAsync", canSlewAsync.ToString());
                    return canSlewAsync;
                }
                catch (Exception ex)
                {
                    LogMessage("CanSlewAsync", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// True if this telescope is capable of programmed syncing to equatorial coordinates.
        /// </summary>
        public bool CanSync
        {
            get
            {
                try
                {
                    CheckConnected("CanSync");
                    bool canSync = TelescopeHardware.CanSync;
                    LogMessage("CanSync", canSync.ToString());
                    return canSync;
                }
                catch (Exception ex)
                {
                    LogMessage("CanSync", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// True if this telescope is capable of programmed syncing to local horizontal coordinates
        /// </summary>
        public bool CanSyncAltAz
        {
            get
            {
                try
                {
                    CheckConnected("CanSyncAltAz");
                    bool canSyncAltAz = TelescopeHardware.CanSyncAltAz;
                    LogMessage("CanSyncAltAz", canSyncAltAz.ToString());
                    return canSyncAltAz;
                }
                catch (Exception ex)
                {
                    LogMessage("CanSyncAltAz", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// True if this telescope is capable of programmed unparking (<see cref="Unpark" /> method).
        /// </summary>
        public bool CanUnpark
        {
            get
            {
                try
                {
                    CheckConnected("CanUnpark");
                    bool canUnpark = TelescopeHardware.CanUnpark;
                    LogMessage("CanUnpark", canUnpark.ToString());
                    return canUnpark;
                }
                catch (Exception ex)
                {
                    LogMessage("CanUnpark", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// The declination (degrees) of the telescope's current equatorial coordinates, in the coordinate system given by the <see cref="EquatorialSystem" /> property.
        /// Reading the property will raise an error if the value is unavailable.
        /// </summary>
        public double Declination
        {
            get
            {
                try
                {
                    CheckConnected("Declination");
                    double declination = TelescopeHardware.Declination;
                    LogMessage("Declination", declination.ToString());
                    return declination;
                }
                catch (Exception ex)
                {
                    LogMessage("Declination", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// The declination tracking rate (arc-seconds per SI second, default = 0.0)
        /// </summary>
        public double DeclinationRate
        {
            get
            {
                try
                {
                    CheckConnected("DeclinationRate Get");
                    double declinationRate = TelescopeHardware.DeclinationRate;
                    LogMessage("DeclinationRate Get", declinationRate.ToString());
                    return declinationRate;
                }
                catch (Exception ex)
                {
                    LogMessage("DeclinationRate Get", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
            set
            {
                try
                {
                    CheckConnected("DeclinationRate Set");
                    LogMessage("DeclinationRate Set", value.ToString());
                    TelescopeHardware.DeclinationRate = value;
                }
                catch (Exception ex)
                {
                    LogMessage("DeclinationRate Set", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// Predict side of pier for German equatorial mounts at the provided coordinates
        /// </summary>
        public PierSide DestinationSideOfPier(double rightAscension, double declination)
        {
            try
            {
                CheckConnected("DestinationSideOfPier");
                PierSide destinationSideOfPier = TelescopeHardware.DestinationSideOfPier(rightAscension, declination);
                LogMessage("DestinationSideOfPier", $"RA: {rightAscension}, Dec: {declination} - {destinationSideOfPier}.");
                return destinationSideOfPier;
            }
            catch (Exception ex)
            {
                LogMessage("DestinationSideOfPier", $"Threw an exception: \r\n{ex}");
                throw;
            }
        }

        /// <summary>
        /// Return the device's operational state in one call
        /// </summary>
        public IStateValueCollection DeviceState
        {
            get
            {
                try
                {
                    CheckConnected("DeviceState");

                    // Create an array list to hold the IStateValue entries
                    List<IStateValue> deviceState = new List<IStateValue>();

                    // Add one entry for each operational state, if possible
                    try { deviceState.Add(new StateValue(nameof(ITelescopeV4.Altitude), Altitude)); } catch { }
                    try { deviceState.Add(new StateValue(nameof(ITelescopeV4.AtHome), AtHome)); } catch { }
                    try { deviceState.Add(new StateValue(nameof(ITelescopeV4.AtPark), AtPark)); } catch { }
                    try { deviceState.Add(new StateValue(nameof(ITelescopeV4.Azimuth), Azimuth)); } catch { }
                    try { deviceState.Add(new StateValue(nameof(ITelescopeV4.Declination), Declination)); } catch { }
                    try { deviceState.Add(new StateValue(nameof(ITelescopeV4.IsPulseGuiding), IsPulseGuiding)); } catch { }
                    try { deviceState.Add(new StateValue(nameof(ITelescopeV4.RightAscension), RightAscension)); } catch { }
                    try { deviceState.Add(new StateValue(nameof(ITelescopeV4.SideOfPier), SideOfPier)); } catch { }
                    try { deviceState.Add(new StateValue(nameof(ITelescopeV4.SiderealTime), SiderealTime)); } catch { }
                    try { deviceState.Add(new StateValue(nameof(ITelescopeV4.Slewing), Slewing)); } catch { }
                    try { deviceState.Add(new StateValue(nameof(ITelescopeV4.Tracking), Tracking)); } catch { }
                    try { deviceState.Add(new StateValue(nameof(ITelescopeV4.UTCDate), UTCDate)); } catch { }
                    try { deviceState.Add(new StateValue(DateTime.Now)); } catch { }

                    // Return the overall device state
                    return new StateValueCollection(deviceState);
                }
                catch (Exception ex)
                {
                    LogMessage("DeviceState", $"Threw an exception: {ex.Message}\r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// True if the telescope or driver applies atmospheric refraction to coordinates.
        /// </summary>
        public bool DoesRefraction
        {
            get
            {
                try
                {
                    CheckConnected("DoesRefraction Get");
                    bool doesRefraction = TelescopeHardware.DoesRefraction;
                    LogMessage("DoesRefraction Get", doesRefraction.ToString());
                    return doesRefraction;
                }
                catch (Exception ex)
                {
                    LogMessage("DoesRefraction Get", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
            set
            {
                try
                {
                    CheckConnected("DoesRefraction Get");
                    LogMessage("DoesRefraction Set", value.ToString());
                    TelescopeHardware.DoesRefraction = value;
                }
                catch (Exception ex)
                {
                    LogMessage("DoesRefraction Set", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// Equatorial coordinate system used by this telescope (e.g. Topocentric or J2000).
        /// </summary>
        public EquatorialCoordinateType EquatorialSystem
        {
            get
            {
                try
                {
                    CheckConnected("EquatorialSystem");
                    EquatorialCoordinateType equatorialSystem = TelescopeHardware.EquatorialSystem;
                    LogMessage("EquatorialSystem", equatorialSystem.ToString());
                    return equatorialSystem;
                }
                catch (Exception ex)
                {
                    LogMessage("EquatorialSystem", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// Locates the telescope's "home" position (synchronous)
        /// </summary>
        public void FindHome()
        {
            try
            {
                CheckConnected("FindHome");
                LogMessage("FindHome", $"Calling method.");
                TelescopeHardware.FindHome();
                LogMessage("FindHome", $"Completed.");
            }
            catch (Exception ex)
            {
                LogMessage("FindHome", $"Threw an exception: \r\n{ex}");
                throw;
            }
        }

        /// <summary>
        /// The telescope's focal length, meters
        /// </summary>
        public double FocalLength
        {
            get
            {
                try
                {
                    CheckConnected("FocalLength");
                    double focalLength = TelescopeHardware.FocalLength;
                    LogMessage("FocalLength", focalLength.ToString());
                    return focalLength;
                }
                catch (Exception ex)
                {
                    LogMessage("FocalLength", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// The current Declination movement rate offset for telescope guiding (degrees/sec)
        /// </summary>
        public double GuideRateDeclination
        {
            get
            {
                try
                {
                    CheckConnected("GuideRateDeclination Get");
                    double guideRateDeclination = TelescopeHardware.GuideRateDeclination;
                    LogMessage("GuideRateDeclination Get", guideRateDeclination.ToString());
                    return guideRateDeclination;
                }
                catch (Exception ex)
                {
                    LogMessage("GuideRateDeclination Get", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
            set
            {
                try
                {
                    CheckConnected("GuideRateDeclination Set");
                    LogMessage("GuideRateDeclination Set", value.ToString());
                    TelescopeHardware.GuideRateDeclination = value;
                }
                catch (Exception ex)
                {
                    LogMessage("GuideRateDeclination Set", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// The current Right Ascension movement rate offset for telescope guiding (degrees/sec)
        /// </summary>
        public double GuideRateRightAscension
        {
            get
            {
                try
                {
                    CheckConnected("GuideRateRightAscension Get");
                    double guideRateRightAscension = TelescopeHardware.GuideRateRightAscension;
                    LogMessage("GuideRateRightAscension Get", guideRateRightAscension.ToString());
                    return guideRateRightAscension;
                }
                catch (Exception ex)
                {
                    LogMessage("GuideRateRightAscension Get", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
            set
            {
                try
                {
                    CheckConnected("GuideRateRightAscension Set");
                    LogMessage("GuideRateRightAscension Set", value.ToString());
                    TelescopeHardware.GuideRateRightAscension = value;
                }
                catch (Exception ex)
                {
                    LogMessage("GuideRateRightAscension Set", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// True if a <see cref="PulseGuide" /> command is in progress, False otherwise
        /// </summary>
        public bool IsPulseGuiding
        {
            get
            {
                try
                {
                    CheckConnected("IsPulseGuiding");
                    bool isPulseGuiding = TelescopeHardware.IsPulseGuiding;
                    LogMessage("IsPulseGuiding", isPulseGuiding.ToString());
                    return isPulseGuiding;
                }
                catch (Exception ex)
                {
                    LogMessage("IsPulseGuiding", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// Move the telescope in one axis at the given rate.
        /// </summary>
        /// <param name="axis">The physical axis about which movement is desired</param>
        /// <param name="rate">The rate of motion (deg/sec) about the specified axis</param>
        public void MoveAxis(TelescopeAxes axis, double rate)
        {
            try
            {
                CheckConnected("MoveAxis");
                LogMessage("MoveAxis", $"Calling method - Axis: {axis}, Rate: {rate}.");
                TelescopeHardware.MoveAxis(axis, rate);
                LogMessage("MoveAxis", $"Completed.");
            }
            catch (Exception ex)
            {
                LogMessage("MoveAxis", $"Threw an exception: \r\n{ex}");
                throw;
            }
        }


        /// <summary>
        /// Move the telescope to its park position, stop all motion (or restrict to a small safe range), and set <see cref="AtPark" /> to True.
        /// </summary>
        public void Park()
        {
            try
            {
                CheckConnected("Park");
                LogMessage("Park", $"Calling method.");
                TelescopeHardware.Park();
                LogMessage("Park", $"Completed.");
            }
            catch (Exception ex)
            {
                LogMessage("Park", $"Threw an exception: \r\n{ex}");
                throw;
            }
        }

        /// <summary>
        /// Moves the scope in the given direction for the given interval or time at
        /// the rate given by the corresponding guide rate property
        /// </summary>
        /// <param name="direction">The direction in which the guide-rate motion is to be made</param>
        /// <param name="duration">The duration of the guide-rate motion (milliseconds)</param>
        public void PulseGuide(GuideDirections direction, int duration)
        {
            try
            {
                CheckConnected("PulseGuide");
                LogMessage("PulseGuide", $"Calling method.");
                TelescopeHardware.PulseGuide(direction, duration);
                LogMessage("PulseGuide", $"Completed.");
            }
            catch (Exception ex)
            {
                LogMessage("PulseGuide", $"Threw an exception: \r\n{ex}");
                throw;
            }
        }

        /// <summary>
        /// The right ascension (hours) of the telescope's current equatorial coordinates,
        /// in the coordinate system given by the EquatorialSystem property
        /// </summary>
        public double RightAscension
        {
            get
            {
                try
                {
                    CheckConnected("RightAscension");
                    double rightAscension = TelescopeHardware.RightAscension;
                    LogMessage("RightAscension", rightAscension.ToString());
                    return rightAscension;
                }
                catch (Exception ex)
                {
                    LogMessage("RightAscension", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// The right ascension tracking rate offset from sidereal (seconds per sidereal second, default = 0.0)
        /// </summary>
        public double RightAscensionRate
        {
            get
            {
                try
                {
                    CheckConnected("RightAscensionRate Get");
                    double rightAscensionRate = TelescopeHardware.RightAscensionRate;
                    LogMessage("RightAscensionRate Get", rightAscensionRate.ToString());
                    return rightAscensionRate;
                }
                catch (Exception ex)
                {
                    LogMessage("RightAscensionRate Get", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
            set
            {
                try
                {
                    CheckConnected("RightAscensionRate Set");
                    LogMessage("RightAscensionRate Set", value.ToString());
                    TelescopeHardware.RightAscensionRate = value;
                }
                catch (Exception ex)
                {
                    LogMessage("RightAscensionRate Set", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// Sets the telescope's park position to be its current position.
        /// </summary>
        public void SetPark()
        {
            try
            {
                CheckConnected("SetPark");
                LogMessage("SetPark", $"Calling method.");
                TelescopeHardware.SetPark();
                LogMessage("SetPark", $"Completed.");
            }
            catch (Exception ex)
            {
                LogMessage("SetPark", $"Threw an exception: \r\n{ex}");
                throw;
            }
        }

        /// <summary>
        /// Indicates the pointing state of the mount. Read the articles installed with the ASCOM Developer
        /// Components for more detailed information.
        /// </summary>
        public PierSide SideOfPier
        {
            get
            {
                try
                {
                    CheckConnected("SideOfPier Get");
                    PierSide sideOfPier = TelescopeHardware.SideOfPier;
                    LogMessage("SideOfPier Get", sideOfPier.ToString());
                    return sideOfPier;
                }
                catch (Exception ex)
                {
                    LogMessage("SideOfPier Get", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
            set
            {
                try
                {
                    CheckConnected("SideOfPier Set");
                    LogMessage("SideOfPier Set", value.ToString());
                    TelescopeHardware.SideOfPier = value;
                }
                catch (Exception ex)
                {
                    LogMessage("SideOfPier Set", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// The local apparent sidereal time from the telescope's internal clock (hours, sidereal)
        /// </summary>
        public double SiderealTime
        {
            get
            {
                try
                {
                    CheckConnected("SiderealTime");
                    double siderealTime = TelescopeHardware.SiderealTime;
                    LogMessage("SiderealTime", siderealTime.ToString());
                    return siderealTime;
                }
                catch (Exception ex)
                {
                    LogMessage("SiderealTime", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// The elevation above mean sea level (meters) of the site at which the telescope is located
        /// </summary>
        public double SiteElevation
        {
            get
            {
                try
                {
                    CheckConnected("SiteElevation Get");
                    double siteElevation = TelescopeHardware.SiteElevation;
                    LogMessage("SiteElevation Get", siteElevation.ToString());
                    return siteElevation;
                }
                catch (Exception ex)
                {
                    LogMessage("SiteElevation Get", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
            set
            {
                try
                {
                    CheckConnected("SiteElevation Set");
                    LogMessage("SiteElevation Set", value.ToString());
                    TelescopeHardware.SiteElevation = value;
                }
                catch (Exception ex)
                {
                    LogMessage("SiteElevation Set", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// The geodetic(map) latitude (degrees, positive North, WGS84) of the site at which the telescope is located.
        /// </summary>
        public double SiteLatitude
        {
            get
            {
                try
                {
                    CheckConnected("SiteLatitude Get");
                    double siteLatitude = TelescopeHardware.SiteLatitude;
                    LogMessage("SiteLatitude Get", siteLatitude.ToString());
                    return siteLatitude;
                }
                catch (Exception ex)
                {
                    LogMessage("SiteLatitude Get", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
            set
            {
                try
                {
                    CheckConnected("SiteLatitude Set");
                    LogMessage("SiteLatitude Set", value.ToString());
                    TelescopeHardware.SiteLatitude = value;
                }
                catch (Exception ex)
                {
                    LogMessage("SiteLatitude Set", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// The longitude (degrees, positive East, WGS84) of the site at which the telescope is located.
        /// </summary>
        public double SiteLongitude
        {
            get
            {
                try
                {
                    CheckConnected("SiteLongitude Get");
                    double siteLongitude = TelescopeHardware.SiteLongitude;
                    LogMessage("SiteLongitude Get", siteLongitude.ToString());
                    return siteLongitude;
                }
                catch (Exception ex)
                {
                    LogMessage("SiteLongitude Get", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
            set
            {
                try
                {
                    CheckConnected("SiteLongitude Set");
                    LogMessage("SiteLongitude Set", value.ToString());
                    TelescopeHardware.SiteLongitude = value;
                }
                catch (Exception ex)
                {
                    LogMessage("SiteLongitude Set", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// Specifies a post-slew settling time (sec.).
        /// </summary>
        public short SlewSettleTime
        {
            get
            {
                try
                {
                    CheckConnected("SlewSettleTime Get");
                    short slewSettleTime = TelescopeHardware.SlewSettleTime;
                    LogMessage("SlewSettleTime Get", slewSettleTime.ToString());
                    return slewSettleTime;
                }
                catch (Exception ex)
                {
                    LogMessage("SlewSettleTime Get", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
            set
            {
                try
                {
                    CheckConnected("SlewSettleTime Set");
                    LogMessage("SlewSettleTime Set", value.ToString());
                    TelescopeHardware.SlewSettleTime = value;
                }
                catch (Exception ex)
                {
                    LogMessage("SlewSettleTime Set", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// Move the telescope to the given local horizontal coordinates
        /// This method must be implemented if <see cref="CanSlewAltAz" /> returns True.
        /// It does not return until the slew is complete.
        /// </summary>
        public void SlewToAltAz(double azimuth, double altitude)
        {
            try
            {
                CheckConnected("SlewToAltAz");
                LogMessage("SlewToAltAz", $"Calling method - Azimuth: {azimuth}, Altitude: {altitude}.");
                TelescopeHardware.SlewToAltAz(azimuth, altitude);
                LogMessage("SlewToAltAz", $"Completed.");
            }
            catch (Exception ex)
            {
                LogMessage("SlewToAltAz", $"Threw an exception: \r\n{ex}");
                throw;
            }
        }

        /// <summary>
        /// Move the telescope to the given local horizontal coordinates.
        /// This method must be implemented if <see cref="CanSlewAltAzAsync" /> returns True.
        /// It returns immediately, with <see cref="Slewing" /> set to True
        /// </summary>
        /// <param name="Azimuth">Azimuth to which to move</param>
        /// <param name="Altitude">Altitude to which to move to</param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Style", "VSTHRD200:Use \"Async\" suffix for async methods", Justification = "Public method name used for many years.")]
        public void SlewToAltAzAsync(double azimuth, double altitude)
        {
            try
            {
                CheckConnected("SlewToAltAzAsync");
                LogMessage("SlewToAltAzAsync", $"Calling method - Azimuth: {azimuth}, Altitude: {altitude}.");
                TelescopeHardware.SlewToAltAzAsync(azimuth, altitude);
                LogMessage("SlewToAltAzAsync", $"Completed.");
            }
            catch (Exception ex)
            {
                LogMessage("SlewToAltAzAsync", $"Threw an exception: \r\n{ex}");
                throw;
            }
        }

        /// <summary>
        /// Move the telescope to the given equatorial coordinates.  
        /// This method must be implemented if <see cref="CanSlew" /> returns True.
        /// It does not return until the slew is complete.
        /// </summary>
        public void SlewToCoordinates(double rightAscension, double declination)
        {
            try
            {
                CheckConnected("SlewToCoordinates");
                LogMessage("SlewToCoordinates", $"Calling method - RightAscension: {rightAscension}, Declination: {declination}.");
                TelescopeHardware.SlewToCoordinates(rightAscension, declination);
                LogMessage("SlewToCoordinates", $"Completed.");
            }
            catch (Exception ex)
            {
                LogMessage("SlewToCoordinates", $"Threw an exception: \r\n{ex}");
                throw;
            }
        }

        /// <summary>
        /// Move the telescope to the given equatorial coordinates.
        /// This method must be implemented if <see cref="CanSlewAsync" /> returns True.
        /// It returns immediately, with <see cref="Slewing" /> set to True
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Style", "VSTHRD200:Use \"Async\" suffix for async methods", Justification = "Public method name used for many years.")]
        public void SlewToCoordinatesAsync(double rightAscension, double declination)
        {
            try
            {
                CheckConnected("SlewToCoordinatesAsync");
                LogMessage("SlewToCoordinatesAsync", $"Calling method - RightAscension: {rightAscension}, Declination: {declination}.");
                TelescopeHardware.SlewToCoordinatesAsync(rightAscension, declination);
                LogMessage("SlewToCoordinatesAsync", $"Completed.");
            }
            catch (Exception ex)
            {
                LogMessage("SlewToCoordinatesAsync", $"Threw an exception: \r\n{ex}");
                throw;
            }
        }

        /// <summary>
        /// Move the telescope to the <see cref="TargetRightAscension" /> and <see cref="TargetDeclination" /> coordinates.
        /// This method must be implemented if <see cref="CanSlew" /> returns True.
        /// It does not return until the slew is complete.
        /// </summary>
        public void SlewToTarget()
        {
            try
            {
                CheckConnected("SlewToTarget");
                LogMessage("SlewToTarget", $"Calling method.");
                TelescopeHardware.SlewToTarget();
                LogMessage("SlewToTarget", $"Completed.");
            }
            catch (Exception ex)
            {
                LogMessage("SlewToTarget", $"Threw an exception: \r\n{ex}");
                throw;
            }
        }

        /// <summary>
        /// Move the telescope to the <see cref="TargetRightAscension" /> and <see cref="TargetDeclination" />  coordinates.
        /// This method must be implemented if <see cref="CanSlewAsync" /> returns True.
        /// It returns immediately, with <see cref="Slewing" /> set to True
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Style", "VSTHRD200:Use \"Async\" suffix for async methods", Justification = "Public method name used for many years.")]
        public void SlewToTargetAsync()
        {
            try
            {
                CheckConnected("SlewToTargetAsync");
                LogMessage("SlewToTargetAsync", $"Calling method.");
                TelescopeHardware.SlewToTargetAsync();
                LogMessage("SlewToTargetAsync", $"Completed.");
            }
            catch (Exception ex)
            {
                LogMessage("SlewToTargetAsync", $"Threw an exception: \r\n{ex}");
                throw;
            }
        }

        /// <summary>
        /// True if telescope is in the process of moving in response to one of the
        /// Slew methods or the <see cref="MoveAxis" /> method, False at all other times.
        /// </summary>
        public bool Slewing
        {
            get
            {
                try
                {
                    CheckConnected("Slewing");
                    bool slewing = TelescopeHardware.Slewing;
                    LogMessage("Slewing", slewing.ToString());
                    return slewing;
                }
                catch (Exception ex)
                {
                    LogMessage("Slewing", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// Matches the scope's local horizontal coordinates to the given local horizontal coordinates.
        /// </summary>
        public void SyncToAltAz(double azimuth, double altitude)
        {
            try
            {
                CheckConnected("SyncToAltAz");
                LogMessage("SyncToAltAz", $"Calling method - Azimuth: {azimuth}, Altitude: {altitude}.");
                TelescopeHardware.SyncToAltAz(azimuth, altitude);
                LogMessage("SyncToAltAz", $"Completed.");
            }
            catch (Exception ex)
            {
                LogMessage("SyncToAltAz", $"Threw an exception: \r\n{ex}");
                throw;
            }
        }

        /// <summary>
        /// Matches the scope's equatorial coordinates to the given equatorial coordinates.
        /// </summary>
        public void SyncToCoordinates(double rightAscension, double declination)
        {
            try
            {
                CheckConnected("SyncToCoordinates");
                LogMessage("SyncToCoordinates", $"Calling method - RightAscension: {rightAscension}, Declination: {declination}.");
                TelescopeHardware.SyncToCoordinates(rightAscension, declination);
                LogMessage("SyncToCoordinates", $"Completed.");
            }
            catch (Exception ex)
            {
                LogMessage("SyncToCoordinates", $"Threw an exception: \r\n{ex}");
                throw;
            }
        }

        /// <summary>
        /// Matches the scope's equatorial coordinates to the target equatorial coordinates.
        /// </summary>
        public void SyncToTarget()
        {
            try
            {
                CheckConnected("SyncToTarget");
                LogMessage("SyncToTarget", $"Calling method.");
                TelescopeHardware.SyncToTarget();
                LogMessage("SyncToTarget", $"Completed.");
            }
            catch (Exception ex)
            {
                LogMessage("SyncToTarget", $"Threw an exception: \r\n{ex}");
                throw;
            }
        }

        /// <summary>
        /// The declination (degrees, positive North) for the target of an equatorial slew or sync operation
        /// </summary>
        public double TargetDeclination
        {
            get
            {
                try
                {
                    CheckConnected("TargetDeclination Get");
                    double targetDeclination = TelescopeHardware.TargetDeclination;
                    LogMessage("TargetDeclination Get", targetDeclination.ToString());
                    return targetDeclination;
                }
                catch (Exception ex)
                {
                    LogMessage("TargetDeclination Get", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
            set
            {
                try
                {
                    CheckConnected("TargetDeclination Set");
                    LogMessage("TargetDeclination Set", value.ToString());
                    TelescopeHardware.TargetDeclination = value;
                }
                catch (Exception ex)
                {
                    LogMessage("TargetDeclination Set", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// The right ascension (hours) for the target of an equatorial slew or sync operation
        /// </summary>
        public double TargetRightAscension
        {
            get
            {
                try
                {
                    CheckConnected("TargetRightAscension Get");
                    double targetRightAscension = TelescopeHardware.TargetRightAscension;
                    LogMessage("TargetRightAscension Get", targetRightAscension.ToString());
                    return targetRightAscension;
                }
                catch (Exception ex)
                {
                    LogMessage("TargetRightAscension Get", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
            set
            {
                try
                {
                    CheckConnected("TargetRightAscension Set");
                    LogMessage("TargetRightAscension Set", value.ToString());
                    TelescopeHardware.TargetRightAscension = value;
                }
                catch (Exception ex)
                {
                    LogMessage("TargetRightAscension Set", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// The state of the telescope's sidereal tracking drive.
        /// </summary>
        public bool Tracking
        {
            get
            {
                try
                {
                    CheckConnected("Tracking Get");
                    bool tracking = TelescopeHardware.Tracking;
                    LogMessage("Tracking Get", tracking.ToString());
                    return tracking;
                }
                catch (Exception ex)
                {
                    LogMessage("Tracking Get", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
            set
            {
                try
                {
                    CheckConnected("Tracking Set");
                    LogMessage("Tracking Set", value.ToString());
                    TelescopeHardware.Tracking = value;
                }
                catch (Exception ex)
                {
                    LogMessage("Tracking Set", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// The current tracking rate of the telescope's sidereal drive
        /// </summary>
        public DriveRates TrackingRate
        {
            get
            {
                try
                {
                    CheckConnected("TrackingRate Get");
                    DriveRates trackingRate = TelescopeHardware.TrackingRate;
                    LogMessage("TrackingRate Get", trackingRate.ToString());
                    return trackingRate;
                }
                catch (Exception ex)
                {
                    LogMessage("TrackingRate Get", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
            set
            {
                try
                {
                    CheckConnected("TrackingRate Set");
                    LogMessage("TrackingRate Set", value.ToString());
                    TelescopeHardware.TrackingRate = value;
                }
                catch (Exception ex)
                {
                    LogMessage("TrackingRate Set", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// Returns a collection of supported <see cref="DriveRates" /> values that describe the permissible
        /// values of the <see cref="TrackingRate" /> property for this telescope type.
        /// </summary>
        public ITrackingRates TrackingRates
        {
            get
            {
                try
                {
                    CheckConnected("TrackingRates");
                    ITrackingRates trackingRates = TelescopeHardware.TrackingRates;
                    LogMessage("TrackingRates", trackingRates.ToString());
                    return trackingRates;
                }
                catch (Exception ex)
                {
                    LogMessage("TrackingRates", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// The UTC date/time of the telescope's internal clock
        /// </summary>
        public DateTime UTCDate
        {
            get
            {
                try
                {
                    CheckConnected("UTCDate Get");
                    DateTime utcDate = TelescopeHardware.UTCDate;
                    LogMessage("UTCDate Get", utcDate.ToString());
                    return utcDate;
                }
                catch (Exception ex)
                {
                    LogMessage("UTCDate Get", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
            set
            {
                try
                {
                    CheckConnected("UTCDate Set");
                    LogMessage("UTCDate Set", value.ToString());
                    TelescopeHardware.UTCDate = value;
                }
                catch (Exception ex)
                {
                    LogMessage("UTCDate Set", $"Threw an exception: \r\n{ex}");
                    throw;
                }
            }
        }

        /// <summary>
        /// Takes telescope out of the Parked state.
        /// </summary>
        public void Unpark()
        {
            LogMessage("Unpark", $"Completed.");
            try
            {
                CheckConnected("Unpark");
                LogMessage("Unpark", $"Calling method.");
                TelescopeHardware.Unpark();
                LogMessage("Unpark", $"Completed.");
            }
            catch (Exception ex)
            {
                LogMessage("Unpark", $"Threw an exception: \r\n{ex}");
                throw;
            }
        }

        #endregion

        #region Private properties and methods
        // Useful properties and methods that can be used as required to help with driver development

        /// <summary>
        /// Use this function to throw an exception if we aren't connected to the hardware
        /// </summary>
        /// <param name="message"></param>
        private void CheckConnected(string message)
        {
            if (!connectedState)
            {
                throw new NotConnectedException($"{DriverDescription} ({DriverProgId}) is not connected: {message}");
            }
        }

        /// <summary>
        /// Log helper function that writes to the driver or local server loggers as required
        /// </summary>
        /// <param name="identifier">Identifier such as method name</param>
        /// <param name="message">Message to be logged.</param>
        private void LogMessage(string identifier, string message)
        {
            // This code is currently set to write messages to an individual driver log AND to the shared hardware log.

            // Write to the individual log for this specific instance (if enabled by the driver having a TraceLogger instance)
            if (tl != null)
            {
                tl.LogMessageCrLf(identifier, message); // Write to the individual driver log
            }

            // Write to the common hardware log shared by all running instances of the driver.
            TelescopeHardware.LogMessage(identifier, message); // Write to the local server logger
        }

        /// <summary>
        /// Read the trace state from the driver's Profile and enable / disable the trace log accordingly.
        /// </summary>
        private void SetTraceState()
        {
            using (Profile driverProfile = new Profile())
            {
                driverProfile.DeviceType = "Telescope";
                tl.Enabled = Convert.ToBoolean(driverProfile.GetValue(DriverProgId, TelescopeHardware.traceStateProfileName, string.Empty, TelescopeHardware.traceStateDefault));
            }
        }

        #endregion
    }
}

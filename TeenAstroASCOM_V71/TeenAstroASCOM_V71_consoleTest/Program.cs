// This is a console application that can be used to test an ASCOM driver

// By default the chooser is used to pick a driver. You can pass a ProgId as the first
// command line argument to bypass the chooser, e.g.:
//    TeenAstroASCOM_V71_consoleTest.exe ASCOM.TeenAstro.Telescope

using System;
using System.Collections.Generic;

namespace ASCOM
{
    internal class Program
    {
        static void Main(string[] args)
        {
            string progId = null;
            string interfaceArg = null;
            string interfaceValue = null;
            string portValue = null;

            if (args != null && args.Length > 0 && !string.IsNullOrWhiteSpace(args[0]))
            {
                progId = args[0];
            }

            // Optional args: [1]=interface (COM|IP), [2]=value (COM3 or IP addr), [3]=port (optional)
            if (args != null && args.Length > 1 && !string.IsNullOrWhiteSpace(args[1]))
                interfaceArg = args[1];
            if (args != null && args.Length > 2 && !string.IsNullOrWhiteSpace(args[2]))
                interfaceValue = args[2];
            if (args != null && args.Length > 3 && !string.IsNullOrWhiteSpace(args[3]))
                portValue = args[3];

            // Local helper: write ASCOM profile settings via reflection (returns false on failure)
            // Improved reflection-based profile writer.
            // Searches for an already-loaded ASCOM.Utilities assembly first, then tries common install locations.
            // Returns true on success. On failure returns false and a detailed error string.
            bool TryWriteProfile(string pid, string intf, string val, string port, out string error)
            {
                error = null;
                if (string.IsNullOrEmpty(intf) || string.IsNullOrEmpty(val))
                    return false;

                System.Reflection.Assembly asm = null;
                string[] probePathsTried = new string[0];
                try
                {
                    // 1) Check already loaded assemblies
                    foreach (var a in AppDomain.CurrentDomain.GetAssemblies())
                    {
                        try
                        {
                            var name = a.GetName().Name;
                            if (string.Equals(name, "ASCOM.Utilities", StringComparison.OrdinalIgnoreCase))
                            {
                                asm = a;
                                break;
                            }
                        }
                        catch { }
                    }

                    // 2) If not loaded, probe common locations
                    if (asm == null)
                    {
                        var exeDir = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
                        var candidates = new List<string>
                        {
                            System.IO.Path.Combine(exeDir, "ASCOM.Utilities.dll"),
                            System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "ASCOM", "Developer", "Components", "Platform", "ASCOM.Utilities.dll"),
                            System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "ASCOM", "Developer", "Components", "Platform", "ASCOM.Utilities.dll")
                        };

                        var tried = new List<string>();
                        foreach (var p in candidates)
                        {
                            if (string.IsNullOrEmpty(p)) continue;
                            tried.Add(p);
                            if (System.IO.File.Exists(p))
                            {
                                asm = System.Reflection.Assembly.LoadFrom(p);
                                break;
                            }
                        }
                        probePathsTried = tried.ToArray();
                    }

                    if (asm == null)
                    {
                        error = "ASCOM.Utilities assembly not found. Probed: " + string.Join(";", probePathsTried);
                        return false;
                    }

                    var profileType = asm.GetType("ASCOM.Utilities.Profile");
                    if (profileType == null)
                    {
                        error = "Type ASCOM.Utilities.Profile not found in assembly.";
                        return false;
                    }

                    // Create instance
                    object profileObj = null;
                    try
                    {
                        profileObj = Activator.CreateInstance(profileType);
                    }
                    catch (System.Reflection.TargetInvocationException tie)
                    {
                        error = "Failed to create Profile instance: " + (tie.InnerException?.Message ?? tie.Message);
                        return false;
                    }

                    try
                    {
                        var deviceTypeProp = profileType.GetProperty("DeviceType");
                        if (deviceTypeProp != null && deviceTypeProp.CanWrite)
                            deviceTypeProp.SetValue(profileObj, "Telescope", null);

                        // Locate WriteValue(string, string, string)
                        var writeMethod = profileType.GetMethod("WriteValue", new Type[] { typeof(string), typeof(string), typeof(string) });
                        if (writeMethod == null)
                        {
                            error = "Profile.WriteValue(string,string,string) not found.";
                            return false;
                        }

                        // Invoke writes
                        try
                        {
                            writeMethod.Invoke(profileObj, new object[] { pid, "Interface", intf });
                            if (intf.Equals("COM", StringComparison.OrdinalIgnoreCase))
                            {
                                writeMethod.Invoke(profileObj, new object[] { pid, "COM Port", val });
                            }
                            else if (intf.Equals("IP", StringComparison.OrdinalIgnoreCase))
                            {
                                writeMethod.Invoke(profileObj, new object[] { pid, "IP Address", val });
                                if (!string.IsNullOrEmpty(port))
                                    writeMethod.Invoke(profileObj, new object[] { pid, "Port", port });
                            }
                        }
                        catch (System.Reflection.TargetInvocationException tie)
                        {
                            error = "Profile API threw: " + (tie.InnerException?.Message ?? tie.Message);
                            return false;
                        }
                    }
                    finally
                    {
                        // Dispose if available
                        try
                        {
                            var disposeMethod = profileType.GetMethod("Dispose", Type.EmptyTypes);
                            if (disposeMethod != null && profileObj != null)
                                disposeMethod.Invoke(profileObj, null);
                        }
                        catch { }
                    }

                    return true;
                }
                catch (Exception ex)
                {
                    // Unwrap aggregate/target invocation exceptions for clarity
                    var msg = ex.Message;
                    if (ex is System.Reflection.TargetInvocationException && ex.InnerException != null)
                        msg = ex.InnerException.Message + " -> " + msg;
                    error = "Exception writing profile: " + msg;
                    return false;
                }
            }

            // Create the device (either via chooser or by ProgId)
            ASCOM.DriverAccess.Telescope device = null;
            try
            {
                if (string.IsNullOrEmpty(progId))
                {
                    // Use the chooser when no ProgId supplied
                    progId = ASCOM.DriverAccess.Telescope.Choose("");
                    if (string.IsNullOrEmpty(progId))
                    {
                        Console.WriteLine("No driver selected. Exiting.");
                        return;
                    }
                }

                // If interface args provided, write them to the ASCOM Profile for this driver so the driver picks them up
                try
                {
                    if (!string.IsNullOrEmpty(interfaceArg) && !string.IsNullOrEmpty(interfaceValue))
                    {
                        // Use reflection to write ASCOM profile so this project doesn't need a compile-time reference to ASCOM.Utilities
                        string exeDir = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
                        string utilPath = System.IO.Path.Combine(exeDir, "ASCOM.Utilities.dll");
                        if (!System.IO.File.Exists(utilPath))
                            utilPath = System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "ASCOM", "Developer", "Components", "Platform", "ASCOM.Utilities.dll");

                        if (!System.IO.File.Exists(utilPath))
                            throw new Exception("ASCOM.Utilities.dll not found; cannot write profile");

                        var asm = System.Reflection.Assembly.LoadFrom(utilPath);
                        var profileType = asm.GetType("ASCOM.Utilities.Profile");
                        if (profileType == null)
                            throw new Exception("ASCOM.Utilities.Profile type not found in assembly");

                        var profileObj = Activator.CreateInstance(profileType);
                        var deviceTypeProp = profileType.GetProperty("DeviceType");
                        if (deviceTypeProp != null && deviceTypeProp.CanWrite)
                            deviceTypeProp.SetValue(profileObj, "Telescope", null);

                        var writeMethod = profileType.GetMethod("WriteValue", new Type[] { typeof(string), typeof(string), typeof(string) });
                        if (writeMethod == null)
                            throw new Exception("Profile.WriteValue(string,string,string) method not found");

                        writeMethod.Invoke(profileObj, new object[] { progId, "Interface", interfaceArg });
                        if (interfaceArg.Equals("COM", StringComparison.OrdinalIgnoreCase))
                        {
                            writeMethod.Invoke(profileObj, new object[] { progId, "COM Port", interfaceValue });
                        }
                        else if (interfaceArg.Equals("IP", StringComparison.OrdinalIgnoreCase))
                        {
                            writeMethod.Invoke(profileObj, new object[] { progId, "IP Address", interfaceValue });
                            if (!string.IsNullOrEmpty(portValue))
                                writeMethod.Invoke(profileObj, new object[] { progId, "Port", portValue });
                        }

                        Console.WriteLine($"Wrote interface settings for {progId}: {interfaceArg}={interfaceValue} {(portValue!=null?"port="+portValue:string.Empty)}");
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Failed to write driver profile: {ex.Message}");
                }

                device = new ASCOM.DriverAccess.Telescope(progId);

                Console.WriteLine($"Attempting to connect to: {progId}");
                device.Connected = true;

                // Basic read-only diagnostics
                Console.WriteLine("--- Driver basic info ---");
                Console.WriteLine($"Name: {device.Name}");
                Console.WriteLine($"Description: {device.Description}");
                Console.WriteLine($"DriverInfo: {device.DriverInfo}");
                Console.WriteLine($"DriverVersion: {device.DriverVersion}");
                Console.WriteLine($"InterfaceVersion: {device.InterfaceVersion}");
                Console.WriteLine($"Connected: {device.Connected}");
                Console.WriteLine();

                Console.WriteLine("--- Capability flags ---");
                Console.WriteLine($"CanSlew: {device.CanSlew}");
                Console.WriteLine($"CanSlewAsync: {device.CanSlewAsync}");
                Console.WriteLine($"CanSlewAltAz: {device.CanSlewAltAz}");
                Console.WriteLine($"CanSync: {device.CanSync}");
                Console.WriteLine($"CanSetTracking: {device.CanSetTracking}");
                Console.WriteLine($"CanPulseGuide: {device.CanPulseGuide}");
                Console.WriteLine($"CanPark: {device.CanPark}");
                Console.WriteLine($"CanFindHome: {device.CanFindHome}");
                Console.WriteLine();

                // Read some safe properties
                Console.WriteLine("--- Read-only values ---");
                try { Console.WriteLine($"UTCDate: {device.UTCDate.ToString("o")} (UTC)"); } catch (Exception ex) { Console.WriteLine($"UTCDate read failed: {ex.Message}"); }
                try { Console.WriteLine($"SiderealTime: {device.SiderealTime}"); } catch (Exception ex) { Console.WriteLine($"SiderealTime read failed: {ex.Message}"); }
                try { Console.WriteLine($"RightAscension (hours): {device.RightAscension}"); } catch (Exception ex) { Console.WriteLine($"RightAscension read failed: {ex.Message}"); }
                try { Console.WriteLine($"Declination (deg): {device.Declination}"); } catch (Exception ex) { Console.WriteLine($"Declination read failed: {ex.Message}"); }
                Console.WriteLine();

                // Test toggling tracking if supported (minimally invasive)
                if (device.CanSetTracking)
                {
                    try
                    {
                        Console.WriteLine("--- Tracking toggle test ---");
                        bool before = device.Tracking;
                        Console.WriteLine($"Tracking before: {before}");
                        // Toggle
                        device.Tracking = !before;
                        Console.WriteLine($"Tracking set to: {!before}");
                        // Restore
                        device.Tracking = before;
                        Console.WriteLine($"Tracking restored to: {before}");
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine($"Tracking toggle failed: {ex.Message}");
                    }
                    Console.WriteLine();
                }

                // Site coordinates read (safe). Do not change these values automatically.
                try { Console.WriteLine($"SiteLatitude: {device.SiteLatitude}"); } catch (Exception ex) { Console.WriteLine($"SiteLatitude read failed: {ex.Message}"); }
                try { Console.WriteLine($"SiteLongitude: {device.SiteLongitude}"); } catch (Exception ex) { Console.WriteLine($"SiteLongitude read failed: {ex.Message}"); }

                Console.WriteLine();
                Console.WriteLine("Diagnostics complete. Disconnecting...");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"ERROR: {ex.GetType().Name}: {ex.Message}");
            }
            finally
            {
                if (device != null)
                {
                    try
                    {
                        if (device.Connected)
                            device.Connected = false;
                        device.Dispose();
                    }
                    catch { }
                }
            }

            // Additional connection sweep if no explicit interface was provided
            if (string.IsNullOrEmpty(interfaceArg))
            {
                Console.WriteLine();
                Console.WriteLine("--- Connection sweep (COM3 and IP 192.168.1.28:9999) ---");
                var endpoints = new List<Tuple<string, string, string>>
                {
                    Tuple.Create("COM", "COM3", (string)null),
                    Tuple.Create("IP", "192.168.1.28", "9999")
                };

                foreach (var ep in endpoints)
                {
                    Console.WriteLine($"\nTesting {ep.Item1}={ep.Item2} {(ep.Item3!=null?"port="+ep.Item3:string.Empty)}");
                    if (!TryWriteProfile(progId, ep.Item1, ep.Item2, ep.Item3, out string err))
                    {
                        Console.WriteLine($"Failed to write profile for {ep.Item1}: {err}");
                        continue;
                    }

                    ASCOM.DriverAccess.Telescope t = null;
                    try
                    {
                        t = new ASCOM.DriverAccess.Telescope(progId);
                        Console.WriteLine("Created driver instance, attempting connect...");
                        t.Connected = true;
                        Console.WriteLine($"Connected: {t.Connected}");
                        try { Console.WriteLine($"UTCDate: {t.UTCDate:o}"); } catch (Exception ex) { Console.WriteLine($"UTCDate read failed: {ex.Message}"); }
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine($"Connection test failed: {ex.GetType().Name}: {ex.Message}");
                    }
                    finally
                    {
                        if (t != null)
                        {
                            try { if (t.Connected) t.Connected = false; t.Dispose(); } catch { }
                        }
                    }
                }

                Console.WriteLine();
                Console.WriteLine("Connection sweep complete.");
            }

            Console.WriteLine("Press Enter to finish");
            Console.ReadLine();
        }
    }
}

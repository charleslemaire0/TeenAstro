//
// ASCOM Focuser driver for TeenAstro
// Shares the same connection (COM or IP) as the Telescope driver.
//

using ASCOM;
using ASCOM.DeviceInterface;
using ASCOM.LocalServer; // SharedResources
using ASCOM.Utilities;
using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace ASCOM.TeenAstro.Focuser
{
  [ComVisible(true)]
  [Guid("A1B2C3D4-E5F6-4A5B-8C9D-0E1F2A3B4C5D")]
  [ProgId("ASCOM.TeenAstro.Focuser")]
  [ServedClassName("ASCOM Focuser Driver for TeenAstro")]
  [ClassInterface(ClassInterfaceType.None)]
  public class Focuser : ReferenceCountedObjectBase, IFocuserV3, IDisposable
  {
    internal static string DriverProgId;
    internal static string DriverDescription;

    internal bool connectedState;
    internal bool connectingState;
    internal Exception connectionException;

    internal TraceLogger tl;
    private bool disposedValue;
    private Guid uniqueId;

    public Focuser()
    {
      try
      {
        var attr = Attribute.GetCustomAttribute(GetType(), typeof(ProgIdAttribute));
        DriverProgId = ((ProgIdAttribute)attr)?.Value ?? "ASCOM.TeenAstro.Focuser";
        attr = Attribute.GetCustomAttribute(GetType(), typeof(ServedClassNameAttribute));
        DriverDescription = ((ServedClassNameAttribute)attr)?.DisplayName ?? "ASCOM Focuser Driver for TeenAstro";

        tl = new TraceLogger("", "TeenAstro.Focuser.Driver");
        SetTraceState();
        FocuserHardware.InitialiseHardware();

        LogMessage("Focuser", "Driver initialisation");
        connectedState = false;
        uniqueId = Guid.NewGuid();
      }
      catch (Exception ex)
      {
        MessageBox.Show(ex.Message, "Exception creating ASCOM.TeenAstro.Focuser", MessageBoxButtons.OK, MessageBoxIcon.Error);
        throw;
      }
    }

    private void SetTraceState()
    {
      try
      {
        using (var profile = new Profile())
        {
          profile.DeviceType = "Focuser";
          tl.Enabled = Convert.ToBoolean(profile.GetValue(DriverProgId, FocuserHardware.traceStateProfileName, string.Empty, "false"));
        }
      }
      catch { }
    }

    private void LogMessage(string method, string message)
    {
      tl?.LogMessageCrLf(method, message);
    }

    #region Common properties and methods

    public void SetupDialog()
    {
      if (connectedState)
      {
        MessageBox.Show("Already connected, disconnect first.");
        return;
      }
      using (var form = new FocuserSetupDialogForm(tl))
      {
        if (form.ShowDialog() == DialogResult.OK)
          FocuserHardware.WriteProfile();
      }
    }

    public ArrayList SupportedActions
    {
      get
      {
        var list = new ArrayList();
        return list;
      }
    }

    public string Action(string actionName, string actionParameters)
    {
      throw new ActionNotImplementedException("Action " + actionName + " is not implemented.");
    }

    public void CommandBlind(string command, bool raw)
    {
      CheckConnected("CommandBlind");
      if (!raw) command = ":" + command + "#";
      string buf = "";
      if (!SharedResources.SendCommand(command, 0, ref buf))
        throw new ASCOM.DriverException("CommandBlind failed.");
    }

    public bool CommandBool(string command, bool raw)
    {
      CheckConnected("CommandBool");
      if (!raw) command = ":" + command + "#";
      string buf = "";
      if (!SharedResources.SendCommand(command, 1, ref buf))
        throw new ASCOM.DriverException("CommandBool failed.");
      return buf == "1";
    }

    public string CommandString(string command, bool raw)
    {
      CheckConnected("CommandString");
      if (!raw) command = ":" + command + "#";
      string buf = "";
      if (!SharedResources.SendCommand(command, 2, ref buf))
        throw new ASCOM.DriverException("CommandString failed.");
      return buf ?? "";
    }

    public bool Connected
    {
      get
      {
        LogMessage("Connected Get", connectedState.ToString());
        return connectedState;
      }
      set
      {
        if (value == connectedState) return;
        if (value)
        {
          FocuserHardware.SetConnected(uniqueId, true);
          connectedState = true;
        }
        else
        {
          connectedState = false;
          FocuserHardware.SetConnected(uniqueId, false);
        }
      }
    }

    public string Description
    {
      get
      {
        CheckConnected("Description");
        return "TeenAstro integrated focuser (LX200 protocol)";
      }
    }

    public string DriverInfo
    {
      get
      {
        return "ASCOM Focuser Driver for TeenAstro. Shares connection with Telescope driver.";
      }
    }

    public string DriverVersion
    {
      get
      {
        var v = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version;
        return $"{v.Major}.{v.Minor}";
      }
    }

    public short InterfaceVersion => 3;

    public string Name
    {
      get => "TeenAstro Focuser";
    }

    #endregion

    #region IFocuserV3

    public bool Absolute => true;

    public bool IsMoving
    {
      get
      {
        CheckConnected("IsMoving");
        return false; // TeenAstro does not report moving state via :F?#; could poll position change if needed
      }
    }

    public bool Link
    {
      get => Connected;
      set => Connected = value;
    }

    public int MaxIncrement
    {
      get
      {
        CheckConnected("MaxIncrement");
        return FocuserHardware.DefaultMaxStep;
      }
    }

    public int MaxStep
    {
      get
      {
        CheckConnected("MaxStep");
        return FocuserHardware.DefaultMaxStep;
      }
    }

    public void Move(int position)
    {
      CheckConnected("Move");
      if (position < 0 || position > FocuserHardware.DefaultMaxStep)
        throw new InvalidValueException("Move", position.ToString(), "0", FocuserHardware.DefaultMaxStep.ToString());
      FocuserHardware.MoveTo(position);
    }

    public int Position
    {
      get
      {
        CheckConnected("Position");
        if (!FocuserHardware.GetFocuserPosition(out int pos, out _, out _))
          throw new ASCOM.DriverException("No focuser or focuser not responding.");
        return pos;
      }
    }

    public double StepSize => 0.0; // Not reported by TeenAstro focuser

    public bool TempComp
    {
      get => false;
      set { if (value) throw new PropertyNotImplementedException("TempComp", false); }
    }

    public bool TempCompAvailable => false;

    public double Temperature
    {
      get
      {
        CheckConnected("Temperature");
        if (!FocuserHardware.GetFocuserPosition(out _, out _, out double temp))
          throw new ASCOM.DriverException("No focuser or focuser not responding.");
        return double.IsNaN(temp) ? 0.0 : temp;
      }
    }

    public void Halt()
    {
      CheckConnected("Halt");
      FocuserHardware.Halt();
    }

    #endregion

    private void CheckConnected(string method)
    {
      if (!connectedState || !FocuserHardware.IsConnected)
        throw new NotConnectedException(method);
    }

    #region IDisposable

    public void Dispose()
    {
      Dispose(true);
      GC.SuppressFinalize(this);
    }

    protected virtual void Dispose(bool disposing)
    {
      if (disposedValue) return;
      if (disposing)
      {
        if (connectedState)
        {
          connectedState = false;
          FocuserHardware.SetConnected(uniqueId, false);
        }
        tl?.Dispose();
        tl = null;
      }
      disposedValue = true;
    }

    #endregion
  }
}

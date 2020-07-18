Public Class ConfigEEPROM
  Dim Foc As Focuser
  Public Sub New()

    ' Dieser Aufruf ist für den Designer erforderlich.
    InitializeComponent()
    Foc = New Focuser
    ' Fügen Sie Initialisierungen nach dem InitializeComponent()-Aufruf hinzu.

  End Sub
  Private Sub ConfigEEPROM_Load(sender As Object, e As EventArgs) Handles MyBase.Load
    '
    Try
      Foc.Connected = True
    Catch ex As Exception
      MsgBox(ex.Message)
      Me.Close()
      Return
    End Try
    ReadEEPROM()

  End Sub

  Private Sub ConfigEEPROM_FormClosing(sender As Object, e As FormClosingEventArgs) Handles MyBase.FormClosing
    Foc.Connected = False
    Foc.Dispose()
  End Sub

  Private Sub ButtonWEEPROM_Click(sender As Object, e As EventArgs) Handles ButtonWEEPROM.Click
    If NumericUpDownPP.Value <> My.Settings.parkPos Then
      SendVal(0, NumericUpDownPP.Value)
    End If
    If NumericUpDownMxP.Value <> My.Settings.maxPos Then
      SendVal(1, NumericUpDownMxP.Value)
    End If
    If NumericUpDownMinS.Value <> My.Settings.minSpeed Then
      SendVal(2, NumericUpDownMinS.Value)
    End If
    If NumericUpDownMaxS.Value <> My.Settings.maxSpeed Then
      SendVal(3, NumericUpDownMaxS.Value)
    End If
    If NumericUpDownAccCmd.Value <> My.Settings.cmdAcc Then
      SendVal(4, NumericUpDownAccCmd.Value)
    End If
    If NumericUpDownAccM.Value <> My.Settings.manAcc Then
      SendVal(5, NumericUpDownAccM.Value)
    End If
    If CheckBoxReverse.Checked <> My.Settings.reverse Then
      If CheckBoxReverse.Checked Then
        SendVal(7, 1)
      Else
        SendVal(7, 0)
      End If
    End If
    If NumericUpDownRes.Value <> My.Settings.resolution Then
      SendVal(8, NumericUpDownRes.Value)
    End If
    If Int(NumericUpDownCurr.Value / 10) <> My.Settings.current Then
      SendVal("c", Int(NumericUpDownCurr.Value / 10))
    End If
    If ComboBoxMicro.SelectedIndex <> My.Settings.micro - 2 Then
      SendVal("m", ComboBoxMicro.SelectedIndex + 2)
    End If
    ReadEEPROM()
  End Sub

  Private Sub ReadEEPROM()
    ReadEEPROMSettings()
    ReadEEPROMMotor()
  End Sub

  Private Sub ReadEEPROMMotor()
    Dim k As Integer = 0
    While True
      Dim s As String = ""
      While Not s.StartsWith("M")
        s = Foc.CommandString("FM")
      End While
      s = s.TrimStart("M")
      Dim info As String() = s.Split(" ")
      If Not SyncSettingsMotor(info) Then
        k += 1
      Else
        CheckBoxReverse.Checked = My.Settings.reverse
        ComboBoxMicro.SelectedIndex = My.Settings.micro - 2
        NumericUpDownRes.Value = My.Settings.resolution
        NumericUpDownCurr.Value = My.Settings.current * 10
        Exit While
      End If
      If k = 10 Then
        MsgBox("Failed to read EEPROM")
        Me.Close()
        Exit While
      End If
    End While
  End Sub

  Private Sub ReadEEPROMSettings()
    Dim k As Integer = 0
    While True
      Dim s As String = ""
      While Not s.StartsWith("~")
        s = Foc.CommandString("F~")
      End While
      s = s.TrimStart("~")
      Dim info As String() = s.Split(" ")
      If Not SyncSettings(info) Then
        k += 1
      Else
        NumericUpDownPP.Value = My.Settings.parkPos
        NumericUpDownMxP.Value = My.Settings.maxPos
        NumericUpDownMinS.Value = My.Settings.minSpeed
        NumericUpDownMaxS.Value = My.Settings.maxSpeed
        NumericUpDownAccCmd.Value = My.Settings.cmdAcc
        NumericUpDownAccM.Value = My.Settings.manAcc
        CheckBoxReverse.Checked = My.Settings.reverse
        Exit While
      End If
      If k = 10 Then
        MsgBox("Failed to read EEPROM")
        Me.Close()
        Exit While
      End If
    End While
  End Sub
  Function SendVal(ByVal val As String, ByVal value As UShort) As Boolean
    Dim s2 As String = "F" & val & "," & value
    Return Foc.CommandBool(s2)
  End Function
End Class
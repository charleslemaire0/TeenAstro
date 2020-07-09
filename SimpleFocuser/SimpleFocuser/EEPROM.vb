Module EEPROM
  Function SyncSettings(ByVal info As String()) As Boolean
    Try
      My.Settings.parkPos = CUShort(info(0))
      My.Settings.maxPos = CUShort(info(1))
      My.Settings.minSpeed = CUShort(info(2))
      My.Settings.maxSpeed = CUShort(info(3))
      My.Settings.cmdAcc = CUShort(info(4))
      My.Settings.manAcc = CUShort(info(5))
      My.Settings.Save()
    Catch ex As Exception
      MsgBox(ex)
      Return False
    End Try
    Return True
  End Function
  Function SyncSettingsMotor(ByVal motorinfo As String()) As Boolean
    Try
      My.Settings.reverse = CBool(motorinfo(0))
      My.Settings.micro = CUShort(motorinfo(1))
      My.Settings.resolution = CUShort(motorinfo(2))
      My.Settings.current = CUShort(motorinfo(3))
      My.Settings.Save()
    Catch ex As Exception
      MsgBox(ex)
      Return False
    End Try
    Return True
  End Function

End Module

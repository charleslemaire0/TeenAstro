Module Module1

  Sub Main()
    Dim dir As System.IO.DirectoryInfo = New System.IO.DirectoryInfo("./")

    For Each file In dir.EnumerateFiles
      If file.Extension = ".h" And file.Name.IndexOf("ta_") = -1 Then
        ProcessCat(file)
      End If
    Next
  End Sub
  Sub ProcessCat(file_in As System.IO.FileInfo)

    Dim file_out As System.IO.FileInfo = New System.IO.FileInfo("ta_" + file_in.Name)
    If file_in.Exists Then
      Dim sw As IO.StreamWriter = New IO.StreamWriter(file_out.FullName)
      Using sr As IO.StreamReader = New IO.StreamReader(file_in.FullName)
        Dim linenumber As Integer = 0
        Dim line As String
        Dim nameit As Integer = 0
        Dim subidit As Integer = 0
        Dim startline As Integer = Integer.MaxValue
        Try
          line = sr.ReadLine()
          While Not (line Is Nothing)
            linenumber += 1
            Dim p As Integer = line.IndexOf("PROGMEM")
            If p <> -1 Then
              startline = linenumber + 1
            End If
            Dim i As Integer = -1
            Dim j As Integer = -1
            i = line.IndexOf("{")
            If (i <> -1) Then
              j = line.IndexOf("}", i + 1)
            End If
            If (linenumber < startline Or i = -1 Or j = -1) Then
              sw.WriteLine(line)
            Else
              Dim f As String = line.Substring(i + 1, j - i - 1)
              Dim elements As String() = f.Split(",")
              Dim pos As Integer = 0
              Dim newline As String = "  {"
              For Each element In elements
                If (pos = 0) Then
                  Dim hasname As Integer = CInt(element)
                  If hasname = 0 Then
                    newline = newline + hasname.ToString.PadLeft(6)
                  Else
                    nameit += 1
                    newline += nameit.ToString.PadLeft(6)
                  End If
                ElseIf (pos = 3) Then
                  Dim hassubid As Integer = CInt(element)
                  If hassubid = 0 Then
                    newline += hassubid.ToString.PadLeft(6)
                  Else
                    subidit += 1
                    newline += subidit.ToString.PadLeft(6)
                  End If
                Else
                  newline += element
                End If
                If (pos = elements.Length - 1) Then
                  newline += " },"
                Else
                  newline += ","
                End If
                pos += 1
              Next
              sw.WriteLine(newline)
            End If
            line = sr.ReadLine()
          End While
          sw.Flush()
          sw.Close()
          sr.Close()
        Catch ex As Exception
          Console.WriteLine(ex.Message)
        End Try
      End Using
    End If
  End Sub


End Module

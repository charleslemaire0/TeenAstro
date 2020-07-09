<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class ConfigEEPROM
    Inherits System.Windows.Forms.Form

    'Das Formular überschreibt den Löschvorgang, um die Komponentenliste zu bereinigen.
    <System.Diagnostics.DebuggerNonUserCode()> _
    Protected Overrides Sub Dispose(ByVal disposing As Boolean)
        Try
            If disposing AndAlso components IsNot Nothing Then
                components.Dispose()
            End If
        Finally
            MyBase.Dispose(disposing)
        End Try
    End Sub

    'Wird vom Windows Form-Designer benötigt.
    Private components As System.ComponentModel.IContainer

    'Hinweis: Die folgende Prozedur ist für den Windows Form-Designer erforderlich.
    'Das Bearbeiten ist mit dem Windows Form-Designer möglich.  
    'Das Bearbeiten mit dem Code-Editor ist nicht möglich.
    <System.Diagnostics.DebuggerStepThrough()> _
    Private Sub InitializeComponent()
    Me.GroupBoxSettings = New System.Windows.Forms.GroupBox()
    Me.TableLayoutPanel2 = New System.Windows.Forms.TableLayoutPanel()
    Me.Label10 = New System.Windows.Forms.Label()
    Me.NumericUpDownRes = New System.Windows.Forms.NumericUpDown()
    Me.NumericUpDownMxP = New System.Windows.Forms.NumericUpDown()
    Me.NumericUpDownMinS = New System.Windows.Forms.NumericUpDown()
    Me.NumericUpDownMaxS = New System.Windows.Forms.NumericUpDown()
    Me.NumericUpDownAccCmd = New System.Windows.Forms.NumericUpDown()
    Me.Label1 = New System.Windows.Forms.Label()
    Me.Label4 = New System.Windows.Forms.Label()
    Me.Label5 = New System.Windows.Forms.Label()
    Me.Label6 = New System.Windows.Forms.Label()
    Me.Label7 = New System.Windows.Forms.Label()
    Me.Label8 = New System.Windows.Forms.Label()
    Me.NumericUpDownAccM = New System.Windows.Forms.NumericUpDown()
    Me.NumericUpDownPP = New System.Windows.Forms.NumericUpDown()
    Me.Label2 = New System.Windows.Forms.Label()
    Me.CheckBoxReverse = New System.Windows.Forms.CheckBox()
    Me.Label11 = New System.Windows.Forms.Label()
    Me.Label12 = New System.Windows.Forms.Label()
    Me.ComboBoxMicro = New System.Windows.Forms.ComboBox()
    Me.NumericUpDownCurr = New System.Windows.Forms.NumericUpDown()
    Me.ButtonWEEPROM = New System.Windows.Forms.Button()
    Me.Label3 = New System.Windows.Forms.Label()
    Me.GroupBoxSettings.SuspendLayout()
    Me.TableLayoutPanel2.SuspendLayout()
    CType(Me.NumericUpDownRes, System.ComponentModel.ISupportInitialize).BeginInit()
    CType(Me.NumericUpDownMxP, System.ComponentModel.ISupportInitialize).BeginInit()
    CType(Me.NumericUpDownMinS, System.ComponentModel.ISupportInitialize).BeginInit()
    CType(Me.NumericUpDownMaxS, System.ComponentModel.ISupportInitialize).BeginInit()
    CType(Me.NumericUpDownAccCmd, System.ComponentModel.ISupportInitialize).BeginInit()
    CType(Me.NumericUpDownAccM, System.ComponentModel.ISupportInitialize).BeginInit()
    CType(Me.NumericUpDownPP, System.ComponentModel.ISupportInitialize).BeginInit()
    CType(Me.NumericUpDownCurr, System.ComponentModel.ISupportInitialize).BeginInit()
    Me.SuspendLayout()
    '
    'GroupBoxSettings
    '
    Me.GroupBoxSettings.Controls.Add(Me.TableLayoutPanel2)
    Me.GroupBoxSettings.Location = New System.Drawing.Point(10, 5)
    Me.GroupBoxSettings.Name = "GroupBoxSettings"
    Me.GroupBoxSettings.Size = New System.Drawing.Size(265, 295)
    Me.GroupBoxSettings.TabIndex = 11
    Me.GroupBoxSettings.TabStop = False
    Me.GroupBoxSettings.Text = "Settings"
    '
    'TableLayoutPanel2
    '
    Me.TableLayoutPanel2.ColumnCount = 2
    Me.TableLayoutPanel2.ColumnStyles.Add(New System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50.0!))
    Me.TableLayoutPanel2.ColumnStyles.Add(New System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50.0!))
    Me.TableLayoutPanel2.Controls.Add(Me.Label10, 0, 8)
    Me.TableLayoutPanel2.Controls.Add(Me.NumericUpDownRes, 1, 8)
    Me.TableLayoutPanel2.Controls.Add(Me.NumericUpDownMxP, 1, 1)
    Me.TableLayoutPanel2.Controls.Add(Me.NumericUpDownMinS, 1, 2)
    Me.TableLayoutPanel2.Controls.Add(Me.NumericUpDownMaxS, 1, 3)
    Me.TableLayoutPanel2.Controls.Add(Me.NumericUpDownAccCmd, 1, 4)
    Me.TableLayoutPanel2.Controls.Add(Me.Label1, 0, 0)
    Me.TableLayoutPanel2.Controls.Add(Me.Label4, 0, 1)
    Me.TableLayoutPanel2.Controls.Add(Me.Label5, 0, 2)
    Me.TableLayoutPanel2.Controls.Add(Me.Label6, 0, 3)
    Me.TableLayoutPanel2.Controls.Add(Me.Label7, 0, 4)
    Me.TableLayoutPanel2.Controls.Add(Me.Label8, 0, 5)
    Me.TableLayoutPanel2.Controls.Add(Me.NumericUpDownAccM, 1, 5)
    Me.TableLayoutPanel2.Controls.Add(Me.NumericUpDownPP, 1, 0)
    Me.TableLayoutPanel2.Controls.Add(Me.Label2, 0, 7)
    Me.TableLayoutPanel2.Controls.Add(Me.CheckBoxReverse, 1, 7)
    Me.TableLayoutPanel2.Controls.Add(Me.Label11, 0, 9)
    Me.TableLayoutPanel2.Controls.Add(Me.Label12, 0, 10)
    Me.TableLayoutPanel2.Controls.Add(Me.ComboBoxMicro, 1, 9)
    Me.TableLayoutPanel2.Controls.Add(Me.NumericUpDownCurr, 1, 10)
    Me.TableLayoutPanel2.Location = New System.Drawing.Point(15, 28)
    Me.TableLayoutPanel2.Name = "TableLayoutPanel2"
    Me.TableLayoutPanel2.RowCount = 11
    Me.TableLayoutPanel2.RowStyles.Add(New System.Windows.Forms.RowStyle())
    Me.TableLayoutPanel2.RowStyles.Add(New System.Windows.Forms.RowStyle())
    Me.TableLayoutPanel2.RowStyles.Add(New System.Windows.Forms.RowStyle())
    Me.TableLayoutPanel2.RowStyles.Add(New System.Windows.Forms.RowStyle())
    Me.TableLayoutPanel2.RowStyles.Add(New System.Windows.Forms.RowStyle())
    Me.TableLayoutPanel2.RowStyles.Add(New System.Windows.Forms.RowStyle())
    Me.TableLayoutPanel2.RowStyles.Add(New System.Windows.Forms.RowStyle())
    Me.TableLayoutPanel2.RowStyles.Add(New System.Windows.Forms.RowStyle())
    Me.TableLayoutPanel2.RowStyles.Add(New System.Windows.Forms.RowStyle())
    Me.TableLayoutPanel2.RowStyles.Add(New System.Windows.Forms.RowStyle())
    Me.TableLayoutPanel2.RowStyles.Add(New System.Windows.Forms.RowStyle())
    Me.TableLayoutPanel2.Size = New System.Drawing.Size(241, 254)
    Me.TableLayoutPanel2.TabIndex = 2
    '
    'Label10
    '
    Me.Label10.Anchor = System.Windows.Forms.AnchorStyles.Left
    Me.Label10.AutoSize = True
    Me.Label10.Location = New System.Drawing.Point(3, 182)
    Me.Label10.Name = "Label10"
    Me.Label10.Size = New System.Drawing.Size(57, 13)
    Me.Label10.TabIndex = 18
    Me.Label10.Text = "Resolution"
    '
    'NumericUpDownRes
    '
    Me.NumericUpDownRes.Location = New System.Drawing.Point(123, 179)
    Me.NumericUpDownRes.Maximum = New Decimal(New Integer() {512, 0, 0, 0})
    Me.NumericUpDownRes.Minimum = New Decimal(New Integer() {1, 0, 0, 0})
    Me.NumericUpDownRes.Name = "NumericUpDownRes"
    Me.NumericUpDownRes.Size = New System.Drawing.Size(94, 20)
    Me.NumericUpDownRes.TabIndex = 17
    Me.NumericUpDownRes.Value = New Decimal(New Integer() {10, 0, 0, 0})
    '
    'NumericUpDownMxP
    '
    Me.NumericUpDownMxP.Location = New System.Drawing.Point(123, 29)
    Me.NumericUpDownMxP.Maximum = New Decimal(New Integer() {65535, 0, 0, 0})
    Me.NumericUpDownMxP.Name = "NumericUpDownMxP"
    Me.NumericUpDownMxP.Size = New System.Drawing.Size(94, 20)
    Me.NumericUpDownMxP.TabIndex = 2
    '
    'NumericUpDownMinS
    '
    Me.NumericUpDownMinS.Location = New System.Drawing.Point(123, 55)
    Me.NumericUpDownMinS.Maximum = New Decimal(New Integer() {999, 0, 0, 0})
    Me.NumericUpDownMinS.Minimum = New Decimal(New Integer() {1, 0, 0, 0})
    Me.NumericUpDownMinS.Name = "NumericUpDownMinS"
    Me.NumericUpDownMinS.Size = New System.Drawing.Size(94, 20)
    Me.NumericUpDownMinS.TabIndex = 3
    Me.NumericUpDownMinS.Value = New Decimal(New Integer() {5, 0, 0, 0})
    '
    'NumericUpDownMaxS
    '
    Me.NumericUpDownMaxS.Location = New System.Drawing.Point(123, 81)
    Me.NumericUpDownMaxS.Maximum = New Decimal(New Integer() {999, 0, 0, 0})
    Me.NumericUpDownMaxS.Minimum = New Decimal(New Integer() {1, 0, 0, 0})
    Me.NumericUpDownMaxS.Name = "NumericUpDownMaxS"
    Me.NumericUpDownMaxS.Size = New System.Drawing.Size(94, 20)
    Me.NumericUpDownMaxS.TabIndex = 4
    Me.NumericUpDownMaxS.Value = New Decimal(New Integer() {5, 0, 0, 0})
    '
    'NumericUpDownAccCmd
    '
    Me.NumericUpDownAccCmd.Location = New System.Drawing.Point(123, 107)
    Me.NumericUpDownAccCmd.Maximum = New Decimal(New Integer() {99, 0, 0, 0})
    Me.NumericUpDownAccCmd.Minimum = New Decimal(New Integer() {10, 0, 0, 0})
    Me.NumericUpDownAccCmd.Name = "NumericUpDownAccCmd"
    Me.NumericUpDownAccCmd.Size = New System.Drawing.Size(94, 20)
    Me.NumericUpDownAccCmd.TabIndex = 5
    Me.NumericUpDownAccCmd.Value = New Decimal(New Integer() {10, 0, 0, 0})
    '
    'Label1
    '
    Me.Label1.Anchor = System.Windows.Forms.AnchorStyles.Left
    Me.Label1.AutoSize = True
    Me.Label1.Location = New System.Drawing.Point(3, 6)
    Me.Label1.Name = "Label1"
    Me.Label1.Size = New System.Drawing.Size(69, 13)
    Me.Label1.TabIndex = 6
    Me.Label1.Text = "Park Position"
    '
    'Label4
    '
    Me.Label4.Anchor = System.Windows.Forms.AnchorStyles.Left
    Me.Label4.AutoSize = True
    Me.Label4.Location = New System.Drawing.Point(3, 32)
    Me.Label4.Name = "Label4"
    Me.Label4.Size = New System.Drawing.Size(67, 13)
    Me.Label4.TabIndex = 8
    Me.Label4.Text = "Max Position"
    '
    'Label5
    '
    Me.Label5.Anchor = System.Windows.Forms.AnchorStyles.Left
    Me.Label5.AutoSize = True
    Me.Label5.Location = New System.Drawing.Point(3, 58)
    Me.Label5.Name = "Label5"
    Me.Label5.Size = New System.Drawing.Size(76, 13)
    Me.Label5.TabIndex = 9
    Me.Label5.Text = "Manual Speed"
    '
    'Label6
    '
    Me.Label6.Anchor = System.Windows.Forms.AnchorStyles.Left
    Me.Label6.AutoSize = True
    Me.Label6.Location = New System.Drawing.Point(3, 84)
    Me.Label6.Name = "Label6"
    Me.Label6.Size = New System.Drawing.Size(64, 13)
    Me.Label6.TabIndex = 10
    Me.Label6.Text = "Goto Speed"
    '
    'Label7
    '
    Me.Label7.Anchor = System.Windows.Forms.AnchorStyles.Left
    Me.Label7.AutoSize = True
    Me.Label7.Location = New System.Drawing.Point(3, 110)
    Me.Label7.Name = "Label7"
    Me.Label7.Size = New System.Drawing.Size(107, 13)
    Me.Label7.TabIndex = 11
    Me.Label7.Text = "Acceleration ASCOM"
    '
    'Label8
    '
    Me.Label8.Anchor = System.Windows.Forms.AnchorStyles.Left
    Me.Label8.AutoSize = True
    Me.Label8.Location = New System.Drawing.Point(3, 136)
    Me.Label8.Name = "Label8"
    Me.Label8.Size = New System.Drawing.Size(104, 13)
    Me.Label8.TabIndex = 12
    Me.Label8.Text = "Acceleration Manual"
    '
    'NumericUpDownAccM
    '
    Me.NumericUpDownAccM.Location = New System.Drawing.Point(123, 133)
    Me.NumericUpDownAccM.Maximum = New Decimal(New Integer() {99, 0, 0, 0})
    Me.NumericUpDownAccM.Minimum = New Decimal(New Integer() {10, 0, 0, 0})
    Me.NumericUpDownAccM.Name = "NumericUpDownAccM"
    Me.NumericUpDownAccM.Size = New System.Drawing.Size(94, 20)
    Me.NumericUpDownAccM.TabIndex = 14
    Me.NumericUpDownAccM.Value = New Decimal(New Integer() {10, 0, 0, 0})
    '
    'NumericUpDownPP
    '
    Me.NumericUpDownPP.Location = New System.Drawing.Point(123, 3)
    Me.NumericUpDownPP.Maximum = New Decimal(New Integer() {65535, 0, 0, 0})
    Me.NumericUpDownPP.Name = "NumericUpDownPP"
    Me.NumericUpDownPP.Size = New System.Drawing.Size(94, 20)
    Me.NumericUpDownPP.TabIndex = 0
    '
    'Label2
    '
    Me.Label2.Anchor = System.Windows.Forms.AnchorStyles.Left
    Me.Label2.AutoSize = True
    Me.Label2.Location = New System.Drawing.Point(3, 159)
    Me.Label2.Name = "Label2"
    Me.Label2.Size = New System.Drawing.Size(69, 13)
    Me.Label2.TabIndex = 17
    Me.Label2.Text = "Reverse Axis"
    '
    'CheckBoxReverse
    '
    Me.CheckBoxReverse.Anchor = System.Windows.Forms.AnchorStyles.Left
    Me.CheckBoxReverse.AutoSize = True
    Me.CheckBoxReverse.Location = New System.Drawing.Point(123, 159)
    Me.CheckBoxReverse.Name = "CheckBoxReverse"
    Me.CheckBoxReverse.Size = New System.Drawing.Size(15, 14)
    Me.CheckBoxReverse.TabIndex = 18
    Me.CheckBoxReverse.UseVisualStyleBackColor = True
    '
    'Label11
    '
    Me.Label11.Anchor = System.Windows.Forms.AnchorStyles.Left
    Me.Label11.AutoSize = True
    Me.Label11.Location = New System.Drawing.Point(3, 209)
    Me.Label11.Name = "Label11"
    Me.Label11.Size = New System.Drawing.Size(56, 13)
    Me.Label11.TabIndex = 19
    Me.Label11.Text = "Mircrostep"
    '
    'Label12
    '
    Me.Label12.Anchor = System.Windows.Forms.AnchorStyles.Left
    Me.Label12.AutoSize = True
    Me.Label12.Location = New System.Drawing.Point(3, 235)
    Me.Label12.Name = "Label12"
    Me.Label12.Size = New System.Drawing.Size(70, 13)
    Me.Label12.TabIndex = 20
    Me.Label12.Text = "Current in mA"
    '
    'ComboBoxMicro
    '
    Me.ComboBoxMicro.DropDownWidth = 80
    Me.ComboBoxMicro.FormattingEnabled = True
    Me.ComboBoxMicro.Items.AddRange(New Object() {"4", "8", "16", "32", "64", "128"})
    Me.ComboBoxMicro.Location = New System.Drawing.Point(123, 205)
    Me.ComboBoxMicro.Name = "ComboBoxMicro"
    Me.ComboBoxMicro.Size = New System.Drawing.Size(94, 21)
    Me.ComboBoxMicro.TabIndex = 21
    '
    'NumericUpDownCurr
    '
    Me.NumericUpDownCurr.Increment = New Decimal(New Integer() {100, 0, 0, 0})
    Me.NumericUpDownCurr.Location = New System.Drawing.Point(123, 232)
    Me.NumericUpDownCurr.Maximum = New Decimal(New Integer() {1600, 0, 0, 0})
    Me.NumericUpDownCurr.Minimum = New Decimal(New Integer() {100, 0, 0, 0})
    Me.NumericUpDownCurr.Name = "NumericUpDownCurr"
    Me.NumericUpDownCurr.Size = New System.Drawing.Size(94, 20)
    Me.NumericUpDownCurr.TabIndex = 22
    Me.NumericUpDownCurr.Value = New Decimal(New Integer() {100, 0, 0, 0})
    '
    'ButtonWEEPROM
    '
    Me.ButtonWEEPROM.Location = New System.Drawing.Point(150, 306)
    Me.ButtonWEEPROM.Name = "ButtonWEEPROM"
    Me.ButtonWEEPROM.Size = New System.Drawing.Size(127, 23)
    Me.ButtonWEEPROM.TabIndex = 12
    Me.ButtonWEEPROM.Text = "Write on EEPROM"
    Me.ButtonWEEPROM.UseVisualStyleBackColor = True
    '
    'Label3
    '
    Me.Label3.AutoSize = True
    Me.Label3.Location = New System.Drawing.Point(12, 287)
    Me.Label3.Name = "Label3"
    Me.Label3.Size = New System.Drawing.Size(0, 13)
    Me.Label3.TabIndex = 13
    '
    'ConfigEEPROM
    '
    Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
    Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
    Me.ClientSize = New System.Drawing.Size(289, 337)
    Me.Controls.Add(Me.Label3)
    Me.Controls.Add(Me.ButtonWEEPROM)
    Me.Controls.Add(Me.GroupBoxSettings)
    Me.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog
    Me.MaximizeBox = False
    Me.MinimizeBox = False
    Me.Name = "ConfigEEPROM"
    Me.Text = "EEPROM Configuration"
    Me.GroupBoxSettings.ResumeLayout(False)
    Me.TableLayoutPanel2.ResumeLayout(False)
    Me.TableLayoutPanel2.PerformLayout()
    CType(Me.NumericUpDownRes, System.ComponentModel.ISupportInitialize).EndInit()
    CType(Me.NumericUpDownMxP, System.ComponentModel.ISupportInitialize).EndInit()
    CType(Me.NumericUpDownMinS, System.ComponentModel.ISupportInitialize).EndInit()
    CType(Me.NumericUpDownMaxS, System.ComponentModel.ISupportInitialize).EndInit()
    CType(Me.NumericUpDownAccCmd, System.ComponentModel.ISupportInitialize).EndInit()
    CType(Me.NumericUpDownAccM, System.ComponentModel.ISupportInitialize).EndInit()
    CType(Me.NumericUpDownPP, System.ComponentModel.ISupportInitialize).EndInit()
    CType(Me.NumericUpDownCurr, System.ComponentModel.ISupportInitialize).EndInit()
    Me.ResumeLayout(False)
    Me.PerformLayout()

  End Sub

  Friend WithEvents GroupBoxSettings As GroupBox
    Friend WithEvents TableLayoutPanel2 As TableLayoutPanel
    Friend WithEvents NumericUpDownPP As NumericUpDown
    Friend WithEvents NumericUpDownMxP As NumericUpDown
    Friend WithEvents NumericUpDownMinS As NumericUpDown
    Friend WithEvents NumericUpDownMaxS As NumericUpDown
    Friend WithEvents NumericUpDownAccCmd As NumericUpDown
    Friend WithEvents Label1 As Label
    Friend WithEvents Label4 As Label
    Friend WithEvents Label5 As Label
    Friend WithEvents Label6 As Label
    Friend WithEvents Label7 As Label
    Friend WithEvents Label8 As Label
    Friend WithEvents NumericUpDownAccM As NumericUpDown
  Friend WithEvents ButtonWEEPROM As Button
  Friend WithEvents Label2 As Label
    Friend WithEvents CheckBoxReverse As CheckBox
    Friend WithEvents Label3 As Label
    Friend WithEvents NumericUpDownCurr As NumericUpDown
    Friend WithEvents Label10 As Label
    Friend WithEvents NumericUpDownRes As NumericUpDown
    Friend WithEvents Label11 As Label
    Friend WithEvents Label12 As Label
    Friend WithEvents ComboBoxMicro As ComboBox
End Class

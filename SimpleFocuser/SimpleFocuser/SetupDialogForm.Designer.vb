<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()>
Partial Class SetupDialogForm
    Inherits System.Windows.Forms.Form

    'Form overrides dispose to clean up the component list.
    <System.Diagnostics.DebuggerNonUserCode()>
    Protected Overrides Sub Dispose(ByVal disposing As Boolean)
        Try
            If disposing AndAlso components IsNot Nothing Then
                components.Dispose()
            End If
        Finally
            MyBase.Dispose(disposing)
        End Try
    End Sub

    'Required by the Windows Form Designer
    Private components As System.ComponentModel.IContainer

    'NOTE: The following procedure is required by the Windows Form Designer
    'It can be modified using the Windows Form Designer.  
    'Do not modify it using the code editor.
    <System.Diagnostics.DebuggerStepThrough()>
    Private Sub InitializeComponent()
        Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(SetupDialogForm))
        Me.TableLayoutPanel1 = New System.Windows.Forms.TableLayoutPanel()
        Me.OK_Button = New System.Windows.Forms.Button()
        Me.Cancel_Button = New System.Windows.Forms.Button()
        Me.ButtonEEPROM = New System.Windows.Forms.Button()
        Me.GroupBoxStepSize = New System.Windows.Forms.GroupBox()
        Me.NumericUpDownStepSize = New System.Windows.Forms.NumericUpDown()
        Me.PictureBoxTeenAstro = New System.Windows.Forms.PictureBox()
        Me.GroupBox2 = New System.Windows.Forms.GroupBox()
        Me.chkTrace = New System.Windows.Forms.CheckBox()
        Me.TextBoxPort = New System.Windows.Forms.TextBox()
        Me.Label1 = New System.Windows.Forms.Label()
        Me.TextBoxIP = New System.Windows.Forms.TextBox()
        Me.RadioButtonIP = New System.Windows.Forms.RadioButton()
        Me.RadioButtonSP = New System.Windows.Forms.RadioButton()
        Me.ComboBoxComPort = New System.Windows.Forms.ComboBox()
        Me.PictureBoxASCOM = New System.Windows.Forms.PictureBox()
        Me.TableLayoutPanel1.SuspendLayout()
        Me.GroupBoxStepSize.SuspendLayout()
        CType(Me.NumericUpDownStepSize, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.PictureBoxTeenAstro, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.GroupBox2.SuspendLayout()
        CType(Me.PictureBoxASCOM, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.SuspendLayout()
        '
        'TableLayoutPanel1
        '
        Me.TableLayoutPanel1.Anchor = CType((System.Windows.Forms.AnchorStyles.Bottom Or System.Windows.Forms.AnchorStyles.Right), System.Windows.Forms.AnchorStyles)
        Me.TableLayoutPanel1.ColumnCount = 2
        Me.TableLayoutPanel1.ColumnStyles.Add(New System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50.0!))
        Me.TableLayoutPanel1.ColumnStyles.Add(New System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50.0!))
        Me.TableLayoutPanel1.Controls.Add(Me.OK_Button, 0, 0)
        Me.TableLayoutPanel1.Controls.Add(Me.Cancel_Button, 1, 0)
        Me.TableLayoutPanel1.Location = New System.Drawing.Point(82, 290)
        Me.TableLayoutPanel1.Name = "TableLayoutPanel1"
        Me.TableLayoutPanel1.RowCount = 1
        Me.TableLayoutPanel1.RowStyles.Add(New System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50.0!))
        Me.TableLayoutPanel1.Size = New System.Drawing.Size(146, 29)
        Me.TableLayoutPanel1.TabIndex = 0
        '
        'OK_Button
        '
        Me.OK_Button.Anchor = System.Windows.Forms.AnchorStyles.None
        Me.OK_Button.Location = New System.Drawing.Point(3, 3)
        Me.OK_Button.Name = "OK_Button"
        Me.OK_Button.Size = New System.Drawing.Size(67, 23)
        Me.OK_Button.TabIndex = 0
        Me.OK_Button.Text = "OK"
        '
        'Cancel_Button
        '
        Me.Cancel_Button.Anchor = System.Windows.Forms.AnchorStyles.None
        Me.Cancel_Button.DialogResult = System.Windows.Forms.DialogResult.Cancel
        Me.Cancel_Button.Location = New System.Drawing.Point(76, 3)
        Me.Cancel_Button.Name = "Cancel_Button"
        Me.Cancel_Button.Size = New System.Drawing.Size(67, 23)
        Me.Cancel_Button.TabIndex = 1
        Me.Cancel_Button.Text = "Cancel"
        '
        'ButtonEEPROM
        '
        Me.ButtonEEPROM.Location = New System.Drawing.Point(85, 100)
        Me.ButtonEEPROM.Name = "ButtonEEPROM"
        Me.ButtonEEPROM.Size = New System.Drawing.Size(117, 21)
        Me.ButtonEEPROM.TabIndex = 10
        Me.ButtonEEPROM.Text = "Options"
        Me.ButtonEEPROM.UseVisualStyleBackColor = True
        '
        'GroupBoxStepSize
        '
        Me.GroupBoxStepSize.Controls.Add(Me.NumericUpDownStepSize)
        Me.GroupBoxStepSize.Location = New System.Drawing.Point(12, 225)
        Me.GroupBoxStepSize.Name = "GroupBoxStepSize"
        Me.GroupBoxStepSize.Size = New System.Drawing.Size(216, 51)
        Me.GroupBoxStepSize.TabIndex = 12
        Me.GroupBoxStepSize.TabStop = False
        Me.GroupBoxStepSize.Text = "Step size in micron"
        '
        'NumericUpDownStepSize
        '
        Me.NumericUpDownStepSize.Location = New System.Drawing.Point(6, 19)
        Me.NumericUpDownStepSize.Minimum = New Decimal(New Integer() {1, 0, 0, 0})
        Me.NumericUpDownStepSize.Name = "NumericUpDownStepSize"
        Me.NumericUpDownStepSize.Size = New System.Drawing.Size(90, 20)
        Me.NumericUpDownStepSize.TabIndex = 0
        Me.NumericUpDownStepSize.Value = New Decimal(New Integer() {10, 0, 0, 0})
        '
        'PictureBoxTeenAstro
        '
        Me.PictureBoxTeenAstro.Image = CType(resources.GetObject("PictureBoxTeenAstro.Image"), System.Drawing.Image)
        Me.PictureBoxTeenAstro.Location = New System.Drawing.Point(102, 13)
        Me.PictureBoxTeenAstro.Name = "PictureBoxTeenAstro"
        Me.PictureBoxTeenAstro.Size = New System.Drawing.Size(126, 69)
        Me.PictureBoxTeenAstro.TabIndex = 13
        Me.PictureBoxTeenAstro.TabStop = False
        '
        'GroupBox2
        '
        Me.GroupBox2.Controls.Add(Me.chkTrace)
        Me.GroupBox2.Controls.Add(Me.TextBoxPort)
        Me.GroupBox2.Controls.Add(Me.Label1)
        Me.GroupBox2.Controls.Add(Me.TextBoxIP)
        Me.GroupBox2.Controls.Add(Me.ButtonEEPROM)
        Me.GroupBox2.Controls.Add(Me.RadioButtonIP)
        Me.GroupBox2.Controls.Add(Me.RadioButtonSP)
        Me.GroupBox2.Controls.Add(Me.ComboBoxComPort)
        Me.GroupBox2.Location = New System.Drawing.Point(12, 88)
        Me.GroupBox2.Name = "GroupBox2"
        Me.GroupBox2.Size = New System.Drawing.Size(216, 130)
        Me.GroupBox2.TabIndex = 14
        Me.GroupBox2.TabStop = False
        Me.GroupBox2.Text = "Connection Interface"
        '
        'chkTrace
        '
        Me.chkTrace.AutoSize = True
        Me.chkTrace.Location = New System.Drawing.Point(9, 104)
        Me.chkTrace.Name = "chkTrace"
        Me.chkTrace.Size = New System.Drawing.Size(69, 17)
        Me.chkTrace.TabIndex = 8
        Me.chkTrace.Text = "Trace on"
        Me.chkTrace.UseVisualStyleBackColor = True
        '
        'TextBoxPort
        '
        Me.TextBoxPort.Enabled = False
        Me.TextBoxPort.Location = New System.Drawing.Point(85, 74)
        Me.TextBoxPort.Name = "TextBoxPort"
        Me.TextBoxPort.Size = New System.Drawing.Size(52, 20)
        Me.TextBoxPort.TabIndex = 15
        Me.TextBoxPort.Text = "9999"
        '
        'Label1
        '
        Me.Label1.AutoSize = True
        Me.Label1.Location = New System.Drawing.Point(27, 74)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(26, 13)
        Me.Label1.TabIndex = 14
        Me.Label1.Text = "Port"
        '
        'TextBoxIP
        '
        Me.TextBoxIP.Enabled = False
        Me.TextBoxIP.Location = New System.Drawing.Point(85, 48)
        Me.TextBoxIP.Name = "TextBoxIP"
        Me.TextBoxIP.Size = New System.Drawing.Size(117, 20)
        Me.TextBoxIP.TabIndex = 13
        '
        'RadioButtonIP
        '
        Me.RadioButtonIP.AutoSize = True
        Me.RadioButtonIP.Location = New System.Drawing.Point(9, 49)
        Me.RadioButtonIP.Name = "RadioButtonIP"
        Me.RadioButtonIP.Size = New System.Drawing.Size(70, 17)
        Me.RadioButtonIP.TabIndex = 12
        Me.RadioButtonIP.Text = "IP Adress"
        Me.RadioButtonIP.UseVisualStyleBackColor = True
        '
        'RadioButtonSP
        '
        Me.RadioButtonSP.AutoSize = True
        Me.RadioButtonSP.Checked = True
        Me.RadioButtonSP.Location = New System.Drawing.Point(9, 22)
        Me.RadioButtonSP.Name = "RadioButtonSP"
        Me.RadioButtonSP.Size = New System.Drawing.Size(73, 17)
        Me.RadioButtonSP.TabIndex = 11
        Me.RadioButtonSP.TabStop = True
        Me.RadioButtonSP.Text = "Serial Port"
        Me.RadioButtonSP.UseVisualStyleBackColor = True
        '
        'ComboBoxComPort
        '
        Me.ComboBoxComPort.FormattingEnabled = True
        Me.ComboBoxComPort.Location = New System.Drawing.Point(85, 21)
        Me.ComboBoxComPort.Name = "ComboBoxComPort"
        Me.ComboBoxComPort.Size = New System.Drawing.Size(117, 21)
        Me.ComboBoxComPort.TabIndex = 9
        '
        'PictureBoxASCOM
        '
        Me.PictureBoxASCOM.Image = Global.ASCOM.TeenAstro.My.Resources.Resources.ASCOM
        Me.PictureBoxASCOM.Location = New System.Drawing.Point(13, 13)
        Me.PictureBoxASCOM.Name = "PictureBoxASCOM"
        Me.PictureBoxASCOM.Size = New System.Drawing.Size(48, 56)
        Me.PictureBoxASCOM.SizeMode = System.Windows.Forms.PictureBoxSizeMode.AutoSize
        Me.PictureBoxASCOM.TabIndex = 15
        Me.PictureBoxASCOM.TabStop = False
        '
        'SetupDialogForm
        '
        Me.AcceptButton = Me.OK_Button
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.CancelButton = Me.Cancel_Button
        Me.ClientSize = New System.Drawing.Size(240, 331)
        Me.Controls.Add(Me.PictureBoxASCOM)
        Me.Controls.Add(Me.GroupBox2)
        Me.Controls.Add(Me.PictureBoxTeenAstro)
        Me.Controls.Add(Me.GroupBoxStepSize)
        Me.Controls.Add(Me.TableLayoutPanel1)
        Me.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog
        Me.MaximizeBox = False
        Me.MinimizeBox = False
        Me.Name = "SetupDialogForm"
        Me.ShowInTaskbar = False
        Me.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen
        Me.Text = "TeenAstro Focuser"
        Me.TopMost = True
        Me.TableLayoutPanel1.ResumeLayout(False)
        Me.GroupBoxStepSize.ResumeLayout(False)
        CType(Me.NumericUpDownStepSize, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.PictureBoxTeenAstro, System.ComponentModel.ISupportInitialize).EndInit()
        Me.GroupBox2.ResumeLayout(False)
        Me.GroupBox2.PerformLayout()
        CType(Me.PictureBoxASCOM, System.ComponentModel.ISupportInitialize).EndInit()
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    Friend WithEvents TableLayoutPanel1 As System.Windows.Forms.TableLayoutPanel
    Friend WithEvents OK_Button As System.Windows.Forms.Button
    Friend WithEvents Cancel_Button As System.Windows.Forms.Button
    Friend WithEvents ButtonEEPROM As Button
    Friend WithEvents GroupBoxStepSize As GroupBox
    Friend WithEvents NumericUpDownStepSize As NumericUpDown
    Friend WithEvents PictureBoxTeenAstro As PictureBox
    Friend WithEvents GroupBox2 As GroupBox
    Friend WithEvents chkTrace As CheckBox
    Friend WithEvents TextBoxPort As TextBox
    Friend WithEvents Label1 As Label
    Friend WithEvents TextBoxIP As TextBox
    Friend WithEvents RadioButtonIP As RadioButton
    Friend WithEvents RadioButtonSP As RadioButton
    Friend WithEvents ComboBoxComPort As ComboBox
    Friend WithEvents PictureBoxASCOM As PictureBox
End Class

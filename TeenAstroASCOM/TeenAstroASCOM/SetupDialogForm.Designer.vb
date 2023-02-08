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
    Me.chkTrace = New System.Windows.Forms.CheckBox()
    Me.ComboBoxComPort = New System.Windows.Forms.ComboBox()
    Me.GroupBox1 = New System.Windows.Forms.GroupBox()
    Me.Button1 = New System.Windows.Forms.Button()
    Me.TextBoxPort = New System.Windows.Forms.TextBox()
    Me.Label1 = New System.Windows.Forms.Label()
    Me.TextBoxIP = New System.Windows.Forms.TextBox()
    Me.RadioButtonIP = New System.Windows.Forms.RadioButton()
    Me.RadioButtonSP = New System.Windows.Forms.RadioButton()
    Me.PictureBoxTeenAstro = New System.Windows.Forms.PictureBox()
    Me.PictureBox1 = New System.Windows.Forms.PictureBox()
        Me.TableLayoutPanel1.SuspendLayout()
        Me.GroupBox1.SuspendLayout()
        CType(Me.PictureBoxTeenAstro, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.PictureBox1, System.ComponentModel.ISupportInitialize).BeginInit()
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
        Me.TableLayoutPanel1.Location = New System.Drawing.Point(98, 224)
        Me.TableLayoutPanel1.Name = "TableLayoutPanel1"
        Me.TableLayoutPanel1.RowCount = 1
        Me.TableLayoutPanel1.RowStyles.Add(New System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50.0!))
        Me.TableLayoutPanel1.Size = New System.Drawing.Size(130, 29)
        Me.TableLayoutPanel1.TabIndex = 0
        '
        'OK_Button
        '
        Me.OK_Button.Anchor = System.Windows.Forms.AnchorStyles.None
        Me.OK_Button.Location = New System.Drawing.Point(6, 3)
        Me.OK_Button.Name = "OK_Button"
        Me.OK_Button.Size = New System.Drawing.Size(53, 23)
        Me.OK_Button.TabIndex = 0
        Me.OK_Button.Text = "OK"
        '
        'Cancel_Button
        '
        Me.Cancel_Button.Anchor = System.Windows.Forms.AnchorStyles.None
        Me.Cancel_Button.DialogResult = System.Windows.Forms.DialogResult.Cancel
        Me.Cancel_Button.Location = New System.Drawing.Point(71, 3)
        Me.Cancel_Button.Name = "Cancel_Button"
        Me.Cancel_Button.Size = New System.Drawing.Size(53, 23)
        Me.Cancel_Button.TabIndex = 1
        Me.Cancel_Button.Text = "Cancel"
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
        'ComboBoxComPort
        '
        Me.ComboBoxComPort.FormattingEnabled = True
        Me.ComboBoxComPort.Location = New System.Drawing.Point(85, 21)
        Me.ComboBoxComPort.Name = "ComboBoxComPort"
        Me.ComboBoxComPort.Size = New System.Drawing.Size(117, 21)
        Me.ComboBoxComPort.TabIndex = 9
        '
        'GroupBox1
        '
        Me.GroupBox1.Controls.Add(Me.Button1)
        Me.GroupBox1.Controls.Add(Me.chkTrace)
        Me.GroupBox1.Controls.Add(Me.TextBoxPort)
        Me.GroupBox1.Controls.Add(Me.Label1)
        Me.GroupBox1.Controls.Add(Me.TextBoxIP)
        Me.GroupBox1.Controls.Add(Me.RadioButtonIP)
        Me.GroupBox1.Controls.Add(Me.RadioButtonSP)
        Me.GroupBox1.Controls.Add(Me.ComboBoxComPort)
        Me.GroupBox1.Location = New System.Drawing.Point(12, 85)
        Me.GroupBox1.Name = "GroupBox1"
        Me.GroupBox1.Size = New System.Drawing.Size(216, 130)
        Me.GroupBox1.TabIndex = 11
        Me.GroupBox1.TabStop = False
        Me.GroupBox1.Text = "Connection Interface"
        '
        'Button1
        '
        Me.Button1.Location = New System.Drawing.Point(84, 100)
        Me.Button1.Name = "Button1"
        Me.Button1.Size = New System.Drawing.Size(118, 22)
        Me.Button1.TabIndex = 16
        Me.Button1.Text = "Options"
        Me.Button1.UseVisualStyleBackColor = True
        Me.Button1.Visible = False
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
        'PictureBoxTeenAstro
        '
        Me.PictureBoxTeenAstro.Image = CType(resources.GetObject("PictureBoxTeenAstro.Image"), System.Drawing.Image)
        Me.PictureBoxTeenAstro.Location = New System.Drawing.Point(97, 10)
        Me.PictureBoxTeenAstro.Name = "PictureBoxTeenAstro"
        Me.PictureBoxTeenAstro.Size = New System.Drawing.Size(130, 69)
        Me.PictureBoxTeenAstro.TabIndex = 14
        Me.PictureBoxTeenAstro.TabStop = False
        '
        'PictureBox1
        '
        Me.PictureBox1.Image = Global.ASCOM.TeenAstro.My.Resources.Resources.ASCOM
        Me.PictureBox1.Location = New System.Drawing.Point(12, 10)
        Me.PictureBox1.Name = "PictureBox1"
        Me.PictureBox1.Size = New System.Drawing.Size(48, 56)
        Me.PictureBox1.SizeMode = System.Windows.Forms.PictureBoxSizeMode.AutoSize
        Me.PictureBox1.TabIndex = 15
        Me.PictureBox1.TabStop = False
        '
        'SetupDialogForm
        '
        Me.AcceptButton = Me.OK_Button
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.CancelButton = Me.Cancel_Button
        Me.ClientSize = New System.Drawing.Size(239, 264)
        Me.Controls.Add(Me.PictureBox1)
        Me.Controls.Add(Me.PictureBoxTeenAstro)
        Me.Controls.Add(Me.GroupBox1)
        Me.Controls.Add(Me.TableLayoutPanel1)
        Me.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog
        Me.MaximizeBox = False
        Me.MinimizeBox = False
        Me.Name = "SetupDialogForm"
        Me.ShowInTaskbar = False
        Me.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen
        Me.Text = "TeenAstro Telescope"
        Me.TopMost = True
        Me.TableLayoutPanel1.ResumeLayout(False)
        Me.GroupBox1.ResumeLayout(False)
        Me.GroupBox1.PerformLayout()
        CType(Me.PictureBoxTeenAstro, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.PictureBox1, System.ComponentModel.ISupportInitialize).EndInit()
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    Friend WithEvents TableLayoutPanel1 As System.Windows.Forms.TableLayoutPanel
  Friend WithEvents OK_Button As System.Windows.Forms.Button
  Friend WithEvents Cancel_Button As System.Windows.Forms.Button
  Friend WithEvents chkTrace As System.Windows.Forms.CheckBox
  Friend WithEvents ComboBoxComPort As System.Windows.Forms.ComboBox
  Friend WithEvents GroupBox1 As GroupBox
  Friend WithEvents TextBoxPort As TextBox
  Friend WithEvents Label1 As Label
  Friend WithEvents TextBoxIP As TextBox
  Friend WithEvents RadioButtonIP As RadioButton
  Friend WithEvents RadioButtonSP As RadioButton
  Friend WithEvents Button1 As Button
  Friend WithEvents PictureBoxTeenAstro As PictureBox
  Friend WithEvents PictureBox1 As PictureBox
End Class

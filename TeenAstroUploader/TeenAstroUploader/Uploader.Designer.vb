<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()>
Partial Class Uploader
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
    Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(Uploader))
    Me.ComboBoxPCBMainUnitT = New System.Windows.Forms.ComboBox()
    Me.Label4 = New System.Windows.Forms.Label()
    Me.TabPage3 = New System.Windows.Forms.TabPage()
    Me.TabPage2 = New System.Windows.Forms.TabPage()
    Me.ButtonUploadF = New System.Windows.Forms.Button()
    Me.ComboBoxPCBMainUnitF = New System.Windows.Forms.ComboBox()
    Me.Label2 = New System.Windows.Forms.Label()
    Me.TabPage1 = New System.Windows.Forms.TabPage()
    Me.ButtonUploadT = New System.Windows.Forms.Button()
    Me.TabControlFirmware = New System.Windows.Forms.TabControl()
    Me.TabPage2.SuspendLayout()
    Me.TabPage1.SuspendLayout()
    Me.TabControlFirmware.SuspendLayout()
    Me.SuspendLayout()
    '
    'ComboBoxPCBMainUnitT
    '
    Me.ComboBoxPCBMainUnitT.FormattingEnabled = True
    Me.ComboBoxPCBMainUnitT.Items.AddRange(New Object() {"2.2 TMC260", "2.3 TMC260", "2.4 TMC2130", "2.4 TMC5160"})
    Me.ComboBoxPCBMainUnitT.Location = New System.Drawing.Point(71, 17)
    Me.ComboBoxPCBMainUnitT.Name = "ComboBoxPCBMainUnitT"
    Me.ComboBoxPCBMainUnitT.Size = New System.Drawing.Size(110, 21)
    Me.ComboBoxPCBMainUnitT.TabIndex = 3
    '
    'Label4
    '
    Me.Label4.AutoSize = True
    Me.Label4.Location = New System.Drawing.Point(6, 20)
    Me.Label4.Name = "Label4"
    Me.Label4.Size = New System.Drawing.Size(59, 13)
    Me.Label4.TabIndex = 9
    Me.Label4.Text = "PCB Board"
    '
    'TabPage3
    '
    Me.TabPage3.Location = New System.Drawing.Point(4, 22)
    Me.TabPage3.Name = "TabPage3"
    Me.TabPage3.Size = New System.Drawing.Size(312, 52)
    Me.TabPage3.TabIndex = 2
    Me.TabPage3.Text = "Hand controler"
    Me.TabPage3.UseVisualStyleBackColor = True
    '
    'TabPage2
    '
    Me.TabPage2.Controls.Add(Me.ButtonUploadF)
    Me.TabPage2.Controls.Add(Me.ComboBoxPCBMainUnitF)
    Me.TabPage2.Controls.Add(Me.Label2)
    Me.TabPage2.Location = New System.Drawing.Point(4, 22)
    Me.TabPage2.Name = "TabPage2"
    Me.TabPage2.Padding = New System.Windows.Forms.Padding(3)
    Me.TabPage2.Size = New System.Drawing.Size(312, 52)
    Me.TabPage2.TabIndex = 1
    Me.TabPage2.Text = "Focuser"
    Me.TabPage2.UseVisualStyleBackColor = True
    '
    'ButtonUploadF
    '
    Me.ButtonUploadF.Location = New System.Drawing.Point(187, 15)
    Me.ButtonUploadF.Name = "ButtonUploadF"
    Me.ButtonUploadF.Size = New System.Drawing.Size(119, 23)
    Me.ButtonUploadF.TabIndex = 18
    Me.ButtonUploadF.Text = "Upload!"
    Me.ButtonUploadF.UseVisualStyleBackColor = True
    '
    'ComboBoxPCBMainUnitF
    '
    Me.ComboBoxPCBMainUnitF.FormattingEnabled = True
    Me.ComboBoxPCBMainUnitF.Items.AddRange(New Object() {"2.2", "2.3"})
    Me.ComboBoxPCBMainUnitF.Location = New System.Drawing.Point(71, 17)
    Me.ComboBoxPCBMainUnitF.Name = "ComboBoxPCBMainUnitF"
    Me.ComboBoxPCBMainUnitF.Size = New System.Drawing.Size(110, 21)
    Me.ComboBoxPCBMainUnitF.TabIndex = 13
    '
    'Label2
    '
    Me.Label2.AutoSize = True
    Me.Label2.Location = New System.Drawing.Point(6, 20)
    Me.Label2.Name = "Label2"
    Me.Label2.Size = New System.Drawing.Size(59, 13)
    Me.Label2.TabIndex = 14
    Me.Label2.Text = "PCB Board"
    '
    'TabPage1
    '
    Me.TabPage1.Controls.Add(Me.ButtonUploadT)
    Me.TabPage1.Controls.Add(Me.ComboBoxPCBMainUnitT)
    Me.TabPage1.Controls.Add(Me.Label4)
    Me.TabPage1.Location = New System.Drawing.Point(4, 22)
    Me.TabPage1.Name = "TabPage1"
    Me.TabPage1.Padding = New System.Windows.Forms.Padding(3)
    Me.TabPage1.Size = New System.Drawing.Size(312, 52)
    Me.TabPage1.TabIndex = 0
    Me.TabPage1.Text = "Telescope"
    Me.TabPage1.UseVisualStyleBackColor = True
    '
    'ButtonUploadT
    '
    Me.ButtonUploadT.Location = New System.Drawing.Point(187, 15)
    Me.ButtonUploadT.Name = "ButtonUploadT"
    Me.ButtonUploadT.Size = New System.Drawing.Size(119, 23)
    Me.ButtonUploadT.TabIndex = 12
    Me.ButtonUploadT.Text = "Upload!"
    Me.ButtonUploadT.UseVisualStyleBackColor = True
    '
    'TabControlFirmware
    '
    Me.TabControlFirmware.Controls.Add(Me.TabPage1)
    Me.TabControlFirmware.Controls.Add(Me.TabPage2)
    Me.TabControlFirmware.Controls.Add(Me.TabPage3)
    Me.TabControlFirmware.Location = New System.Drawing.Point(12, 12)
    Me.TabControlFirmware.Name = "TabControlFirmware"
    Me.TabControlFirmware.SelectedIndex = 0
    Me.TabControlFirmware.Size = New System.Drawing.Size(320, 78)
    Me.TabControlFirmware.TabIndex = 14
    '
    'Uploader
    '
    Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
    Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
    Me.ClientSize = New System.Drawing.Size(344, 103)
    Me.Controls.Add(Me.TabControlFirmware)
    Me.Icon = CType(resources.GetObject("$this.Icon"), System.Drawing.Icon)
    Me.MaximizeBox = False
    Me.Name = "Uploader"
    Me.Text = "TeenAstro Firmware Uploader"
    Me.TopMost = True
    Me.TabPage2.ResumeLayout(False)
    Me.TabPage2.PerformLayout()
    Me.TabPage1.ResumeLayout(False)
    Me.TabPage1.PerformLayout()
    Me.TabControlFirmware.ResumeLayout(False)
    Me.ResumeLayout(False)

  End Sub

  Friend WithEvents ComboBoxPCBMainUnitT As ComboBox
  Friend WithEvents Label4 As Label
  Friend WithEvents TabPage3 As TabPage
  Friend WithEvents TabPage2 As TabPage
  Friend WithEvents TabPage1 As TabPage
  Friend WithEvents ButtonUploadT As Button
  Friend WithEvents TabControlFirmware As TabControl
  Friend WithEvents ButtonUploadF As Button
  Friend WithEvents ComboBoxPCBMainUnitF As ComboBox
  Friend WithEvents Label2 As Label
End Class

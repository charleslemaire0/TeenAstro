﻿<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()>
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
    Me.ButtonWIFISHC = New System.Windows.Forms.Button()
    Me.TextBoxIP = New System.Windows.Forms.TextBox()
    Me.Label5 = New System.Windows.Forms.Label()
    Me.ComboBoxCOMSHC = New System.Windows.Forms.ComboBox()
    Me.Label3 = New System.Windows.Forms.Label()
    Me.ButtonUploadSHC = New System.Windows.Forms.Button()
    Me.ComboBoxPCBSHC = New System.Windows.Forms.ComboBox()
    Me.Label1 = New System.Windows.Forms.Label()
    Me.TabPage2 = New System.Windows.Forms.TabPage()
    Me.ButtonUploadF = New System.Windows.Forms.Button()
    Me.ComboBoxPCBMainUnitF = New System.Windows.Forms.ComboBox()
    Me.Label2 = New System.Windows.Forms.Label()
    Me.TabPage1 = New System.Windows.Forms.TabPage()
    Me.ButtonUploadT = New System.Windows.Forms.Button()
    Me.TabControlFirmware = New System.Windows.Forms.TabControl()
    Me.Label6 = New System.Windows.Forms.Label()
    Me.ComboBoxFirmwareVersion = New System.Windows.Forms.ComboBox()
    Me.TabPage3.SuspendLayout()
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
    Me.TabPage3.Controls.Add(Me.ButtonWIFISHC)
    Me.TabPage3.Controls.Add(Me.TextBoxIP)
    Me.TabPage3.Controls.Add(Me.Label5)
    Me.TabPage3.Controls.Add(Me.ComboBoxCOMSHC)
    Me.TabPage3.Controls.Add(Me.Label3)
    Me.TabPage3.Controls.Add(Me.ButtonUploadSHC)
    Me.TabPage3.Controls.Add(Me.ComboBoxPCBSHC)
    Me.TabPage3.Controls.Add(Me.Label1)
    Me.TabPage3.Location = New System.Drawing.Point(4, 22)
    Me.TabPage3.Name = "TabPage3"
    Me.TabPage3.Size = New System.Drawing.Size(364, 155)
    Me.TabPage3.TabIndex = 2
    Me.TabPage3.Text = "Hand controler"
    Me.TabPage3.UseVisualStyleBackColor = True
    '
    'ButtonWIFISHC
    '
    Me.ButtonWIFISHC.Location = New System.Drawing.Point(187, 76)
    Me.ButtonWIFISHC.Name = "ButtonWIFISHC"
    Me.ButtonWIFISHC.Size = New System.Drawing.Size(119, 23)
    Me.ButtonWIFISHC.TabIndex = 28
    Me.ButtonWIFISHC.Text = "Upload over WIFI!"
    Me.ButtonWIFISHC.UseVisualStyleBackColor = True
    '
    'TextBoxIP
    '
    Me.TextBoxIP.Location = New System.Drawing.Point(71, 78)
    Me.TextBoxIP.Name = "TextBoxIP"
    Me.TextBoxIP.Size = New System.Drawing.Size(110, 20)
    Me.TextBoxIP.TabIndex = 25
    '
    'Label5
    '
    Me.Label5.AutoSize = True
    Me.Label5.Location = New System.Drawing.Point(6, 81)
    Me.Label5.Name = "Label5"
    Me.Label5.Size = New System.Drawing.Size(52, 13)
    Me.Label5.TabIndex = 24
    Me.Label5.Text = "IP Adress"
    '
    'ComboBoxCOMSHC
    '
    Me.ComboBoxCOMSHC.FormattingEnabled = True
    Me.ComboBoxCOMSHC.Location = New System.Drawing.Point(71, 49)
    Me.ComboBoxCOMSHC.Name = "ComboBoxCOMSHC"
    Me.ComboBoxCOMSHC.Size = New System.Drawing.Size(110, 21)
    Me.ComboBoxCOMSHC.TabIndex = 23
    '
    'Label3
    '
    Me.Label3.AutoSize = True
    Me.Label3.Location = New System.Drawing.Point(6, 52)
    Me.Label3.Name = "Label3"
    Me.Label3.Size = New System.Drawing.Size(47, 13)
    Me.Label3.TabIndex = 22
    Me.Label3.Text = "ComPort"
    '
    'ButtonUploadSHC
    '
    Me.ButtonUploadSHC.Location = New System.Drawing.Point(187, 47)
    Me.ButtonUploadSHC.Name = "ButtonUploadSHC"
    Me.ButtonUploadSHC.Size = New System.Drawing.Size(119, 23)
    Me.ButtonUploadSHC.TabIndex = 21
    Me.ButtonUploadSHC.Text = "Upload over COM!"
    Me.ButtonUploadSHC.UseVisualStyleBackColor = True
    '
    'ComboBoxPCBSHC
    '
    Me.ComboBoxPCBSHC.FormattingEnabled = True
    Me.ComboBoxPCBSHC.Items.AddRange(New Object() {"0.x"})
    Me.ComboBoxPCBSHC.Location = New System.Drawing.Point(71, 17)
    Me.ComboBoxPCBSHC.Name = "ComboBoxPCBSHC"
    Me.ComboBoxPCBSHC.Size = New System.Drawing.Size(110, 21)
    Me.ComboBoxPCBSHC.TabIndex = 19
    '
    'Label1
    '
    Me.Label1.AutoSize = True
    Me.Label1.Location = New System.Drawing.Point(6, 20)
    Me.Label1.Name = "Label1"
    Me.Label1.Size = New System.Drawing.Size(59, 13)
    Me.Label1.TabIndex = 20
    Me.Label1.Text = "PCB Board"
    '
    'TabPage2
    '
    Me.TabPage2.Controls.Add(Me.ButtonUploadF)
    Me.TabPage2.Controls.Add(Me.ComboBoxPCBMainUnitF)
    Me.TabPage2.Controls.Add(Me.Label2)
    Me.TabPage2.Location = New System.Drawing.Point(4, 22)
    Me.TabPage2.Name = "TabPage2"
    Me.TabPage2.Padding = New System.Windows.Forms.Padding(3)
    Me.TabPage2.Size = New System.Drawing.Size(364, 155)
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
    Me.ComboBoxPCBMainUnitF.Items.AddRange(New Object() {"2.2", "2.3", "2.4 TMC2130", "2.4 TMC5160"})
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
    Me.TabPage1.Size = New System.Drawing.Size(312, 155)
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
    Me.TabControlFirmware.Location = New System.Drawing.Point(12, 52)
    Me.TabControlFirmware.Name = "TabControlFirmware"
    Me.TabControlFirmware.SelectedIndex = 0
    Me.TabControlFirmware.Size = New System.Drawing.Size(372, 181)
    Me.TabControlFirmware.TabIndex = 14
    '
    'Label6
    '
    Me.Label6.AutoSize = True
    Me.Label6.Location = New System.Drawing.Point(13, 20)
    Me.Label6.Name = "Label6"
    Me.Label6.Size = New System.Drawing.Size(87, 13)
    Me.Label6.TabIndex = 15
    Me.Label6.Text = "Firmware Version"
    '
    'ComboBoxFirmwareVersion
    '
    Me.ComboBoxFirmwareVersion.FormattingEnabled = True
    Me.ComboBoxFirmwareVersion.Items.AddRange(New Object() {"1.0", "1.1"})
    Me.ComboBoxFirmwareVersion.Location = New System.Drawing.Point(124, 17)
    Me.ComboBoxFirmwareVersion.Name = "ComboBoxFirmwareVersion"
    Me.ComboBoxFirmwareVersion.Size = New System.Drawing.Size(143, 21)
    Me.ComboBoxFirmwareVersion.TabIndex = 13
    '
    'Uploader
    '
    Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
    Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
    Me.ClientSize = New System.Drawing.Size(390, 240)
    Me.Controls.Add(Me.ComboBoxFirmwareVersion)
    Me.Controls.Add(Me.Label6)
    Me.Controls.Add(Me.TabControlFirmware)
    Me.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle
    Me.Icon = CType(resources.GetObject("$this.Icon"), System.Drawing.Icon)
    Me.MaximizeBox = False
    Me.Name = "Uploader"
    Me.Text = "TeenAstro Firmware Uploader 1.1.0"
    Me.TopMost = True
    Me.TabPage3.ResumeLayout(False)
    Me.TabPage3.PerformLayout()
    Me.TabPage2.ResumeLayout(False)
    Me.TabPage2.PerformLayout()
    Me.TabPage1.ResumeLayout(False)
    Me.TabPage1.PerformLayout()
    Me.TabControlFirmware.ResumeLayout(False)
    Me.ResumeLayout(False)
    Me.PerformLayout()

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
  Friend WithEvents ButtonWIFISHC As Button
  Friend WithEvents TextBoxIP As TextBox
  Friend WithEvents Label5 As Label
  Friend WithEvents ComboBoxCOMSHC As ComboBox
  Friend WithEvents Label3 As Label
  Friend WithEvents ButtonUploadSHC As Button
  Friend WithEvents ComboBoxPCBSHC As ComboBox
  Friend WithEvents Label1 As Label
  Friend WithEvents Label6 As Label
  Friend WithEvents ComboBoxFirmwareVersion As ComboBox
End Class

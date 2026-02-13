namespace ASCOM.TeenAstro.Telescope
{
  partial class SetupDialogForm
  {
    /// <summary>
    /// Required designer variable.
    /// </summary>
    private System.ComponentModel.IContainer components = null;

    /// <summary>
    /// Clean up any resources being used.
    /// </summary>
    /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
    protected override void Dispose(bool disposing)
    {
      if (disposing && (components != null))
      {
        components.Dispose();
      }
      base.Dispose(disposing);
    }

    #region Windows Form Designer generated code

    /// <summary>
    /// Required method for Designer support - do not modify
    /// the contents of this method with the code editor.
    /// </summary>
    private void InitializeComponent()
    {
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(SetupDialogForm));
      this.picASCOM = new System.Windows.Forms.PictureBox();
      this.PictureBoxTeenAstro = new System.Windows.Forms.PictureBox();
      this.GroupBox1 = new System.Windows.Forms.GroupBox();
      this.Button1 = new System.Windows.Forms.Button();
      this.chkTrace = new System.Windows.Forms.CheckBox();
      this.TextBoxPort = new System.Windows.Forms.TextBox();
      this.Label1 = new System.Windows.Forms.Label();
      this.TextBoxIP = new System.Windows.Forms.TextBox();
      this.RadioButtonIP = new System.Windows.Forms.RadioButton();
      this.RadioButtonSP = new System.Windows.Forms.RadioButton();
      this.comboBoxComPort = new System.Windows.Forms.ComboBox();
      this.TableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
      this.OK_Button = new System.Windows.Forms.Button();
      this.Cancel_Button = new System.Windows.Forms.Button();
      ((System.ComponentModel.ISupportInitialize)(this.picASCOM)).BeginInit();
      ((System.ComponentModel.ISupportInitialize)(this.PictureBoxTeenAstro)).BeginInit();
      this.GroupBox1.SuspendLayout();
      this.TableLayoutPanel1.SuspendLayout();
      this.SuspendLayout();
      // 
      // picASCOM
      // 
      this.picASCOM.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
      this.picASCOM.Cursor = System.Windows.Forms.Cursors.Hand;
      this.picASCOM.Image = ((System.Drawing.Image)(resources.GetObject("picASCOM.Image")));
      this.picASCOM.Location = new System.Drawing.Point(11, 12);
      this.picASCOM.Name = "picASCOM";
      this.picASCOM.Size = new System.Drawing.Size(48, 56);
      this.picASCOM.SizeMode = System.Windows.Forms.PictureBoxSizeMode.AutoSize;
      this.picASCOM.TabIndex = 3;
      this.picASCOM.TabStop = false;
      this.picASCOM.Click += new System.EventHandler(this.BrowseToAscom);
      this.picASCOM.DoubleClick += new System.EventHandler(this.BrowseToAscom);
      // 
      // PictureBoxTeenAstro
      // 
      this.PictureBoxTeenAstro.Image = ((System.Drawing.Image)(resources.GetObject("PictureBoxTeenAstro.Image")));
      this.PictureBoxTeenAstro.Location = new System.Drawing.Point(116, 12);
      this.PictureBoxTeenAstro.Name = "PictureBoxTeenAstro";
      this.PictureBoxTeenAstro.Size = new System.Drawing.Size(130, 69);
      this.PictureBoxTeenAstro.TabIndex = 15;
      this.PictureBoxTeenAstro.TabStop = false;
      this.PictureBoxTeenAstro.Click += new System.EventHandler(this.PictureBoxTeenAstro_Click);
      // 
      // GroupBox1
      // 
      this.GroupBox1.Controls.Add(this.Button1);
      this.GroupBox1.Controls.Add(this.chkTrace);
      this.GroupBox1.Controls.Add(this.TextBoxPort);
      this.GroupBox1.Controls.Add(this.Label1);
      this.GroupBox1.Controls.Add(this.TextBoxIP);
      this.GroupBox1.Controls.Add(this.RadioButtonIP);
      this.GroupBox1.Controls.Add(this.RadioButtonSP);
      this.GroupBox1.Controls.Add(this.comboBoxComPort);
      this.GroupBox1.Location = new System.Drawing.Point(28, 83);
      this.GroupBox1.Name = "GroupBox1";
      this.GroupBox1.Size = new System.Drawing.Size(216, 130);
      this.GroupBox1.TabIndex = 16;
      this.GroupBox1.TabStop = false;
      this.GroupBox1.Text = "Connection Interface";
      // 
      // Button1
      // 
      this.Button1.Location = new System.Drawing.Point(84, 100);
      this.Button1.Name = "Button1";
      this.Button1.Size = new System.Drawing.Size(118, 22);
      this.Button1.TabIndex = 16;
      this.Button1.Text = "Options";
      this.Button1.UseVisualStyleBackColor = true;
      this.Button1.Visible = false;
      // 
      // chkTrace
      // 
      this.chkTrace.AutoSize = true;
      this.chkTrace.Location = new System.Drawing.Point(9, 104);
      this.chkTrace.Name = "chkTrace";
      this.chkTrace.Size = new System.Drawing.Size(69, 17);
      this.chkTrace.TabIndex = 8;
      this.chkTrace.Text = "Trace on";
      this.chkTrace.UseVisualStyleBackColor = true;
      this.chkTrace.CheckedChanged += new System.EventHandler(this.chkTrace_CheckedChanged);
      // 
      // TextBoxPort
      // 
      this.TextBoxPort.Enabled = false;
      this.TextBoxPort.Location = new System.Drawing.Point(85, 74);
      this.TextBoxPort.Name = "TextBoxPort";
      this.TextBoxPort.Size = new System.Drawing.Size(52, 20);
      this.TextBoxPort.TabIndex = 15;
      this.TextBoxPort.Text = "9999";
      this.TextBoxPort.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.TextBoxPort_KeyPress);
      // 
      // Label1
      // 
      this.Label1.AutoSize = true;
      this.Label1.Location = new System.Drawing.Point(27, 74);
      this.Label1.Name = "Label1";
      this.Label1.Size = new System.Drawing.Size(26, 13);
      this.Label1.TabIndex = 14;
      this.Label1.Text = "Port";
      // 
      // TextBoxIP
      // 
      this.TextBoxIP.Enabled = false;
      this.TextBoxIP.Location = new System.Drawing.Point(85, 48);
      this.TextBoxIP.Name = "TextBoxIP";
      this.TextBoxIP.Size = new System.Drawing.Size(117, 20);
      this.TextBoxIP.TabIndex = 13;
      // 
      // RadioButtonIP
      // 
      this.RadioButtonIP.AutoSize = true;
      this.RadioButtonIP.Location = new System.Drawing.Point(9, 49);
      this.RadioButtonIP.Name = "RadioButtonIP";
      this.RadioButtonIP.Size = new System.Drawing.Size(70, 17);
      this.RadioButtonIP.TabIndex = 12;
      this.RadioButtonIP.Text = "IP Adress";
      this.RadioButtonIP.UseVisualStyleBackColor = true;
      this.RadioButtonIP.CheckedChanged += new System.EventHandler(this.RadioButtonIP_CheckedChanged);
      // 
      // RadioButtonSP
      // 
      this.RadioButtonSP.AutoSize = true;
      this.RadioButtonSP.Checked = true;
      this.RadioButtonSP.Location = new System.Drawing.Point(9, 22);
      this.RadioButtonSP.Name = "RadioButtonSP";
      this.RadioButtonSP.Size = new System.Drawing.Size(73, 17);
      this.RadioButtonSP.TabIndex = 11;
      this.RadioButtonSP.TabStop = true;
      this.RadioButtonSP.Text = "Serial Port";
      this.RadioButtonSP.UseVisualStyleBackColor = true;
      this.RadioButtonSP.CheckedChanged += new System.EventHandler(this.RadioButtonSP_CheckedChanged);
      // 
      // comboBoxComPort
      // 
      this.comboBoxComPort.FormattingEnabled = true;
      this.comboBoxComPort.Location = new System.Drawing.Point(85, 21);
      this.comboBoxComPort.Name = "comboBoxComPort";
      this.comboBoxComPort.Size = new System.Drawing.Size(117, 21);
      this.comboBoxComPort.TabIndex = 9;
      // 
      // TableLayoutPanel1
      // 
      this.TableLayoutPanel1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
      this.TableLayoutPanel1.ColumnCount = 2;
      this.TableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
      this.TableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
      this.TableLayoutPanel1.Controls.Add(this.OK_Button, 0, 0);
      this.TableLayoutPanel1.Controls.Add(this.Cancel_Button, 1, 0);
      this.TableLayoutPanel1.Location = new System.Drawing.Point(113, 228);
      this.TableLayoutPanel1.Name = "TableLayoutPanel1";
      this.TableLayoutPanel1.RowCount = 1;
      this.TableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
      this.TableLayoutPanel1.Size = new System.Drawing.Size(130, 29);
      this.TableLayoutPanel1.TabIndex = 17;
      // 
      // OK_Button
      // 
      this.OK_Button.Location = new System.Drawing.Point(3, 3);
      this.OK_Button.Name = "OK_Button";
      this.OK_Button.Size = new System.Drawing.Size(59, 23);
      this.OK_Button.TabIndex = 0;
      this.OK_Button.Text = "OK";
      this.OK_Button.Click += new System.EventHandler(this.OK_Button_Click);
      // 
      // Cancel_Button
      // 
      this.Cancel_Button.Anchor = System.Windows.Forms.AnchorStyles.None;
      this.Cancel_Button.DialogResult = System.Windows.Forms.DialogResult.Cancel;
      this.Cancel_Button.Location = new System.Drawing.Point(71, 3);
      this.Cancel_Button.Name = "Cancel_Button";
      this.Cancel_Button.Size = new System.Drawing.Size(53, 23);
      this.Cancel_Button.TabIndex = 1;
      this.Cancel_Button.Text = "Cancel";
      this.Cancel_Button.Click += new System.EventHandler(this.Cancel_Button_Click);
      // 
      // SetupDialogForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(258, 279);
      this.Controls.Add(this.TableLayoutPanel1);
      this.Controls.Add(this.GroupBox1);
      this.Controls.Add(this.PictureBoxTeenAstro);
      this.Controls.Add(this.picASCOM);
      this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
      this.MaximizeBox = false;
      this.MinimizeBox = false;
      this.Name = "SetupDialogForm";
      this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
      this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
      this.Text = "TeenAstro Setup";
      this.Load += new System.EventHandler(this.SetupDialogForm_Load);
      ((System.ComponentModel.ISupportInitialize)(this.picASCOM)).EndInit();
      ((System.ComponentModel.ISupportInitialize)(this.PictureBoxTeenAstro)).EndInit();
      this.GroupBox1.ResumeLayout(false);
      this.GroupBox1.PerformLayout();
      this.TableLayoutPanel1.ResumeLayout(false);
      this.ResumeLayout(false);
      this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.PictureBox picASCOM;
    internal System.Windows.Forms.PictureBox PictureBoxTeenAstro;
    internal System.Windows.Forms.GroupBox GroupBox1;
    internal System.Windows.Forms.Button Button1;
    internal System.Windows.Forms.CheckBox chkTrace;
    internal System.Windows.Forms.TextBox TextBoxPort;
    internal System.Windows.Forms.Label Label1;
    internal System.Windows.Forms.TextBox TextBoxIP;
    internal System.Windows.Forms.RadioButton RadioButtonIP;
    internal System.Windows.Forms.RadioButton RadioButtonSP;
    internal System.Windows.Forms.ComboBox comboBoxComPort;
    internal System.Windows.Forms.TableLayoutPanel TableLayoutPanel1;
    internal System.Windows.Forms.Button OK_Button;
    internal System.Windows.Forms.Button Cancel_Button;
  }
}
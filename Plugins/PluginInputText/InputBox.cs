using System;
using System.Drawing;
using System.Windows.Forms;

namespace InputText
{
    public partial class InputBox : Form
    {
        public Rainmeter.Settings.InstanceSettings Instance = null;

        #region Default form initializer
        public InputBox()
        {
            InitializeComponent();
        }
        #endregion

        // This event will make sure we have focus when the inputbox is shown ot the user.
        // it will only cause focus if the window itself has focus (i.e. doesn't steal).
        private void InputBox_Shown(object sender, EventArgs e)
        {
            this.Activate();
            this.BringToFront();
            this.Activate();
            this.BringToFront();
            this.txtInput.Focus();
        }

        // All exceptions are swallowed for the sake of this example.  Since the majority
        // (if not all) of these are rather simple in nature, debugging without errors 
        // should be fairly simple anyway.
        //
        // However, if you wanted to log errors, a simple way would be to add something like
        // this:
        //
        //      try
        //      {
        //           ... // code you are testing
        //      }
        //      catch (Exception ex)
        //      {
        //           Rainmeter.Log(Rainmeter.LogLevel.Warning, "InputText :: exception :: " + ex.GetType().ToString() + ": " + ex.Message);
        //      }

        #region TextInput -- returns the inputted text
        public string TextInput
        {
            get
            {
                return this.txtInput.Text.Trim();
            }
        }
        #endregion
        #region ChangeFontFace(string) -- changes the text's font name
        public void ChangeFontFace(string sFont)
        {
            try
            {
                this.txtInput.Font = new Font(sFont, this.txtInput.Font.Size);
            }
            catch { }
        }
        #endregion
        #region ChangeFontColor(string) -- changes the font's color, supports 6 and 8-digit hex or a series of 3 or 4 comma-separated numbers
        public void ChangeFontColor(string sColor)
        {
            try
            {
                sColor = sColor.Trim();

                if (sColor.Contains(",")) // #,#,# or #,#,#,# format
                {
                    string[] sParts = sColor.Trim().Replace(" ", string.Empty).Split(new char[] { ',' }, StringSplitOptions.None);
                    this.txtInput.ForeColor = Color.FromArgb(int.Parse(sParts[0]), int.Parse(sParts[1]), int.Parse(sParts[2]));
                    this.txtInput.ForeColor = Color.FromArgb(int.Parse(sParts[3]), int.Parse(sParts[0]), int.Parse(sParts[1]), int.Parse(sParts[2]));
                }
                else
                {
                    if (sColor.Length == 6) // 6-digit hex (no alpha transparency)
                    {
                        uint iColor = uint.Parse(sColor, System.Globalization.NumberStyles.HexNumber);
                        // Since it was omitted, force full opacity (not transparent at all)
                        this.txtInput.ForeColor = Color.FromArgb((int)(0xFF000000 | iColor));
                    }
                    else if (sColor.Length == 8) // 8-digit hex (with alpha transparency)
                    {
                        uint iColor = uint.Parse(sColor, System.Globalization.NumberStyles.HexNumber);
                        // This swaps RRGGBBAA for AARRGGBB, which Color.FromArgb() wants
                        iColor = ((iColor & 0xFFFFFF00) >> 8) | ((iColor & 0x000000FF) << 24);
                        this.txtInput.ForeColor = Color.FromArgb((int)iColor);
                    }
                }
            }
            catch { }
        }
        #endregion
        #region ChangeBackColor(string) -- changes the background color, supports 6 and 8-digit hex or a series of 3 or 4 comma-separated numbers
        public void ChangeBackColor(string sColor)
        {
            try
            {
                sColor = sColor.Trim();

                if (sColor.Contains(",")) // #,#,# or #,#,#,# format
                {
                    string[] sParts = sColor.Trim().Replace(" ", string.Empty).Split(new char[] { ',' }, StringSplitOptions.None);
                    this.txtInput.BackColor = Color.FromArgb(int.Parse(sParts[0]), int.Parse(sParts[1]), int.Parse(sParts[2]));

                    // Change 0-255 transparency to a 0.0-1.0 transparency range
                    double dTrans = (double)int.Parse(sParts[3]);
                    dTrans /= 255.0;
                    this.Opacity = dTrans;
                }
                else
                {
                    if (sColor.Length == 6) // 6-digit hex (no alpha transparency)
                    {
                        uint iColor = uint.Parse(sColor, System.Globalization.NumberStyles.HexNumber);
                        this.txtInput.BackColor = Color.FromArgb((int)(0xFF000000 | iColor));
                        // Since it was omitted, force full opacity (not transparent at all)
                        this.Opacity = 1.0;
                    }
                    else if (sColor.Length == 8) // 8-digit hex (with alpha transparency)
                    {
                        uint iColor = uint.Parse(sColor, System.Globalization.NumberStyles.HexNumber);
                        // This swaps RRGGBBAA for AARRGGBB, which Color.FromArgb() wants
                        iColor = ((iColor & 0xFFFFFF00) >> 8) | ((iColor & 0x000000FF) << 24);
                        this.txtInput.BackColor = Color.FromArgb((int)((0xFF000000) | (0x00FFFFFF & iColor)));

                        // Change 0-255 transparency to a 0.0-1.0 transparency range
                        double dTrans = (double)((iColor & 0xFF000000) >> 24);
                        dTrans /= 255.0;
                        this.Opacity = dTrans;
                    }
                }
            }
            catch { }
        }
        #endregion
        #region ChangeStringAlign(string) -- Changes text horizontal alignment
        public void ChangeStringAlign(string sAlign)
        {
            try
            {
                sAlign = sAlign.ToUpper().Trim();

                if (sAlign == "LEFT")
                    this.txtInput.TextAlign = HorizontalAlignment.Left;
                else if (sAlign == "CENTER")
                    this.txtInput.TextAlign = HorizontalAlignment.Center;
                else if (sAlign == "RIGHT")
                    this.txtInput.TextAlign = HorizontalAlignment.Right;
            }
            catch { }
        }
        #endregion
        #region ChangeFontSize(string) -- changes the font's point size (supports floating-point values)
        public void ChangeFontSize(string sSize)
        {
            try
            {
                this.txtInput.Font = new Font(this.txtInput.Font.OriginalFontName, float.Parse(sSize));
            }
            catch { }
        }
        #endregion
        #region MakeTopmost() -- forces the form to have the 'Always On Top' property
        public void MakeTopmost()
        {
            this.TopMost = true;
        }
        #endregion
        #region MakePassword() -- causes the textbox to be password style
        public void MakePassword()
        {
            this.txtInput.PasswordChar = '*';
        }
        #endregion
        #region ChangeFontStringStyle() -- changes the font's bold/italic styles
        public void ChangeFontStringStyle(string sStyle)
        {
            try
            {
                sStyle = sStyle.ToUpper().Trim();

                if (sStyle == "NORMAL")
                    this.txtInput.Font = new Font(this.txtInput.Font, FontStyle.Regular);
                else if (sStyle == "BOLD")
                    this.txtInput.Font = new Font(this.txtInput.Font, FontStyle.Bold);
                else if (sStyle == "ITALIC")
                    this.txtInput.Font = new Font(this.txtInput.Font, FontStyle.Italic);
                else if (sStyle == "BOLDITALIC")
                    this.txtInput.Font = new Font(this.txtInput.Font, FontStyle.Bold | FontStyle.Italic);
            }
            catch { }
        }
        #endregion
        #region ChangeW(string) -- changes the width of the input textbox
        public void ChangeW(string sWidth)
        {
            try
            {
                this.txtInput.Width = int.Parse(sWidth);
            }
            catch { }
        }
        #endregion
        #region ChangeH(string) -- changes the height of the input textbox
        public void ChangeH(string sHeight)
        {
            try
            {
                this.txtInput.Height = int.Parse(sHeight);
            }
            catch { }
        }
        #endregion
        #region ChangeX(string) -- changes the X (horizontal) position of the input textbox, relative to its parent skin
        public void ChangeX(Rainmeter.Settings.InstanceSettings Instance, string sX)
        {
            try
            {
                // If the position is changed, make sure the form's auto-location is disabled.
                if (this.StartPosition != FormStartPosition.Manual)
                    this.StartPosition = FormStartPosition.Manual;

                // Notice that we need the position of the parent window for offset location.
                //
                // The Rainmeter class does this for us
                this.Location = new System.Drawing.Point(Rainmeter.ConfigX(Rainmeter.SkinName(Instance)) + int.Parse(sX), this.Location.Y);
            }
            catch { }
        }
        #endregion
        #region ChangeY(string) -- changes the Y (vertical) position of the input textbox, relative to its parent skin
        public void ChangeY(Rainmeter.Settings.InstanceSettings Instance, string sY)
        {
            try
            {
                // If the position is changed, make sure the form's auto-location is disabled.
                if (this.StartPosition != FormStartPosition.Manual)
                    this.StartPosition = FormStartPosition.Manual;

                // Notice that we need the position of the parent window for offset location.
                //
                // The Rainmeter class does this for us
                this.Location = new System.Drawing.Point(this.Location.X, Rainmeter.ConfigY(Rainmeter.SkinName(Instance)) + int.Parse(sY));
            }
            catch { }
        }
        #endregion
        #region DefaultValue(string) -- sets the default text to appear in the input textbox
        public void DefaultValue(string val)
        {
            this.txtInput.Text = val;
        }
        #endregion
        #region MakeFocusDismiss(bool) -- dismisses the input textbox if it loses cursor/window focus
        
        private bool _FocusDismiss = true;
        public DialogResult drBackup = DialogResult.None;
        public string sTextValue = string.Empty;

        public void MakeFocusDismiss(bool bDismissing)
        {
            this._FocusDismiss = bDismissing;
        }

        private void txtInput_Leave(object sender, EventArgs e)
        {
            if (this._FocusDismiss)
            {
                this.drBackup = DialogResult.Cancel;
                this.Close();
            }
        }

        private void InputBox_Leave(object sender, EventArgs e)
        {
            if (this._FocusDismiss)
            {
                this.drBackup = DialogResult.Cancel;
                this.Close();
            }
        }

        private void InputBox_Deactivate(object sender, EventArgs e)
        {
            if (this._FocusDismiss)
            {
                this.drBackup = DialogResult.Cancel;
                this.Close();
            }
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            _FocusDismiss = false;
            this.sTextValue = this.txtInput.Text.Trim();
            this.drBackup = DialogResult.OK;
            this.DialogResult = DialogResult.OK;
            this.Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            _FocusDismiss = false;
            this.drBackup = DialogResult.Cancel;
            this.DialogResult = DialogResult.Cancel;
            this.Close();
        }

        #endregion

        private void InputBox_Load(object sender, EventArgs e)
        {
            this.Activate();
            this.BringToFront();
            this.Activate();
            this.BringToFront();
        }
    }
}

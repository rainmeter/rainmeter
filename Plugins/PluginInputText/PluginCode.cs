using System;
using System.Collections.Generic;

// The bulk of your plugin's code belongs in this file.
namespace InputText
{
    class PluginCode
    {
        // 'Update', 'Update2', and 'GetString' all return data back to Rainmeter, depending on
        // if the Rainmeter measure wants a numeric value or a string/text data.
        //
        // The 'Instance' member contains all of the data necessary to read the INI file
        // passed to your plugin when this instance was first initialized.  Remember: your plugin
        // may be initialized multiple times.  For example, a plugin that reads the free space
        // of a hard drive may be called multiple times -- once for each installed hard drive.

        public UInt32 Update(Rainmeter.Settings.InstanceSettings Instance)
        {
            return 0;
        }

        //public double Update2(Rainmeter.Settings.InstanceSettings Instance)
        //{
        //    return 0.0;
        //}

        public string GetString(Rainmeter.Settings.InstanceSettings Instance)
        {
            // This plugin is unique as it is one of the first to not be used to display data
            // back in Rainmeter, but to request user input and use that input during batch
            // operations (and other purposes).
            // 
            // However, Rainmeter requires that data be sent back, so we'll return temporary data
            // In this case, the data is whatever the user last entered into an input textbox.
            return Instance.GetTempValue("LastInput", string.Empty).ToString();
        }


        // 'ExecuteBang' is a way of Rainmeter telling your plugin to do something *right now*.
        // What it wants to do can be defined by the 'Command' parameter.
        public void ExecuteBang(Rainmeter.Settings.InstanceSettings Instance, string Command)
        {
            #region Handle a single parameter

            // If our parameter list only contains a single word, then open a textbox immediately
            // and set a value.  This mode does not do any batching.
            if (!Command.Trim().Contains(" "))
            {
                // Assume that the parameter is the name of the variable
                string sVariableName = Command.Trim(); 

                // Ask for input
                string sInput = GetUserInput(Instance);

                // If the user cancelled out of the inputbox (ESC key, etc.), then abort
                if (sInput == null)
                    return;

                // Ask Rainmeter to set the variable using a bang (http://rainmeter.net/RainCMS/?q=Bangs)
                Rainmeter.Bang("!RainmeterSetVariable " + sVariableName + " \"" + sInput.Replace("\"", "\\\"") + "\"");

                // Note that the skin needs DynamicVariables=1 in the measure's settings or the above
                // code will have no effect.
                return;
            }

            #endregion
            #region Handle multiple parameters

            // Our parameter list contains at least two words, so split them up
            string[] sParts = Command.Trim().Split(new string[] { " " }, StringSplitOptions.None);

            // If the first parameter is 'ExecuteBatch' (not case sensitive)...
            if (sParts[0].Trim().ToUpper() == "EXECUTEBATCH")
            {
                // ExecuteBatch tells this plugin to go through the measure's settings to look
                // for lines beginning with "Command" and executing Rainmeter bangs for each one.
                // If a line contains $UserInput$, then an input textbox is opened and command
                // execution pauses until the user enters text or dismisses the textbox.  If the
                // textbox is dismissed (Escape key, for example), all processing ends, otherwise
                // it continues depending on the range of commands selected.
                //
                // Each "Command" line allows overriding all settings that the input textbox
                // supports, therefor some checking and substitution is performed, thus a
                // more complex parser has been implemented.
                //
                // ExecuteBatch expects this syntax:
                //       ExecuteBatch [All|#|#-#]
                //
                // This allows Rainmeter to call the plugin to execute a range including:
                //       All      All commands in the measure
                //       #        Only a single command in the measure
                //       #-#      A specific range of commands in the measure

                #region Determine range
                // Determine range.  Default is 1 to 1,000,000,000, although if processing finds
                // that a requested line is blank, it will stop all processing (so 'All' will
                // only parse 14 lines if "Command15" does not exist or is blank).
                int iMin = 1;
                int iMax = 1000000000;
                try
                {
                    if (sParts[1].Trim().ToUpper() != "ALL")
                    {
                        if (sParts[1].Contains("-"))
                        {
                            string[] sSubParts = sParts[1].Split(new string[] { "-" }, StringSplitOptions.None);
                            iMin = int.Parse(sSubParts[0]);
                            iMax = int.Parse(sSubParts[1]);
                        }
                        else
                            iMin = iMax = int.Parse(sParts[1]);
                    }
                }
                catch // handle all errors above
                {
                    // Any error above will be ignored and the default range used instead.
                    // This can occur if the measure asks to ExecuteBatch an invalid range
                    // or the range could not be translated to an acceptable format.
                    //
                    // For example:  ExecuteBatch asdf
                    iMin = 1;
                    iMax = 1000000000;
                }
                #endregion
                #region Parse commands in range
                // Parse each command in the range, aborting if any line returns 'false' or
                // the requested command line does not exist in the config for that measure.
                for (int i = iMin; i <= iMax; i++)
                {
                    // Read this command's line
                    string sCurrentLine = Instance.INI_Value("Command" + i.ToString());

                    // If empty/non-existent, abort
                    if (string.IsNullOrEmpty(sCurrentLine))
                        break;

                    // Execute the line, but if there's a problem (error or they cancel the
                    // input textbox), then abort
                    if (!ExecuteLine(Instance, sCurrentLine))
                        break;

                    // Continue to the next line, if there is any
                }
                #endregion
                return;
            }

            // Unhandled command, log the message but otherwise do nothing
            Rainmeter.Log(Rainmeter.LogLevel.Debug, "InputText: Received command \"" + sParts[0].Trim() + "\", left unhandled");

            #endregion

            return;
        }

        #region This is all code custom to this plugin

        #region Parse the current command line
        private bool ExecuteLine(Rainmeter.Settings.InstanceSettings Instance, string sLine)
        {
            // If this line contains a $UserInput$ token, then we need to do some extra
            // parsing
            if (sLine.ToUpper().Contains("$USERINPUT$"))
            {
                try
                {
                    #region Handle in-line overrides
                    // Create a blank list of overrides
                    Dictionary<string, string> Overrides = new Dictionary<string, string>();

                    // Start looking for overridable settings and adjust the list accordingly,
                    // stripping out settings from the line if they are discovered.
                    //
                    // The supporting TagData() function allows for whitespace if quotes are
                    // used.  For example:
                    //
                    // DefaultValue="hello there, how are you"
                    sLine = ScanAndReplace(sLine, "DefaultValue", Overrides);
                    sLine = ScanAndReplace(sLine, "X", Overrides);
                    sLine = ScanAndReplace(sLine, "Y", Overrides);
                    sLine = ScanAndReplace(sLine, "W", Overrides);
                    sLine = ScanAndReplace(sLine, "H", Overrides);
                    sLine = ScanAndReplace(sLine, "StringStyle", Overrides);
                    sLine = ScanAndReplace(sLine, "StringAlign", Overrides);
                    sLine = ScanAndReplace(sLine, "FocusDismiss", Overrides);
                    sLine = ScanAndReplace(sLine, "FontColor", Overrides);
                    sLine = ScanAndReplace(sLine, "FontFace", Overrides);
                    sLine = ScanAndReplace(sLine, "SolidColor", Overrides);
                    sLine = ScanAndReplace(sLine, "Password", Overrides);
                    sLine = ScanAndReplace(sLine, "FontSize", Overrides);
                    sLine = ScanAndReplace(sLine, "TopMost", Overrides);
                    #endregion

                    // Get user input
                    string sInput = GetUserInput(Instance, Overrides);
                    if (sInput == null)
                    {
                        // Rainmeter.Log(Rainmeter.LogLevel.Debug, "InputText: Aborted, user cancelled text box");
                        return false;
                    }

                    // Swap out the $UserInput$ token with what the user typed
                    sLine = Replace(sLine, "$USERINPUT$", sInput);
                }
                catch (Exception ex)
                {
                    // If there was an error doing any of the above, log it for debugging purposes.
                    Rainmeter.Log(Rainmeter.LogLevel.Warning, "InputText: Exception " + ex.GetType().ToString() + ": " + ex.Message);
                    return false;
                }
            }

            // Execute the bang.  This will either be the original line from CommandX= (sLine variable) or
            // an adjusted line based on special parsing above.
            // Rainmeter.Log(Rainmeter.LogLevel.Debug, "InputText: Executing bang: " + sLine);
            Rainmeter.Bang(sLine);
            return true;
        }
        #endregion
        #region GetUserInput() -- creates an input textbox and handles all INI and override settings

        private string GetUserInput(Rainmeter.Settings.InstanceSettings Instance)
        {
            // No INI overrides provided, so create an empty list
            return GetUserInput(Instance, new Dictionary<string,string>());
        }
        private string GetUserInput(Rainmeter.Settings.InstanceSettings Instance, Dictionary<string,string> sOverrides)
        {
            // Create the form.  'InputBox' is a .NET form with a textbox and two button controls on it.
            InputBox input = new InputBox();
            input.Instance = Instance;
            input.ChangeX(Instance, "0");
            input.ChangeY(Instance, "0");

            // Change the styles of the InputBox form based on overrides or INI values
            #region Style and preference tweaks (INI and override settings)

            if (sOverrides.ContainsKey("FontFace"))
                input.ChangeFontFace(sOverrides["FontFace"]);
            else if (!string.IsNullOrEmpty(Instance.INI_Value("FontFace")))
                input.ChangeFontFace(Instance.INI_Value("FontFace"));

            if (sOverrides.ContainsKey("FontSize"))
                input.ChangeFontSize(sOverrides["FontSize"]);
            else if (!string.IsNullOrEmpty(Instance.INI_Value("FontSize")))
                input.ChangeFontSize(Instance.INI_Value("FontSize"));

            if (sOverrides.ContainsKey("W"))
                input.ChangeW(sOverrides["W"]);
            else if (!string.IsNullOrEmpty(Instance.INI_Value("W")))
                input.ChangeW(Instance.INI_Value("W"));

            if (sOverrides.ContainsKey("H"))
                input.ChangeH(sOverrides["H"]);
            else if (!string.IsNullOrEmpty(Instance.INI_Value("H")))
                input.ChangeH(Instance.INI_Value("H"));

            if (sOverrides.ContainsKey("X"))
                input.ChangeX(Instance, sOverrides["X"]);
            else if (!string.IsNullOrEmpty(Instance.INI_Value("X")))
                input.ChangeX(Instance, Instance.INI_Value("X"));

            if (sOverrides.ContainsKey("Y"))
                input.ChangeY(Instance, sOverrides["Y"]);
            else if (!string.IsNullOrEmpty(Instance.INI_Value("Y")))
                input.ChangeY(Instance, Instance.INI_Value("Y"));

            if (sOverrides.ContainsKey("StringStyle"))
                input.ChangeFontStringStyle(sOverrides["StringStyle"]);
            else if (!string.IsNullOrEmpty(Instance.INI_Value("StringStyle")))
                input.ChangeFontStringStyle(Instance.INI_Value("StringStyle"));

            if (sOverrides.ContainsKey("StringAlign"))
                input.ChangeStringAlign(sOverrides["StringAlign"]);
            else if (!string.IsNullOrEmpty(Instance.INI_Value("StringAlign")))
                input.ChangeStringAlign(Instance.INI_Value("StringAlign"));

            bool bFocusDismiss = true;
            if (sOverrides.ContainsKey("FocusDismiss"))
            {
                input.MakeFocusDismiss(sOverrides["FocusDismiss"] == "1");
                bFocusDismiss = sOverrides["FocusDismiss"] == "1";
            }
            else
            {
                input.MakeFocusDismiss(!(Instance.INI_Value("FocusDismiss").Trim().ToUpper() != "1"));
                bFocusDismiss = !(Instance.INI_Value("FocusDismiss").Trim().ToUpper() != "1");
            }

            if (sOverrides.ContainsKey("FontColor"))
                input.ChangeFontColor(sOverrides["FontColor"]);
            else if (!string.IsNullOrEmpty(Instance.INI_Value("FontColor")))
                input.ChangeFontColor(Instance.INI_Value("FontColor"));

            if (sOverrides.ContainsKey("SolidColor"))
                input.ChangeBackColor(sOverrides["SolidColor"]);
            else if (!string.IsNullOrEmpty(Instance.INI_Value("SolidColor")))
                input.ChangeBackColor(Instance.INI_Value("SolidColor"));

            if (sOverrides.ContainsKey("Passwords"))
            {
                if (sOverrides["DefaultValue"] == "1")
                    input.MakePassword();
            }
            else if (Instance.INI_Value("Password").Trim().ToUpper() == "1")
                input.MakePassword();

            bool bAutoTopMost = true;
            if (sOverrides.ContainsKey("TopMost"))
            {
                if (sOverrides["TopMost"] == "1")
                {
                    input.MakeTopmost();
                    bAutoTopMost = false;
                }
                else if (sOverrides["TopMost"] == "0")
                    bAutoTopMost = false;
            }
            else if (Instance.INI_Value("TopMost").Trim().ToUpper() == "1")
            {
                input.MakeTopmost();
                bAutoTopMost = false;
            }
            else if (Instance.INI_Value("TopMost").Trim().ToUpper() != "AUTO")
                if (!string.IsNullOrEmpty(Instance.INI_Value("TopMost").Trim()))
                    bAutoTopMost = false;
            if (bAutoTopMost)
                if (Rainmeter.ParentIsTopmost(Instance))
                    input.MakeTopmost();

            if (sOverrides.ContainsKey("DefaultValue"))
                input.DefaultValue(sOverrides["DefaultValue"]);
            else if (!string.IsNullOrEmpty(Instance.INI_Value("DefaultValue")))
                input.DefaultValue(Instance.INI_Value("DefaultValue").Trim());

            #endregion

            if (bFocusDismiss)
            {
                input.Show(new WindowWrapper(Rainmeter.GetConfigWindow(Instance)));

                for (; ; )
                {
                    if (input.DialogResult != System.Windows.Forms.DialogResult.None || input.drBackup != System.Windows.Forms.DialogResult.None)
                        break;
                    System.Windows.Forms.Application.DoEvents();
                    System.Threading.Thread.Sleep(44);
                }
            }
            else
            {
                input.ShowDialog(new WindowWrapper(Rainmeter.GetConfigWindow(Instance)));
            }

            if (input.drBackup != System.Windows.Forms.DialogResult.None)
            {
                if (input.drBackup != System.Windows.Forms.DialogResult.OK)
                    return null;
            }
            else if (input.DialogResult != System.Windows.Forms.DialogResult.None)
            {
                if (input.DialogResult != System.Windows.Forms.DialogResult.OK)
                    return null;
            }

            Instance.SetTempValue("LastInput", input.sTextValue);
            return input.sTextValue;
        }
        #endregion
        #region Replace() -- case-insensitive string replacement
        private static string Replace(string sIn, string sFind, string sReplace)
        {
            int iReplace = sIn.ToUpper().IndexOf(sFind.ToUpper());

            string sLineNew = string.Empty;

            if (iReplace > 0)
                sLineNew += sIn.Substring(0, iReplace);
            sLineNew += sReplace;
            sLineNew += sIn.Substring(iReplace + (sFind.ToUpper()).Length);

            return sLineNew;
        }
        private static string Replace(string sIn, int iStart, int iLength, string sReplace)
        {
            int iReplace = iStart;

            string sLineNew = string.Empty;

            if (iReplace > 0)
                sLineNew += sIn.Substring(0, iReplace);
            sLineNew += sReplace;
            sLineNew += sIn.Substring(iReplace + iLength);

            return sLineNew;
        }
        #endregion
        #region TagLoc(), TagData(), FindTag() -- text parsing utilities for the override tags
        private int TagLoc(string sLine, string sTag)
        {
            if (!FindTag(sLine, sTag))
                return -1;

            return sLine.ToUpper().IndexOf(" " + sTag.ToUpper() + "=") + 1;
        }

        private string TagData(string sLine, string sTag)
        {
            if (!FindTag(sLine, sTag))
                return string.Empty;

            int iStart = TagLoc(sLine, sTag) + sTag.Length + 1;

            string sTagData = string.Empty;
            bool bInQuote = false;

            try
            {

                for (int i = 0; ; i++)
                {
                    if (i == 0)
                    {
                        if (sLine[i + iStart] == '"')
                        {
                            bInQuote = true;
                            continue;
                        }
                    }

                    if (sLine[i + iStart] == '"')
                        break;

                    if (!bInQuote)
                        if (char.IsWhiteSpace(sLine[i + iStart]))
                            break;

                    sTagData += sLine[i + iStart];
                }
            }
            catch { }

            if (bInQuote == true)
                return "\"" + sTagData + "\"";

            return sTagData;
        }

        private bool FindTag(string sLine, string sTag)
        {
            return (sLine.ToUpper().Contains(" " + sTag.ToUpper() + "="));
        }
        #endregion
        #region ScanAndReplace() -- searches for a tag and its value, adding it to overrides if found, and then removing it from the input line
        private string ScanAndReplace(string sLine, string sTagName, Dictionary<string, string> Overrides)
        {
            if (FindTag(sLine, sTagName))
            {
                string sTagData = TagData(sLine, sTagName);
                // Rainmeter.Log(Rainmeter.LogLevel.Debug, "InputText: Overriding " + sTagName + " with " + sTagData);
                if (sTagData.StartsWith("\""))
                    Overrides.Add(sTagName, sTagData.Substring(1, sTagData.Length - 2));
                else
                    Overrides.Add(sTagName, sTagData);
                sLine = Replace(sLine, TagLoc(sLine, sTagName) - 1, 1 + sTagName.Length + 1 + sTagData.Length, string.Empty);
            }

            return sLine;
        }
        #endregion

        #endregion
    }
}

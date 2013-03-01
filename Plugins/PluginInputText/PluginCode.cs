/*
  Copyright (C) 2013 Rainmeter Team

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

using System;
using System.Collections.Generic;
using Rainmeter;

// The bulk of your plugin's code belongs in this file.
namespace InputText
{
    internal partial class Measure
    {
        private void ExecuteCommands(string Command)
        {
            Command = Command.Trim();

            // Get default options
            Dictionary<string, string> Options = new Dictionary<string, string>();
            ReadOption("DefaultValue", Options);
            ReadOption("X", Options);
            ReadOption("Y", Options);
            ReadOption("W", Options);
            ReadOption("H", Options);
            ReadOption("StringStyle", Options);
            ReadOption("StringAlign", Options);
            ReadOption("FocusDismiss", Options);
            ReadOption("FontColor", Options);
            ReadOption("FontFace", Options);
            ReadOption("FontSize", Options);
            ReadOption("SolidColor", Options);
            ReadOption("Password", Options);
            ReadOption("TopMost", Options);

            #region Handle a single parameter

            // If our parameter list only contains a single word, then open a textbox immediately
            // and set a value.  This mode does not do any batching.
            if (!Command.Contains(" "))
            {
                // Assume that the parameter is the name of the variable

                // Ask for input
                string sInput = GetUserInput(Options);

                // If the user cancelled out of the inputbox (ESC key, etc.), then abort
                if (sInput == null)
                    return;

                // Ask Rainmeter to set the variable using a bang (http://rainmeter.net/RainCMS/?q=Bangs)
                API.Execute(rm.GetSkin(), "!SetVariable " + Command + " \"" + sInput + "\"");

                // Note that the skin needs DynamicVariables=1 in the measure's settings or the above
                // code will have no effect.
                return;
            }

            #endregion
            #region Handle multiple parameters

            // Our parameter list contains at least two words, so split them up
            string[] sParts = Command.Split(new string[] { " " }, StringSplitOptions.None);

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
                List<string> commands = new List<string>();

                for (int i = iMin; i <= iMax; i++)
                {
                    // Read this command's line
                    string sCurrentLine = rm.ReadString("Command" + i.ToString(), "", false);

                    // If empty/non-existent, abort
                    if (string.IsNullOrEmpty(sCurrentLine))
                        break;

                    commands.Add(sCurrentLine);
                }

                foreach (string sCurrentLine in commands)
                {
                    // Execute the line, but if there's a problem (error or they cancel the
                    // input textbox), then abort
                    if (!ExecuteLine(sCurrentLine, Options))
                        break;

                    // Continue to the next line, if there is any
                }
                #endregion
                return;
            }

            // Unhandled command, log the message but otherwise do nothing
            API.Log(API.LogType.Debug, "InputText: Received command \"" + sParts[0].Trim() + "\", left unhandled");

            #endregion

            return;
        }

        #region This is all code custom to this plugin

        #region Parse the current command line
        private bool ExecuteLine(string sLine, Dictionary<string, string> Options)
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
                    sLine = ScanAndReplace(sLine, "FontSize", Overrides);
                    sLine = ScanAndReplace(sLine, "SolidColor", Overrides);
                    sLine = ScanAndReplace(sLine, "Password", Overrides);
                    sLine = ScanAndReplace(sLine, "TopMost", Overrides);
                    #endregion

                    // Get user input
                    string sInput = GetUserInput(Options, Overrides);
                    if (sInput == null)
                    {
                        // API.Log(API.LogType.Debug, "InputText: Aborted, user cancelled text box");
                        return false;
                    }

                    // Swap out the $UserInput$ token with what the user typed
                    sLine = Replace(sLine, "$USERINPUT$", sInput);
                }
                catch (Exception ex)
                {
                    // If there was an error doing any of the above, log it for debugging purposes.
                    API.Log(API.LogType.Warning, "InputText: Exception " + ex.GetType().ToString() + ": " + ex.Message);
                    return false;
                }
            }

            // Execute the bang.  This will either be the original line from CommandX= (sLine variable) or
            // an adjusted line based on special parsing above.
            // API.Log(API.LogType.Debug, "InputText: Executing bang: " + sLine);
            API.Execute(rm.GetSkin(), sLine);
            return true;
        }
        #endregion
        #region GetUserInput() -- creates an input textbox and handles all INI and override settings

        delegate void ChangeSettingFromString(string value);
        delegate void ChangeInputBoxSetting(string option, ChangeSettingFromString method);

        private string GetUserInput(Dictionary<string, string> Options)
        {
            // No INI overrides provided, so create an empty list
            return GetUserInput(Options, new Dictionary<string, string>());
        }
        private string GetUserInput(Dictionary<string, string> Options, Dictionary<string, string> Overrides)
        {
            SkinWindow skin = new SkinWindow(rm);

            // Create the form.  'InputBox' is a .NET form with a textbox and two button controls on it.
            InputBox input = new InputBox(skin);
            input.ChangeX("0");
            input.ChangeY("0");

            // Change the styles of the InputBox form based on overrides or INI values
            #region Style and preference tweaks (INI and override settings)

            ChangeInputBoxSetting changeSetting = (opt, change) =>
            {
                if (Overrides.ContainsKey(opt))
                    change(Overrides[opt]);
                else if (Options.ContainsKey(opt))
                    change(Options[opt]);
            };

            changeSetting("FontFace", input.ChangeFontFace);
            changeSetting("FontSize", input.ChangeFontSize);

            changeSetting("W", input.ChangeW);
            changeSetting("H", input.ChangeH);
            changeSetting("X", input.ChangeX);
            changeSetting("Y", input.ChangeY);

            changeSetting("StringStyle", input.ChangeFontStringStyle);
            changeSetting("StringAlign", input.ChangeStringAlign);

            changeSetting("FontColor", input.ChangeFontColor);
            changeSetting("SolidColor", input.ChangeBackColor);

            if (Overrides.ContainsKey("FocusDismiss"))
                input.MakeFocusDismiss(Overrides["FocusDismiss"] == "1");
            else if (Options.ContainsKey("FocusDismiss"))
                input.MakeFocusDismiss(Options["FocusDismiss"].Trim() == "1");

            if (Overrides.ContainsKey("Password"))
                input.MakePassword(Overrides["Password"] == "1");
            else if (Options.ContainsKey("Password"))
                input.MakePassword(Options["Password"].Trim() == "1");

            string topmost = null;
            if (Overrides.ContainsKey("TopMost"))
                topmost = Overrides["TopMost"];
            else if (Options.ContainsKey("TopMost"))
                topmost = Options["TopMost"].Trim();
            switch (topmost)
            {
                case "1":
                    input.MakeTopmost();
                    break;
                case "0":
                    break;
                default:  // AUTO
                    if (skin.IsTopmost)
                        input.MakeTopmost();
                    break;
            }

            changeSetting("DefaultValue", input.DefaultValue);

            #endregion

            if (!input.ShowInputBox())
                return null;

            lock (this.locker)
            {
                this.LastInput = input.TextValue;
            }
            return input.TextValue;
        }
        #endregion

        #region ReadOption() -- reads given option's value from Rainmeter
        private void ReadOption(string optionName, Dictionary<string, string> Options)
        {
            string value = rm.ReadString(optionName, "");
            if (!string.IsNullOrEmpty(value))
            {
                Options.Add(optionName, value);
            }
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
                // API.Log(API.LogType.Debug, "InputText: Overriding " + sTagName + " with " + sTagData);
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

/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

using System;
using System.Runtime.InteropServices;

namespace Rainmeter
{
    /// <summary>
    /// Wrapper around the Rainmeter C API.
    /// </summary>
    public class API
    {
        private IntPtr m_Rm;

        public API(IntPtr rm)
        {
            m_Rm = rm;
        }

        static public implicit operator API(IntPtr rm)
        {
            return new Rainmeter.API(rm);
        }

        [DllImport("Rainmeter.dll", CharSet = CharSet.Unicode)]
        private extern static IntPtr RmReadString(IntPtr rm, string option, string defValue, bool replaceMeasures);

        [DllImport("Rainmeter.dll", CharSet = CharSet.Unicode)]
        private extern static double RmReadFormula(IntPtr rm, string option, double defValue);

        [DllImport("Rainmeter.dll", CharSet = CharSet.Unicode)]
        private extern static IntPtr RmReplaceVariables(IntPtr rm, string str);

        [DllImport("Rainmeter.dll", CharSet = CharSet.Unicode)]
        private extern static IntPtr RmPathToAbsolute(IntPtr rm, string relativePath);

        /// <summary>
        /// Executes a command
        /// </summary>
        /// <param name="skin">Pointer to current skin (See API.GetSkin)</param>
        /// <param name="command">Bang to execute</param>
        /// <returns>No return type</returns>
        /// <example>
        /// <code>
        /// [DllExport]
        /// internal double Update(IntPtr data)
        /// {
        ///     Measure measure = (Measure)data;
        ///     Rainmeter.API.Execute(measure->skin, "!SetVariable SomeVar 10");  // 'measure->skin' stored previously in the Initialize function
        ///     return 0.0;
        /// }
        /// </code>
        /// </example>
        [DllImport("Rainmeter.dll", EntryPoint = "RmExecute", CharSet = CharSet.Unicode)]
        public extern static void Execute(IntPtr skin, string command);

        [DllImport("Rainmeter.dll")]
        private extern static IntPtr RmGet(IntPtr rm, RmGetType type);

        [DllImport("Rainmeter.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        private extern static int LSLog(int type, string unused, string message);

        [DllImport("Rainmeter.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.StdCall)]
        private extern static int RmLog(IntPtr rm, LogType type, string message);

        private enum RmGetType
        {
            MeasureName = 0,
            Skin = 1,
            SettingsFile = 2,
            SkinName = 3,
            SkinWindowHandle = 4
        }

        public enum LogType
        {
            Error = 1,
            Warning = 2,
            Notice = 3,
            Debug = 4
        }

        /// <summary>
        /// Retrieves the option defined in the skin file
        /// </summary>
        /// <param name="option">Option name to be read from skin</param>
        /// <param name="defValue">Default value for the option if it is not found or invalid</param>
        /// <param name="replaceMeasures">If true, replaces section variables in the returned string</param>
        /// <returns>Returns the option value as a string</returns>
        /// <example>
        /// <code>
        /// [DllExport]
        /// public static void Reload(IntPtr data, IntPtr rm, ref double maxValue)
        /// {
        ///     Measure measure = (Measure)data;
        ///     Rainmeter.API api = (Rainmeter.API)rm;
        ///     string value = api.ReadString("Value", "DefaultValue");
        ///     string action = api.ReadString("Action", "", false);  // [MeasureNames] will be parsed/replaced when the action is executed with RmExecute
        /// }
        /// </code>
        /// </example>
        public string ReadString(string option, string defValue, bool replaceMeasures = true)
        {
            return Marshal.PtrToStringUni(RmReadString(m_Rm, option, defValue, replaceMeasures));
        }

        /// <summary>
        /// Retrieves the option defined in the skin file and converts a relative path to a absolute path
        /// </summary>
        /// <param name="option">Option name to be read from skin</param>
        /// <param name="defValue">Default value for the option if it is not found or invalid</param>
        /// <returns>Returns the absolute path of the option value as a string</returns>
        /// <example>
        /// <code>
        /// [DllExport]
        /// public static void Reload(IntPtr data, IntPtr rm, ref double maxValue)
        /// {
        ///     Measure measure = (Measure)data;
        ///     Rainmeter.API api = (Rainmeter.API)rm;
        ///     string path = api.ReadPath("MyPath", "C:\\");
        /// }
        /// </code>
        /// </example>
        public string ReadPath(string option, string defValue)
        {
            return Marshal.PtrToStringUni(RmPathToAbsolute(m_Rm, ReadString(option, defValue)));
        }

        /// <summary>
        /// Retrieves the option defined in the skin file and converts it to a double
        /// </summary>
        /// <remarks>If the option is a formula, the returned value will be the result of the parsed formula</remarks>
        /// <param name="option">Option name to read from skin</param>
        /// <param name="defValue">Default value for the option if it is not found, invalid, or a formula could not be parsed</param>
        /// <returns>Returns the option value as a double</returns>
        /// <example>
        /// <code>
        /// [DllExport]
        /// public static void Reload(IntPtr data, IntPtr rm, ref double maxValue)
        /// {
        ///     Measure measure = (Measure)data;
        ///     Rainmeter.API api = (Rainmeter.API)rm;
        ///     double value = api.ReadDouble("Value", 20.0);
        /// }
        /// </code>
        /// </example>
        public double ReadDouble(string option, double defValue)
        {
            return RmReadFormula(m_Rm, option, defValue);
        }

        /// <summary>
        /// Retrieves the option defined in the skin file and converts it to an integer
        /// </summary>
        /// <remarks>If the option is a formula, the returned value will be the result of the parsed formula</remarks>
        /// <param name="option">Option name to be read from skin</param>
        /// <param name="defValue">Default value for the option if it is not found, invalid, or a formula could not be parsed</param>
        /// <returns>Returns the option value as an integer</returns>
        /// <example>
        /// <code>
        /// [DllExport]
        /// public static void Reload(IntPtr data, IntPtr rm, ref double maxValue)
        /// {
        ///     Measure measure = (Measure)data;
        ///     Rainmeter.API api = (Rainmeter.API)rm;
        ///     int value = api.ReadInt("Value", 20);
        /// }
        /// </code>
        /// </example>
        public int ReadInt(string option, int defValue)
        {
            return (int)RmReadFormula(m_Rm, option, defValue);
        }

        /// <summary>
        /// Returns a string, replacing any variables (or section variables) within the inputted string
        /// </summary>
        /// <param name="str">String with unresolved variables</param>
        /// <returns>Returns a string replacing any variables in the 'str'</returns>
        /// <example>
        /// <code>
        /// [DllExport]
        /// public static double Update(IntPtr data)
        /// {
        ///     Measure measure = (Measure)data;
        ///     string myVar = measure.api.ReplaceVariables("#MyVar#").ToUpperInvariant(); // 'measure.api' stored previously in the Initialize function
        ///     if (myVar == "SOMETHING") { return 1.0; }
        ///     return 0.0;
        /// }
        /// </code>
        /// </example>
        public string ReplaceVariables(string str)
        {
            return Marshal.PtrToStringUni(RmReplaceVariables(m_Rm, str));
        }

        /// <summary>
        /// Retrieves the name of the measure
        /// </summary>
        /// <remarks>Call GetMeasureName() in the Initialize function and store the results for later use</remarks>
        /// <returns>Returns the current measure name as a string</returns>
        /// <example>
        /// <code>
        /// [DllExport]
        /// public static void Initialize(ref IntPtr data, IntPtr rm)
        /// {
        ///     Measure measure = new Measure();
        ///     Rainmeter.API api = (Rainmeter.API)rm;
        ///     measure.myName = api.GetMeasureName();  // declare 'myName' as a string in measure class
        ///     data = GCHandle.ToIntPtr(GCHandle.Alloc(measure));
        /// }
        /// </code>
        /// </example>
        public string GetMeasureName()
        {
            return Marshal.PtrToStringUni(RmGet(m_Rm, RmGetType.MeasureName));
        }

        /// <summary>
        /// Retrieves an internal pointer to the current skin
        /// </summary>
        /// <remarks>Call GetSkin() in the Initialize function and store the results for later use</remarks>
        /// <returns>Returns an IntPtr to the current skin</returns>
        /// <example>
        /// <code>
        /// [DllExport]
        /// public static void Initialize(ref IntPtr data, IntPtr rm)
        /// {
        ///     Measure measure = new Measure();
        ///     Rainmeter.API api = (Rainmeter.API)rm;
        ///     measure.mySkin = api.GetSkin();  // declare 'mySkin' as a IntPtr in measure class
        ///     data = GCHandle.ToIntPtr(GCHandle.Alloc(measure));
        /// }
        /// </code>
        /// </example>
        public IntPtr GetSkin()
        {
            return RmGet(m_Rm, RmGetType.Skin);
        }

        /// <summary>
        /// Retrieves a path to the Rainmeter data file (Rainmeter.data)
        /// </summary>
        /// <remarks>Call GetSettingsFile() in the Initialize function and store the results for later use</remarks>
        /// <returns>Returns the path and filename of the Rainmeter data file as a string</returns>
        /// <example>
        /// <code>
        /// public static void Initialize(ref IntPtr data, IntPtr rm)
        /// {
        ///     data = GCHandle.ToIntPtr(GCHandle.Alloc(new Measure()));
        ///     Rainmeter.API api = (Rainmeter.API)rm;
        ///     if (rmDataFile == null) { rmDataFile = API.GetSettingsFile(); }  // declare 'rmDataFile' as a string in global scope
        /// }
        /// </code>
        /// </example>
        public static string GetSettingsFile()
        {
            return Marshal.PtrToStringUni(RmGet(IntPtr.Zero, RmGetType.SettingsFile));
        }

        /// <summary>
        /// Retrieves full path and name of the skin
        /// </summary>
        /// <remarks>Call GetSkinName() in the Initialize function and store the results for later use</remarks>
        /// <returns>Returns the path and filename of the skin as a string</returns>
        /// <example>
        /// <code>
        /// [DllExport]
        /// public static void Initialize(ref IntPtr data, IntPtr rm)
        /// {
        ///     Measure measure = new Measure();
        ///     Rainmeter.API api = (Rainmeter.API)rm;
        ///     measure.skinName = api.GetSkinName(); }  // declare 'skinName' as a string in measure class
        ///     data = GCHandle.ToIntPtr(GCHandle.Alloc(measure));
        /// }
        /// </code>
        /// </example>
        public string GetSkinName()
        {
            return Marshal.PtrToStringUni(RmGet(m_Rm, RmGetType.SkinName));
        }

        /// <summary>
        /// Executes a command auto getting the skin reference
        /// </summary>
        /// <param name="command">Bang to execute</param>
        /// <returns>No return type</returns>
        /// <example>
        /// <code>
        /// [DllExport]
        /// public static double Update(IntPtr data)
        /// {
        ///     Measure measure = (Measure)data;
        ///     measure.api.Execute("!SetVariable SomeVar 10");  // 'measure.api' stored previously in the Initialize function
        ///     return 0.0;
        /// }
        /// </code>
        /// </example>
        public void Execute(string command)
        {
            Execute(this.GetSkin(), command);
        }

        /// <summary>
        /// Returns a pointer to the handle of the skin window (HWND)
        /// </summary>
        /// <remarks>Call GetSkinWindow() in the Initialize function and store the results for later use</remarks>
        /// <returns>Returns a handle to the skin window as a IntPtr</returns>
        /// <example>
        /// <code>
        /// [DllExport]
        /// internal void Initialize(Rainmeter.API rm)
        /// {
        ///     Measure measure = new Measure();
        ///     Rainmeter.API api = (Rainmeter.API)rm;
        ///     measure.skinWindow = api.GetSkinWindow(); }  // declare 'skinWindow' as a IntPtr in measure class
        ///     data = GCHandle.ToIntPtr(GCHandle.Alloc(measure));
        /// }
        /// </code>
        /// </example>
        public IntPtr GetSkinWindow()
        {
            return RmGet(m_Rm, RmGetType.SkinWindowHandle);
        }

        /// <summary>
        /// DEPRECATED: Save your rm or api reference and use Log(rm, type, message). Sends a message to the Rainmeter log with no source.
        /// </summary>
        public static void Log(int type, string message)
        {
            LSLog(type, null, message);
        }

        /// <summary>
        /// Sends a message to the Rainmeter log with source
        /// </summary>
        /// <remarks>LOG_DEBUG messages are logged only when Rainmeter is in debug mode</remarks>
        /// <param name="rm">Pointer to the plugin measure</param>
        /// <param name="type">Log type, use API.LogType enum (Error, Warning, Notice, or Debug)</param>
        /// <param name="message">Message to be logged</param>
        /// <returns>No return type</returns>
        /// <example>
        /// <code>
        /// Rainmeter.API.Log(rm, API.LogType.Notice, "I am a 'notice' log message with a source");
        /// </code>
        /// </example>
        public static void Log(IntPtr rm, LogType type, string message)
        {
            RmLog(rm, type, message);
        }

        /// <summary>
        /// <summary>
        /// Sends a formatted message to the Rainmeter log
        /// </summary>
        /// <remarks>LOG_DEBUG messages are logged only when Rainmeter is in debug mode</remarks>
        /// <param name="rm">Pointer to the plugin measure</param>
        /// <param name="type">Log type, use API.LogType enum (Error, Warning, Notice, or Debug)</param>
        /// <param name="format">Formatted message to be logged, follows string.Format syntax</param>
        /// <param name="args">Comma separated list of args referenced in the formatted message</param>
        /// <returns>No return type</returns>
        /// <example>
        /// <code>
        /// [DllExport]
        /// public static double Update(IntPtr data)
        /// {
        ///     Measure measure = (Measure)data;
        ///     string notice = "notice";
        ///     measure.api.LogF(measure.rm, API.LogType.Notice, "I am a '{0}' log message with a source", notice); // 'measure.rm' stored previously in the Initialize function
        ///     
        ///     return 0.0;
        /// }
        /// </code>
        /// </example>
        public static void LogF(IntPtr rm, LogType type, string format, params Object[] args)
        {
            RmLog(rm, type, string.Format(format, args));
        }

        /// <summary>
        /// Sends a message to the Rainmeter log with source
        /// </summary>
        /// <remarks>LOG_DEBUG messages are logged only when Rainmeter is in debug mode</remarks>
        /// <param name="type">Log type, use API.LogType enum (Error, Warning, Notice, or Debug)</param>
        /// <param name="message">Message to be logged</param>
        /// <returns>No return type</returns>
        /// <example>
        /// <code>
        /// [DllExport]
        /// public static double Update(IntPtr data)
        /// {
        ///     Measure measure = (Measure)data;
        ///     measure.api.Log(api, API.LogType.Notice, "I am a 'notice' log message with a source"); // 'measure.api' stored previously in the Initialize function
        ///     
        ///     return 0.0;
        /// }
        /// </code>
        /// </example>
        public void Log(LogType type, string message)
        {
            RmLog(this.m_Rm, type, message);
        }

        /// <summary>
        /// Sends a formatted message to the Rainmeter log
        /// </summary>
        /// <remarks>LOG_DEBUG messages are logged only when Rainmeter is in debug mode</remarks>
        /// <param name="type">Log type, use API.LogType enum (Error, Warning, Notice, or Debug)</param>
        /// <param name="format">Formatted message to be logged, follows string.Format syntax</param>
        /// <param name="args">Comma separated list of args referenced in the formatted message</param>
        /// <returns>No return type</returns>
        /// <example>
        /// <code>
        /// [DllExport]
        /// public static double Update(IntPtr data)
        /// {
        ///     Measure measure = (Measure)data;
        ///     string notice = "notice";
        ///     measure.api.LogF(API.LogType.Notice, "I am a '{0}' log message with a source", notice); // 'measure.api' stored previously in the Initialize function
        ///     
        ///     return 0.0;
        /// }
        /// </code>
        /// </example>
        public void LogF(LogType type, string format, params Object[] args)
        {
            RmLog(this.m_Rm, type, string.Format(format, args));
        }
    }
    /// <summary>
    /// Dummy attribute to mark method as exported for DllExporter.exe.
    /// </summary>
    [AttributeUsage(AttributeTargets.Method)]
    public class DllExport : Attribute
    {
        public DllExport()
        {

        }
    }
}

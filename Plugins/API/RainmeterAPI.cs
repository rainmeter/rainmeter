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

        [DllImport("Rainmeter.dll", CharSet = CharSet.Unicode)]
        private extern static IntPtr RmReadString(IntPtr rm, string option, string defValue, bool replaceMeasures);

        [DllImport("Rainmeter.dll", CharSet = CharSet.Unicode)]
        private extern static double RmReadFormula(IntPtr rm, string option, double defValue);

        [DllImport("Rainmeter.dll", CharSet = CharSet.Unicode)]
        private extern static IntPtr RmReplaceVariables(IntPtr rm, string str);

        [DllImport("Rainmeter.dll", CharSet = CharSet.Unicode)]
        private extern static IntPtr RmPathToAbsolute(IntPtr rm, string relativePath);

        [DllImport("Rainmeter.dll", EntryPoint = "RmExecute", CharSet = CharSet.Unicode)]
        public extern static void Execute(IntPtr skin, string command);

        [DllImport("Rainmeter.dll")]
        private extern static IntPtr RmGet(IntPtr rm, RmGetType type);

        [DllImport("Rainmeter.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        private extern static int LSLog(LogType type, string unused, string message);

        [DllImport("Rainmeter.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.StdCall)]
        private extern static int RmLog(IntPtr rm, LogType type, string message);

        [DllImport("Rainmeter.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        private extern static int RmLogF(IntPtr rm, LogType type, string format, params string[] message);

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
        /// internal void Reload(Rainmeter.API rm, ref double maxValue)
        /// {
        ///     string value = rm.ReadString("Value", "DefaultValue");
        ///     string action = rm.ReadString("Action", "", false);  // [MeasureNames] will be parsed/replaced when the action is executed with RmExecute
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
        /// internal void Reload(Rainmeter.API rm, ref double maxValue)
        /// {
        ///     string path = rm.ReadPath("MyPath", "C:\\");
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
        /// internal void Reload(Rainmeter.API rm, ref double maxValue)
        /// {
        ///     double value = rm.ReadDouble("Value", 20.0);
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
        /// internal void Reload(Rainmeter.API rm, ref double maxValue)
        /// {
        ///     int value = rm.ReadInt("Value", 20);
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
        /// internal double Update()
        /// {
        ///     string myVar = ReplaceVariables("#MyVar#").ToUpperInvariant();
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
        /// internal void Initialize(Rainmeter.API rm)
        /// {
        ///     myName = GetMeasureName();  // declare 'myName' as a string in class scope
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
        /// internal void Initialize(Rainmeter.API rm)
        /// {
        ///     mySkin = GetSkin();  // declare 'mySkin' as a IntPtr in class scope
        /// }
        /// </code>
        /// </example>
        public IntPtr GetSkin()
        {
            return RmGet(m_Rm, RmGetType.Skin);
        }

        /// <summary>
        /// Retrieves a path to the Rainmeter data file (Rainmeter.data).
        /// </summary>
        /// <remarks>Call GetSettingsFile() in the Initialize function and store the results for later use</remarks>
        /// <returns>Returns the path and filename of the Rainmeter data file as a string</returns>
        /// <example>
        /// <code>
        /// internal void Initialize(Rainmeter.API rm)
        /// {
        ///     if (rmDataFile == null) { rmDataFile = GetSettingsFile(); }  // declare 'rmDataFile' as a string in global scope
        /// }
        /// </code>
        /// </example>
        public string GetSettingsFile()
        {
            return Marshal.PtrToStringUni(RmGet(m_Rm, RmGetType.SettingsFile));
        }

        /// <summary>
        /// Retrieves full path and name of the skin
        /// </summary>
        /// <remarks>Call GetSkinName() in the Initialize function and store the results for later use</remarks>
        /// <returns>Returns the path and filename of the skin as a string</returns>
        /// <example>
        /// <code>
        /// internal void Initialize(Rainmeter.API rm)
        /// {
        ///     skinName = GetSkinName(); }  // declare 'skinName' as a string in class scope
        /// }
        /// </code>
        /// </example>
        public string GetSkinName()
        {
            return Marshal.PtrToStringUni(RmGet(m_Rm, RmGetType.SkinName));
        }

        /// <summary>
        /// Returns a pointer to the handle of the skin window (HWND)
        /// </summary>
        /// <remarks>Call GetSkinWindow() in the Initialize function and store the results for later use</remarks>
        /// <returns>Returns a handle to the skin window as a IntPtr</returns>
        /// <example>
        /// <code>
        /// internal void Initialize(Rainmeter.API rm)
        /// {
        ///     skinWindow = GetSkinWindow(); }  // declare 'skinWindow' as a IntPtr in class scope
        /// }
        /// </code>
        /// </example>
        public IntPtr GetSkinWindow()
        {
            return RmGet(m_Rm, RmGetType.SkinWindowHandle);
        }

        /// <summary>
        /// DEPRECATED: Use Log(rm, type, message). Sends a message to the Rainmeter log.
        /// </summary>
        public static void Log(LogType type, string message)
        {
            LSLog(type, null, message);
        }

        /// <summary>
        /// Sends a message to the Rainmeter log with source
        /// </summary>
        /// <remarks>LOG_DEBUG messages are logged only when Rainmeter is in debug mode</remarks>
        /// <param name="type">Log type (LOG_ERROR, LOG_WARNING, LOG_NOTICE, or LOG_DEBUG)</param>
        /// <param name="message">Message to be logged</param>
        /// <returns>No return type</returns>
        /// <example>
        /// <code>
        /// Log(rm, LOG_NOTICE, "I am a 'notice' log message with a source");
        /// </code>
        /// </example>
        public static void Log(IntPtr rm, LogType type, string message)
        {
            RmLog(rm, type, message);
        }

        /// <summary>
        /// Sends a formatted message to the Rainmeter log
        /// </summary>
        /// <remarks>LOG_DEBUG messages are logged only when Rainmeter is in debug mode</remarks>
        /// <param name="rm">Pointer to the plugin measure</param>
        /// <param name="level">Log level (LOG_ERROR, LOG_WARNING, LOG_NOTICE, or LOG_DEBUG)</param>
        /// <param name="format">Formatted message to be logged, follows printf syntax</param>
        /// <param name="args">Comma separated list of args referenced in the formatted message</param>
        /// <returns>No return type</returns>
        /// <example>
        /// <code>
        /// string notice = "notice";
        /// LogF(rm, LOG_NOTICE, "I am a '%s' log message with a source", notice);
        /// </code>
        /// </example>
        public static void LogF(IntPtr rm, LogType type, string format, params string[] args)
        {
            RmLogF(rm, type, format, args);
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

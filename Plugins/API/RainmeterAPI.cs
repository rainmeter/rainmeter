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

        public string ReadString(string option, string defValue, bool replaceMeasures = true)
        {
            return Marshal.PtrToStringUni(RmReadString(m_Rm, option, defValue, replaceMeasures));
        }

        public string ReadPath(string option, string defValue)
        {
            return Marshal.PtrToStringUni(RmPathToAbsolute(m_Rm, ReadString(option, defValue)));
        }

        public double ReadDouble(string option, double defValue)
        {
            return RmReadFormula(m_Rm, option, defValue);
        }

        public int ReadInt(string option, int defValue)
        {
            return (int)RmReadFormula(m_Rm, option, defValue);
        }

        public string ReplaceVariables(string str)
        {
            return Marshal.PtrToStringUni(RmReplaceVariables(m_Rm, str));
        }

        public string GetMeasureName()
        {
            return Marshal.PtrToStringUni(RmGet(m_Rm, RmGetType.MeasureName));
        }

        public IntPtr GetSkin()
        {
            return RmGet(m_Rm, RmGetType.Skin);
        }

        public string GetSettingsFile()
        {
            return Marshal.PtrToStringUni(RmGet(m_Rm, RmGetType.SettingsFile));
        }

        public string GetSkinName()
        {
            return Marshal.PtrToStringUni(RmGet(m_Rm, RmGetType.SkinName));
        }

        public IntPtr GetSkinWindow()
        {
            return RmGet(m_Rm, RmGetType.SkinWindowHandle);
        }

        public static void Log(LogType type, string message)
        {
            LSLog(type, null, message);
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

/*
  Copyright (C) 2011 Birunthan Mohanathas

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

        public static unsafe char* ToUnsafe(string s)
        {
            fixed (char* p = s) return p;
        }

        [DllImport("Rainmeter.dll", CharSet = CharSet.Auto)]
        private extern static unsafe char* RmReadString(void* rm, char* option, char* defValue, int replaceMeasures);

        [DllImport("Rainmeter.dll", CharSet = CharSet.Auto)]
        private extern static unsafe double RmReadFormula(void* rm, char* option, double defValue);

        [DllImport("Rainmeter.dll", CharSet = CharSet.Auto)]
        private extern static unsafe char* RmPathToAbsolute(void* rm, char* relativePath);

        [DllImport("Rainmeter.dll", CharSet = CharSet.Auto)]
        private extern static unsafe void RmExecute(void* rm, char* command);

        [DllImport("Rainmeter.dll", CharSet = CharSet.Auto)]
        private extern static unsafe void* RmGet(void* rm, int type);

        [DllImport("Rainmeter.dll", CharSet = CharSet.Auto, CallingConvention = CallingConvention.Cdecl)]
        private extern static unsafe int LSLog(int type, char* unused, char* message);

        public enum LogType
        {
            Error = 1,
            Warning = 2,
            Notice = 3,
            Debug = 4
        }

        public unsafe string ReadString(string option, string defValue)
        {
            char* value = RmReadString((void*)m_Rm, ToUnsafe(option), ToUnsafe(defValue), 1);
            return new string(value);
        }

        public unsafe string ReadPath(string option, string defValue)
        {
            char* relativePath = RmReadString((void*)m_Rm, ToUnsafe(option), ToUnsafe(defValue), 1);
            char* value = RmPathToAbsolute((void*)m_Rm, relativePath);
            return new string(value);
        }

        public unsafe double ReadFormula(string option, double defValue)
        {
            return RmReadFormula((void*)m_Rm, ToUnsafe(option), defValue);
        }

        public unsafe int ReadInt(string option, int defValue)
        {
            string value = ReadString(option, "");
            try
            {
                return Convert.ToInt32(value);
            }
            catch
            {
                return defValue;
            }
        }

        public unsafe string GetMeasureName()
        {
            char* value = (char*)RmGet((void*)m_Rm, 0);
            return new string(value);
        }

        public unsafe IntPtr GetSkin()
        {
            return (IntPtr)RmGet((void*)m_Rm, 1);
        }

        public static unsafe void Execute(IntPtr skin, string command)
        {
            RmExecute((void*)skin, ToUnsafe(command));
        }

        public static unsafe void Log(LogType type, string message)
        {
            LSLog((int)type, null, ToUnsafe(message));
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

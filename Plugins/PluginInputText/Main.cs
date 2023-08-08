/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Threading;
using Rainmeter;

namespace InputText
{
    /**
     * Utility class
     */
    public class NumberParser
    {
        public static double NaN = 0xfff8000000000000;

        public static bool TryParse(string input, out double result)
        {
            // Following
            // https://docs.rainmeter.net/manual/measures/calc/#Bases

            input = input.Trim().Replace(",", ""); // Remove commas from numbers like 1,000
            double sign = 1.0;

            if (input.StartsWith("+")) {
                input = input.Substring(1);
            } else if (input.StartsWith("-"))
            {
                input = input.Substring(1);
                sign = -1;
            }

            if (string.IsNullOrWhiteSpace(input))
            {
                result = NaN;
                return false;
            }

            bool res = TryParseNumberWithPrefix(input, "0b", NumberStyles.Integer, out result)
                || TryParseNumberWithPrefix(input, "0o", NumberStyles.Integer, out result)
                || TryParseNumberWithPrefix(input, "0x", NumberStyles.HexNumber, out result)
                || double.TryParse(input, NumberStyles.Float, CultureInfo.InvariantCulture, out result);

            result *= sign; // NaN safe

            return res;
        }

        private static bool TryParseNumberWithPrefix(string input, string prefix, NumberStyles style, out double result)
        {
            if (input.StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
            {
                input = input.Substring(prefix.Length);
                if (int.TryParse(input, style, CultureInfo.InvariantCulture, out int intValue))
                {
                    result = (double)intValue;
                    return true;
                }
            }
            result = NaN;
            return false;
        }
    }
    internal partial class Measure
    {
        private static double StringToDoubleOrNaN(string input)
        {
            double result;
            if (NumberParser.TryParse(input, out result))
            {
                return result;
            }
            return NumberParser.NaN;
        }

        private Rainmeter.API rm;

        private string lastInput = string.Empty;
        private double LastInputParsed = NumberParser.NaN;
        private string LastInput {
            get { return lastInput; }
            set {
                lastInput = value;
                // This value is always parsed, but the calculation is rather low overhead and
                // we can certainly afford it
                LastInputParsed = StringToDoubleOrNaN(value);
            }
        }
        private bool IsExecuteBangRunning = false;

        private object locker = new object();

        internal Measure(Rainmeter.API rm)
        {
            this.rm = rm;
        }

        internal void Reload(Rainmeter.API rm, ref double maxValue)
        {
            // Do nothing here.
        }

        internal double Update()
        {
            // Does not need a lock
            return this.LastInputParsed;
        }

        internal string GetString()
        {
            lock (this.locker)
            {
                return this.LastInput;
            }
        }

        internal void ExecuteBang(string args)
        {
            bool go = false;

            lock (this.locker)
            {
                if (!this.IsExecuteBangRunning)
                {
                    this.IsExecuteBangRunning = true;
                    go = true;
                }
            }

            if (go)
            {
                ExecuteBangParam param = new ExecuteBangParam(args);
                if (ReadOptions(param))  // Read all options in main thread for thread-safety
                {
                    ThreadPool.QueueUserWorkItem(_ =>
                    {
                        try
                        {
                            ExecuteCommands(param);
                        }
                        catch (Exception ex)
                        {
                            rm.Log(API.LogType.Error, "C# plugin in ExecuteBang(), " + ex.GetType().ToString() + ": " + ex.Message);
                        }

                        lock (this.locker)
                        {
                            this.IsExecuteBangRunning = false;
                        }
                    });
                }
                else
                {
                    // No need to continue
                    lock (this.locker)
                    {
                        this.IsExecuteBangRunning = false;
                    }
                }
            }
        }

        private class ExecuteBangParam
        {
            internal enum BangType
            {
                Unknown,
                SetVariable,
                ExecuteBatch
            };
            internal Dictionary<string, string> Options;
            internal List<Dictionary<string, string>> OverrideOptions;
            internal List<string> Commands;
            internal string Command;
            internal string DismissAction;
            internal BangType Type;

            internal ExecuteBangParam(string args)
            {
                this.Options = new Dictionary<string, string>();
                this.OverrideOptions = new List<Dictionary<string, string>>();
                this.Commands = new List<string>();
                this.Command = args.Trim();
                this.DismissAction = null;
                this.Type = BangType.Unknown;
            }
        };

        private bool _IsFinalizing = false;

        internal void Dispose()
        {
            this._IsFinalizing = true;
            FinalizePluginCode();
        }
    }

    #region Plugin

    public static class Plugin
    {
        static IntPtr StringBuffer = IntPtr.Zero;

        [DllExport]
        public static void Initialize(ref IntPtr data, IntPtr rm)
        {
            data = GCHandle.ToIntPtr(GCHandle.Alloc(new Measure(new Rainmeter.API(rm))));
        }

        [DllExport]
        public static void Finalize(IntPtr data)
        {
            Measure measure = (Measure)GCHandle.FromIntPtr(data).Target;
            measure.Dispose();
            GCHandle.FromIntPtr(data).Free();

            if (StringBuffer != IntPtr.Zero)
            {
                Marshal.FreeHGlobal(StringBuffer);
                StringBuffer = IntPtr.Zero;
            }
        }

        [DllExport]
        public static void Reload(IntPtr data, IntPtr rm, ref double maxValue)
        {
            Measure measure = (Measure)GCHandle.FromIntPtr(data).Target;
            measure.Reload(new Rainmeter.API(rm), ref maxValue);
        }

        [DllExport]
        public static double Update(IntPtr data)
        {
            Measure measure = (Measure)GCHandle.FromIntPtr(data).Target;
            return measure.Update();
        }

        [DllExport]
        public static IntPtr GetString(IntPtr data)
        {
            Measure measure = (Measure)GCHandle.FromIntPtr(data).Target;
            if (StringBuffer != IntPtr.Zero)
            {
                Marshal.FreeHGlobal(StringBuffer);
                StringBuffer = IntPtr.Zero;
            }

            string stringValue = measure.GetString();
            if (stringValue != null)
            {
                StringBuffer = Marshal.StringToHGlobalUni(stringValue);
            }

            return StringBuffer;
        }

        [DllExport]
        public static void ExecuteBang(IntPtr data, IntPtr args)
        {
            Measure measure = (Measure)GCHandle.FromIntPtr(data).Target;
            measure.ExecuteBang(Marshal.PtrToStringUni(args));
        }
    }

    #endregion
}

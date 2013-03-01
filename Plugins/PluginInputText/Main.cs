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
using System.Runtime.InteropServices;
using System.Threading;
using Rainmeter;

namespace InputText
{
    internal partial class Measure
    {
        private Rainmeter.API rm;

        private string LastInput = string.Empty;
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
            return 0.0;
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
                ThreadPool.QueueUserWorkItem(ExecuteBangThread, args);
            }
        }

        private void ExecuteBangThread(object state)
        {
            string command = (string)state;

            try
            {
                ExecuteCommands(command);
            }
            catch (Exception ex)
            {
                API.Log(API.LogType.Error, "C# plugin in ExecuteBang(), " + ex.GetType().ToString() + ": " + ex.Message);
            }

            lock (this.locker)
            {
                this.IsExecuteBangRunning = false;
            }
        }
    }

    #region Plugin

    public static class Plugin
    {
        internal static Dictionary<uint, Measure> Measures = new Dictionary<uint, Measure>();

        [DllExport]
        public unsafe static void Initialize(void** data, void* rm)
        {
            uint id = (uint)((void*)*data);
            Measures.Add(id, new Measure(new Rainmeter.API((IntPtr)rm)));
        }

        [DllExport]
        public unsafe static void Finalize(void* data)
        {
            uint id = (uint)data;
            Measures.Remove(id);
        }

        [DllExport]
        public unsafe static void Reload(void* data, void* rm, double* maxValue)
        {
            uint id = (uint)data;
            Measures[id].Reload(new Rainmeter.API((IntPtr)rm), ref *maxValue);
        }

        [DllExport]
        public unsafe static double Update(void* data)
        {
            uint id = (uint)data;
            return Measures[id].Update();
        }

        [DllExport]
        public unsafe static char* GetString(void* data)
        {
            uint id = (uint)data;
            fixed (char* s = Measures[id].GetString()) return s;
        }

        [DllExport]
        public unsafe static void ExecuteBang(void* data, char* args)
        {
            uint id = (uint)data;
            Measures[id].ExecuteBang(new string(args));
        }
    }

    #endregion
}

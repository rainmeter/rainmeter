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

// Define/undefine to control exports. Undefine to infrom that this plugin does not support
// a particular function.
#define ExportUpdate
#undef ExportGetString
#undef ExportExecuteBang

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Rainmeter;

namespace ExampleCS
{
    /// <summary>
    /// Represents a measure. Members are specific to a measure. Methods are called by Rainmeter
    /// when needed.
    /// </summary>
    internal class Measure
    {
        /// <summary>
        /// Called when a measure is created.
        /// </summary>
        internal Measure()
        {
        }

        /// <summary>
        /// Called when a measure is destroyed. Use this rather than a destructor to perform
        /// cleanup.
        /// </summary>
        internal void Cleanup()
        {
        }

        /// <summary>
        /// Called when the measure settings must be (re)read.
        /// </summary>
        internal void Reload(Rainmeter.API rm, ref double maxValue)
        {
            // Examples:
            //   string value = rm.ReadString("TestOption", "DefaultValue");
            //   double value = rm.ReadFormula("TestOption", 20);
        }

        /// <summary>
        /// Called when the measure settings must be (re)read.
        /// </summary>
#if ExportUpdate
        internal double Update()
        {
            return 42.0;
        }
#endif

        /// <summary>
        /// Called when the string representation of the measure value is required.
        /// </summary>
        /// <remarks>
        /// Can be called multiple times per update cycle. Do not call heavy functions here.
        /// Instead create a string member, set it in Update, and simply return it here.
        /// </remarks>
#if ExportGetString
        internal string GetString()
        {
            return "Hello, world!";
        }
#endif

        /// <summary>
        /// Called when as a result of a !CommandMeasure bang aimed at the measure.
        /// </summary>
#if ExportExecuteBang
        internal void ExecuteBang(string args)
        {
        }
#endif
    }

    /// <summary>
    /// Handles communication between Rainmeter and the plugin.
    /// </summary>
    public static class Plugin
    {
        [DllExport]
        public unsafe static void Initialize(void** data)
        {
            IntPtr dataPtr = (IntPtr)((void*)*data);
            Measures.Add(dataPtr, new Measure());
        }

        [DllExport]
        public unsafe static void Finalize(void* data)
        {
            IntPtr dataPtr = (IntPtr)data;
            Measures[dataPtr].Cleanup();
            Measures.Remove(dataPtr);
        }

        [DllExport]
        public unsafe static void Reload(void* data, void* rm, double* maxValue)
        {
            IntPtr dataPtr = (IntPtr)data;
            Measures[dataPtr].Reload(new Rainmeter.API((IntPtr)rm), ref *maxValue);
        }

#if ExportUpdate
        [DllExport]
        public unsafe static double Update(void* data)
        {
            IntPtr dataPtr = (IntPtr)data;
            return Measures[dataPtr].Update();
        }
#endif

#if ExportGetString
        [DllExport]
        public unsafe static char* GetString(void* data)
        {
            IntPtr dataPtr = (IntPtr)data;
            return Rainmeter.API.ToUnsafe(Measures[dataPtr].GetString());
        }
#endif

#if ExportExecuteBang
        [DllExport]
        public unsafe static void ExecuteBang(void* data, char* args)
        {
            IntPtr dataPtr = (IntPtr)data;
            Measures[dataPtr].ExecuteBang(new string(args));
            Measures.Remove(dataPtr);
        }
#endif

        internal static Dictionary<IntPtr, Measure> Measures = new Dictionary<IntPtr, Measure>();
    }
}

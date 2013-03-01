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
using System.Runtime.InteropServices;
using Rainmeter;

// This is a utility class / toolkit for communicating with Rainmeter and managing
// window positioning.
//
// You should not need to edit this code except to expand the toolkit support.
//
// Rather, most of your plugin's code should go in PluginCode.cs or additional files
// that you create (such as new forms, classes, and controls).
namespace InputText
{
    public class SkinWindow : System.Windows.Forms.IWin32Window
    {
        private string _SkinName;
        private IntPtr _Handle;

        private int _X = 0;
        private int _Y = 0;
        private int _W = 0;
        private int _H = 0;

        private bool _IsTopmost = false;

        public SkinWindow(Rainmeter.API rm)
        {
            UpdateStatus(rm);
        }

        #region Methods for getting the screen-relative location of the current skin

        public IntPtr Handle
        {
            get { return this._Handle; }
        }

        public int X
        {
            get { return this._X; }
        }

        public int Y
        {
            get { return this._Y; }
        }

        public int Width
        {
            get { return this._W; }
        }

        public int Height
        {
            get { return this._H; }
        }

        public bool IsTopmost
        {
            get { return this._IsTopmost; }
        }

        public void UpdateStatus(Rainmeter.API rm = null)
        {
            if (rm != null)
            {
                this._SkinName = rm.GetSkinName();
                this._Handle = rm.GetSkinWindow();
            }

            RECT rct;
            if (GetWindowRect(this._Handle, out rct))
            {
                this._X = rct.Left;
                this._Y = rct.Top;
                this._W = rct.Right - rct.Left;
                this._H = rct.Bottom - rct.Top;
            }
            else
            {
                API.Log(API.LogType.Error,
                    "Rainmeter told us the HWND for window '" + this._SkinName + "' is " + this._Handle.ToInt32().ToString() + "L, but couldn't receive a proper RECT from it");
            }

            this._IsTopmost = ((GetWindowLong(this._Handle, GWL_EXSTYLE) & WS_EX_TOPMOST) > 0);
        }

        #region GetWindowRect() platform invoke to get the size/location of a window

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool GetWindowRect(IntPtr hwnd, out RECT lpRect);

        [StructLayout(LayoutKind.Sequential)]
        public struct RECT
        {
            public int Left;        // x position of upper-left corner
            public int Top;         // y position of upper-left corner
            public int Right;       // x position of lower-right corner
            public int Bottom;      // y position of lower-right corner
        }
        #endregion
        #region GetWindowLong() -- platform invoke to check a window's Z-order (Topmost=Auto)

        [DllImport("user32.dll")]
        private static extern int GetWindowLong(IntPtr hwnd, int nIndex);

        private const int GWL_EXSTYLE   = -20;
        private const int WS_EX_TOPMOST = 8;

        #endregion

        #endregion
    }
}

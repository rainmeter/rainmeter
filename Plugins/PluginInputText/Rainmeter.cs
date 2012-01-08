using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Threading;

// This is a utility class / toolkit for communicating with Rainmeter and managing
// logging, INI settings, bangs, window positioning, multiple instances, and temporary
// data storage.
//
// You should not need to edit this code except to expand the toolkit support.
//
// Rather, most of your plugin's code should go in PluginCode.cs or additional files
// that you create (such as new forms, classes, and controls).
namespace InputText
{
    public class WindowWrapper : System.Windows.Forms.IWin32Window
    {
        public WindowWrapper(IntPtr handle)
        {
            _hwnd = handle;
        }

        public IntPtr Handle
        {
            get { return _hwnd; }
        }

        private IntPtr _hwnd;
    }

    public class Rainmeter
    {
        #region Methods for getting the screen-relative location of the current skin

        public static IntPtr GetConfigWindow(Rainmeter.Settings.InstanceSettings Instance)
        {
            return (IntPtr)(UInt32.Parse(Rainmeter.PluginBridge("GetWindow", Rainmeter.PluginBridge("GetConfig", Instance.INI_File))));
        }
        public static int ConfigX(string sSkin)
        {
            IntPtr hwnd = (IntPtr)(UInt32.Parse(Rainmeter.PluginBridge("GetWindow", sSkin)));
            RECT rct;
            if (!GetWindowRect(hwnd, out rct))
            {
                Rainmeter.Log(LogLevel.Error, "Rainmeter told us the HWND for window '" + sSkin + "' is " + hwnd.ToInt32().ToString() + "L, but couldn't receive a proper RECT from it");
                return 0;
            }
            return rct.Left;
        }
        public static int ConfigX(Rainmeter.Settings.InstanceSettings Instance)
        {
            IntPtr hwnd = (IntPtr)(UInt32.Parse(Rainmeter.PluginBridge("GetWindow", Rainmeter.PluginBridge("GetConfig", Instance.INI_File))));
            RECT rct;
            if (!GetWindowRect(hwnd, out rct))
            {
                Rainmeter.Log(LogLevel.Error, "Rainmeter told us the HWND for window '" + Rainmeter.PluginBridge("GetConfig", Instance.INI_File) + "' is " + hwnd.ToInt32().ToString() + "L, but couldn't receive a proper RECT from it");
                return 0;
            }
            return rct.Left;
        }

        public static int ConfigY(string sSkin)
        {
            IntPtr hwnd = (IntPtr)(UInt32.Parse(Rainmeter.PluginBridge("GetWindow", sSkin)));
            RECT rct;
            if (!GetWindowRect(hwnd, out rct))
            {
                Rainmeter.Log(LogLevel.Error, "Rainmeter told us the HWND for window '" + sSkin + "' is " + hwnd.ToInt32().ToString() + "L, but couldn't receive a proper RECT from it");
                return 0;
            }
            return rct.Top;
        }
        public static int ConfigY(Rainmeter.Settings.InstanceSettings Instance)
        {
            IntPtr hwnd = (IntPtr)(UInt32.Parse(Rainmeter.PluginBridge("GetWindow", Rainmeter.PluginBridge("GetConfig", Instance.INI_File))));
            RECT rct;
            if (!GetWindowRect(hwnd, out rct))
            {
                Rainmeter.Log(LogLevel.Error, "Rainmeter told us the HWND for window '" + Rainmeter.PluginBridge("GetConfig", Instance.INI_File) + "' is " + hwnd.ToInt32().ToString() + "L, but couldn't receive a proper RECT from it");
                return 0;
            }
            return rct.Top;
        }

        public static int ConfigWidth(string sSkin)
        {
            IntPtr hwnd = (IntPtr)(UInt32.Parse(Rainmeter.PluginBridge("GetWindow", sSkin)));
            RECT rct;
            if (!GetWindowRect(hwnd, out rct))
            {
                Rainmeter.Log(LogLevel.Error, "Rainmeter told us the HWND for window '" + sSkin + "' is " + hwnd.ToInt32().ToString() + "L, but couldn't receive a proper RECT from it");
                return 0;
            }
            return rct.Right - rct.Left;
        }
        public static int ConfigWidth(Rainmeter.Settings.InstanceSettings Instance)
        {
            IntPtr hwnd = (IntPtr)(UInt32.Parse(Rainmeter.PluginBridge("GetWindow", Rainmeter.PluginBridge("GetConfig", Instance.INI_File))));
            RECT rct;
            if (!GetWindowRect(hwnd, out rct))
            {
                Rainmeter.Log(LogLevel.Error, "Rainmeter told us the HWND for window '" + Rainmeter.PluginBridge("GetConfig", Instance.INI_File) + "' is " + hwnd.ToInt32().ToString() + "L, but couldn't receive a proper RECT from it");
                return 0;
            }
            return rct.Right - rct.Left;
        }

        public static int ConfigHeight(string sSkin)
        {
            IntPtr hwnd = (IntPtr)(UInt32.Parse(Rainmeter.PluginBridge("GetWindow", sSkin)));
            RECT rct;
            if (!GetWindowRect(hwnd, out rct))
            {
                Rainmeter.Log(LogLevel.Error, "Rainmeter told us the HWND for window '" + sSkin + "' is " + hwnd.ToInt32().ToString() + "L, but couldn't receive a proper RECT from it");
                return 0;
            }
            return rct.Bottom - rct.Top;
        }
        public static int ConfigHeight(Rainmeter.Settings.InstanceSettings Instance)
        {
            IntPtr hwnd = (IntPtr)(UInt32.Parse(Rainmeter.PluginBridge("GetWindow", Rainmeter.PluginBridge("GetConfig", Instance.INI_File))));
            RECT rct;
            if (!GetWindowRect(hwnd, out rct))
            {
                Rainmeter.Log(LogLevel.Error, "Rainmeter told us the HWND for window '" + Rainmeter.PluginBridge("GetConfig", Instance.INI_File) + "' is " + hwnd.ToInt32().ToString() + "L, but couldn't receive a proper RECT from it");
                return 0;
            }
            return rct.Bottom - rct.Top;
        }

        #region GetWindowRect() platform invoke to get the size/location of a window

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool GetWindowRect(IntPtr hwnd, out RECT lpRect);

        #endregion
        #region GetParent() platform invoke to get the handle of a parent window

        [DllImport("user32.dll", ExactSpelling = true, CharSet = CharSet.Auto)]
        private static extern IntPtr GetParent(IntPtr hWnd);

        [StructLayout(LayoutKind.Sequential)]
        public struct RECT
        {
            public int Left;        // x position of upper-left corner
            public int Top;         // y position of upper-left corner
            public int Right;       // x position of lower-right corner
            public int Bottom;      // y position of lower-right corner
        }
        #endregion
        #region SendMessage -- SendMessage (this variant is only for WM_COPYSTRUCT messages)

        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        private static extern IntPtr SendMessage(IntPtr hWnd, UInt32 Msg, IntPtr wParam, ref COPYDATASTRUCT lParam);

        [StructLayout(LayoutKind.Sequential)]
        struct COPYDATASTRUCT
        {
            public UInt32 dwData;
            public int cbData;
            public IntPtr lpData;
        }

        #endregion
        #region FindWindowByClass -- platform invoke to find a window given a class name

        [DllImport("user32.dll", EntryPoint = "FindWindow", SetLastError = true)]
        private static extern IntPtr FindWindowByClass(string lpClassName, IntPtr ZeroOnly);

        #endregion

        #region GetWindowInfo -- platform invoke to check a window's Z-order (Topmost=Auto)

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool GetWindowInfo(IntPtr hwnd, ref WINDOWINFO pwi);

        [StructLayout(LayoutKind.Sequential)]
        private struct WINDOWINFO
        {
            public uint cbSize;
            public RECT rcWindow;
            public RECT rcClient;
            public uint dwStyle;
            public uint dwExStyle;
            public uint dwWindowStatus;
            public uint cxWindowBorders;
            public uint cyWindowBorders;
            public ushort atomWindowType;
            public ushort wCreatorVersion;

            public WINDOWINFO(Boolean? filler)
                : this()   // Allows automatic initialization of "cbSize" with "new WINDOWINFO(null/true/false)".
            {
                cbSize = (UInt32)(Marshal.SizeOf(typeof(WINDOWINFO)));
            }
        }

        // Call this function to determine if the parent skin is topmost
        public static bool ParentIsTopmost(Rainmeter.Settings.InstanceSettings Instance)
        {
            IntPtr hwnd = (IntPtr)(UInt32.Parse(Rainmeter.PluginBridge("GetWindow", Rainmeter.PluginBridge("GetConfig", Instance.INI_File))));
            WINDOWINFO info = new WINDOWINFO(true);
            GetWindowInfo(hwnd, ref info);
            return ((info.dwExStyle & 0x00000008L) > 0);
        }

        #endregion
        #region SkinName -- gets the current skin name

        public static string SkinName(Rainmeter.Settings.InstanceSettings Instance)
        {
            try
            {
                return Instance.ConfigName;
                /*
                if (Instance.GetTempValue("_internal_SkinPath", string.Empty).ToString() == string.Empty)
                {
                    string sAppDataPath = System.Environment.GetEnvironmentVariable("AppData").Trim();
                    string sRainmeterINIText = System.IO.File.ReadAllText(sAppDataPath + "\\Rainmeter\\Rainmeter.ini");
                    string sSkinPath = Chopper(sRainmeterINIText.Replace('\n', '\r'), "SkinPath=", "\r").Trim().TrimEnd(new char[] { '\\' });
                    Instance.SetTempValue("_internal_SkinPath", sSkinPath);
                }

                System.IO.FileInfo fi = new System.IO.FileInfo(Instance.INI_File);
                return fi.DirectoryName.Replace(Instance.GetTempValue("_internal_SkinPath", string.Empty).ToString(), string.Empty).TrimEnd(new char[] { '\\' }).TrimStart(new char[] { '\\' });
                */
            }
            catch { }

            return string.Empty;
        }
        #endregion

        #endregion

        #region Chopper -- string manipulation

        public static string Chopper(string sText, string sSearch, string sEnd, int offset)
        {
            string sIntermediate = "";

            try
            {
                if ((sSearch == null) || (sSearch == string.Empty))
                {
                    sIntermediate = sText.Substring(offset);
                }
                else
                {
                    if (sText.Contains(sSearch) == false)
                        return sText;

                    sIntermediate = sText.Substring(sText.IndexOf(sSearch) + sSearch.Length + offset);
                }

                if ((sEnd == null) || (sEnd == string.Empty))
                    return sIntermediate;

                if (sIntermediate.Contains(sEnd) == false)
                    return sIntermediate;

                return sIntermediate.Substring(0, sIntermediate.IndexOf(sEnd));
            }
            catch
            {
                if (sIntermediate == "")
                    return sText;
                return sIntermediate;
            }
        }

        public static string Chopper(string sText, string sSearch, string sEnd)
        {
            return Chopper(sText, sSearch, sEnd, 0);
        }

        #endregion

        #region Version helpers (converts "1.04" or "1, 4" to 1004, etc.)

        public static UInt32 Version(double dVersion)
        {
            return (UInt32)(dVersion * 1000.0);
        }

        public static UInt32 Version(int iMajor, int iMinor)
        {
            return (UInt32)((iMajor * 1000) + iMinor);
        }

        #endregion

        #region Converts a C# 'string' to a C++ 'char *'
        public static unsafe char* String(string s)
        {
            fixed (char* p = s) return p;
        }
        #endregion

        #region Export for Rainmeter's new plugin bridge

        [DllImport("Rainmeter.dll", CharSet = CharSet.Auto)]
        private extern static unsafe char* PluginBridge(char* sCommand, char* sData);

        private unsafe static string PluginBridge(string sCommand, string sData)
        {
            try
            {
                return new string(PluginBridge(Rainmeter.String(sCommand), Rainmeter.String(sData)));
            }
            catch { }

            return string.Empty;
        }

        #endregion

        #region Read INI values using Rainmeter's 'ReadConfigString' export

        // We have to use this method rather than loading the .INI file manually because Rainmeter will
        // replace tokens for us.  See:  http://rainmeter.net/RainCMS/?q=Settings_BuiltInVariables
        //
        // Please note that this is done during plugin initialization and stored in the Instance
        // variable.  This is done automatically, so this function should not be used by plugin developers
        // directly, therefore the methods here are 'private'.

        [DllImport("Rainmeter.dll", CharSet = CharSet.Auto)]
        private extern static unsafe char* ReadConfigString(char* sSection, char* key, char* defValue);

        // If an INI is missing, a blank string will be returned to avoid raising exceptions
        private unsafe static string ReadConfigString(string sSection, string sKey)
        {
            try
            {
                char* szString = ReadConfigString(Rainmeter.String(sSection), Rainmeter.String(sKey), Rainmeter.String(string.Empty));

                if (szString != null)
                    return new string(szString);
            }
            catch { }

            return string.Empty;
        }

        #endregion

        #region Use Rainmeter's 'LSLog' export to log using its format and settings

        [DllImport("Rainmeter.dll", CharSet = CharSet.Auto)]
        private extern static unsafe UInt16 LSLog(int nLevel, char* pszModule, char* pszMessage);
        public enum LogLevel : int
        {
            Error = 1,
            Warning = 2,
            Notice = 3,
            Debug = 4
        }

        // You can call this function directly to log to Rainmeter.log
        //
        // Rainmeter needs to be configured to allow debug logging, of course.

        public static unsafe bool Log(LogLevel level, string sText)
        {
            return (Rainmeter.LSLog((int)level, Rainmeter.String("C# plugin"), Rainmeter.String(sText)) != 0);
        }

        #endregion

        #region Send a bang to Rainmeter (prepends a '!' if necessary)
        public static void Bang(string sBang)
        {
            if (!sBang.StartsWith("!"))
                sBang = "!" + sBang;
            System.Diagnostics.Process.Start(System.Windows.Forms.Application.ExecutablePath, sBang);
        }
        #endregion

        #region Settings are automatically created (and set at the top of 'Main.cs'), don't edit here

        // WARNING: DO NOT EDIT THIS HERE.  Change your author name, version, etc. in 'Main.cs'
        public class Settings
        {
            public string Author = "(unknown)";
            public double Version = 0.01;
            public string Email = string.Empty;
            public string Comments = string.Empty;

            public Settings(string _Author, double _Version)
            {
                this.Author = _Author;
                this.Version = _Version;
            }
            public Settings(string _Author, double _Version, string _Email)
            {
                this.Author = _Author;
                this.Version = _Version;
                this.Email = _Email;
            }
            public Settings(string _Author, double _Version, string _Email, string _Comments)
            {
                this.Author = _Author;
                this.Version = _Version;
                this.Email = _Email;
                this.Comments = _Comments;
            }

            public Dictionary<UInt32, InstanceSettings> Instances = new Dictionary<uint, InstanceSettings>();
            public List<SectionKey> KeyValues = new List<SectionKey>();

            public struct SectionKey
            {
                public UInt32 id;
                public string INI_File;
                public string Section;
                public string Key;
                public string Value;
            }

            public unsafe void AddInstance(char* iniFile, char* section, UInt32 id)
            {
                InstanceSettings new_instance = new InstanceSettings();
                new_instance.Initialize(this, iniFile, section, id);
                this.Instances.Add(id, new_instance);

                bool bInSection = false;
                foreach (string line in System.IO.File.ReadAllLines(new_instance.INI_File))
                {
                    if (line.Trim().StartsWith(";")) continue; // ignore comments
                    if (line.Trim().StartsWith("[")) bInSection = false; // new section

                    // section test
                    if (line.Trim().ToLower() == ("[" + new_instance.Section.ToLower() + "]"))
                        bInSection = true;

                    if (!bInSection) continue; // abort this pass if not in section
                    if (!line.Contains("=")) continue; // abort this pass if there's no INI setting here

                    string[] sLineParts = line.Trim().Split(new char[] { '=' });

                    SectionKey sk = new SectionKey();
                    sk.id = id;
                    sk.INI_File = new_instance.INI_File;
                    sk.Key = sLineParts[0].Trim();
                    sk.Section = new_instance.Section;
                    sk.Value = ReadConfigString(new_instance.Section, sLineParts[0].Trim());

                    this.KeyValues.Add(sk);
                }
            }

            public void RemoveInstance(UInt32 id)
            {
                try
                {
                start_over:
                    for (int i = 0; i < this.KeyValues.Count; i++)
                    {
                        if (this.KeyValues[i].id == id)
                        {
                            this.KeyValues.RemoveAt(i);
                            goto start_over; // start over since the IEnumerable has been modified
                        }
                    }
                    this.Instances.Remove(id);
                }
                catch { }
            }

            public class InstanceSettings
            {
                private UInt32 _ID = 0;
                private string _INI_File = string.Empty;
                private string _Section = string.Empty;
                private Settings PluginSettings = null;

                private Dictionary<string, object> TempData = new Dictionary<string, object>();

                private object locker = new object();

                public object GetTempValue(string name, object oDefault)
                {
                    lock (this.locker)
                    {
                        if (this.TempData.ContainsKey(name))
                            return this.TempData[name];
                        return oDefault;
                    }
                }

                public void SetTempValue(string name, object o)
                {
                    lock (this.locker)
                    {
                        if (this.TempData.ContainsKey(name))
                            this.TempData[name] = o;
                        else
                            this.TempData.Add(name, o);
                    }
                }

                public string INI_Value(string name)
                {
                    try
                    {
                        foreach (SectionKey sk in PluginSettings.KeyValues)
                            if (sk.id == this.ID)
                                if (sk.Section == this.Section)
                                    if (sk.Key.Trim().ToLower() == name.Trim().ToLower())
                                        return sk.Value;
                    }
                    catch { }

                    return string.Empty;
                }

                public unsafe void Initialize(Settings _PluginSettings, char* iniFile, char* section, UInt32 id)
                {
                    this.PluginSettings = _PluginSettings;
                    this._ID = id;
                    this._INI_File = new string(iniFile);
                    this._Section = new string(section);

                    this.ConfigName = Rainmeter.PluginBridge("GetConfig", this.INI_File);
                }

                public string GetVariable(string sVariable)
                {
                    return Rainmeter.PluginBridge("GetVariable", "\"" + this.ConfigName + "\" " + sVariable.Trim() + "");
                }

                public string SetVariable(string sVariable, object oData)
                {
                    return Rainmeter.PluginBridge("SetVariable", "\"" + this.ConfigName + "\" " + sVariable.Trim() + " \"" + oData.ToString().Trim() + "\"");
                }

                public string ConfigName = string.Empty;

                public UInt32 ID
                {
                    get
                    {
                        return this._ID;
                    }
                    set
                    {
                        this._ID = value;
                    }
                }

                public string INI_File
                {
                    get
                    {
                        return this._INI_File;
                    }
                }

                public string Section
                {
                    get
                    {
                        return this._Section;
                    }
                }
            }
        }
        #endregion
    }

    public class YourPlugin
    {
        #region YourPlugin class -- do not modify

        // This class is a simple launcher for your actual code in PluginCode.cs
        //
        // We make use of non-volatile data and threading to let your work run in another
        // thread, making Rainmeter nice and responsive.  Checks are automatically performed
        // so that overlapping of execution does not occur.

        // Default values of a blank string for GetUpdate() and zero for Update()/Update2()
        // are returned.

        public UInt32 Update(Rainmeter.Settings Plugin, UInt32 id)
        {
            bool bAlreadyRunning = (bool)Plugin.Instances[id].GetTempValue("__RMT_U_AlreadyRunning", false);
            if (!bAlreadyRunning)
            {
                UpdateThread thread_details = new UpdateThread(Plugin.Instances[id]);
                Thread thread = new Thread(new ThreadStart(thread_details.Go));
                thread.Start();
            }

            try
            {
                return (UInt32)Plugin.Instances[id].GetTempValue("__RMT_U_LastValue", 0);
            }
            catch
            {
                return 0;
            }
        }

        private class UpdateThread
        {
            private Rainmeter.Settings.InstanceSettings Instance = null;

            public UpdateThread(Rainmeter.Settings.InstanceSettings _Instance)
            {
                this.Instance = _Instance;
            }

            public void Go()
            {
                this.Instance.SetTempValue("__RMT_U_AlreadyRunning", true);

                try
                {
                    this.Instance.SetTempValue("__RMT_U_LastValue", new PluginCode().Update(this.Instance));
                }
                catch (Exception ex)
                {
                    Rainmeter.Log(Rainmeter.LogLevel.Error, "C# plugin in GetString(), " + ex.GetType().ToString() + ": " + ex.Message);
                }

                this.Instance.SetTempValue("__RMT_U_AlreadyRunning", false);
            }
        }

        public double Update2(Rainmeter.Settings Plugin, UInt32 id)
        {
            bool bAlreadyRunning = (bool)Plugin.Instances[id].GetTempValue("__RMT_U2_AlreadyRunning", false);
            if (!bAlreadyRunning)
            {
                Update2Thread thread_details = new Update2Thread(Plugin.Instances[id]);
                Thread thread = new Thread(new ThreadStart(thread_details.Go));
                thread.Start();
            }

            try
            {
                return (double)Plugin.Instances[id].GetTempValue("__RMT_U2_LastValue", 0.0);
            }
            catch
            {
                return 0.0;
            }
        }

        private class Update2Thread
        {
            private Rainmeter.Settings.InstanceSettings Instance = null;

            public Update2Thread(Rainmeter.Settings.InstanceSettings _Instance)
            {
                this.Instance = _Instance;
            }

            public void Go()
            {
                this.Instance.SetTempValue("__RMT_U2_AlreadyRunning", true);

                try
                {
                    this.Instance.SetTempValue("__RMT_U2_LastValue", new PluginCode().Update2(this.Instance));
                }
                catch (Exception ex)
                {
                    Rainmeter.Log(Rainmeter.LogLevel.Error, "C# plugin in GetString(), " + ex.GetType().ToString() + ": " + ex.Message);
                }

                this.Instance.SetTempValue("__RMT_U2_AlreadyRunning", false);
            }
        }

        public string GetString(Rainmeter.Settings Plugin, UInt32 id)
        {
            bool bAlreadyRunning = (bool)Plugin.Instances[id].GetTempValue("__RMT_GS_AlreadyRunning", false);
            if (!bAlreadyRunning)
            {
                GetStringThread thread_details = new GetStringThread(Plugin.Instances[id]);
                Thread thread = new Thread(new ThreadStart(thread_details.Go));
                thread.Start();
            }

            try
            {
                return (string)Plugin.Instances[id].GetTempValue("__RMT_GS_LastValue", string.Empty);
            }
            catch
            {
                return string.Empty;
            }
        }

        private class GetStringThread
        {
            private Rainmeter.Settings.InstanceSettings Instance = null;

            public GetStringThread(Rainmeter.Settings.InstanceSettings _Instance)
            {
                this.Instance = _Instance;
            }

            public void Go()
            {
                this.Instance.SetTempValue("__RMT_GS_AlreadyRunning", true);

                try
                {
                    this.Instance.SetTempValue("__RMT_GS_LastValue", new PluginCode().GetString(this.Instance));
                }
                catch (Exception ex)
                {
                    Rainmeter.Log(Rainmeter.LogLevel.Error, "C# plugin in GetString(), " + ex.GetType().ToString() + ": " + ex.Message);
                }

                this.Instance.SetTempValue("__RMT_GS_AlreadyRunning", false);
            }
        }

        public void ExecuteBang(Rainmeter.Settings Plugin, UInt32 id, string sArguments)
        {
            bool bAlreadyRunning = (bool)Plugin.Instances[id].GetTempValue("__RMT_EB_AlreadyRunning", false);
            if (!bAlreadyRunning)
            {
                ExecuteBangThread thread_details = new ExecuteBangThread(Plugin.Instances[id], sArguments);
                Thread thread = new Thread(new ThreadStart(thread_details.Go));
                thread.Start();
            }
            return;
        }

        private class ExecuteBangThread
        {
            private Rainmeter.Settings.InstanceSettings Instance = null;
            private string Command = string.Empty;

            public ExecuteBangThread(Rainmeter.Settings.InstanceSettings _Instance, string _Command)
            {
                this.Instance = _Instance;
                this.Command = _Command;
            }

            public void Go()
            {
                this.Instance.SetTempValue("__RMT_EB_AlreadyRunning", true);

                try
                {
                    new PluginCode().ExecuteBang(this.Instance, this.Command);
                }
                catch (Exception ex)
                {
                    Rainmeter.Log(Rainmeter.LogLevel.Error, "C# plugin in GetString(), " + ex.GetType().ToString() + ": " + ex.Message);
                }

                this.Instance.SetTempValue("__RMT_EB_AlreadyRunning", false);
            }
        }

        #endregion
    }
}

using System;
using Rainmeter;

namespace InputText
{
    public class Main
    {
        /// <summary>
        /// Your name (author) and the version of the plugin go here.  The code will
        /// automatically format the values and send them back when a call to GetAuthor()
        /// or GetVersion() is made.
        /// 
        /// Optionally, you may also provide an e-mail address and additional comments
        /// </summary>
        private static Rainmeter.Settings Plugin = new Rainmeter.Settings
        (
            // Author name
            "Peter Souza IV",

            // Version
            1.06,

            // E-mail
            "psouza4@gmail.com",

            // Comments (try to keep it under 50 characters)
            "Please visit the forums for support."
        );

        #region GetAuthor() and GetVersion() exports -- no need to modify

        [DllExport]
        public static UInt32 GetPluginVersion()
        {
            return Rainmeter.Version(Plugin.Version);
        }

        [DllExport]
        public unsafe static char* GetPluginAuthor()
        {
            if (!string.IsNullOrEmpty(Plugin.Email) && !string.IsNullOrEmpty(Plugin.Comments))
                return Rainmeter.String(Plugin.Author + " (" + Plugin.Email + "): " + Plugin.Comments);
            if (!string.IsNullOrEmpty(Plugin.Email))
                return Rainmeter.String(Plugin.Author + " (" + Plugin.Email + ")");
            if (!string.IsNullOrEmpty(Plugin.Comments))
                return Rainmeter.String(Plugin.Author + ": " + Plugin.Comments);
            return Rainmeter.String(Plugin.Author);
        }

        #endregion

        #region Initialize() and Finalize() -- you may add to these functions if necessary

        [DllExport]
        public unsafe static UInt32 Initialize(IntPtr instance, char* iniFile, char* section, UInt32 id)
        {
            Plugin.AddInstance(iniFile, section, id); // required: do not remove

            ////////////////////////////////////////////////////////////////
            //
            // You may add code here, if necessary
            //

            ////////////////////////////////////////////////////////////////
            return 0;
        }

        [DllExport]
        public static void Finalize(IntPtr instance, UInt32 id)
        {
            Plugin.RemoveInstance(id); // required: do not remove

            ////////////////////////////////////////////////////////////////
            //
            // You may add code here, if necessary
            //

            ////////////////////////////////////////////////////////////////
            return;
        }
        #endregion

        #region Update(), Update2(), and GetString() exports -- please read notes

        // *** WARNING / NOTES ***
        //
        // Do not add to this code:  change your code in PluginCode.cs instead
        //
        // However, due to the way Rainmeter works, you will probably want to
        // comment-out either 'Update' or 'Update2' if your plugin will be returning
        // numeric values.
        //
        // Rainmeter will look for 'Update' for positive integers.  If you want to
        // allow negative numbers or floating-point numbers, use 'Update2' instead.
        //
        // You *MUST* comment-out whichever function you don't want to use for this
        // to work.
        //
        // If you don't care, such as a plugin that either doesn't return data or
        // only returns string/text data, then you can leave them both here (it won't
        // hurt anything).
        //
        // *** WARNING / NOTES ***


        /// <summary>
        /// Rainmeter's request for numeric data from the plugin.  This version can only
        /// return positive integers ranging from 0 to 4,294,967,296.  Comment this member
        /// out and use 'Update2' if you want to return negative or floating-point values.
        /// </summary>
        /// <param name="id">The unique instance ID of this request.</param>
        /// <returns>Current value for this meter.</returns>
        [DllExport]
        public static UInt32 Update(UInt32 id)
        {
            // Do not modify this member (although you can comment it out).  Instead, update 
            // your code in 'PluginCode.cs'.
            return new YourPlugin().Update(Plugin, id);
        }

        /// <summary>
        /// Rainmeter's request for numeric data from the plugin.  This version can return
        /// positive or negative floating-point numbers (32-bit precision).  Comment this
        /// member out and use 'Update' if you want to only return positive integers.
        /// </summary>
        /// <param name="id">The unique instance ID of this request.</param>
        /// <returns>Current value for this meter.</returns>
        //[DllExport]
        //public static double Update2(UInt32 id)
        //{
        //    // Do not modify this member (although you can comment it out).  Instead, update 
        //    // your code in 'PluginCode.cs'.
        //    return new YourPlugin().Update2(Plugin, id);
        //}

        /// <summary>
        /// Rainmeter's request for text data from the plugin.
        /// </summary>
        /// <param name="id">The unique instance ID of this request.</param>
        /// <param name="flags">Unused still as of Dec 2nd, 2010.</param>
        /// <returns></returns>
        [DllExport]
        public unsafe static char* GetString(UInt32 id, UInt32 flags)
        {
            // Do not modify this member.  Instead, update your code in 'PluginCode.cs'.
            return Rainmeter.String(new YourPlugin().GetString(Plugin, id));
        }

        #endregion

        #region ExecuteBang() export -- no need to modify (change code in PluginCode.cs)

        [DllExport]
        public static unsafe void ExecuteBang(char* args, UInt32 id)
        {
            new YourPlugin().ExecuteBang(Plugin, id, new string(args));
            return;
        }

        #endregion
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

/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using Microsoft.Build.Utilities;
using System.IO;

namespace DllExporter
{
    class Program
    {
        static int Main(string[] args)
        {
            if (args.Length < 4)
            {
                Console.WriteLine("DllExporter error: Invalid arguments");
                return 1;
            }

            string configurationName = args[0];
            string platformTarget = args[1];
            string targetDirectory = args[2];
            string targetDllName = targetDirectory + args[3];
            string targetIlName = targetDllName + ".il";
            string targetResName = targetDllName + ".res";

            bool is64 = platformTarget.ToLower().Equals("x64");
            bool isDebug = configurationName.ToLower().Equals("debug");

            string ilasmPath = FindIlasmPath(is64);
            if (ilasmPath == null)
            {
                Console.WriteLine("DllExporter error: ilasm.exe not found");
                return 1;
            }

            string ildasmPath = FindIldasmPath();
            if (ildasmPath == null)
            {
                Console.WriteLine("DllExporter error: ildasm.exe not found");
                return 1;
            }

            Directory.SetCurrentDirectory(targetDirectory);

            // Disassemble
            Process ildasmProc = new Process();
            string ildasmArgs = string.Format(
                "/nobar {0} /output=\"{1}\" \"{2}\"",
                isDebug ? "/linenum" : "",
                targetIlName,
                targetDllName);

            ildasmProc.StartInfo = new ProcessStartInfo(ildasmPath, ildasmArgs);
            ildasmProc.StartInfo.UseShellExecute = false;
            ildasmProc.StartInfo.CreateNoWindow = false;
            ildasmProc.StartInfo.RedirectStandardOutput = true;
            ildasmProc.Start();
            ildasmProc.WaitForExit();

            if (ildasmProc.ExitCode != 0)
            {
                Console.WriteLine("DllExporter error: Unable to disassemble!");
                Console.WriteLine(ildasmProc.StandardOutput.ReadToEnd());
                return ildasmProc.ExitCode;
            }

            bool hasResource = File.Exists(targetResName);

            // Read disassembly and find methods marked with DllExport attribute
            List<string> lines = new List<string>(File.ReadAllLines(targetIlName));
            int attributeIndex = 0;
            int exportCount = 0;
            while (true)
            {
                attributeIndex = lines.FindIndex(attributeIndex, new Predicate<string>(x => x.Contains(".custom instance void") && x.Contains("DllExport::.ctor()")));
                if (attributeIndex < 8) break;

                int methodIndex = lines.FindLastIndex(attributeIndex, attributeIndex, new Predicate<string>(x => x.Contains(".method")));
                if (methodIndex == -1)
                {
                    Console.WriteLine("DllExporter error: Unable to parse disassembly (.method not found)!");
                    return 1;
                }

                int functionIndex = lines.FindIndex(methodIndex, new Predicate<string>(x => x.Contains("(")));
                if (functionIndex == -1)
                {
                    Console.WriteLine("DllExporter error: Unable to parse disassembly (bracket not found)!");
                    return 1;
                }

                int bracketPos = lines[functionIndex].IndexOf('(');
                int functionNamePos = lines[functionIndex].LastIndexOf(' ', bracketPos);
                string functionName = lines[functionIndex].Substring(functionNamePos, bracketPos - functionNamePos);

                // Change calling convention to cdecl
                lines[functionIndex] = string.Format("{0} modopt([mscorlib]System.Runtime.CompilerServices.CallConvCdecl) {1}", lines[functionIndex].Substring(0, functionNamePos - 1), lines[functionIndex].Substring(functionNamePos));

                int attributeBeginPos = lines[attributeIndex].IndexOf('.');
                string spaces = new string(' ', attributeBeginPos);

                // Replace attribute with export
                ++exportCount;
                lines[attributeIndex] = string.Format("{0}.export [{1}] as {2}", spaces, exportCount, functionName);

                ++attributeIndex;
            }

            if (exportCount == 0)
            {
                Console.WriteLine("DllExporter warning: Nothing found to export.");
            }

            // Remove the DllExport class
            int classIndex = lines.FindIndex(new Predicate<string>(x => x.Contains(".class ") && x.EndsWith(".DllExport")));
            if (classIndex == -1)
            {
                Console.WriteLine("DllExporter error: Unable to parse disassembly (DllExport class not found)!");
                return 1;
            }
            else
            {
                int classEndIndex = lines.FindIndex(classIndex, new Predicate<string>(x => x.Contains("} // end of class") && x.EndsWith(".DllExport")));
                if (classEndIndex == -1)
                {
                    Console.WriteLine("DllExporter error: Unable to parse disassembly (DllExport class end not found)!");
                    return 1;
                }

                lines.RemoveRange(classIndex, classEndIndex - classIndex + 2);
            }

            // Write everything back
            File.WriteAllLines(targetIlName, lines.ToArray());

            // Reassemble
            Process ilasmProc = new Process();
            string resource = hasResource ? string.Format("/resource=\"{0}\"", targetResName) : "";
            string ilasmArgs = string.Format("/nologo /quiet /dll {0} {1} /output=\"{2}\" {3} \"{4}\"", isDebug ? "/debug /pdb" : "/optimize", is64 ? "/x64 /PE64" : "", targetDllName, resource, targetIlName);
            ilasmProc.StartInfo = new ProcessStartInfo(ilasmPath, ilasmArgs);
            ilasmProc.StartInfo.UseShellExecute = false;
            ilasmProc.StartInfo.CreateNoWindow = false;
            ilasmProc.StartInfo.RedirectStandardOutput = true;
            ilasmProc.Start();
            ilasmProc.WaitForExit();

            if (ilasmProc.ExitCode != 0)
            {
                Console.WriteLine("DllExporter error: Unable to assemble!");
                Console.WriteLine(ilasmProc.StandardOutput.ReadToEnd());
                return ilasmProc.ExitCode;
            }

            // Cleanup
            File.Delete(targetIlName);
            File.Delete(targetResName);

            Console.WriteLine("DllExporter: Processed {0}", args[3]);

            return 0;
        }

        /// <summary>
        /// Finds path to ilasm.exe.
        /// </summary>
        private static string FindIlasmPath(bool x64)
        {
            var arch = x64 ? DotNetFrameworkArchitecture.Bitness64 : DotNetFrameworkArchitecture.Bitness32;
            var path = ToolLocationHelper.GetPathToDotNetFrameworkFile(
                "ilasm.exe", TargetDotNetFrameworkVersion.VersionLatest, arch);
            return File.Exists(path) ? path : null;
        }

        /// <summary>
        /// Finds path to ildasm.exe.
        /// </summary>
        private static string FindIldasmPath()
        {
            var sdkPath = Environment.ExpandEnvironmentVariables(@"%ProgramFiles(x86)%\Microsoft SDKs\Windows\");
            if (!Directory.Exists(sdkPath))
            {
                sdkPath = Environment.ExpandEnvironmentVariables(@"%ProgramFiles%\Microsoft SDKs\Windows\");
            }

            if (!Directory.Exists(sdkPath))
            {
                throw new DirectoryNotFoundException("'Microsoft SDKs' directory not found");
            }

            // Get the version directories.
            var sdkVersionDirectories = Directory.GetDirectories(sdkPath);
            foreach (var sdkVersionDirectory in sdkVersionDirectories)
            {
                var binDirectory = Path.Combine(sdkVersionDirectory, @"bin");
                if (!Directory.Exists(binDirectory))
                {
                    continue;
                }

                // Check for e.g. 'Microsoft SDKs\v8.0A\bin\ildasm.exe'.
                var ildasmPath = Path.Combine(binDirectory, @"ildasm.exe");
                if (File.Exists(ildasmPath))
                {
                    return ildasmPath;
                }

                // Check for e.g. 'Microsoft SDKs\v8.0A\bin\NETFX 4.0 Tools\ildasm.exe'.
                var toolsDirectories = Directory.GetDirectories(binDirectory, "NETFX*Tools");
                foreach (var toolDirectory in toolsDirectories)
                {
                    ildasmPath = Path.Combine(toolDirectory, @"ildasm.exe");
                    if (File.Exists(ildasmPath))
                    {
                        return ildasmPath;
                    }
                }
            }

            return null;
        }
    }
}

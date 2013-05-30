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
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using Microsoft.Build.Utilities;

namespace DllExporter
{
    class Program
    {
        static int Main(string[] args)
        {
            string configurationName = args[0];
            string platformTarget = args[1];
            string targetDirectory = args[2];
            string targetDllName = targetDirectory + args[3];
            string targetIlName = targetDllName + ".il";
            string targetResName = targetDllName + ".res";

            string ilasmPath = ToolLocationHelper.GetPathToDotNetFrameworkFile("ilasm.exe", TargetDotNetFrameworkVersion.Version20);
            if (!System.IO.File.Exists(ilasmPath))
            {
                Console.WriteLine("DllExporter error: ilasm.exe not found");
                return 1;
            }

            string ildasmPath = Environment.ExpandEnvironmentVariables(@"%ProgramFiles%\Microsoft SDKs\Windows\v7.0A\Bin\ildasm.exe");
            if (!System.IO.File.Exists(ildasmPath))
            {
                ildasmPath = Environment.ExpandEnvironmentVariables(@"%ProgramFiles(x86)%\Microsoft SDKs\Windows\v7.0A\Bin\ildasm.exe");
                if (!System.IO.File.Exists(ildasmPath))
                {
                    ildasmPath = Environment.ExpandEnvironmentVariables(@"%ProgramFiles(x86)%\Microsoft SDKs\Windows\v8.0A\bin\NETFX 4.0 Tools\ildasm.exe");
                    if (!System.IO.File.Exists(ildasmPath))
                    {
                        Console.WriteLine("DllExporter error: ildasm.exe not found");
                        return 1;
                    }
                }
            }

            System.IO.Directory.SetCurrentDirectory(targetDirectory);

            bool is64 = platformTarget.ToLower().Equals("x64");
            bool isDebug = configurationName.ToLower().Equals("debug");

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

            bool hasResource = System.IO.File.Exists(targetResName);

            // Read disassembly and find methods marked with DllExport attribute
            List<string> lines = new List<string>(System.IO.File.ReadAllLines(targetIlName));
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
            System.IO.File.WriteAllLines(targetIlName, lines.ToArray());

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
            System.IO.File.Delete(targetIlName);
            System.IO.File.Delete(targetResName);

            return 0;
        }
    }
}

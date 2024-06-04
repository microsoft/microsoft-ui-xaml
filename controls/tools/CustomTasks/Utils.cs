using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Xml;

namespace CustomTasks
{
    class Utils
    {
        //The XmlWriter can't handle &#xE0E5 unless we escape/unescape the ampersand
        public static string UnEscapeAmpersand(string s)
        {
            return s.Replace("&amp;", "&");
        }

        public static string EscapeAmpersand(string s)
        {
            return s.Replace("&", "&amp;");
        }

        public static string DocumentToString(Action<XmlWriter> action)
        {
            StringWriter sw = new StringWriter();
            XmlWriterSettings settings = new XmlWriterSettings { Indent = true, OmitXmlDeclaration = true, Encoding = Encoding.UTF8 };
            XmlWriter writer = XmlWriter.Create(sw, settings);
            action(writer);
            writer.Flush();
            return Utils.UnEscapeAmpersand(sw.ToString());
        }

        public static string RewriteFileIfNecessary(string path, string contents)
        {
            bool rewrite = true;
            var fullPath = Path.GetFullPath(path);
            try
            {
                string existingContents = File.ReadAllText(fullPath);
                if (String.Equals(existingContents, contents))
                {
                    rewrite = false;
                }
            }
            catch
            {
            }

            if (rewrite)
            {
                File.WriteAllText(fullPath, contents);
            }

            return fullPath;
        }

        // Validate that input winmds use consistent casing for their specified implementation DLLs
        public static bool ValidateImplementationDllCasing(IEnumerable<string> dllNames, Dictionary<Assembly, string> winmdToImplementationDll, out string errorMessage)
        {
            bool isValid = true;
            errorMessage = String.Empty;

            var normalizedDllNames = new HashSet<string>(dllNames.Select(name => name.ToLowerInvariant()));
            var problematicDllNames = new List<string>();

            foreach (var dllName in dllNames)
            {
                var matches = dllNames.Where(candidate => 
                    !String.Equals(candidate, dllName) 
                    && String.Equals(candidate.ToLowerInvariant(), dllName.ToLowerInvariant()));

                if (matches.Count() > 0)
                {
                    problematicDllNames.Add(dllName);
                }
            }

            if (problematicDllNames.Count() > 0)
            {
                StringBuilder builder = new StringBuilder();

                builder.AppendLine(
                    "Error encountered during WinRT Class Registration generation. " +
                    "Implementation DLL specifications with inconsistent casing were found. " +
                    "Verify that all input winmds that share the same implementation DLL " +
                    "use consistent casing in order to avoid manifest errors.");

                var dllToWinmd = problematicDllNames
                    .Select(name => new {
                        ImplementationDll = name,
                        WinMDs = winmdToImplementationDll
                            .Where(kvp => kvp.Value.Equals(name))
                            .Select(kvp => Path.GetFileName(kvp.Key.Location))
                    });

                foreach (var mapping in dllToWinmd.OrderBy(item => item.ImplementationDll))
                {
                    builder.AppendLine($"  * {mapping.ImplementationDll} => [{String.Join(", ", mapping.WinMDs)}]");
                }

                errorMessage = builder.ToString();
                isValid = false;
            }

            return isValid;
        }
    }

    static class TaskExtensions
    {
        public static void LogMessage(this Task task, string message, params object[] messageParams)
        {
            LogMessage(task, MessageImportance.Normal, message, messageParams);
        }

        public static void LogMessage(this Task task, MessageImportance messageImportance, string message, params object[] messageParams)
        {
            // If BuildEngine is null, we're not running in an MSBuild context,
            // so we'll output to the console in that case.
            // Otherwise, we'll use the Log object.
            if (task.BuildEngine == null)
            {
                Console.WriteLine(string.Format(message, messageParams));
            }
            else
            {
                task.Log.LogMessage(messageImportance, message, messageParams);
            }
        }

        public static void LogError(this Microsoft.Build.Utilities.Task task, string message, params object[] messageParams)
        {
            // If BuildEngine is null, we're not running in an MSBuild context,
            // so we'll output to the console in that case.
            // Otherwise, we'll use the Log object.
            if (task.BuildEngine == null)
            {
                Console.WriteLine(string.Format(message, messageParams));
            }
            else
            {
                task.Log.LogError(message, messageParams);
            }
        }
    }
}

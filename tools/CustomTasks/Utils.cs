using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;
using System;
using System.IO;
using System.Text;
using System.Xml;

namespace CustomTasks
{
    class Utils
    {
        //The XmlWriter can't handle &#xE70D unless we escape/unescape the ampersand
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

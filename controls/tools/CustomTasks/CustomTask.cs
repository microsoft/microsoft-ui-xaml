using Microsoft.Build.Utilities;
using System;
using System.Drawing;

namespace CustomTasks
{
    public abstract class CustomTask : Task
    {
        protected bool HasLoggedErrors
        {
            get
            {
                if (BuildEngine != null)
                {
                    return Log.HasLoggedErrors;
                }
                else
                {
                    return hasLoggedErrors;
                }
            }
        }

        private bool hasLoggedErrors = false;

        protected void LogMessage(string message, ConsoleColor? foregroundColor = null, ConsoleColor? backgroundColor = null)
        {
            message = AddColorsToString(message, foregroundColor, backgroundColor);

            if (BuildEngine != null)
            {
                Log.LogMessage(message);
            }
            else
            {
                Console.WriteLine(message);
            }
        }

        protected void LogWarning(string message)
        {
            LogWarning(string.Empty, string.Empty, string.Empty, string.Empty, -1, -1, message);
        }

        protected void LogWarning(string subcategory, string code, string helpKeyword, string file, int lineNumber, int columnNumber, string message)
        {
            if (BuildEngine != null)
            {
                Log.LogWarning(subcategory, code, helpKeyword, file, lineNumber, columnNumber, -1, -1, message);
            }
            else
            {
                Console.WriteLine($"Warning: {file} (Line {lineNumber}, column {columnNumber}: {message}");
            }
        }

        protected void LogError(string message)
        {
            LogError(string.Empty, string.Empty, string.Empty, string.Empty, -1, -1, message);
        }

        protected void LogError(string subcategory, string code, string helpKeyword, string file, int lineNumber, int columnNumber, string message)
        {
            if (BuildEngine != null)
            {
                Log.LogError(subcategory, code, helpKeyword, file, lineNumber, columnNumber, -1, -1, message);
            }
            else
            {
                Console.WriteLine($"Error: {file} (Line {lineNumber}, column {columnNumber}: {message}");
                hasLoggedErrors = true;
            }
        }

        private string AddColorsToString(string s, ConsoleColor? foregroundColor, ConsoleColor? backgroundColor)
        {
            // If we're building inside Visual Studio, we should not add any colors to the string.
            // That only works when we're building in a console window. We'll detect that we're building
            // inside Visual Studio by checking for a global property that is only set in that context.
            if (BuildEngine6 != null &&
                BuildEngine6.GetGlobalProperties().TryGetValue("VSIDEResolvedNonMSBuildProjectOutputs", out _))
            {
                return s;
            }

            if (foregroundColor != null && backgroundColor != null)
            {
                return $"\u001b[38;2;{ConsoleColorToRgbString(foregroundColor.Value)};48;2;{ConsoleColorToRgbString(backgroundColor.Value)}m{s}\u001b[0m";
            }
            else if (foregroundColor != null)
            {
                return $"\u001b[38;2;{ConsoleColorToRgbString(foregroundColor.Value)}m{s}\u001b[0m";
            }
            else if (backgroundColor != null)
            {
                return $"\u001b[48;2;{ConsoleColorToRgbString(backgroundColor.Value)}m{s}\u001b[0m";
            }
            else
            {
                return s;
            }
        }

        private string ConsoleColorToRgbString(ConsoleColor consoleColor)
        {
            Color color;

            switch (consoleColor)
            {
                case ConsoleColor.Black:
                    color = Color.FromArgb(0x00, 0x00, 0x00);
                    break;
                case ConsoleColor.DarkBlue:
                    color = Color.FromArgb(0x00, 0x00, 0x80);
                    break;
                case ConsoleColor.DarkGreen:
                    color = Color.FromArgb(0x00, 0x80, 0x00);
                    break;
                case ConsoleColor.DarkCyan:
                    color = Color.FromArgb(0x00, 0x80, 0x80);
                    break;
                case ConsoleColor.DarkRed:
                    color = Color.FromArgb(0x80, 0x00, 0x00);
                    break;
                case ConsoleColor.DarkMagenta:
                    color = Color.FromArgb(0x80, 0x00, 0x80);
                    break;
                case ConsoleColor.DarkYellow:
                    color = Color.FromArgb(0x80, 0x80, 0x00);
                    break;
                case ConsoleColor.DarkGray:
                    color = Color.FromArgb(0x80, 0x80, 0x80);
                    break;
                case ConsoleColor.Gray:
                    color = Color.FromArgb(0xC0, 0xC0, 0xC0);
                    break;
                case ConsoleColor.Blue:
                    color = Color.FromArgb(0x00, 0x00, 0xFF);
                    break;
                case ConsoleColor.Green:
                    color = Color.FromArgb(0x00, 0xFF, 0x00);
                    break;
                case ConsoleColor.Cyan:
                    color = Color.FromArgb(0x00, 0xFF, 0xFF);
                    break;
                case ConsoleColor.Red:
                    color = Color.FromArgb(0xFF, 0x00, 0x00);
                    break;
                case ConsoleColor.Magenta:
                    color = Color.FromArgb(0xFF, 0x00, 0xFF);
                    break;
                case ConsoleColor.Yellow:
                    color = Color.FromArgb(0xFF, 0xFF, 0x00);
                    break;
                case ConsoleColor.White:
                    color = Color.FromArgb(0xFF, 0xFF, 0xFF);
                    break;
                default:
                    throw new ArgumentException($"Invalid console color: {consoleColor}");
            }

            return $"{color.R};{color.G};{color.B}";
        }
    }
}

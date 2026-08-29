// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop;
using System;
using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Markup.Compiler.Executable
{
    internal class ConsoleLogger : ILog
    {
        public ConsoleLogger()
        {
        }

        public List<MSBuildLogEntry> Entries { get; private set; } = new List<MSBuildLogEntry>();

        public void LogError(string subcategory, string errorCode, string helpKeyword, string file, int lineNumber, int columnNumber, int endLineNumber, int endColumnNumber, string message)
        {
            Entries.Add(new MSBuildLogEntry(MSBuildLogEntry.EntryType.Error, subcategory, errorCode, helpKeyword, file, lineNumber, columnNumber, endLineNumber, endColumnNumber, message));
        }

        public void LogWarning(string subcategory, string warningCode, string helpKeyword, string file, int lineNumber, int columnNumber, int endLineNumber, int endColumnNumber, string message)
        {
            Entries.Add(new MSBuildLogEntry(MSBuildLogEntry.EntryType.Warning, subcategory, warningCode, helpKeyword, file, lineNumber, columnNumber, endLineNumber, endColumnNumber, message));
        }

        public void LogDiagnosticMessage(string message)
        {
            Entries.Add(new MSBuildLogEntry(MSBuildLogEntry.EntryType.Message, message));
        }
    }
}

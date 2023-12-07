// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop
{
    public sealed class MSBuildLogEntry
    {
        public enum EntryType
        {
            Message,
            Warning,
            Error
        };

        public EntryType Type;

        public string SubCategory { get; set; }
        public string ErrorCode { get; set; }
        public string HelpKeyword { get; set; }
        public string File { get; set; }
        public int LineNumber { get; set; }
        public int ColumnNumber { get; set; }
        public int EndLineNumber { get; set; }
        public int EndColumnNumber{ get; set; }

        public string Message { get; set; }

        public MSBuildLogEntry()
        {

        }

        public MSBuildLogEntry(EntryType type, string message)
        {
            Type = type;
            Message = message;
        }

        public MSBuildLogEntry(EntryType type, string subcategory, string errorCode, string helpKeyword, string file, int lineNumber, int columnNumber, int endLineNumber, int endColumnNumber, string message)
        {
            Type = type;
            SubCategory = subcategory;
            ErrorCode = errorCode;
            HelpKeyword = helpKeyword;
            File = file;
            LineNumber = lineNumber;
            ColumnNumber = columnNumber;
            EndLineNumber = endLineNumber;
            EndColumnNumber = endColumnNumber;
            Message = message;
        }
    }
}

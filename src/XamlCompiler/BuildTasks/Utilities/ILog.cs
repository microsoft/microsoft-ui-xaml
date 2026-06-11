// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    interface ILog
    {
        void LogError(
            string subcategory,
            string errorCode,
            string helpKeyword,
            string file,
            int lineNumber,
            int columnNumber,
            int endLineNumber,
            int endColumnNumber,
            string message
        );

        void LogWarning(
            string subcategory,
            string warningCode,
            string helpKeyword,
            string file,
            int lineNumber,
            int columnNumber,
            int endLineNumber,
            int endColumnNumber,
            string message
        );

        void LogDiagnosticMessage(string message);
    }
}

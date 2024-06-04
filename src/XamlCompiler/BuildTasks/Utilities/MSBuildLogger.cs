// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Build.Framework;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Microsoft.UI.Xaml.Markup.Compiler.Tasks
{
    internal class MSBuildLogger : ILog
    {
        private Microsoft.Build.Utilities.TaskLoggingHelper logger;

        public MSBuildLogger(Microsoft.Build.Utilities.TaskLoggingHelper logger)
        {
            this.logger = logger;
        }

        public void LogError(string subcategory, string errorCode, string helpKeyword, string file, int lineNumber, int columnNumber, int endLineNumber, int endColumnNumber, string message)
        {
            logger.LogError(subcategory, errorCode, helpKeyword, file, lineNumber, columnNumber, endLineNumber, endColumnNumber, message);
        }

        public void LogWarning(string subcategory, string warningCode, string helpKeyword, string file, int lineNumber, int columnNumber, int endLineNumber, int endColumnNumber, string message)
        {
            logger.LogWarning(subcategory, warningCode, helpKeyword, file, lineNumber, columnNumber, endLineNumber, endColumnNumber, message);
        }

        public void LogDiagnosticMessage(string message)
        {
            logger.LogMessage(MessageImportance.Low, message);
        }
    }
}

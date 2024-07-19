// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Build.Evaluation;
using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;
using Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;

namespace Microsoft.UI.Xaml.Markup.Compiler.IO
{
    public class OutputDeserializer : Task, ICompileXamlOutputs
    {
        #region ICompileXamlOutputs backing fields
        private List<ITaskItem> _generatedCodeFiles = new List<ITaskItem>();
        private List<ITaskItem> _generatedXamlFiles = new List<ITaskItem>();
        private List<ITaskItem> _generatedXbfFiles = new List<ITaskItem>();
        private List<ITaskItem> _generatedXamlPagesFiles = new List<ITaskItem>();
        #endregion

        #region ICompileXamlOutputs implementation

        [Output]
        public ITaskItem[] GeneratedCodeFiles
        {
            get
            {
                return _generatedCodeFiles.ToArray();
            }
        }

        [Output]
        public ITaskItem[] GeneratedXamlFiles
        {
            get
            {
                return _generatedXamlFiles.ToArray();
            }
        }

        [Output]
        public ITaskItem[] GeneratedXbfFiles
        {
            get
            {
                return _generatedXbfFiles.ToArray();
            }
        }

        [Output]
        public ITaskItem[] GeneratedXamlPagesFiles
        {
            get
            {
                return _generatedXamlPagesFiles.ToArray();
            }
        }
        #endregion


        #region Task Input Properties
        public string JsonFilePath { get; set; }
        #endregion

        public override bool Execute()
        {
            try
            {
                using (FileStream jsonFileStream = new FileStream(JsonFilePath, FileMode.Open, FileAccess.Read))
                {
                    CompilerOutputs outputs = JsonSerializer.Deserialize<CompilerOutputs>(jsonFileStream);
                    ExtractWrapperResults(outputs);
                    return true;
                }
            }
            catch (Exception e)
            {
                Log.LogError(e.Message);
                Log.LogMessage(MessageImportance.Low, e.StackTrace);
                return false;
            }
        }

        private void ExtractWrapperResults(CompilerOutputs wrapper)
        {
            foreach (string file in wrapper.GeneratedCodeFiles)
            {
                // File Paths need to be "escaped".  If they have URL escapes in them like %23
                // they will get processed, and we want to leave them alone
                TaskItem genFile = new TaskItem(ProjectCollection.Escape(file));
                this._generatedCodeFiles.Add(genFile);
            }

            foreach (var item in wrapper.GeneratedXamlFiles)
            {
                this._generatedXamlFiles.Add(new TaskItem(item));
            }

            foreach (var item in wrapper.GeneratedXbfFiles)
            {
                this._generatedXbfFiles.Add(new TaskItem(item));
            }

            foreach (string file in wrapper.GeneratedXamlPagesFiles)
            {
                TaskItem item = new TaskItem(ProjectCollection.Escape(file));
                this._generatedXamlPagesFiles.Add(item);
            }

            foreach (MSBuildLogEntry entry in wrapper.MSBuildLogEntries)
            {
                if (entry.Type == MSBuildLogEntry.EntryType.Message)
                {
                    Log.LogMessage(MessageImportance.Low, entry.Message);
                }
                else if (entry.Type == MSBuildLogEntry.EntryType.Warning)
                {
                    Log.LogWarning(
                        entry.SubCategory, entry.ErrorCode, entry.HelpKeyword, entry.File, entry.LineNumber,
                        entry.ColumnNumber, entry.EndLineNumber, entry.EndColumnNumber, entry.Message);
                }
                else if (entry.Type == MSBuildLogEntry.EntryType.Error)
                {
                    Log.LogError(
                        entry.SubCategory, entry.ErrorCode, entry.HelpKeyword, entry.File, entry.LineNumber,
                        entry.ColumnNumber, entry.EndLineNumber, entry.EndColumnNumber, entry.Message);
                }
                else
                {
                    Log.LogError($"Unhandled MSBuildLogEntry type: {entry.Type}");
                }
            }
        }
    }
}

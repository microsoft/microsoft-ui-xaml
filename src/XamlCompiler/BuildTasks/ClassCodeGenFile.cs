// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;

namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    internal class ClassCodeGenFile
    {
        private string _classFullName;
        private List<TaskItemFilename> _xamlTaskItems;

        public ClassCodeGenFile(string classFullName)
        {
            _classFullName = classFullName;
        }

        public string TargetFolderFullPath { get; set; }

        public string BaseFileName { get; set; }

        private void EnsureXamlTaskItems()
        {
            if (_xamlTaskItems == null)
            {
                _xamlTaskItems = new List<TaskItemFilename>();
            }
        }

        private void RecalculateBaseFileNameWithTif(TaskItemFilename tif)
        {
            string xamlFileName = Path.GetFileNameWithoutExtension(tif.SourceXamlFullPath);
            if (String.IsNullOrEmpty(this.BaseFileName) || this.BaseFileName.Length > xamlFileName.Length)
            {
                this.BaseFileName = xamlFileName;
            }
        }

        private void ResetBaseFileName()
        {
            this.BaseFileName = null;
            foreach (TaskItemFilename tif in this._xamlTaskItems)
            {
                this.RecalculateBaseFileNameWithTif(tif);
            }
        }

        public void AddTaskItem(TaskItemFilename tif)
        {
            this.EnsureXamlTaskItems();
            this._xamlTaskItems.Add(tif);
            this.RecalculateBaseFileNameWithTif(tif);
        }

        public void RemoveTaskItem(TaskItemFilename tif)
        {
            Debug.Assert(this._xamlTaskItems != null);
            Debug.Assert(this._xamlTaskItems.Contains(tif));
            this._xamlTaskItems.Remove(tif);
            this.ResetBaseFileName();
        }

        public ReadOnlyCollection<TaskItemFilename> XamlTaskItems
        {
            get
            {
                this.EnsureXamlTaskItems();
                return _xamlTaskItems.AsReadOnly();
            }
        }

    }
}

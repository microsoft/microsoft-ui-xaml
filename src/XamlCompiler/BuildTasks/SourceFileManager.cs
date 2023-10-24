// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using MSBuildInterop;
    using CodeGen;
    using Utilities;

    internal class SourceFileManager
    {
        private Dictionary<string, ClassCodeGenFile> _classXamlFilesMap;

        public string ProjectFolderFullpath { get; private set; }
        public string OutputFolderFullpath { get; private set; }
        public List<string> IncludeFolderList { get; private set; }
        public bool IsPass1 { get; private set; }
        public bool XbfGenerationIsDisabled { get; private set; }
        public List<TaskItemFilename> ProjectXamlTaskItems { get; private set; }
        public List<TaskItemFilename> Sdk80XamlTaskItems { get; private set; }
        public List<TaskItemFilename> SdkNon80XamlTaskItems { get; private set; }
        public List<TaskItemFilename> SdkXamlTaskItems { get; private set; }
        public BuildTaskFileService TaskFileService { get; private set; }
        public SavedStateManager SavedState { get; private set; }
        public List<TaskItemFilename> ClasslessXamlFiles { get; private set; }
        public string GeneratedFileExtension { get; private set; }
        public SourceFileManager(CompileXamlInternal compiler)
        {
            IsPass1 = compiler.IsPass1;
            SavedState = compiler.SaveState;
            XbfGenerationIsDisabled = compiler.DisableXbfGeneration;
            ProjectFolderFullpath = compiler.ProjectFolderFullpath;
            OutputFolderFullpath = compiler.OutputFolderFullpath;
            IncludeFolderList = compiler.IncludeFolderList;
            TaskFileService = compiler.TaskFileService;
            GeneratedFileExtension = compiler.GeneratedExtension;

            ClasslessXamlFiles = new List<TaskItemFilename>();
            ProjectXamlTaskItems = new List<TaskItemFilename>();

            if (compiler.XamlApplications != null)
            {
                foreach (var item in compiler.XamlApplications)
                {
                    LoadTaskItem(ProjectXamlTaskItems, item, isApplication: true, isSdkXaml: false);
                }
            }
            if (compiler.XamlPages != null)
            {
                foreach (var item in compiler.XamlPages)
                {
                    LoadTaskItem(ProjectXamlTaskItems, item, isApplication: false, isSdkXaml: false);
                }
            }

            if (compiler.SdkXamlPages != null)
            {
                Sdk80XamlTaskItems = new List<TaskItemFilename>();
                SdkNon80XamlTaskItems = new List<TaskItemFilename>();
                SdkXamlTaskItems = new List<TaskItemFilename>();

                foreach (var item in compiler.SdkXamlPages)
                {
                    var sdkTif = LoadTaskItem(SdkXamlTaskItems, item, isApplication: false, isSdkXaml: true);
                    SdkNon80XamlTaskItems.Add(sdkTif);
                }
            }
        }

        public void SaveState()
        {
            this.PrepareToSaveState();
            this.SavedState.Save();
        }

        private void PrepareToSaveState()
        {
            if (_classXamlFilesMap != null)
            {
                foreach (ClassCodeGenFile classCode in _classXamlFilesMap.Values)
                {
                    foreach (TaskItemFilename xamlItem in classCode.XamlTaskItems)
                    {
                        this.SavedState.SetGeneratedCodeFilePathPrefix(xamlItem.XamlGivenPath, Path.Combine(classCode.TargetFolderFullPath, classCode.BaseFileName));
                    }
                }
            }
        }

        public void PropagateOutOfDateStatus(DirectUI.DirectUISchemaContext context)
        {
            HashSet<string> outOfDateClasses = null;

            foreach (TaskItemFilename tif in ProjectXamlTaskItems)
            {
                if (tif.OutOfDate())
                {
                    if (outOfDateClasses == null)
                    {
                        outOfDateClasses = new HashSet<string>();
                    }

                    // invalidate the cached class
                    if (!string.IsNullOrEmpty(tif.ClassFullName))
                    {
                        outOfDateClasses.Add(tif.ClassFullName);
                    }

                    // look to see if a new class was specified; if so, we guessed wrong before, undo that.
                    // Note that both the old and new classes are out of date.
                    string newClassFullName = null;
                    using (var fileReader = TaskFileService.GetFileContents(tif.SourceXamlFullPath))
                    {
                        newClassFullName = XamlNodeStreamHelper.ReadXClassFromXamlFileStream(fileReader, context);
                    }

                    if (newClassFullName != tif.ClassFullName)
                    {
                        UnregisterClassOfTaskItem(tif);
                        tif.ClassFullName = newClassFullName;
                        RegisterClassOfTaskItem(tif);

                        if (!string.IsNullOrEmpty(tif.ClassFullName))
                        {
                            outOfDateClasses.Add(tif.ClassFullName);
                        }
                    }
                }
            }

            if (outOfDateClasses != null)
            {
                foreach (string className in outOfDateClasses)
                {
                    IReadOnlyCollection<TaskItemFilename> xamlTaskItems = _classXamlFilesMap[className].XamlTaskItems;
                    if (xamlTaskItems.Count == 0)
                    {
                        _classXamlFilesMap.Remove(className);
                    }
                    else
                    {
                        foreach (TaskItemFilename tif in xamlTaskItems)
                        {
                            tif.IsForcedOutOfDate = true;
                            if (File.Exists(tif.XamlOutputFilename))
                            {
                                // delete the output file so we will be sure to rebuild it in Pass2. Timestamps are insufficient since multiple files per class are involved.
                                File.Delete(tif.XamlOutputFilename);
                            }
                        }
                    }
                }
            }
        }

        public TaskItemFilename FindTaskItemByFullPath(string fullPath)
        {
            foreach (TaskItemFilename tif in ProjectXamlTaskItems)
            {
                if (fullPath == tif.SourceXamlFullPath)
                    return tif;
            }
            Debug.Fail(String.Format("TIF lookup for {0} failed", fullPath));
            return null;
        }

        DateTime GetSourceFileChanged(TaskItemFilename tif)
        {
            return TaskFileService.GetLastChangeTime(tif.SourceXamlFullPath);
        }

        private void AddToXamlClassFileMap(string classFullName, TaskItemFilename tif)
        {
            if (_classXamlFilesMap == null)
            {
                _classXamlFilesMap = new Dictionary<string, ClassCodeGenFile>();
            }
            ClassCodeGenFile codeFile;
            if (!_classXamlFilesMap.TryGetValue(classFullName, out codeFile))
            {
                codeFile = new ClassCodeGenFile(classFullName);
                codeFile.TargetFolderFullPath = tif.TargetFolder;
                _classXamlFilesMap.Add(classFullName, codeFile);
            }
            else
            {
                codeFile.TargetFolderFullPath = FileHelpers.ComputeBaseFolder(codeFile.TargetFolderFullPath, tif.TargetFolder);
            }
            codeFile.AddTaskItem(tif);
        }

        private void RemoveFromXamlClassFileMap(string classFullName, TaskItemFilename tif)
        {
            ClassCodeGenFile codeFile = _classXamlFilesMap[tif.ClassFullName];
            codeFile.RemoveTaskItem(tif);
        }

        public string GetTargetFolderOfClass(string classFullName)
        {
            if (_classXamlFilesMap != null)
                return null;

            ClassCodeGenFile codeFile;
            if (_classXamlFilesMap.TryGetValue(classFullName, out codeFile))
            {
                return codeFile.TargetFolderFullPath;
            }
            return null;
        }

        public IEnumerable<ClassCodeGenFile> CodeGenFiles
        {
            get
            {
                if (_classXamlFilesMap == null)
                {
                    return new List<ClassCodeGenFile>();
                }
                else
                {
                    return _classXamlFilesMap.Values;
                }
            }
        }

        private TaskItemFilename LoadTaskItem(List<TaskItemFilename> tifList, IFileItem item, bool isApplication, bool isSdkXaml)
        {
            TaskItemFilename tif = new TaskItemFilename(item, this, isApplication, isSdkXaml);
            tif.XamlLastChangeTime = GetSourceFileChanged(tif);
            SavedState.LoadSavedTaskItemInfo(tif);
            this.RegisterClassOfTaskItem(tif);
            tifList.Add(tif);
            return tif;
        }

        private void RegisterClassOfTaskItem(TaskItemFilename tif)
        {
            if (string.IsNullOrEmpty(tif.ClassFullName))
            {
                ClasslessXamlFiles.Add(tif);
            }
            else
            {
                AddToXamlClassFileMap(tif.ClassFullName, tif);
            }
        }

        private void UnregisterClassOfTaskItem(TaskItemFilename tif)
        {
            if (string.IsNullOrEmpty(tif.ClassFullName))
            {
                ClasslessXamlFiles.Remove(tif);
            }
            else
            {
                RemoveFromXamlClassFileMap(tif.ClassFullName, tif);
            }
        }
    }
}

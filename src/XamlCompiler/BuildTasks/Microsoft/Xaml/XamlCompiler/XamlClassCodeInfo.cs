// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Text;
    using System.Xaml;
    using CodeGen;
    using Properties;
    using Utilities;
    using XamlDom;

    // Code Gen Information about the XAML Class as a whole, contains a list of per-XamlFile code infos.
    internal class XamlClassCodeInfo : IXamlClassCodeInfo
    {
        private Dictionary<string, FieldDefinition> fieldDefinitions;
        private List<FieldDefinition> fieldDeclarations;
        private List<XamlFileCodeInfo> xamlFileCodeInfoList = new List<XamlFileCodeInfo>();
        private List<BindUniverse> bindUniverses;
        private bool? hasEventAssignments;
        private BindStatus? bindStatus;
        private bool? hasPhaseAssignments;
        private string baseFileName;
        private int lastConnectionId = 0;

        public XamlClassCodeInfo(string classFullName, bool isApplication)
        {
            this.ClassName = new ClassName(classFullName);
            this.IsApplication = isApplication;
        }

        public bool IsApplication { get; private set; }
        public string RootNamespace { get; set; }
        public ClassName ClassName { get; set; }

        public string BaseFileName
        {
            get
            {
                if (string.IsNullOrEmpty(this.baseFileName))
                {
                    this.baseFileName = this.ComputeBaseFileName();
                }

                return this.baseFileName;
            }
        }
        public string BaseApparentRelativeFolder { get; private set; }
        public string BaseApparentRelativePath
        {
            get
            {
                return Path.Combine(this.BaseApparentRelativeFolder, this.BaseFileName) + KnownStrings.XamlExtension;
            }
        }

        public string TargetFolder { get; set; }
        public string PriIndexName { get; set; }

        /// <summary>
        /// Class type, only availible in Pass2.
        /// This can also be null if the class is not public.  (like the App Class)
        /// </summary>
        public XamlType ClassXamlType { get; set; }
        public TypeForCodeGen ClassType { get; set; }

        public string BaseTypeName { get; set; }
        public XamlType BaseType { get; set; }

        public string XamlResourceMapName { get; set; }
        public string XamlComponentResourceLocation { get; set; }

        public bool IsResourceDictionary
        {
            get
            {
                return this.ClassXamlType != null && this.ClassXamlType.IsDerivedFromResourceDictionary();
            }
        }

        public bool HasFieldDefinitions
        {
            get
            {
                return this.fieldDefinitions != null && this.fieldDefinitions.Count > 0;
            }
        }

        public bool HasEventAssignments
        {
            get
            {
                if (!this.hasEventAssignments.HasValue)
                {
                    this.hasEventAssignments = this.xamlFileCodeInfoList.Any(x => x.HasEventAssignments);
                }
                return this.hasEventAssignments.Value;
            }
        }

        public BindStatus BindStatus
        {
            get
            {
                if (!this.bindStatus.HasValue)
                {
                    this.bindStatus = BindStatus.None;
                    foreach (XamlFileCodeInfo fileCodeInfo in this.xamlFileCodeInfoList)
                    {
                        this.bindStatus |= fileCodeInfo.BindStatus;
                    }
                }
                return this.bindStatus.Value;
            }
        }

        public bool HasPhaseAssignments
        {
            get
            {
                if (!this.hasPhaseAssignments.HasValue)
                {
                    this.hasPhaseAssignments = this.xamlFileCodeInfoList.Any(x => x.HasPhaseAssignments);
                }
                return this.hasPhaseAssignments.Value;
            }
        }

        public List<BindUniverse> BindUniverses
        {
            get
            {
                if (this.bindUniverses == null)
                {
                    this.bindUniverses = new List<BindUniverse>();
                }
                return this.bindUniverses;
            }
        }

        // The field info for "declarations" are stored in the Code Info.
        public IEnumerable<FieldDefinition> FieldDeclarations
        {
            get
            {
                if (this.fieldDeclarations == null)
                {
                    this.fieldDeclarations = new List<FieldDefinition>();
                    if (this.fieldDefinitions != null)
                    {
                        this.fieldDeclarations.AddRange(this.fieldDefinitions.Values);
                    }
                }
                return this.fieldDeclarations;
            }
        }

        public List<XamlFileCodeInfo> PerXamlFileInfo
        {
            get
            {
                return this.xamlFileCodeInfoList;
            }
        }

        public void AddXamlFileInfo(XamlFileCodeInfo fileCodeInfo)
        {
            this.xamlFileCodeInfoList.Add(fileCodeInfo);
            foreach (ConnectionIdElement connectionIdElement in fileCodeInfo.ConnectionIdElements)
            {
                if (connectionIdElement.FieldDefinition != null)
                {
                    bool wasMerged = this.AddFieldDefinition(connectionIdElement.FieldDefinition);
                    if (wasMerged)
                    {
                        // MultiXaml TODO:  Consider if I need to fix up the Per File Field Definition
                    }
                }
            }

            string apparentRelativeFolder = Path.GetDirectoryName(fileCodeInfo.ApparentRelativePath);
            if (this.BaseApparentRelativeFolder == null)
            {
                this.BaseApparentRelativeFolder = apparentRelativeFolder;
            }
            else
            {
                this.BaseApparentRelativeFolder = FileHelpers.ComputeBaseFolder(this.BaseApparentRelativeFolder, apparentRelativeFolder);
            }
        }

        private bool AddFieldDefinition(FieldDefinition newFieldDef)
        {
            if (this.fieldDefinitions == null)
            {
                this.fieldDefinitions = new Dictionary<string, FieldDefinition>();
            }

            bool merged = false;

            FieldDefinition fieldDef;
            string name = newFieldDef.FieldName;

            if (this.fieldDefinitions.TryGetValue(name, out fieldDef))
            {
                if (fieldDef.HasSameAttributes(newFieldDef))
                {
                    // already the same, no reconciling necessary.
                    return false;
                }

                if (!fieldDef.CanBeMerged(newFieldDef))
                {
                    // MultiXaml TODO report error
                    return false;
                }

                this.fieldDefinitions.Remove(name);
                FieldDefinition mergedFieldDef = FieldDefinition.CreateMerged(fieldDef, newFieldDef);
                newFieldDef = mergedFieldDef;
                merged = true;
            }

            this.fieldDefinitions.Add(name, newFieldDef);

            return merged;
        }

        private string ComputeBaseFileName()
        {
            Debug.Assert(this.xamlFileCodeInfoList.Count != 0);

            int index = 0;
            List<string[]> fileNamesTokens = new List<string[]>();
            StringBuilder strBuilder = new StringBuilder();

            foreach (XamlFileCodeInfo xamlFileCodeInfo in this.xamlFileCodeInfoList)
            {
                fileNamesTokens.Add(Path.GetFileNameWithoutExtension(xamlFileCodeInfo.ApparentRelativePath).Split(new char[] { '.' }, StringSplitOptions.RemoveEmptyEntries));
            }

            Action getLongestCommonPrefix = () =>
            {
                string currentToken;
                do
                {
                    currentToken = null;
                    foreach (string[] fileNameTokenized in fileNamesTokens)
                    {
                        if (fileNameTokenized.Length <= index) { return; }

                        if (currentToken == null)
                        {
                            currentToken = fileNameTokenized[index];
                        }
                        else if (currentToken != fileNameTokenized[index])
                        {
                            if (index == 0)
                            {
                                // No common prefix could be found
                                throw new XamlException(string.Format(XamlCompilerResources.XamlCompiler_BaseFilenamesMustBeTheSame, this.ClassName.FullName, currentToken, fileNameTokenized[index]));
                            }

                            return;
                        }
                    }

                    strBuilder.Append((index == 0 ? "" : ".") + currentToken);
                    index++;

                } while (currentToken != null);
            };

            getLongestCommonPrefix();

            return strBuilder.ToString();
        }

        public bool IsUsingCompiledBinding
        {
            get { return this.HasBindAssignments || this.HasBoundEventAssignments; }
        }

        public bool HasBindingSetters
        {
            get { return this.BindUniverses.SelectMany(bu => bu.BindAssignments).Where(ba => ba.HasSetValueHelper).Any(); }
        }

        public bool HasBindAssignments
        {
            get { return this.BindStatus.HasFlag(BindStatus.HasBinding); }
        }

        public bool HasBoundEventAssignments
        {
            get { return this.BindStatus.HasFlag(BindStatus.HasEventBinding); }
        }

        public bool HasInComponentBase
        {
            get { return DomHelper.IsLocalType(BaseType); }
        }

        public int NextConnectionId
        {
            get { return ++lastConnectionId; }
        }
    }
}

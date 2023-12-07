// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Xml;
using System.Linq;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    internal class SavedStateManager
    {
        string _filename;

        public SavedStateManager()
        {
            LocalAssemblyName = String.Empty;
            ReferenceAssemblyList = new HashSet<string>();
            ReferenceAssemblyGuids = new Dictionary<string, Guid>();
            XamlPerFileInfo = new Dictionary<string, SaveStatePerXamlFile>(StringComparer.InvariantCultureIgnoreCase);
        }

        private Queue<XamlFileCodeInfo> _fileCodeInfosToProcess = new Queue<XamlFileCodeInfo>();

        public String LocalAssemblyName { get; set; }

        public string XamlFeatureControlFlags { get; set; }
        public HashSet<String> ReferenceAssemblyList { get; private set; }
        public Dictionary<String, Guid> ReferenceAssemblyGuids { get; private set; }
        public Dictionary<String, SaveStatePerXamlFile> XamlPerFileInfo { get; private set; }

        static public SavedStateManager Load(string fileName)
        {
            SavedStateManager saveState = new SavedStateManager();
            try
            {
                saveState.LoadFile(fileName);
            }
            catch (Exception)
            {
                Debug.Assert(false, "This is a bug! Check to see why we failed to load the SaveState file.");
                saveState = new SavedStateManager(); // chuck the incorrectly loaded file and return an empty one.
                saveState._filename = fileName;
            }

            return saveState;
        }

        public void Save()
        {
            SaveFile(_filename);
        }

        public void LoadSavedTaskItemInfo(TaskItemFilename tif)
        {
            SaveStatePerXamlFile perFile;

            if (!XamlPerFileInfo.TryGetValue(tif.XamlGivenPath, out perFile))
            {
                tif.IsForcedOutOfDate = true;
                return;
            }

            tif.ClassFullName = perFile.ClassFullName;
            tif.XamlFileTimeAtLastCompile = perFile.XamlFileTimeAtLastCompile;

            if (perFile.XamlFileTimeAtLastCompile != tif.XamlLastChangeTime.Ticks)
            {
                tif.IsForcedOutOfDate = true;
            }
        }

        public void SetXamlFileTimeAtLastCompile(string fileName, long fileTime)
        {
            SaveStatePerXamlFile perFile;

            if (!XamlPerFileInfo.TryGetValue(fileName, out perFile))
            {
                perFile = new SaveStatePerXamlFile(fileName);
                XamlPerFileInfo.Add(perFile.FileName, perFile);
            }
            perFile.XamlFileTimeAtLastCompile = fileTime;
        }

        public void SetClassFullName(string fileName, string classFullName)
        {
            SaveStatePerXamlFile perFile;

            if (!XamlPerFileInfo.TryGetValue(fileName, out perFile))
            {
                perFile = new SaveStatePerXamlFile(fileName);
                XamlPerFileInfo.Add(perFile.FileName, perFile);
            }
            perFile.ClassFullName = classFullName;
        }

        public void SetGeneratedCodeFilePathPrefix(string fileName, string namePrefix)
        {
            SaveStatePerXamlFile perFile;
            if (!XamlPerFileInfo.TryGetValue(fileName, out perFile))
            {
                perFile = new SaveStatePerXamlFile(fileName);
                XamlPerFileInfo.Add(perFile.FileName, perFile);
            }
            perFile.GeneratedCodeFilePathPrefix = namePrefix;
        }

        internal void AddBindingInfo(XamlFileCodeInfo fileCodeInfo)
        {
            _fileCodeInfosToProcess.Enqueue(fileCodeInfo);
        }

        internal void ProcessBindingInfo()
        {
            while (_fileCodeInfosToProcess.Count > 0)
            {
                XamlFileCodeInfo fileCodeInfo = _fileCodeInfosToProcess.Dequeue();
                SaveStatePerXamlFile perFile;

                if (!XamlPerFileInfo.TryGetValue(fileCodeInfo.SourceXamlGivenPath, out perFile))
                {
                    perFile = new SaveStatePerXamlFile(fileCodeInfo.SourceXamlGivenPath);
                    XamlPerFileInfo.Add(perFile.FileName, perFile);
                }
                perFile.HasBoundEventAssignments = fileCodeInfo.BindStatus.HasFlag(BindStatus.HasEventBinding);
                foreach (ConnectionIdElement connectionId in fileCodeInfo.ConnectionIdElements)
                {
                    foreach (BindPathStep step in connectionId.BindUniverse.BindPathSteps.Values)
                    {
                        if (step.ImplementsIObservableVector)
                        {
                            SaveStateXamlType item = new SaveStateXamlType(step.ValueType.ItemType);
                            if (!perFile.BindingObservableVectorTypes.ContainsKey(item.ToString()))
                            {
                                perFile.BindingObservableVectorTypes.Add(item.ToString(), item);
                            }
                        }

                        if (step.ImplementsIObservableMap)
                        {
                            SaveStateXamlType item = new SaveStateXamlType(step.ValueType.ItemType);
                            if (!perFile.BindingObservableMapTypes.ContainsKey(item.ToString()))
                            {
                                perFile.BindingObservableMapTypes.Add(item.ToString(), item);
                            }
                        }

                        foreach (BindAssignment bindAssignment in step.BindAssignments.Where(ba => ba.HasSetValueHelper))
                        {
                            SaveStateXamlMember member = new SaveStateXamlMember(bindAssignment);
                            if (!perFile.BindingSetters.ContainsKey(bindAssignment.MemberFullName))
                            {
                                perFile.BindingSetters.Add(member.ToString(), member);
                            }
                        }
                    }
                }
            }
        }

        public long GetXamlFileTimeAtLastCompile(string fileName)
        {
            SaveStatePerXamlFile perFile;

            if (!XamlPerFileInfo.TryGetValue(fileName, out perFile))
            {
                return 0;
            }
            return perFile.XamlFileTimeAtLastCompile;
        }

        public string GetClassFullName(string fileName)
        {
            SaveStatePerXamlFile perFile;

            if (!XamlPerFileInfo.TryGetValue(fileName, out perFile))
            {
                return String.Empty;
            }
            return perFile.ClassFullName;
        }

        private const string XMLNAME_RootNode = "xml";
        private const string XMLNAME_XamlCompilerSaveState = "XamlCompilerSaveState";
        private const string XMLNAME_ReferenceAssemblyList = "ReferenceAssemblyList";
        private const string XMLNAME_LocalAssembly = "LocalAssembly";
        private const string XMLNAME_ReferenceAssembly = "ReferenceAssembly";
        private const string XMLNAME_PathName = "PathName";
        private const string XMLNAME_HashGuid = "HashGuid";
        private const string XMLNAME_XamlSourceFileDataList = "XamlSourceFileDataList";
        private const string XMLNAME_XamlSourceFileData = "XamlSourceFileData";
        private const string XMLNAME_XamlFeatureControlFlags = "XamlFeatureControlFlags";

        // ------  private -------------

        private void LoadFile(string fileName)
        {
            _filename = fileName;

            // If file does not exist then return an empty SaveState.
            if (File.Exists(fileName))
            {
                using (XmlTextReader xmlReader = new XmlTextReader(fileName))
                {
                    XmlDocument doc = new XmlDocument();
                    doc.Load(xmlReader);

                    foreach (XmlNode root in doc.ChildNodes)
                    {
                        switch (root.Name)
                        {
                            case XMLNAME_RootNode:
                                break;

                            case XMLNAME_XamlCompilerSaveState:
                                foreach (XmlNode child in root.ChildNodes)
                                {
                                    switch (child.Name)
                                    {
                                        case XMLNAME_XamlFeatureControlFlags:
                                            XamlFeatureControlFlags = root.InnerText;
                                            break;

                                        case XMLNAME_ReferenceAssemblyList:
                                            ReadReferenceAssemblyList(child);
                                            break;

                                        case XMLNAME_XamlSourceFileDataList:
                                            ReadSourceFileDataList(child);
                                            break;

                                        default:
                                            Debug.Assert(false, String.Format("Unknown node '{0}'", root.Name));
                                            break;
                                    }
                                }
                                break;

                            default:
                                Debug.Assert(false, String.Format("Unknown node '{0}'", root.Name));
                                break;
                        }
                    }
                }
            }
        }

        private void ReadReferenceAssemblyList(XmlNode listNode)
        {
            foreach (XmlNode node in listNode.ChildNodes)
            {
                String path = null;
                Guid guid = Guid.Empty;
                XmlNode attribute = null;

                attribute = node.Attributes.GetNamedItem(XMLNAME_PathName);
                path = attribute.Value;

                attribute = node.Attributes.GetNamedItem(XMLNAME_HashGuid);
                guid = Guid.Parse(attribute.Value);

                switch (node.Name)
                {
                    case XMLNAME_LocalAssembly:
                        LocalAssemblyName = path;
                        break;

                    case XMLNAME_ReferenceAssembly:
                        ReferenceAssemblyList.Add(path);
                        break;

                    default:
                        Debug.Assert(false, String.Format("Unknown node '{0}'", node.Name));
                        break;
                }
                ReferenceAssemblyGuids.Add(path, guid);
            }
        }
        private void ReadSourceFileDataList(XmlNode listNode)
        {
            foreach (XmlNode node in listNode.ChildNodes)
            {
                switch (node.Name)
                {
                    case XMLNAME_XamlSourceFileData:
                        {
                            SaveStatePerXamlFile perFile = new SaveStatePerXamlFile(node);
                            Debug.Assert(!String.IsNullOrEmpty(perFile.FileName));
                            XamlPerFileInfo.Add(perFile.FileName, perFile);
                        }
                        break;

                    default:
                        Debug.Assert(false, String.Format("Unknown node '{0}'", node.Name));
                        break;
                }
            }
        }

        private void SaveFile(string fileName)
        {
            XmlWriter writer = XmlWriter.Create(fileName);
            using (writer)
            {
                writer.WriteStartElement(XMLNAME_XamlCompilerSaveState);

                writer.WriteElementString(XMLNAME_XamlFeatureControlFlags, XamlFeatureControlFlags);

                // Write: ReferenceAssemblyList
                writer.WriteStartElement(XMLNAME_ReferenceAssemblyList);
                if (!String.IsNullOrEmpty(LocalAssemblyName))
                {
                    Guid guid;
                    if (ReferenceAssemblyGuids.TryGetValue(LocalAssemblyName, out guid))
                    {
                        writer.WriteStartElement(XMLNAME_LocalAssembly);
                        writer.WriteAttributeString(XMLNAME_PathName, LocalAssemblyName);
                        writer.WriteAttributeString(XMLNAME_HashGuid, guid.ToString());
                        writer.WriteEndElement();
                    }
                }
                foreach (string pathname in ReferenceAssemblyList)
                {
                    Guid guid;
                    if (ReferenceAssemblyGuids.TryGetValue(pathname, out guid))
                    {
                        writer.WriteStartElement(XMLNAME_ReferenceAssembly);
                        writer.WriteAttributeString(XMLNAME_PathName, pathname);
                        writer.WriteAttributeString(XMLNAME_HashGuid, guid.ToString());
                        writer.WriteEndElement();
                    }
                }
                writer.WriteEndElement();

                // Write: XamlSourceFileDataList
                writer.WriteStartElement(XMLNAME_XamlSourceFileDataList);
                foreach (string filename in this.XamlPerFileInfo.Keys)
                {
                    SaveStatePerXamlFile perFile = XamlPerFileInfo[filename];
                    perFile.WriteXmlElement(writer, XMLNAME_XamlSourceFileData);
                }
                writer.WriteEndElement();

                writer.WriteEndElement();                
            }
        }
    }
}

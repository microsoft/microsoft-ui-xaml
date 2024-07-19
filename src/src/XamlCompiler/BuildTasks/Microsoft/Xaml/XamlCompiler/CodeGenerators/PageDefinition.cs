// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using Utilities;

    internal class PageDefinition : Definition
    {
        private List<ConnectionIdElement> _allConnectionIdElements;
        private List<ForwardDeclaringNamespace> _forwardDeclarations;
        private HashSet<string> _neededLocalXamlHeaderFiles = new HashSet<string>();
        private List<string> _neededCppWinRTProjectionHeaderFiles = new List<string>();
        private bool _neededXamlHeaderFilesCalculated = false;
        private string _checksumAlgorithmGuid;
        private IEnumerable<ApiInformation> _allApiInformations;
        private List<FileNameAndChecksumPair> _XamlFileFullPathAndCheckSums;
        private List<xProperty> _xProperties;

        public PageDefinition(XamlProjectInfo projectInfo, XamlSchemaCodeInfo schemaInfo)
            : base(projectInfo, schemaInfo)
        {
        }

        public XamlClassCodeInfo CodeInfo { get; set; }
        
        public string ChecksumAlgorithmGuid
        {
            get
            {
                if (_checksumAlgorithmGuid == null)
                {
                    string sha256GuidString = new Guid(0x8829D00F, 0x11B8, 0x4213, 0x87, 0x8B, 0x77, 0x0E, 0x85, 0x97, 0xAC, 0x16).ToString();
                    _checksumAlgorithmGuid = "{" + sha256GuidString + "}";
                }
                return _checksumAlgorithmGuid;
            }
        }

        public IEnumerable<xProperty> XProperties
        {
            get
            {
                if (_xProperties == null)
                {
                    _xProperties = new List<xProperty>();
                    foreach (XamlFileCodeInfo fileInfo in CodeInfo.PerXamlFileInfo)
                    {
                        var props = fileInfo?.XPropertyInfo?.xProperties;
                        if (props != null)
                        {
                            foreach (xProperty xProp in props)
                            {
                                _xProperties.Add(xProp);
                            }
                        }
                    }
                }

                return _xProperties;
            }
        }

        public IEnumerable<FileNameAndChecksumPair> XamlFileFullPathAndCheckSums
        {
            get
            {
                if (_XamlFileFullPathAndCheckSums == null)
                {
                    _XamlFileFullPathAndCheckSums = LookupXamlFileFullPathAndCheckSums();
                }
                return _XamlFileFullPathAndCheckSums;
            }
        }

        public IEnumerable<string> NeededLocalXamlHeaderFiles
        {
            get
            {
                EnsureNeededXamlHeaderFilesCalculated();
                return _neededLocalXamlHeaderFiles;
            }
        }

        public IEnumerable<string> NeededCppWinRTProjectionHeaderFiles
        {
            get
            {
                EnsureNeededXamlHeaderFilesCalculated();
                return _neededCppWinRTProjectionHeaderFiles;
            }
        }

        public IEnumerable<ForwardDeclaringNamespace> ForwardDeclarations
        {
            get
            {
                if (_forwardDeclarations == null)
                {
                    _forwardDeclarations = LookupForwardDeclarations();
                }
                return _forwardDeclarations;
            }
        }

        public IEnumerable<ConnectionIdElement> AllConnectionIdElements
        {
            get
            {
                if (_allConnectionIdElements == null)
                {
                    var elements = new List<ConnectionIdElement>();
                    elements.AddRange(CodeInfo.PerXamlFileInfo.SelectMany(f => f.ConnectionIdElements));
                    _allConnectionIdElements = elements;
                }
                return _allConnectionIdElements;
            }
        }

        public IEnumerable<ConnectionIdElement> ConnectableElements
        {
            get
            {
                return AllConnectionIdElements.Where(x => x.NeedsConnectCase);
            }
        }

        public IEnumerable<ConnectionIdElement> UnloadableFields
        {
            get
            {
                return ConnectableElements.Where(e => e.IsUnloadableRoot).Where(e => e.HasFieldDefinition);
            }
        }

        public IEnumerable<ConnectionIdElement> DeferrableElements
        {
            get
            {
                return ConnectableElements.Where(e => e.CanBeInstantiatedLater);
            }
        }

        public IEnumerable<ApiInformation> ApiInformationDeclarations
        { 
            get
            {
                if (_allApiInformations == null)
                {
                    List<ApiInformation> allApiInformations = new List<ApiInformation>();

                    foreach (var el in this.AllConnectionIdElements)
                    {
                        if (el.ApiInformation != null)
                        {
                            allApiInformations.Add(el.ApiInformation);
                        }
                        allApiInformations.AddRange(el.EventAssignments.Where(e => e.ApiInformation != null).Select(e => e.ApiInformation));
                    }

                    foreach (var bindUniverse in this.CodeInfo.BindUniverses)
                    {
                        allApiInformations.AddRange(bindUniverse.BoundElements.Where(e => e.ApiInformation != null).Select(e => e.ApiInformation));
                        allApiInformations.AddRange(bindUniverse.BindAssignments.Where(e => e.ApiInformation != null).Select(e => e.ApiInformation));
                        allApiInformations.AddRange(bindUniverse.BoundEventAssignments.Where(e => e.ApiInformation != null).Select(e => e.ApiInformation));
                    }

                    _allApiInformations = allApiInformations.Distinct().OrderBy(a => a.UniqueName);
                }
                return _allApiInformations;
            }
        }


        public string GetLoadComponentUri(string priIndexName, string xamlRelativePath)
        {
            string priPackagePath = (priIndexName != null) ? priIndexName + "/" : String.Empty;
            string relativePriPath = xamlRelativePath.Replace('\\', '/');
            string prefix = "ms-appx:///";

            if (!String.IsNullOrEmpty(CodeInfo.XamlResourceMapName))
            {
                prefix = "ms-appx://" + CodeInfo.XamlResourceMapName + "/";
            }

            return prefix + priPackagePath + relativePriPath;
        }

        private void EnsureNeededXamlHeaderFilesCalculated()
        {
            HashSet<string> neededCppWinRTProjectionHeaderFiles;

            void addCppWinRTHeaderForTypeIfNecessary(Type type)
            {
                // If the input type is an array type then we need to use its element type instead
                //
                // MSDN recommends using `typeof(Array).IsAssignableFrom(type)` instead of `IsArray` to check
                // if a given type is an array type but the recommended approach doesn't work with LMR. We will
                // want to switch once we switch to System.Reflection.
                Type adjustedType = ((type != null) && type.IsArray) ? type.GetElementType() : type;

                // Primitive WinRT types are not defined in headers that follow the standard naming
                // convention as they are part of the C++/WinRT project system itself (e.g. base.h)
                if (adjustedType != null && !XamlSchemaCodeInfo.IsProjectedPrimitiveCppType(adjustedType.FullName))
                {
                    neededCppWinRTProjectionHeaderFiles.Add($"winrt/{adjustedType.Namespace}.h");

                    if (adjustedType.IsGenericType)
                    {
                        foreach (var nestedType in adjustedType.GetGenericArguments())
                        {
                            addCppWinRTHeaderForTypeIfNecessary(nestedType);
                        }
                    }
                }
            }

            if (!_neededXamlHeaderFilesCalculated)
            {
                neededCppWinRTProjectionHeaderFiles = new HashSet<string>();

                string headerFile;
                if (ProjectInfo.ClassToHeaderFileMap.TryGetValue(CodeInfo.ClassName.FullName, out headerFile))
                {
                    _neededLocalXamlHeaderFiles.Add(headerFile);
                }

                foreach (XamlFileCodeInfo fileCodeInfo in CodeInfo.PerXamlFileInfo)
                {
                    if (fileCodeInfo.ConnectionIdElements.Any())
                    {
                        // Need Microsoft.UI.Xaml.Markup.h for IComponentConnector
                        neededCppWinRTProjectionHeaderFiles.Add("winrt/Microsoft.UI.Xaml.Markup.h");
                    }

                    // iterate over all the fields
                    foreach (FieldDefinition fieldData in from c in fileCodeInfo.ConnectionIdElements
                                                          where c.FieldDefinition != null
                                                          select c.FieldDefinition)
                    {
                        if (ProjectInfo.ClassToHeaderFileMap.TryGetValue(fieldData.FieldTypeName, out headerFile))
                        {
                            _neededLocalXamlHeaderFiles.Add(headerFile);
                        }
                        else
                        {
                            // Not a local type so header is produced by C++/WinRT
                            addCppWinRTHeaderForTypeIfNecessary(fieldData.FieldXamlType?.UnderlyingType);
                        }
                    }

                    // iterate over all the events
                    foreach (List<EventAssignment> events in from c in fileCodeInfo.ConnectionIdElements
                                                             where c.HasEventAssignments == true
                                                             select c.EventAssignments)
                    {
                        foreach (EventAssignment xamlEvent in events)
                        {
                            // Event handler type (e.g. RoutedEventHandler)
                            if (ProjectInfo.ClassToHeaderFileMap.TryGetValue(xamlEvent.EventType.StandardName, out headerFile))
                            {
                                _neededLocalXamlHeaderFiles.Add(headerFile);
                            }
                            else
                            {
                                addCppWinRTHeaderForTypeIfNecessary(xamlEvent.EventType?.UnderlyingType);
                            }

                            // Event's declaring type (e.g. MUXC.Primitives.ButtonBase)
                            if (ProjectInfo.ClassToHeaderFileMap.TryGetValue(xamlEvent.DeclaringType.StandardName, out headerFile))
                            {
                                _neededLocalXamlHeaderFiles.Add(headerFile);
                            }
                            else
                            {
                                addCppWinRTHeaderForTypeIfNecessary(xamlEvent.DeclaringType?.UnderlyingType);
                            }
                        }
                    }

                    // Iterate over all the bindings as Source/Target may require additional headers for their projections
                    // when using C++/WinRT.
                    foreach (List<BindAssignment> bindAssignments in from c in fileCodeInfo.ConnectionIdElements
                                                                     where c.HasBindAssignments == true
                                                                     select c.BindAssignments)
                    {
                        foreach (BindAssignment bindAssignment in bindAssignments)
                        {
                            addCppWinRTHeaderForTypeIfNecessary(bindAssignment.MemberType?.UnderlyingType);
                            addCppWinRTHeaderForTypeIfNecessary(bindAssignment.MemberDeclaringType?.UnderlyingType);
                        }
                    }
                }

                // Sort the projection headers by namespace
                _neededCppWinRTProjectionHeaderFiles = neededCppWinRTProjectionHeaderFiles.OrderBy(
                    value => 
                    {
                        if (value.EndsWith(".h"))
                        {
                            return value.Substring(0, value.Length - 2);
                        }
                        else
                        {
                            return value;
                        }
                    }).ToList();

                _neededXamlHeaderFilesCalculated = true;
            }
        }

        private List<FileNameAndChecksumPair> LookupXamlFileFullPathAndCheckSums()
        {
            var checksums = new List<FileNameAndChecksumPair>();

            foreach (XamlFileCodeInfo fileCodeInfo in CodeInfo.PerXamlFileInfo)
            {
                string checkSum = ChecksumHelper.Instance.ComputeCheckSumForXamlFile(fileCodeInfo.FullPathToXamlFile);
                var pair = new FileNameAndChecksumPair(fileCodeInfo.FullPathToXamlFile, checkSum);
                checksums.Add(pair);
            }
            return checksums;
        }

        private List<ForwardDeclaringNamespace> LookupForwardDeclarations()
        {
            List<ForwardDeclaringNamespace> forwardDeclarations = new List<ForwardDeclaringNamespace>();
            Dictionary<string, List<string>> forwardTypesTemp = new Dictionary<string, List<string>>();

            foreach (XamlFileCodeInfo fileCodeInfo in CodeInfo.PerXamlFileInfo)
            {
                foreach (FieldDefinition fieldData in from c in fileCodeInfo.ConnectionIdElements
                                                      where c.FieldDefinition != null
                                                      select c.FieldDefinition)
                {
                    if (!fieldData.IsValueType)
                    {
                        List<string> typeList;
                        if (!forwardTypesTemp.TryGetValue(fieldData.FieldTypePath, out typeList))
                        {
                            typeList = new List<string>();
                            forwardTypesTemp.Add(fieldData.FieldTypePath, typeList);
                        }
                        if (!typeList.Contains(fieldData.FieldTypeShortName))
                        {
                            typeList.Add(fieldData.FieldTypeShortName);
                        }
                    }
                }
                foreach (string typepath in forwardTypesTemp.Keys)
                {
                    ForwardDeclaringNamespace fnspace = new ForwardDeclaringNamespace(typepath, forwardTypesTemp[typepath]);
                    forwardDeclarations.Add(fnspace);
                }
            }
            return forwardDeclarations;
        }
    }
}

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
        private List<string> _neededCppWinRTProjectionNamespaces = new List<string>();
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

        public IEnumerable<string> DeclarationCppWinRTProjectionNamespaces
        {
            get
            {
                var namespaces = new HashSet<string>(StringComparer.Ordinal)
                {
                    KnownNamespaces.WindowsFoundation, KnownNamespaces.Xaml, KnownNamespaces.XamlMarkup
                };
                if (CodeInfo == null || String.IsNullOrWhiteSpace(CodeInfo.BaseTypeName))
                {
                    // A missing class/base is an unharvested model, not an empty declaration.
                    throw new InvalidOperationException("XAML class and base type must be harvested before collecting declaration dependencies.");
                }
                string baseTypeName = CodeInfo.BaseTypeName;
                int separator = baseTypeName.LastIndexOf('.');
                string baseNamespace = separator > 0 ? baseTypeName.Substring(0, separator) : null;
                foreach (var ns in CppWinRTProjectionDependency.GetNamespaces(CodeInfo.BaseType?.UnderlyingType, baseNamespace))
                {
                    namespaces.Add(ns);
                }
                foreach (var field in CodeInfo.FieldDeclarations)
                {
                    foreach (var ns in CppWinRTProjectionDependency.GetNamespaces(field.FieldXamlType?.UnderlyingType, field.FieldTypePath))
                    {
                        namespaces.Add(ns);
                    }
                }
                if (CodeInfo.IsApplication)
                {
                    namespaces.Add(KnownNamespaces.WindowsXamlInterop);
                }
                return namespaces.OrderBy(ns => ns, StringComparer.Ordinal);
            }
        }

        public IEnumerable<string> NeededCppWinRTProjectionNamespaces
        {
            get
            {
                EnsureNeededXamlHeaderFilesCalculated();
                return _neededCppWinRTProjectionNamespaces;
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
            HashSet<string> neededCppWinRTProjectionNamespaces;

            void addCppWinRTProjectionForTypeIfNecessary(Type type, string unresolvedNamespace = null)
            {
                foreach (var projectionNamespace in CppWinRTProjectionDependency.GetNamespaces(type, unresolvedNamespace))
                {
                    neededCppWinRTProjectionNamespaces.Add(projectionNamespace);
                }
            }

            if (!_neededXamlHeaderFilesCalculated)
            {
                // These projections are referenced by the generated Page Pass1/Pass2
                // scaffolding even when the XAML contains no named elements, events or bindings.
                // Keep them as unconditional semantic dependencies; relying on transitive
                // projection headers happens to work in header mode but is not valid for named
                // modules where every directly referenced namespace must be reachable.
                neededCppWinRTProjectionNamespaces = new HashSet<string>
                {
                    KnownNamespaces.WindowsFoundation,
                    KnownNamespaces.Xaml,
                    KnownNamespaces.XamlControlsPrimitives,
                    KnownNamespaces.XamlMarkup,
                };

                string headerFile;
                if (ProjectInfo.ClassToHeaderFileMap.TryGetValue(CodeInfo.ClassName.FullName, out headerFile))
                {
                    _neededLocalXamlHeaderFiles.Add(headerFile);
                }

                foreach (XamlFileCodeInfo fileCodeInfo in CodeInfo.PerXamlFileInfo)
                {
                    // Connection-id targets are cast to their concrete XAML type in Pass2 even
                    // when the element has no x:Name and therefore no generated field.
                    foreach (ConnectionIdElement connectionIdElement in fileCodeInfo.ConnectionIdElements)
                    {
                        addCppWinRTProjectionForTypeIfNecessary(connectionIdElement.Type?.UnderlyingType);
                    }

                    // iterate over all the fields
                    foreach (FieldDefinition fieldData in from c in fileCodeInfo.ConnectionIdElements
                                                          where c.FieldDefinition != null
                                                          select c.FieldDefinition)
                    {
                        // Local using: types are intentionally unresolved during Pass1. In that
                        // case FieldDefinition retains the namespace in FieldTypePath even though
                        // FieldXamlType.UnderlyingType is unavailable. The generated Pass1
                        // interface still spells the projected field type, so preserve that
                        // semantic dependency for named-module reachability.
                        addCppWinRTProjectionForTypeIfNecessary(fieldData.FieldXamlType?.UnderlyingType, fieldData.FieldTypePath);
                        if (ProjectInfo.ClassToHeaderFileMap.TryGetValue(fieldData.FieldTypeName, out headerFile))
                        {
                            _neededLocalXamlHeaderFiles.Add(headerFile);
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
                            addCppWinRTProjectionForTypeIfNecessary(xamlEvent.EventType?.UnderlyingType);
                            if (ProjectInfo.ClassToHeaderFileMap.TryGetValue(xamlEvent.EventType.StandardName, out headerFile))
                            {
                                _neededLocalXamlHeaderFiles.Add(headerFile);
                            }

                            // Event's declaring type (e.g. MUXC.Primitives.ButtonBase)
                            addCppWinRTProjectionForTypeIfNecessary(xamlEvent.DeclaringType?.UnderlyingType);
                            if (ProjectInfo.ClassToHeaderFileMap.TryGetValue(xamlEvent.DeclaringType.StandardName, out headerFile))
                            {
                                _neededLocalXamlHeaderFiles.Add(headerFile);
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
                            addCppWinRTProjectionForTypeIfNecessary(bindAssignment.MemberType?.UnderlyingType);
                            addCppWinRTProjectionForTypeIfNecessary(bindAssignment.MemberDeclaringType?.UnderlyingType);
                            addCppWinRTProjectionForTypeIfNecessary(bindAssignment.MemberTargetType?.UnderlyingType);
                        }
                    }
                }

                // Binding Pass2 code is generated from the complete bind-path graph, not only
                // from the final target assignment. Intermediate steps are materialized in
                // casts, observable-vector/map tracking caches and function calls, so every
                // type that can be spelled by that generated C++ must be part of the semantic
                // dependency closure. Pass2 lowers this closure directly to imports/includes;
                // Pass1 may know only the subset available before the intermediate WinMD exists.
                foreach (BindUniverse bindUniverse in CodeInfo.BindUniverses)
                {
                    addCppWinRTProjectionForTypeIfNecessary(bindUniverse.DataRootType?.UnderlyingType);

                    foreach (BindPathStep bindPathStep in bindUniverse.BindPathSteps.Values)
                    {
                        var valueType = bindPathStep.ValueType;
                        addCppWinRTProjectionForTypeIfNecessary(valueType?.UnderlyingType);
                        if (valueType != null && (bindPathStep.ImplementsIObservableVector || bindPathStep.ImplementsIObservableMap))
                        {
                            addCppWinRTProjectionForTypeIfNecessary(valueType.ItemType?.UnderlyingType);
                        }

                        if (bindPathStep is DependencyPropertyStep dependencyPropertyStep)
                        {
                            addCppWinRTProjectionForTypeIfNecessary(dependencyPropertyStep.OwnerType?.UnderlyingType);
                        }

                        if (bindPathStep is FunctionStep functionStep)
                        {
                            addCppWinRTProjectionForTypeIfNecessary(functionStep.Method?.OwnerType?.UnderlyingType);

                            foreach (FunctionParam parameter in functionStep.Parameters)
                            {
                                addCppWinRTProjectionForTypeIfNecessary(parameter.ParameterType);
                                addCppWinRTProjectionForTypeIfNecessary(parameter.ValueType?.UnderlyingType);
                                addCppWinRTProjectionForTypeIfNecessary(parameter.AssignmentType?.UnderlyingType);
                            }
                        }
                    }
                }

                _neededCppWinRTProjectionNamespaces = neededCppWinRTProjectionNamespaces.OrderBy(value => value).ToList();

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

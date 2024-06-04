// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Xaml;
using System.Xaml.Schema;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using CodeGen;
    using DirectUI;
    using Properties;
    using Utilities;
    using XamlDom;

    internal class XamlHarvester
    {
        private string _projectFolder;
        private bool _isPass1;
        private Platform _targPlat;
        private Dictionary<XamlDomObject, ConnectionIdElement> _collectedObjects;

        public XamlHarvester(String projectFolder, bool isPass1, Platform targPlat)
        {
            _isPass1 = isPass1;
            _projectFolder = Path.GetFullPath(projectFolder);
            _targPlat = targPlat;
            _collectedObjects = new Dictionary<XamlDomObject, ConnectionIdElement>();
            DirectoryInfo di = new DirectoryInfo(projectFolder);
            if (!di.Exists)
            {
                throw new ArgumentException(XamlCompilerResources.Harvester_ProjectFolderIsNotADirectory, projectFolder);
            }
        }

        public bool SkipNameFieldsForRootElements { get; set; }

        public XamlClassCodeInfo HarvestClassInfo(string classFullName, XamlDomObject domRoot, bool isApplication)
        {
            Debug.Assert(!string.IsNullOrWhiteSpace(classFullName));
            if (domRoot == null)
            {
                throw new ArgumentNullException("domRoot");
            }

            XamlClassCodeInfo classCodeInfo = new XamlClassCodeInfo(classFullName, isApplication);
            classCodeInfo.BaseTypeName = GetFullTypePath(domRoot.Type);
            classCodeInfo.BaseType = domRoot.Type;
            if (!_isPass1)
            {
                if (!string.IsNullOrEmpty(classFullName))
                {
                    XamlTypeName classXamlTypeName = XamlSchemaCodeInfo.GetXamlTypeNameFromFullName(classFullName);
                    XamlType classXamlType = domRoot.SchemaContext.GetXamlType(classXamlTypeName);

                    if (classXamlType != null)
                    {
                        classCodeInfo.ClassXamlType = classXamlType;
                        classCodeInfo.ClassType = new TypeForCodeGen(classXamlType);
                    }
                }
            }
            return classCodeInfo;
        }

        public XamlFileCodeInfo HarvestXamlFileInfo(XamlClassCodeInfo classCodeInfo, XamlDomObject domRoot)
        {
            if (domRoot == null)
            {
                throw new ArgumentNullException("domRoot");
            }

            XamlFileCodeInfo fileCodeInfo = new XamlFileCodeInfo();

            if (!String.IsNullOrWhiteSpace(classCodeInfo.ClassName.FullName))
            {
                if (!CollectCodeBehindElements(domRoot, classCodeInfo, fileCodeInfo))
                {
                    return null;
                }
            }

            return fileCodeInfo;
        }

        private String GetFullTypePath(XamlType type)
        {
            string fullName = String.Empty;
            if (type.IsUnknown)
            {
                string usingTypePath;
                if (IsPossiblyALocalType(type, out usingTypePath))
                {
                    fullName = usingTypePath + "." + type.Name;
                }
            }
            else
            {
                fullName = type.UnderlyingType.FullName;
            }

            // The validator should ensure that we only see correct code.
            // this Assert should not fire.
            Debug.Assert(fullName != null, "Local type failed to resolve");
            return fullName;
        }

        private bool IsACollectableCodeBehindElement(XamlDomObject domObject)
        {
            try
            {
                if (domObject.IsGetObject)
                {
                    return false;
                }
                if (DomHelper.IsNamedCollectableObject(domObject, _isPass1))
                {
                    return true;
                }
                foreach (XamlDomMember eventMember in from mem in domObject.MemberNodes
                                                      where mem.Member.IsEvent
                                                      select mem)
                {
                    return true;
                }
                foreach (XamlDomMember bindMember in from mem in domObject.MemberNodes
                                                     where mem.Items.Count == 1 && mem.Item is XamlDomObject && domObject.SchemaContext is DirectUISchemaContext && ((XamlDomObject)mem.Item).Type == ((DirectUISchemaContext)domObject.SchemaContext).DirectUIXamlLanguage.BindExtension
                                                     select mem)
                {
                    return true;
                }
            }
            catch (TypeLoadException typeLoadEx)
            {
                DirectUISchemaContext schema = domObject.Type.SchemaContext as DirectUISchemaContext;
                if (schema == null)
                {
                    throw;  // let the error throw through
                }
                schema.SchemaErrors.Add(new XamlSchemaError_TypeLoadException(domObject, domObject.Type.Name, typeLoadEx.Message));
            }
            return false;
        }

        private bool IsAPotentialBindingRoot(XamlDomObject domObject)
        {
            try
            {
                if (domObject.Parent != null && domObject.Parent.Parent != null)
                {
                    return DomHelper.IsDerivedFromDataTemplate(domObject.Parent.Parent) || DomHelper.IsDerivedFromControlTemplate(domObject.Parent.Parent);
                }
            }
            catch (TypeLoadException typeLoadEx)
            {
                DirectUISchemaContext schema = domObject.Type.SchemaContext as DirectUISchemaContext;
                if (schema == null)
                {
                    throw;  // let the error throw through
                }
                schema.SchemaErrors.Add(new XamlSchemaError_TypeLoadException(domObject, domObject.Type.Name, typeLoadEx.Message));
            }
            return false;
        }

        internal static bool IsPossiblyALocalType(XamlType xamlType, out string usingTypePath)
        {
            foreach (string xmlns in xamlType.GetXamlNamespaces())
            {
                string assemblyName;
                string error;
                if (xmlns.HasUsingPrefix())
                {
                    usingTypePath = xmlns.StripUsingPrefix();
                    if (usingTypePath.IsConditionalNamespace())
                    {
                        try
                        {
                            usingTypePath = ConditionalNamespace.Parse(usingTypePath).UnconditionalNamespace;
                        }
                        catch (ParseException)
                        {
                            Debug.Assert(false, "Validator should have checked this scenario");
                            return false;
                        }
                    }
                    return true;
                }

                // This may no longer be legal in Jupiter XAML 
                if (ClrNamespaceParser.TryParseUri(xmlns, out usingTypePath, out assemblyName, out error, false))
                {
                    // Valid clr-namespace with no assembly.
                    //
                    if (String.IsNullOrEmpty(assemblyName))
                        return true;
                }
            }
            usingTypePath = null;
            return false;
        }

        class XamlDomObjectConnectionIdPair
        {
            public XamlDomObjectConnectionIdPair(XamlDomObject obj, XamlType dataRootType)
            {
                this.Obj = obj;
                this.ConnectionId = null;
                this.DataRootType = dataRootType;
                this.DomObjectToConnectionIdElement = new Dictionary<XamlDomObject, ConnectionIdElement>();
            }

            public XamlDomObject FindParentWithConnectionIdElement(XamlDomObject domObject)
            {
                XamlDomObject scopeRoot = this.Obj;
                XamlDomObject potentialParent = domObject.Parent?.Parent;

                while (potentialParent != null)
                {
                    if (this.DomObjectToConnectionIdElement.ContainsKey(potentialParent))
                    {
                        // found the first connection id element
                        return potentialParent;
                    }

                    if (potentialParent == scopeRoot)
                    {
                        // reached the scope root without finding a connection id element
                        return null;
                    }

                    potentialParent = potentialParent.Parent?.Parent;
                }

                return null;
            }

            public XamlDomObject Obj { get; set; }
            public ConnectionIdElement ConnectionId { get; set; }
            public XamlType DataRootType { get; set; }
            public IDictionary<XamlDomObject, ConnectionIdElement> DomObjectToConnectionIdElement { get; }
        };

        private bool CollectCodeBehindElements(XamlDomObject domRoot, XamlClassCodeInfo classCodeInfo, XamlFileCodeInfo fileCodeInfo)
        {
            Stack<XamlDomObjectConnectionIdPair> roots = new Stack<XamlDomObjectConnectionIdPair>();

            XamlDomIteratorEnterNewScopeEvent enterScope = (XamlDomObject obj) =>
            {
                roots.Push(new XamlDomObjectConnectionIdPair(obj, classCodeInfo.ClassXamlType));
            };

            XamlDomIteratorExitNewScopeEvent exitScope = () =>
            {
                Debug.Assert(roots.Count > 0);
                ConnectionIdElement rootConnectionId = roots.Peek().ConnectionId;

                // We may collect BindUniverses here that don't end up used in generated code.  However, we can't prune them until after the x:Binds have been parsed
                // in case an out-of-scope binding needs the scope to be alive.
                if (rootConnectionId?.BindUniverse != null)
                {
                    classCodeInfo.BindUniverses.Add(rootConnectionId.BindUniverse);
                }
                roots.Pop();

                // If we just handled a nested scope, our parent is in the stack above us.  Get the parent
                // BindUniverse and add our BindUniverse as a child of it
                if (roots.Count > 0)
                {
                    ConnectionIdElement parentConnectionId = roots.Peek().ConnectionId;
                    if (parentConnectionId?.BindUniverse != null && rootConnectionId?.BindUniverse != null)
                    {
                        Debug.Assert(parentConnectionId.BindUniverse != rootConnectionId.BindUniverse);
                        rootConnectionId.BindUniverse.Parent = parentConnectionId.BindUniverse;
                        parentConnectionId.BindUniverse.Children.Add(rootConnectionId.BindUniverse);
                    }
                }
            };

            var iterator = new XamlDomIterator(domRoot);
            iterator.EnterNewScopeCallback += enterScope;
            iterator.ExitScopeCallback += exitScope;

            enterScope(domRoot);

            // Crawl the entire tree (including non-connection ID elements) for x:DefaultBindMode members.  We need to strip them out
            // before GenXBF gets them, and we can't just check connection ID elements since they can be on non-connection ID elements.
            foreach (XamlDomObject domObject in from obj in iterator.DescendantsAndSelf()
                                                where DomHelper.HasDefaultBindModeMember(obj)
                                                select obj)
            {
                fileCodeInfo.StrippableMembers.Add(new StrippableMember(DomHelper.GetDefaultBindModeMember(domObject)));
            }

            // If the root uses x:Properties, we need to strip them out.
            if (domRoot.XPropertyInfo != null)
            {
                fileCodeInfo.StrippableObjects.Add(new StrippableObject(domRoot.XPropertyInfo.xPropertiesNode));
                fileCodeInfo.XPropertyInfo = domRoot.XPropertyInfo;
            }

            foreach (XamlDomObject domObject in from obj in iterator.DescendantsAndSelf()
                                                select obj)
            {
                if (DomHelper.IsObjectInvalidForPlatform(domObject, _targPlat))
                {
                    fileCodeInfo.StrippableObjects.Add(new StrippableObject(domObject));
                }

                XamlDomObject myObj = domObject;
                var members = myObj.MemberNodes;
                foreach (XamlDomMember member in domObject.MemberNodes)
                {
                    if (DomHelper.IsMemberInvalidForPlatform(member, _targPlat))
                    {
                        fileCodeInfo.StrippableMembers.Add(new StrippableMember(member));
                    }
                }

                var namespaces = myObj.Namespaces;
                foreach (XamlDomNamespace domName in myObj.Namespaces)
                {
                    string xmlns = domName.NamespaceDeclaration.Namespace;
                    if (xmlns.HasUsingPrefix())
                    {
                        string xmlnsNoUsing = xmlns.StripUsingPrefix();
                        if (xmlnsNoUsing.IsConditionalNamespace())
                        {
                            Platform namespacePlatform = Platform.Any;
                            try
                            {
                                namespacePlatform = ConditionalNamespace.Parse(xmlnsNoUsing).PlatConditional;
                            }
                            catch (ParseException)
                            {
                                Debug.Assert(false, "Validator should have checked this scenario");
                            }

                            // If there was a target platform specified, we need to strip the target platform out.  For non-UWP platforms, we need to take out the entire
                            // declaration.
                            if (namespacePlatform != Platform.Any)
                            {
                                fileCodeInfo.StrippableNamespaces.Add(new StrippableNamespace(domName, !DomHelper.ConditionalValidForPlatform(namespacePlatform, _targPlat)));
                            }
                        }
                    }
                }
            }

            foreach (XamlDomObject domObject in from obj in iterator.DescendantsAndSelf()
                                                where IsACollectableCodeBehindElement(obj) || IsAPotentialBindingRoot(obj) || obj == roots.Peek().Obj
                                                select obj)
            {
                XamlDomObjectConnectionIdPair scopeRoot = roots.Peek();
                bool evaluatingDataTemplateRoot = false;

                // For Templates, we need to change the scope root to the root of the template
                if (DomHelper.IsDerivedFromDataTemplate(scopeRoot.Obj))
                {
                    if (domObject.Parent != null && domObject.Parent.Parent != null && scopeRoot.Obj.Equals(domObject.Parent.Parent))
                    {
                        evaluatingDataTemplateRoot = true;
                        XamlType dataRootType = null;
                        XamlDomMember dataTypeMember = DomHelper.GetDataTypeMember(scopeRoot.Obj);
                        string targetTypeName = DomHelper.GetStringValueOfProperty(dataTypeMember);
                        if (!String.IsNullOrEmpty(targetTypeName))
                        {
                            dataRootType = scopeRoot.Obj.ResolveXmlName(targetTypeName);
                            if (!_isPass1)
                            {
                                // Add to DataTemplateList
                                DataTypeAssignment dataTypeAssignment = new DataTypeAssignment(dataTypeMember);
                                fileCodeInfo.DataTypeAssignments.Add(dataTypeAssignment);
                            }

                            // Collect the DataTemplate itself in-case its scope is used for an out-of-scope binding.
                            ConnectionIdElement dataTemplateConnectionIdElement = CollectElement(
                            scopeRoot.Obj,
                            scopeRoot.Obj,
                            null,
                            classCodeInfo,
                            fileCodeInfo,
                            dataRootType);

                            scopeRoot.DomObjectToConnectionIdElement[scopeRoot.Obj] = dataTemplateConnectionIdElement;
                            scopeRoot.ConnectionId = dataTemplateConnectionIdElement;


                            EnsureTemplateUniverse(dataTemplateConnectionIdElement, dataRootType, classCodeInfo);
                        }
                        else
                        {
                            // In the event we don't have an x:DataType, just use the type of the scope root (which will be the template).
                            // We can get into this case when we've collected a template but don't know if it's needed for an out-of-scope binding.
                            // We don't actually care about the root type in this case, perhaps we could handle it better?
                            dataRootType = scopeRoot.Obj.Type;
                        }

                        scopeRoot.Obj = domObject;
                        scopeRoot.DataRootType = dataRootType;
                    }
                }
                else if (DomHelper.IsDerivedFromControlTemplate(scopeRoot.Obj))
                {
                    if (domObject.Parent != null && domObject.Parent.Parent != null && scopeRoot.Obj.Equals(domObject.Parent.Parent))
                    {
                        XamlType dataRootType = null;
                        XamlDomMember targetTypeMember = DomHelper.GetDataTypeMember(scopeRoot.Obj);
                        string targetTypeName = DomHelper.GetStringValueOfProperty(targetTypeMember);
                        if (!String.IsNullOrEmpty(targetTypeName))
                        {
                            dataRootType = scopeRoot.Obj.ResolveXmlName(targetTypeName);
                            ConnectionIdElement controlTemplateConnectionIdElement = CollectElement(
                                scopeRoot.Obj,
                                scopeRoot.Obj,
                                null,
                                classCodeInfo,
                                fileCodeInfo,
                                dataRootType);

                            EnsureTemplateUniverse(controlTemplateConnectionIdElement, dataRootType, classCodeInfo);

                            // We keep the scope root as the control template so that the x:ConnectionId goes on it, rather than the first templated child.
                            scopeRoot.DomObjectToConnectionIdElement[scopeRoot.Obj] = controlTemplateConnectionIdElement;
                            scopeRoot.ConnectionId = controlTemplateConnectionIdElement;
                            scopeRoot.DataRootType = dataRootType;
                        }
                        else
                        {
                            // In the event we don't have an x:DataType, just use the type of the scope root (which will be the template).
                            // We can get into this case when we've collected a template but don't know if it's needed for an out-of-scope binding.
                            // We don't actually care about the root type in this case, perhaps we could handle it better?
                            dataRootType = scopeRoot.Obj.Type;
                        }
                    }
                }

                ConnectionIdElement connectionIdElement = CollectElement(
                                            scopeRoot.Obj,
                                            domObject,
                                            (domObject == scopeRoot.Obj || scopeRoot.ConnectionId == null) ? null : scopeRoot.ConnectionId.BindUniverse,
                                            classCodeInfo,
                                            fileCodeInfo,
                                            scopeRoot.DataRootType);

                if (evaluatingDataTemplateRoot)
                {
                    if (!connectionIdElement.BindUniverse.BoundElements.Contains(connectionIdElement))
                    {
                        connectionIdElement.BindUniverse.BoundElements.Add(connectionIdElement);
                    }
                }

                if (domObject == scopeRoot.Obj)
                {
                    scopeRoot.ConnectionId = connectionIdElement;
                }

                //Add the mapping of ConnectionDomObject to ConnectionIdElement
                scopeRoot.DomObjectToConnectionIdElement[domObject] = connectionIdElement;

                //The iterator's DescendantsAndSelf() method goes through parents before children, so the parent of this Dom node should already be in our dictionary (if it exists)
                if (domObject != scopeRoot.Obj && domObject.Parent != null)
                {
                    XamlDomObject potentialParent = scopeRoot.FindParentWithConnectionIdElement(domObject);
                    if (potentialParent != null)
                    {
                        var parentIdElement = scopeRoot.DomObjectToConnectionIdElement[potentialParent];
                        parentIdElement.Children.Add(connectionIdElement);
                    }
                }

                // Check for valid DataType if elements under a data template use x:Bind.
                if (!_isPass1 &&
                    connectionIdElement != null &&
                    connectionIdElement.HasBindAssignments &&
                    DomHelper.UnderANamescope(domObject, _isPass1))
                {
                    bool hasDataRootType = scopeRoot?.ConnectionId?.BindUniverse?.DataRootType != null;
                    if (!hasDataRootType)
                    {
                        LineNumberInfo lineNumberInfo = new LineNumberInfo(domObject);
                        DirectUISchemaContext schema = domObject.Type.SchemaContext as DirectUISchemaContext;
                        if (schema == null)
                        {
                            throw new XamlException(String.Format("Unexpected: Cannot get a schema for domObject {0}", domObject.Type.Name));
                        }

                        if (scopeRoot!= null && scopeRoot.Obj.Type.IsDerivedFromControlTemplate())
                        {
                            schema.SchemaErrors.Add(new XamlXBindControlTemplateDoesNotDefineTargetTypeError(domObject));
                        }
                        else
                        {
                            schema.SchemaErrors.Add(new XamlXBindDataTemplateDoesNotDefineDataTypeError(domObject));
                        }
                        return false;
                    }
                }
            }

            exitScope();
            iterator.EnterNewScopeCallback -= enterScope;
            iterator.ExitScopeCallback -= exitScope;

            Debug.Assert(roots.Count == 0);
            return true;
        }

        // Ensures that if the given template element needs a connection ID because its children
        // need connection IDs (e.g. they have x:Binds, event subscriptions, etc.) that
        // a BindUniverse with the template as its root element is created
        private void EnsureTemplateUniverse(ConnectionIdElement templateConnectionIdElement, XamlType dataRootType, XamlClassCodeInfo classCodeInfo)
        {
            if (templateConnectionIdElement.BindUniverse.RootElement != templateConnectionIdElement)
            {
                BindUniverse newRootUniverse = new BindUniverse(templateConnectionIdElement, dataRootType, false, classCodeInfo.ClassName.ShortName);
                templateConnectionIdElement.BindUniverse = newRootUniverse;
            }
        }

        private ConnectionIdElement CollectElement(XamlDomObject domRoot, XamlDomObject domObject, BindUniverse bindUniverse, XamlClassCodeInfo classCodeInfo, XamlFileCodeInfo fileCodeInfo, XamlType dataRootType)
        {
            bool dontHarvestName = (SkipNameFieldsForRootElements && (domObject == domRoot)) || DomHelper.UnderANamescope(domObject, _isPass1);
            ConnectionIdElement connectionIdElement = null;

            // If we've already collected domObject and made a ConnectionIdElement for it,
            // return that instead of making a new one.
            if (_collectedObjects.TryGetValue(domObject, out connectionIdElement))
            {
                return connectionIdElement;
            }

            if (domObject.Type.IsUnknown)
            {
                string usingTypePath;
                if (_isPass1 && IsPossiblyALocalType(domObject.Type, out usingTypePath))
                {
                    connectionIdElement = new ConnectionIdElement(domObject, bindUniverse, fileCodeInfo, classCodeInfo, dataRootType, dontHarvestName, usingTypePath);
                }
            }
            else
            {
                connectionIdElement = new ConnectionIdElement(domObject, bindUniverse, fileCodeInfo, classCodeInfo, dataRootType, dontHarvestName);
            }
            if (connectionIdElement != null)
            {
                fileCodeInfo.ConnectionIdElements.Add(connectionIdElement);
                _collectedObjects.Add(domObject, connectionIdElement);
            }

            return connectionIdElement;
        }

        public static string GetClassFullName(XamlDomObject domRoot)
        {
            string classFullName = DomHelper.GetStringValueOfProperty(domRoot, XamlLanguage.Class);
            if (String.IsNullOrEmpty(classFullName))
            {
                return null;
            }

            string[] pathParts = classFullName.Split('.');
            if (pathParts.Length == 1)
            {
                string error = ResourceUtilities.FormatString(XamlCompilerResources.Harvester_ClassMustHaveANamespace, classFullName);
                throw new XamlException(error, null, domRoot.StartLineNumber, domRoot.StartLinePosition);
            }
            foreach (string part in pathParts)
            {
                string error;
                if (String.IsNullOrWhiteSpace(part))
                {
                    error = ResourceUtilities.FormatString(XamlCompilerResources.Harvester_ClassNameEmptyPathPart, classFullName);
                    throw new XamlException(error, null, domRoot.StartLineNumber, domRoot.StartLinePosition);
                }
                else if (part.Contains(' ') || part.Contains('\t') || part.Contains('\n'))
                {
                    error = ResourceUtilities.FormatString(XamlCompilerResources.Harvester_ClassNameNoWhiteSpace, classFullName, part);
                    throw new XamlException(error, null, domRoot.StartLineNumber, domRoot.StartLinePosition);
                }
                else if (!XamlDomValidator.IsValidIdentifierName(part))
                {
                    error = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_BadName, part, "Class", domRoot.Type.Name);
                    throw new XamlException(error, null, domRoot.StartLineNumber, domRoot.StartLinePosition);
                }
            }

            return classFullName;
        }

    }
}

// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Reflection;
using System.Collections;

namespace Win8Xaml.CompilerProxies
{
    public class XamlFileCodeInfo
    {
        static ProxyHelper _xamlFileCodeInfoType;
        static PropertyInfo _apparentRelativePathProperty;
        static PropertyInfo _sourceXamlGivenPathProperty;
        static PropertyInfo _fullPathToXamlFileProperty;
        static PropertyInfo _relativePathFromGeneratedCodeToXamlFileProperty;
        static PropertyInfo _connectionIdElementsProperty;
        static PropertyInfo _hasEventAssignmentsProperty;

        object _instance;

        static XamlFileCodeInfo()
        {
            _xamlFileCodeInfoType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.XamlFileCodeInfo");
            _apparentRelativePathProperty = _xamlFileCodeInfoType.GetProperty("ApparentRelativePath");
            _sourceXamlGivenPathProperty = _xamlFileCodeInfoType.GetProperty("SourceXamlGivenPath");
            _fullPathToXamlFileProperty = _xamlFileCodeInfoType.GetProperty("FullPathToXamlFile");
            _relativePathFromGeneratedCodeToXamlFileProperty = _xamlFileCodeInfoType.GetProperty("RelativePathFromGeneratedCodeToXamlFile");
            _connectionIdElementsProperty = _xamlFileCodeInfoType.GetProperty("ConnectionIdElements");
            _hasEventAssignmentsProperty = _xamlFileCodeInfoType.GetProperty("HasEventAssignments");
        }

        public XamlFileCodeInfo()
        {
            _instance = _xamlFileCodeInfoType.CreateInstance();
        }

        public XamlFileCodeInfo(object instance)
        {
            _instance = instance;
        }

        public object Instance
        {
            get { return _instance; }
        }

        public string ApparentRelativePath
        {
            get { return (string)_apparentRelativePathProperty.GetValue(_instance, null); }
            set { _apparentRelativePathProperty.SetValue(_instance, value); }
        }

        public string SourceXamlGivenPath
        {
            get { return (string)_sourceXamlGivenPathProperty.GetValue(_instance, null); }
            set { _sourceXamlGivenPathProperty.SetValue(_instance, value); }
        }

        public string FullPathToXamlFile
        {
            get { return (string)_fullPathToXamlFileProperty.GetValue(_instance, null); }
            set { _fullPathToXamlFileProperty.SetValue(_instance, value); }
        }

        public string RelativePathFromGeneratedCodeToXamlFile
        {
            get { return (string)_relativePathFromGeneratedCodeToXamlFileProperty.GetValue(_instance, null); }
            set { _relativePathFromGeneratedCodeToXamlFileProperty.SetValue(_instance, value); }
        }

        public List<ConnectionIdElement> ConnectionIdElements
        {
            get
            {
                IEnumerable objectList = (IEnumerable)_connectionIdElementsProperty.GetValue(_instance, null);
                List<ConnectionIdElement> ConnectionIdElementList = new List<ConnectionIdElement>();
                foreach (Object obj in objectList)
                {
                    ConnectionIdElement connectionIdElement = new ConnectionIdElement(obj);
                    ConnectionIdElementList.Add(connectionIdElement);
                }
                return ConnectionIdElementList;
            }
        }

        public bool HasEventAssignments
        {
            get { return (bool)_hasEventAssignmentsProperty.GetValue(_instance, null); }
        }
    }
}

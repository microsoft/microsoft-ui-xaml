// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System.Collections;
using System.Collections.Generic;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class PageDefinition
    {
        static readonly ProxyHelper _pageDefinitionType;
        static readonly PropertyInfo _codeInfo;
        static readonly PropertyInfo _neededCppWinRTProjectionNamespaces;

        readonly object _instance;

        static PageDefinition()
        {
            _pageDefinitionType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.CodeGen.PageDefinition");
            _codeInfo = _pageDefinitionType.GetProperty("CodeInfo");
            _neededCppWinRTProjectionNamespaces = _pageDefinitionType.GetProperty("NeededCppWinRTProjectionNamespaces");
        }

        public PageDefinition(XamlProjectInfo projectInfo, XamlSchemaCodeInfo schemaInfo)
        {
            _instance = _pageDefinitionType.CreateInstance(new object[] { projectInfo.Instance, schemaInfo.Instance });
        }

        public PageDefinition(object instance)
        {
            _instance = instance;
        }

        public object Instance => _instance;

        public XamlClassCodeInfo CodeInfo
        {
            get
            {
                object value = _codeInfo.GetValue(_instance, null);
                return value == null ? null : new XamlClassCodeInfo(value);
            }
            set
            {
                _codeInfo.SetValue(_instance, value?.Instance, null);
            }
        }

        public List<string> NeededCppWinRTProjectionNamespaces
        {
            get
            {
                var result = new List<string>();
                foreach (object value in (IEnumerable)_neededCppWinRTProjectionNamespaces.GetValue(_instance, null))
                {
                    result.Add((string)value);
                }
                return result;
            }
        }
    }
}

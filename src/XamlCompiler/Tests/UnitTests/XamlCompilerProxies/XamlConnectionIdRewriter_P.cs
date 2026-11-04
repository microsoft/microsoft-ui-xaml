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
    public class XamlConnectionIdRewriter
    {
        static ProxyHelper _xamlEditorType;
        static MethodInfo _parseMethod;
        static MethodInfo _editMethod;
        static PropertyInfo _errorsProperty;

        List<XamlCompileError> _errorList = new List<XamlCompileError>();
        object _instance;

        static XamlConnectionIdRewriter()
        {
            _xamlEditorType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.XamlConnectionIdRewriter");
            _parseMethod = _xamlEditorType.GetMethod("Parse");
            _editMethod = _xamlEditorType.GetMethod("Edit");
            _errorsProperty = _xamlEditorType.GetProperty("Errors");
        }

        public XamlConnectionIdRewriter()
        {
            _instance = _xamlEditorType.CreateInstance();
        }

        public String Parse(String xamlText, XamlClassCodeInfo codeInfo, XamlFileCodeInfo fileInfo)
        {
            Object[] args = new Object[] { xamlText, codeInfo.Instance, fileInfo.Instance };
            Object result = _parseMethod.Invoke(_instance, args);
            return (string)result;
        }

        public String Edit(String xamlFileName, XamlClassCodeInfo codeInfo, XamlFileCodeInfo fileInfo)
        {
            Object[] args = new Object[] { xamlFileName, codeInfo.Instance, fileInfo.Instance };
            Object result = _parseMethod.Invoke(_instance, args);
            return (string)result;
        }

        public List<XamlCompileError> Errors
        {
            get
            {
                IEnumerable listVal = (IEnumerable)_errorsProperty.GetValue(_instance, null);
                XamlDomValidator.ConvertListOfErrors(_errorList, listVal);
                return _errorList;
            }
        }


    }
}

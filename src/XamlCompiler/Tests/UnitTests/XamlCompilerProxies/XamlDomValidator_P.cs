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
    public class XamlDomValidator
    {
        static ProxyHelper _xamlDomValidatorType;
        static MethodInfo _validateMethod;
        static MethodInfo _isValidKeyIdentifierNameMethod;
        static MethodInfo _isValidIdentifierNameMethod;
        static PropertyInfo _isPass1Property;
        static PropertyInfo _errorsProperty;
        static PropertyInfo _warningsProperty;
        static PropertyInfo _targetPlatformMinVersionProperty;

        List<XamlCompileError> _errorList = new List<XamlCompileError>();
        List<XamlCompileError> _warningList = new List<XamlCompileError>();
        object _instance;

        static XamlDomValidator()
        {
            _xamlDomValidatorType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.XamlDomValidator");
            _validateMethod = _xamlDomValidatorType.GetMethod("Validate");
            _isValidKeyIdentifierNameMethod = _xamlDomValidatorType.GetStaticMethod("IsValidKeyIdentifierName");
            _isValidIdentifierNameMethod = _xamlDomValidatorType.GetStaticMethod("IsValidIdentifierName", 1);

            _isPass1Property = _xamlDomValidatorType.GetProperty("IsPass1");
            _errorsProperty = _xamlDomValidatorType.GetProperty("Errors");
            _warningsProperty = _xamlDomValidatorType.GetProperty("Warnings");
            _targetPlatformMinVersionProperty = _xamlDomValidatorType.GetProperty("TargetPlatformMinVersion");
        }

        public XamlDomValidator()
        {
            _instance = _xamlDomValidatorType.CreateInstance();
        }

        public bool Validate(CompilerDomRootToken domRoot)
        {
            //If the min version wasn't overridden by a test, use the default value
            if (_targetPlatformMinVersionProperty.GetValue(_instance, null) == null)
            {
                _targetPlatformMinVersionProperty.SetValue(_instance, ProxyHelper.TargetPlatformMinVersion);
            }
            Object[] args = new Object[] { domRoot.Instance };
            Object result = _validateMethod.Invoke(_instance, args);
            return (bool) result;
        }

        public bool IsPass1
        {
            get { return (bool)_isPass1Property.GetValue(_instance, null); }
            set { _isPass1Property.SetValue(_instance, value); }
        }

        public Version TargetPlatformMinVersion
        {
            get { return (Version)_targetPlatformMinVersionProperty.GetValue(_instance, null); }
            set { _targetPlatformMinVersionProperty.SetValue(_instance, value); }
        }

        public List<XamlCompileError> Errors
        {
            get
            {
                IEnumerable listVal = (IEnumerable)_errorsProperty.GetValue(_instance, null);
                ConvertListOfErrors(_errorList, listVal);
                return _errorList;
            }
        }

        public List<XamlCompileError> Warnings
        {
            get
            {
                IEnumerable listVal = (IEnumerable)_warningsProperty.GetValue(_instance, null);
                ConvertListOfErrors(_warningList, listVal);
                return _warningList;
            }
        }

        public static bool IsValidKeyIdentifierName(string name)
        {
            Object[] args = new Object[] { name };
            return (bool)_isValidKeyIdentifierNameMethod.Invoke(null, args);
        }

        public static bool IsValidIdentifierName(string name)
        {
            Object[] args = new Object[] { name };
            return (bool)_isValidIdentifierNameMethod.Invoke(null, args);
        }

        internal static void ConvertListOfErrors(List<XamlCompileError> errorList, IEnumerable objList)
        {
            errorList.Clear();
            foreach (object obj in objList)
            {
                XamlCompileError error = new XamlCompileError(obj);
                errorList.Add(error);
            }
        }
    }
}

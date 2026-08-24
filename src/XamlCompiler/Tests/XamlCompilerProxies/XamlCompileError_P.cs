// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class XamlCompileError
    {
        static ProxyHelper _xamlCompileErrorType;
        static PropertyInfo _messageProperty;
        static PropertyInfo _ErrorCodeProperty;
        static PropertyInfo _LineNumberProperty;
        static PropertyInfo _lineOffsetProperty;

        object _instance;

        static XamlCompileError()
        {
            _xamlCompileErrorType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.XamlCompileError");
            _messageProperty = _xamlCompileErrorType.GetProperty("Message");
            _ErrorCodeProperty = _xamlCompileErrorType.GetProperty("Code");
            _LineNumberProperty = _xamlCompileErrorType.GetProperty("LineNumber");
            _lineOffsetProperty = _xamlCompileErrorType.GetProperty("LineOffset");
        }

        public XamlCompileError(object instance)
        {
            _instance = instance;
        }

        public String Message
        {
            get
            {
                return (string)_messageProperty.GetValue(_instance, null);
            }
        }

        public String ErrorCode
        {
            get
            {
                int errorCode = (int) _ErrorCodeProperty.GetValue(_instance, null);
                return $"WMC{errorCode.ToString("D4")}";
            }
        }

        public int LineNumber
        {
            get
            {
                return (int)_LineNumberProperty.GetValue(_instance, null);
            }
        }

        public int LineOffset
        {
            get
            {
                return (int)_lineOffsetProperty.GetValue(_instance, null);
            }
        }
    }
}

// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class LineNumberInfo
    {
        static ProxyHelper _codeBehindElementType;
        static PropertyInfo _startLineNumberProperty;
        static PropertyInfo _startLinePositionProperty;
        static PropertyInfo _endLineNumberProperty;
        static PropertyInfo _endLinePositionProperty;

        object _instance;

        static LineNumberInfo()
        {
            _codeBehindElementType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.LineNumberInfo");
            _startLineNumberProperty = _codeBehindElementType.GetProperty("StartLineNumber");
            _startLinePositionProperty = _codeBehindElementType.GetProperty("StartLinePosition");
            _endLineNumberProperty = _codeBehindElementType.GetProperty("EndLineNumber");
            _endLinePositionProperty = _codeBehindElementType.GetProperty("EndLinePosition");
        }

        public LineNumberInfo() { }

        public LineNumberInfo(object instance)
        {
            _instance = instance;
        }

        public int StartLineNumber
        {
            get { return (int)_startLineNumberProperty.GetValue(_instance, null); }
        }

        public int StartLinePosition
        {
            get { return (int)_startLinePositionProperty.GetValue(_instance, null); }
        }

        public int EndLineNumber
        {
            get { return (int)_endLineNumberProperty.GetValue(_instance, null); }
        }

        public int EndLinePosition
        {
            get { return (int)_endLinePositionProperty.GetValue(_instance, null); }
        }
    }
}

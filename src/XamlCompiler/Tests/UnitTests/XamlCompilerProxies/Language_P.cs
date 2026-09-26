// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class Language
    {
        static ProxyHelper _codeBehindElementType;
        static MethodInfo _parseMethod;

        object _instance;

        static Language()
        {
            _codeBehindElementType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.Language");
            _parseMethod = _codeBehindElementType.GetStaticMethod("Parse");
        }

        public Language(object instance)
        {
            _instance = instance;
        }

        public object Instance { get { return _instance; } }

        public static Language Parse(string name)
        {
            object[] args = new object[] { name };
            return new Language(_parseMethod.Invoke(null, args));
        }
    }
}

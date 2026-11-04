// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class BindPathParser
    {
        static ProxyHelper _bindPathParserType;
        static MethodInfo _parsePathMethod;

        object _instance;

        static BindPathParser()
        {
            _bindPathParserType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.BindPathParser");
            _parsePathMethod = _bindPathParserType.GetStaticMethod("ParsePath", 4);
        }

        public BindPathParser()
        {
            _instance = _bindPathParserType.CreateInstance();
        }

        public BindPathStep ParsePath(
            string path,
            object bindingItem,
            object bindUniverse,
            object rootFieldDefinitions
            )
        {
            object[] args = new object[] {
                path,
                bindingItem,
                bindUniverse,
                rootFieldDefinitions
                };
            object result = _parsePathMethod.Invoke(_instance, args);
            return new BindPathStep(result);
        }
    }
}
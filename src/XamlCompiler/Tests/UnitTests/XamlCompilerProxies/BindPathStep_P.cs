// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class BindPathStep
    {
        static ProxyHelper _bindPathStepType;

        object _instance;

        static BindPathStep()
        {
            _bindPathStepType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.BindPathStep");
        }

        public BindPathStep(object instance)
        {
            _instance = instance;
        }

        public object Instance
        {
            get
            {
                return _instance;
            }
        }
    }
}
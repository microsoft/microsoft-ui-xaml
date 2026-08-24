// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System.Collections.Generic;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class BindUniverse
    {
        static ProxyHelper _bindUniverseType;
        object _instance;
        static MethodInfo _parseMethod;

        static BindUniverse()
        {
            _bindUniverseType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.BindUniverse");
            _parseMethod = _bindUniverseType.GetMethod("Parse", BindingFlags.Instance | BindingFlags.NonPublic);
        }

        public BindUniverse(object instance)
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

        public IEnumerable<XamlCompileError> Parse(XamlClassCodeInfo classCodeInfo)
        {
            List<XamlCompileError> errors = new List<XamlCompileError>();
            object[] args = new object[] { classCodeInfo.Instance, new System.Version(KnownVersions.Latest) };
            IEnumerable<object> result = _parseMethod.Invoke(_instance, args) as IEnumerable<object>;
            if (result != null)
            {
                foreach (var obj in result as IEnumerable<object>)
                {
                    errors.Add(new XamlCompileError(obj));
                }
            }
            return errors;
        }
    }
}
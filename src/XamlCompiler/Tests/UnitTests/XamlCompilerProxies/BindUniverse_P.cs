// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System.Collections;
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

        public void AddUnresolvedRootStepForTest(string key)
        {
            var rootStepType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.RootStep");
            object unresolvedStep = rootStepType.CreateInstance(new object[] { null, false });
            FieldInfo bindPathStepsField = _instance.GetType().GetField("BindPathSteps", BindingFlags.Public | BindingFlags.Instance);
            IDictionary bindPathSteps = (IDictionary)bindPathStepsField.GetValue(_instance);
            bindPathSteps.Add(key, unresolvedStep);
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
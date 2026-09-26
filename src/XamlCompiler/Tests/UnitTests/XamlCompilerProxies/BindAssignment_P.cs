// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class BindAssignment
    {
        static ProxyHelper _bindAssignmentType;
        static PropertyInfo _pathProperty;
        static PropertyInfo _pathStepProperty;
        static MethodInfo _parsePathMethod;

        object _instance;

        static BindAssignment()
        {
            _bindAssignmentType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.BindAssignment");
            _pathProperty = _bindAssignmentType.GetProperty("Path");
            _pathStepProperty = _bindAssignmentType.GetProperty("PathStep");
            _parsePathMethod = _bindAssignmentType.GetMethod("ParsePath");
        }

        public BindAssignment(object instance)
        {
            _instance = instance;
        }

        public string Path
        {
            get
            {
                object inst = _pathProperty.GetValue(_instance, null);
                return inst as string;
            }
        }

        public BindPathStep PathStep
        {
            get
            {
                object inst = _pathStepProperty.GetValue(_instance, null);
                return new BindPathStep(inst);
            }
        }

        public void ParsePath(XamlClassCodeInfo classCodeInfo)
        {
            object[] args = new object[] { classCodeInfo.Instance };
            object result = _parsePathMethod.Invoke(_instance, args);
        }
    }
}
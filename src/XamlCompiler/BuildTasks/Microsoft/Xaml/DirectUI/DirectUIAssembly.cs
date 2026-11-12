// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Reflection;
using Microsoft.UI.Xaml.Markup.Compiler.Lmr;

namespace Microsoft.UI.Xaml.Markup.Compiler.DirectUI
{
    internal class DirectUIAssembly : LmrAssemblyBase
    {
        private readonly Assembly _assembly;
        private AssemblyName _assemblyName;

        private DirectUIAssembly(Assembly assembly)
        {
            if (assembly == null)
            {
                throw new ArgumentNullException("assembly");
            }
            _assembly = assembly;
            _assemblyName = assembly.GetName();
        }

        public static DirectUIAssembly Wrap(Assembly assembly)
        {
            if (assembly == null)
            {
                throw new ArgumentNullException("assembly");
            }
            return new DirectUIAssembly(assembly);
        }

        public static IEnumerable<DirectUIAssembly> Wrap(IEnumerable<Assembly> assemblies)
        {
            if (assemblies != null)
            {
                foreach (Assembly assembly in assemblies)
                    yield return Wrap(assembly);
            }
        }

        public Assembly WrappedAssembly
        {
            get { return _assembly; }
        }

        public bool IsWinmd
        {
            get { return _assemblyName.ContentType == AssemblyContentType.WindowsRuntime; }
        }

        public string BaseName
        {
            get { return _assemblyName.Name; }
        }

        public override object[] GetCustomAttributes(bool inherit)
        {
            return IsWinmd ? new Attribute[0] : _assembly.GetCustomAttributes(inherit);
        }

        public override object[] GetCustomAttributes(Type attributeType, bool inherit)
        {
            return IsWinmd ? new Attribute[0] : _assembly.GetCustomAttributes(attributeType, inherit);
        }

        public override AssemblyName GetName()
        {
            return _assemblyName;
        }

        public override IList<CustomAttributeData> GetCustomAttributesData()
        {
            List<CustomAttributeData> list;
            if (!IsWinmd)
            {
                try
                {
                    return _assembly.GetCustomAttributesData();
                }
                catch (TypeLoadException)
                {
                    list = new List<CustomAttributeData>();
                }
            }
            else
            {
                list = new List<CustomAttributeData>();
            }
            return list;
        }

        public override bool ReflectionOnly
        {
            get { return _assembly.ReflectionOnly; }
        }

        internal override Type GetTypeCore(string name)
        {
            Type type = null;
            try
            {
                type = _assembly.GetTypeLmr(name);
            }
            catch (TypeLoadException)
            {
                // Even though GetType() is documented to not throw it does throws sometimes.
            }
            return type;
        }

        internal override Type[] GetTypesCore()
        {
            return _assembly.GetTypesLmr();
        }

        public override string FullName
        {
            get { return _assembly.FullName; }
        }

        internal override string LocationCore
        {
            get { return _assembly.GetLocationLmr(); }
        }
    }
}

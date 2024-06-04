// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Reflection;

namespace Microsoft.UI.Xaml.Markup.Compiler.DirectUI
{
    internal class ReflectionHelper
    {
        private static ConcurrentDictionary<String, IList<CustomAttributeData>> _typeAttrs = new ConcurrentDictionary<String, IList<CustomAttributeData>>();

        internal static void Release()
        {
            ReflectionHelper._typeAttrs = new ConcurrentDictionary<String, IList<CustomAttributeData>>();
        }

        internal static IEnumerable<CustomAttributeData> GetCustomAttributeData(Type type, bool inherit, string attributeTypeFullName)
        {
            var allAttributesOnTheType = GetCustomAttributeData(type, inherit);
            foreach (CustomAttributeData customAttr in allAttributesOnTheType)
            {
                Type attrType = customAttr.Constructor.DeclaringType;

                if (attrType.FullName == attributeTypeFullName)
                {   
                    yield return customAttr;
                }
            }
        }

        internal static CustomAttributeData FindAttributeByTypeName(Type type, bool inherit, string attributeTypeFullName)
        {
            var attrList = GetCustomAttributeData(type, inherit);
            return FindAttributeByTypeName(attrList, attributeTypeFullName);
        }

        internal static CustomAttributeData FindAttributeByTypeName(MethodInfo mi, string attributeTypeFullName)
        {
            var attrList = GetCustomAttributeData(mi);
            return FindAttributeByTypeName(attrList, attributeTypeFullName);
        }

        internal static CustomAttributeData FindAttributeByTypeName(PropertyInfo pi, string attributeTypeFullName)
        {
            var attrList = GetCustomAttributeData(pi);
            return FindAttributeByTypeName(attrList, attributeTypeFullName);
        }

        internal static CustomAttributeData FindAttributeByShortTypeName(MemberInfo memberInfo, string attributeTypeShortName)
        {
            var attrList = GetCustomAttributeData(memberInfo);
            return FindAttributeByShortTypeName(attrList, attributeTypeShortName);
        }

        // Find Attributes on a Method
        internal static IList<CustomAttributeData> GetCustomAttributeData(MethodInfo mi)
        {
            string propertyFullName = mi.DeclaringType.AssemblyQualifiedName + "." + mi.Name;
            IList<CustomAttributeData> data = null;
            if (!_typeAttrs.TryGetValue(propertyFullName, out data))
            {
                if (mi != null)
                {
                    data = mi.GetCustomAttributesData();
                    _typeAttrs[propertyFullName] = data;
                }
            }
            return data;
        }

        // Find Attributes on a Property
        internal static IList<CustomAttributeData> GetCustomAttributeData(PropertyInfo pi)
        {
            string propertyFullName = pi.DeclaringType.AssemblyQualifiedName + "." + pi.Name;
            IList<CustomAttributeData> data = null;
            if (!_typeAttrs.TryGetValue(propertyFullName, out data))
            {
                if (pi != null)
                {
                    data = pi.GetCustomAttributesData();
                    _typeAttrs[propertyFullName] = data;
                }
            }
            return data;
        }

        internal static IList<CustomAttributeData> GetCustomAttributeData(MemberInfo memberInfo)
        {
            string memberFullName = memberInfo.DeclaringType.AssemblyQualifiedName + "." + memberInfo.Name;
            IList<CustomAttributeData> data = null;
            if (!_typeAttrs.TryGetValue(memberFullName, out data))
            {
                if (memberInfo != null)
                {
                    data = memberInfo.GetCustomAttributesData();
                    _typeAttrs[memberFullName] = data;
                }
            }
            return data;
        }

        internal static IList<CustomAttributeData> GetCustomAttributeData(Type type)
        {
            IList<CustomAttributeData> data;
            if (!_typeAttrs.TryGetValue(type.AssemblyQualifiedName, out data))
            {
                data = type.GetCustomAttributesData();
                _typeAttrs[type.AssemblyQualifiedName] = data;
            }
            return data;
        }

        internal static IEnumerable<CustomAttributeData> GetCustomAttributeData(Type type, bool inherit)
        {
            Type currentType = type;
            do
            {
                foreach (CustomAttributeData data in GetCustomAttributeData(currentType))
                {
                    yield return data;
                }
                currentType = currentType.BaseType;
            } while (inherit && currentType != null);
        }

        internal static CustomAttributeData FindAttributeByTypeName(IEnumerable<CustomAttributeData> attrData, string attributeTypeFullName)
        {
            foreach (CustomAttributeData customAttr in attrData)
            {
                Type attrType = customAttr.AttributeType;
                if (attrType.FullName == attributeTypeFullName || attrType.FullName.Replace("Windows.UI.Xaml", "Microsoft.UI.Xaml") == attributeTypeFullName)
                {
                    return customAttr;
                }
            }
            return null;
        }

        internal static CustomAttributeData FindAttributeByShortTypeName(IEnumerable<CustomAttributeData> attrData, string attributeTypeShortName)
        {
            foreach (CustomAttributeData customAttr in attrData)
            {
                Type attrType = customAttr.AttributeType;
                if (attrType.FullName.EndsWith(attributeTypeShortName))
                {
                    return customAttr;
                }
            }
            return null;
        }

        // Looks in both the names and positional argument lists.
        internal static object GetAttributeConstructorArgument(CustomAttributeData customAttr, int idx, string name)
        {
            if (idx >= 0)
            {
                if (customAttr.ConstructorArguments.Count > idx)
                {
                    CustomAttributeTypedArgument arg = customAttr.ConstructorArguments[idx];
                    return arg.Value;
                }
            }
            if (!String.IsNullOrEmpty(name))
            {
                foreach (CustomAttributeNamedArgument arg in customAttr.NamedArguments)
                {
                    if (arg.MemberName == name)
                    {
                        return arg.TypedValue.Value;
                    }
                }
            }
            return null;
        }

        // FxCop says this is not called
#if FxCop_Says_This_Is_Not_Called
        private static readonly ConcurrentDictionary<Assembly, IList<CustomAttributeData>> _asmAttrs = new ConcurrentDictionary<Assembly, IList<CustomAttributeData>>();

        internal static IList<CustomAttributeData> GetCustomAttributeData(Assembly asm)
        {
            IList<CustomAttributeData> data;
            if (!_asmAttrs.TryGetValue(asm, out data))
            {
                data = asm.GetCustomAttributesData();
                _asmAttrs[asm] = data;
            }
            return data;
        }

        internal static XmlnsDefinitionAttribute[] GetAssemblyXmlNsDefinitionAttributes(Assembly asm, Type attributeType)
        {
            int defCount = 0;
            List<XmlnsDefinitionAttribute> defList = new List<XmlnsDefinitionAttribute>();

            try
            {
                foreach (CustomAttributeData cad in asm.GetCustomAttributesData())
                {
                    ConstructorInfo cinfo = cad.Constructor;
                    if (cinfo.ReflectedType == attributeType)
                    {
                        defCount += 1;
                        String xmlns = cad.ConstructorArguments[0].Value as String;
                        String clrpath = cad.ConstructorArguments[1].Value as String;
                        XmlnsDefinitionAttribute def = new XmlnsDefinitionAttribute(xmlns, clrpath);
                        defList.Add(def);
                    }
                }
            }
            catch (TypeLoadException ex)
            {
                Debug.WriteLine("Attribute Reflection Error: " + ex.Message);
            }
            return defList.ToArray();
        }

        internal static XmlnsPrefixAttribute[] GetAssemblyXmlNsPrefixAttributes(Assembly asm, Type attributeType)
        {
            int defCount = 0;
            List<XmlnsPrefixAttribute> defList = new List<XmlnsPrefixAttribute>();

            foreach (CustomAttributeData cad in asm.GetCustomAttributesData())
            {
                ConstructorInfo cinfo = cad.Constructor;
                if (cinfo.ReflectedType == attributeType)
                {
                    defCount += 1;
                    String xmlns = cad.ConstructorArguments[0].Value as String;
                    String prefix = cad.ConstructorArguments[1].Value as String;
                    XmlnsPrefixAttribute def = new XmlnsPrefixAttribute(xmlns, prefix);
                    defList.Add(def);
                }
            }
            return defList.ToArray();

        }
#endif

    }
}

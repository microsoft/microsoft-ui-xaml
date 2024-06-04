// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using OM;

namespace XamlGen
{
    internal sealed class QueryHelper 
    {
        private static IEnumerable<ClassDefinition> GetBaseClasses(IEnumerable<ClassDefinition> classes)
        {
            return classes.Where(t => t.BaseClass != null).Select(t => t.BaseClass).Distinct();
        }

        private static IEnumerable<PropertyDefinition> GetPublicReferenceTypeProperties(IEnumerable<PropertyDefinition> properties)
        {
            return properties
                .Where(p => {
                    ClassDefinition typeAsClass = p.DeclaringType as ClassDefinition;
                    return typeAsClass != null && !typeAsClass.IsValueType;
                })
                .Where(p => p.Modifier == Modifier.Public);
        }

        private static IEnumerable<PropertyDefinition> GetAccessibleReferenceTypeProperties(IEnumerable<PropertyDefinition> properties)
        {
            return GetPublicReferenceTypeProperties(properties)
                .Where(p => {
                    DependencyPropertyDefinition dp = p as DependencyPropertyDefinition;
                    if (dp != null)
                    {
                        return dp.DependencyPropertyModifier == Modifier.Public;
                    }
                    else
                    {
                        return true;
                    }
                });
        }

        private static IEnumerable<TypeDefinition> GetNonAutomationPeerTypes(IEnumerable<TypeDefinition> types)
        {
            return types.Where(t => {
                var c = t as ClassDefinition;
                return c == null || !c.IsAnAutomationPeer;
            });
        }

        public static IEnumerable<KeyValuePair<string, int>> GetPhoneXamlStrings(OMContextView context)
        {
            List<KeyValuePair<string, int>> result;
            List<string> list = new List<string>();
            var types = GetNonAutomationPeerTypes(context.GetAllClassesAndEnums()).OrderBy(t => t.Name);
            var baseClasses = GetBaseClasses(types.OfType<ClassDefinition>());
            var properties = types.OfType<ClassDefinition>().SelectMany(t => t.TypeTableProperties);
            var publicReferenceTypeProperties = GetPublicReferenceTypeProperties(properties);
            var accessibleNonStructProperties = GetAccessibleReferenceTypeProperties(properties);
            var dependencies = baseClasses.Concat(publicReferenceTypeProperties.Select(p => p.PropertyType.Type)).OrderBy(t => t.Name);
            var enums = types.OfType<EnumDefinition>();
            var genericTypeInstances = publicReferenceTypeProperties.Select(p => p.PropertyType.Type).Where(t =>
                t.IsGenericType &&
                !t.IsGenericTypeDefinition).Distinct();

            list.AddRange(types.Concat(dependencies).Select(t => t.MetadataFullName));
            list.AddRange(types.Concat(genericTypeInstances).Select(t => t.MetadataName));
            list.AddRange(accessibleNonStructProperties.Select(p => p.Name));
            list.AddRange(enums.SelectMany(e => e.Values.Select(v => v.Name)));

            result = new List<KeyValuePair<string, int>>();
            int index = 0;
            foreach (var str in list.Distinct())
            {
                result.Add(new KeyValuePair<string, int>(str, index));
                index += str.Length + 2;
            }
            return result;
        }

        public static IEnumerable<TypeDefinition> GetPhoneXamlTypes(OMContextView context)
        {
            List<TypeDefinition> result = new List<TypeDefinition>();
            var types = GetNonAutomationPeerTypes(context.GetAllClassesAndEnums()).OrderBy(t => t.Name);
            var baseClasses = GetBaseClasses(types.OfType<ClassDefinition>());
            var properties = types.OfType<ClassDefinition>().SelectMany(t => t.TypeTableProperties);
            var publicReferenceTypeProperties = GetPublicReferenceTypeProperties(properties);
            var dependencies = baseClasses
                .Concat(publicReferenceTypeProperties.Select(p => p.PropertyType.Type))
                .OrderByDescending(t => t.IsGenericType && !t.IsGenericTypeDefinition)
                .ThenBy(t => t.Name);

            foreach (var t in types.Concat(dependencies).Distinct())
            {
                result.Add(t);
            }

            return result;
        }

        public static IEnumerable<TypeDefinition> GetPhoneXamlUserTypes(OMContextView context)
        {
            var types = GetNonAutomationPeerTypes(context.GetAllClassesAndEnums()).OrderBy(t => t.Name);
            var properties = types.OfType<ClassDefinition>().SelectMany(t => t.TypeTableProperties);
            var publicReferenceTypeProperties = GetPublicReferenceTypeProperties(properties);
            var genericTypeInstances = publicReferenceTypeProperties
                .Select(p => p.PropertyType.Type)
                .Where(t => t.IsGenericType && !t.IsGenericTypeDefinition)
                .Distinct()
                .OrderBy(t => t.Name);
            return types.Concat(genericTypeInstances);
        }

        public static IEnumerable<PropertyDefinition> GetPhoneXamlUserProperties(OMContextView context)
        {
            var types = GetNonAutomationPeerTypes(context.GetAllClassesAndEnums()).OrderBy(t => t.Name);
            var properties = types.OfType<ClassDefinition>().SelectMany(t => t.TypeTableProperties);
            return GetAccessibleReferenceTypeProperties(properties);
        }

        public static IEnumerable<object> GetPhoneXamlUserPropertiesWithSeparator(OMContextView context)
        {
            List<object> result = new List<object>();
            var phoneXamlUserProperties = GetPhoneXamlUserProperties(context);
            foreach (var t in GetPhoneXamlUserTypes(context).OfType<ClassDefinition>().Where(t => !t.IsValueType && !t.IsAEventArgs))
            {
                result.AddRange(phoneXamlUserProperties.Where(p => p.DeclaringType == t));
                result.Add(t);
            }
            return result;
        }
    }
}

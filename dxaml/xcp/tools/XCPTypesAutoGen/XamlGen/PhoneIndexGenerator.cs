// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace XamlGen
{
    /// <summary>
    /// Assigns indexes to phone-specific types that participate in the type table.
    /// </summary>
    class PhoneIndexGenerator
    {
        private OMContext _context;

        private PhoneIndexGenerator(OMContext context)
        {
            _context = context;
        }

        internal static void GenerateIndexes(OMContext context)
        {
            new PhoneIndexGenerator(context).Run();
        }

        private void Run()
        {
            int nextIndex;

            Func<TypeDefinition, bool> excludePhoneTypes = (type) => type.IdlTypeInfo.Group == null || !type.IdlTypeInfo.Group.Equals("Phone", StringComparison.OrdinalIgnoreCase);
            Func<TypeDefinition, bool> justPhoneTypes = (type) => !excludePhoneTypes(type);
            OMContextView phoneView = _context.GetView(justPhoneTypes);

            // TypeTableIndex
            nextIndex = 0;
            foreach (var t in QueryHelper.GetPhoneXamlTypes(phoneView))
            {
                t.IsPhoneSystemType = !QueryHelper.GetPhoneXamlUserTypes(phoneView).Where(u => u == t).Any();
                t.PhoneTypeTableIndex = nextIndex;
                nextIndex++;
            }

            // PhoneMemberTableIndex for PhoneXamlUserPropertiesWithSeparator
            nextIndex = 0;
            foreach (var o in QueryHelper.GetPhoneXamlUserPropertiesWithSeparator(phoneView))
            {
                var t = o as TypeDefinition;
                var m = o as MemberDefinition;

                if (m != null)
                {
                    t = m.DeclaringClass;
                }
                if (t.PhoneMemberTableIndex == -1)
                {
                    t.PhoneMemberTableIndex = nextIndex;
                }
                nextIndex++;
            }

            // MemberTableIndex for PhoneXamlUserProperties
            nextIndex = 0;
            foreach (var m in QueryHelper.GetPhoneXamlUserProperties(phoneView))
            {
                m.PhoneMemberTableIndex = nextIndex++;
            }

            // EnumIndex
            nextIndex = 0;
            foreach (var e in phoneView.GetAllEnums())
            {
                e.PhoneEnumIndex = nextIndex++;
            }
        }

        private static string GetDefaultIndexName(NamespaceDefinition ns)
        {
            return ns.Name.Replace('.', '_');
        }

        private static string GetDefaultIndexName(TypeDefinition type)
        {
            if (type.IsGenericType)
            {
                StringBuilder builder = new StringBuilder(type.Name);
                builder.Append("Of");
                foreach (TypeReference argType in type.GenericArguments)
                {
                    builder.Append(GetDefaultIndexName(argType.Type));
                }
                return builder.ToString();
            }
            return type.Name;
        }

        private static string GetDefaultIndexName(PropertyDefinition member)
        {
            return GetDefaultIndexName(member.DeclaringType) + "_" + member.Name;
        }

        private static string GetDefaultIndexName(EventDefinition member)
        {
            return GetDefaultIndexName(member.DeclaringType) + "_" + member.Name;
        }
    }
}

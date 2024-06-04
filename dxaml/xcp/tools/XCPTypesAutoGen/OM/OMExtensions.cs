// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;

namespace OM
{
    public static class OMExtensions
    {
        public static IEnumerable<ClassDefinition> OrderByHierarchy(this IEnumerable<ClassDefinition> classes)
        {
            return classes.OrderBy(c => GetAncestorCount(c, t => t.BaseClass));
        }

        public static IEnumerable<ClassDefinition> OrderByTypeTableHierarchy(this IEnumerable<ClassDefinition> classes)
        {
            return classes.OrderBy(c => GetAncestorCount(c, t => t.BaseClassInTypeTable));
        }

        public static IEnumerable<ClassDefinition> OrderByIdlHierarchy(this IEnumerable<ClassDefinition> classes)
        {
            return classes.OrderBy(c => GetAncestorCount(c, t => t.IdlClassInfo.BaseClass));
        }

        public static OMContextView WithCustomIdlFileNameImport(this OMContextView view, string idlFileName)
        {
            view.AddCustomIdlFileNameImport(idlFileName);
            return view;
        }

        internal static Guid GetGuid(this Dictionary<int, Guid> guids, TypeDefinition type, string guidType, int version)
        {
            Guid guid = Guid.Empty;
            if (guids.ContainsKey(version))
            {
                guid = guids[version];
            }

            return guid;
        }

        private static int GetAncestorCount(ClassDefinition type, Func<ClassDefinition, ClassDefinition> resolveBaseClass)
        {
            int ancestorCount = 0;
            for (ClassDefinition baseClass = resolveBaseClass(type); baseClass != null; baseClass = resolveBaseClass(baseClass))
            {
                ancestorCount++;
            }
            return ancestorCount;
        }
    }
}

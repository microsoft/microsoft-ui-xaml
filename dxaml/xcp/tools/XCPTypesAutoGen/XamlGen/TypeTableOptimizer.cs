// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using OM.Visitors;
using System;
using System.Collections.Generic;
using System.Linq;

namespace XamlGen
{
    /// <summary>
    /// Removes types from the type table if they:
    /// 1. Don't have any type table properties.
    /// 2. Are not activatable and are not abstract.
    /// 3. Are not primitive.
    /// 4. Are not referenced by any other type table properties.
    /// </summary>
    class TypeTableOptimizer : OMVisitor
    {
        private HashSet<TypeDefinition> _typesToKeep = new HashSet<TypeDefinition>();

        private TypeTableOptimizer(OMContextView view)
            : base(view)
        {
        }

        internal static void Optimize(OMContext context)
        {
            Func<NamespaceDefinition, bool> includeNamespaceInTypeTable = ns => !ns.IsExcludedFromTypeTable;
            Func<TypeDefinition, bool> includeTypeInTypeTable = type => !type.IsExcludedFromTypeTable;
            OMContextView view = context.GetView(includeNamespaceInTypeTable, includeTypeInTypeTable);
            TypeTableOptimizer optimizer = new TypeTableOptimizer(view);
            optimizer.Run();

            foreach (TypeDefinition type in view.GetAllTypeTableTypes())
            {
                if (!optimizer._typesToKeep.Contains(type))
                {
                    type.IsExcludedFromTypeTable = true;
                }
            }
        }

        protected override void VisitClass(ClassDefinition c)
        {
            // TODO: Remove IsActivationDisabledInTypeTable check once we can.
            bool shouldKeep = (c.ForceIncludeInTypeTable || c.IsAEventArgs || c.IsPrimitive || c.IsActivatableInCore || c.IsActivatable || c.IsAbstract || c.IsActivationDisabledInTypeTable || c.IsXbfType) && !c.IsExcludedFromTypeTable;
            foreach (PropertyDefinition p in c.TypeTableProperties)
            {
                _typesToKeep.Add(p.PropertyType.Type);
                shouldKeep = true;
            }

            if (shouldKeep)
            {
                _typesToKeep.Add(c);
                ClassDefinition baseClass = c.BaseClassInTypeTable;
                if (baseClass != ClassDefinition.UnknownType)
                {
                    _typesToKeep.Add(baseClass);
                }
            }

            VisitTypeInternal(c.BaseClassInTypeTable);
        }

        protected override void VisitEnum(EnumDefinition e)
        {
            _typesToKeep.Add(e);
        }
    }
}

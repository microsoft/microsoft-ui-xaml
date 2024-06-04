// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using OM.Visitors;

namespace XamlGen
{
    /// <summary>
    /// Assigns default indexes to types that participate in the type table.
    /// </summary>
    class DependencyGenerator: OMVisitor
    {
        private OMContextView _view;
        private TypeDefinition _currentType;

        private DependencyGenerator(OMContextView view)
            : base(view)
        {
            _view = view;
        }

        internal static void GenerateDependencies(OMContext context)
        {
            new DependencyGenerator(context.GetView()).Run();
        }

        protected override void VisitType(TypeDefinition type)
        {
            if (!type.IsGenericType)
            {
                TypeDefinition previousType = _currentType;
                _currentType = type;
                base.VisitType(type);
                _currentType = previousType;
            }
        }

        protected override void VisitClass(ClassDefinition c)
        {
            if (c.BaseClass != null)
            {
                RegisterDependency(c.BaseClass);
            }
            base.VisitClass(c);
        }

        protected override void VisitTypeRef(TypeReference typeRef)
        {
            if (!typeRef.IsVoid)
            {
                RegisterDependency(typeRef.IdlInfo.Type);
            }
        }

        private void RegisterDependency(TypeDefinition dependency)
        {
            if (_currentType != dependency && dependency != null && !dependency.IsObjectType && !dependency.IsPrimitive)
            {
                _currentType.Dependencies.Add(dependency);
            }
        }
    }
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies which type tables this type or member should be excluded from.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Delegate | AttributeTargets.Enum | AttributeTargets.Interface | AttributeTargets.Property | AttributeTargets.Method | AttributeTargets.Event, Inherited = false)]
    public class TypeTableAttribute : Attribute,
        NewBuilders.ITypeBuilder, NewBuilders.IMethodBuilder
    {
        private bool? _isExcludedFromDXaml;
        private bool? _isExcludedFromCore;
        private bool? _isExcludedFromNewTypeTable;
        private bool? _isExcludedFromVisualTree;
        private bool? _isExcludedFromReferenceTrackerWalk;

        public bool ExcludeGuid
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets whether to include this type in the type table, even if it appears this type doesn't need to be in the 
        /// type table because nothing references it.
        /// </summary>
        public bool ForceInclude
        {
            get;
            set;
        }

        public bool IsExcludedFromDXaml
        {
            get
            {
                return _isExcludedFromDXaml.HasValue && _isExcludedFromDXaml.Value;
            }
            set
            {
                _isExcludedFromDXaml = value;
            }
        }
        
        public bool IsExcludedFromCore
        {
            get
            {
                return _isExcludedFromCore.HasValue && _isExcludedFromCore.Value;
            }
            set
            {
                _isExcludedFromCore = value;
            }
        }

        public bool IsExcludedFromNewTypeTable
        {
            get
            {
                return _isExcludedFromNewTypeTable.HasValue && _isExcludedFromNewTypeTable.Value;
            }
            set
            {
                _isExcludedFromNewTypeTable = value;
            }
        }

        /// <summary>
        /// Gets or sets whether properties of this type should be excluded from things like the render type 
        /// table and the enter type table.
        /// </summary>
        public bool IsExcludedFromVisualTree
        {
            get
            {
                return _isExcludedFromVisualTree.HasValue && _isExcludedFromVisualTree.Value;
            }
            set
            {
                _isExcludedFromVisualTree = value;
            }
        }

        /// <summary>
        /// Gets or sets whether properties of this type should be excluded from the reference tracker walk.
        /// </summary>
        public bool IsExcludedFromReferenceTrackerWalk
        {
            get
            {
                return _isExcludedFromReferenceTrackerWalk.HasValue && _isExcludedFromReferenceTrackerWalk.Value;
            }
            set
            {
                _isExcludedFromReferenceTrackerWalk = value;
            }
        }

        public bool IsXbfType
        {
            get;
            set;
        }

        public void BuildNewType(OM.TypeDefinition definition, Type source)
        {
            if (_isExcludedFromNewTypeTable.HasValue)
            {
                definition.IsExcludedFromTypeTable = _isExcludedFromNewTypeTable.Value;
            }
            else if (IsExcludedFromDXaml && IsExcludedFromCore)
            {
                definition.IsExcludedFromTypeTable = true;
            }

            if (IsExcludedFromVisualTree)
            {
                definition.IsExcludedFromVisualTreeNodeConsideration = true;
            }

            if (IsExcludedFromReferenceTrackerWalk)
            {
                definition.IsExcludedFromReferenceTrackerWalk = true;
            }

            definition.ExcludeGuidFromTypeTable = ExcludeGuid;

            if (IsXbfType)
            {
                definition.IsXbfType = true;
            }

            definition.ForceIncludeInTypeTable = ForceInclude;
        }

        public void BuildNewMethod(OM.MethodDefinition definition, MethodInfo source)
        {
            if (!source.IsAbstract)
            {
                definition.GenerateDefaultBody = false;
            }
        }
    }
}

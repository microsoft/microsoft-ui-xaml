// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM
{
    public class DependencyPropertyDefinition : PropertyDefinition
    {
        /// <summary>
        /// Controls if property will be iterated over in MetadataAPI iteration.
        /// </summary>
        public bool AllowEnumeration { get; set; } = true;

        /// <summary>
        /// Visibility of the DependencyProperty property, such as DependencyObject.NameProperty.
        /// To see the visibility of e.g. DependencyObject.Name, look at the Modifier property instead.
        /// </summary>
        public Modifier DependencyPropertyModifier
        {
            get;
            set;
        }

        public string DescriptiveName
        {
            get
            {
                return DeclaringType.FullName + "." + Name;
            }
        }

        /// <summary>
        /// Should stable index be genarated for this property?
        /// </summary>
        public bool GenerateStableIndex { get; set; } = true;

        public string HandleGetterName
        {
            get
            {
                return string.Format("get_{0}Property", IdlDPInfo.Name);
            }
        }

        public bool HasField
        {
            get
            {
                return !string.IsNullOrEmpty(FieldName);
            }
        }

        public bool HasPropertyMethod
        {
            get
            {
                return !string.IsNullOrEmpty(MethodName);
            }
        }

        public bool HasRenderDirtyFlagCallback
        {
            get
            {
                return (!string.IsNullOrEmpty(RenderDirtyFlagClassName) && !string.IsNullOrEmpty(RenderDirtyFlagMethodName));
            }
        }

        public Idl.IdlDependencyPropertyInfo IdlDPInfo
        {
            get
            {
                return (Idl.IdlDependencyPropertyInfo)IdlPropertyInfo;
            }
        }

        public string InternalHandleGetterName
        {
            get
            {
                return string.Format("Get{0}PropertyStatic", IdlDPInfo.Name);
            }
        }

        public bool IsEnterProperty
        {
            get
            {
                return IsRenderProperty || XamlPropertyFlags.NeedsInvoke;
            }
        }

        /// <summary>
        /// 'Handle' refers to the DependencyProperty object representing the property.
        /// Example: UIElement.WidthProperty
        /// </summary>
        public bool IsHandlePublic
        {
            get
            {
                return DependencyPropertyModifier == OM.Modifier.Public;
            }
        }

        /// <summary>
        /// Gets or sets whether this is a property with field storage of an object type.
        /// </summary>
        public bool IsObjectProperty
        {
            get
            {
                return IsObjectOrDependencyObjectTreeProperty && HasField;
            }
        }

        /// <summary>
        /// Gets or sets whether this is a property with field storage of an object type and should participate in render walk.
        /// </summary>
        public bool IsRenderProperty
        {
            get
            {
                return IsVisualTreeProperty && HasField;
            }
        }

        public bool IsSimpleProperty
        {
            get
            {
                return SimpleStorage.HasValue;
            }
        }

        public bool IsSparse
        {
            get
            {
                return (!HasField && !HasPropertyMethod && !IsInStorageGroup) && !IsImplicitContentProperty && !IsSimpleProperty;
            }
        }

        public bool IsVisualTreeProperty
        {
            get;
            set;
        }

        public bool IsObjectOrDependencyObjectTreeProperty
        {
            get;
            set;
        }

        public DependencyPropertyDefinition NextPropertyInTypeTable
        {
            get
            {
                if (!AllowEnumeration)
                {
                    return UnknownProperty;
                }

                ClassDefinition declaringClass = (ClassDefinition)DeclaringType;
                bool foundProperty = false;
                DependencyPropertyDefinition nextProperty = null;
                foreach (DependencyPropertyDefinition property in declaringClass.TypeTableProperties.Where((p) => p.AllowEnumeration))
                {
                    if (property == this)
                    {
                        foundProperty = true;
                    }
                    else if (foundProperty)
                    {
                        nextProperty = property;
                        break;
                    }
                }

                if (nextProperty != null)
                {
                    return nextProperty;
                }

                return declaringClass.BaseClassInTypeTable.FirstPropertyInTypeTable;
            }
        }

        public DependencyPropertyDefinition NextEnterPropertyInTypeTable
        {
            get
            {
                ClassDefinition declaringClass = (ClassDefinition)EffectiveTargetType.Type;
                bool foundEnterProperty = false;
                DependencyPropertyDefinition nextEnterProperty = null;
                foreach (DependencyPropertyDefinition property in declaringClass.EffectiveTypeTableProperties.Where(p => p.IsEnterProperty && p.AllowEnumeration))
                {
                    if (property == this)
                    {
                        foundEnterProperty = true;
                    }
                    else if (foundEnterProperty)
                    {
                        nextEnterProperty = property;
                        break;
                    }
                }

                if (nextEnterProperty != null)
                {
                    return nextEnterProperty;
                }

                if (declaringClass.BaseClass == null)
                {
                    return UnknownProperty;
                }

                return declaringClass.BaseClass.FirstEnterPropertyInTypeTable;
            }
        }

        public DependencyPropertyDefinition NextObjectPropertyInTypeTable
        {
            get
            {
                ClassDefinition declaringClass = (ClassDefinition)EffectiveTargetType.Type;
                bool foundObjectProperty = false;
                DependencyPropertyDefinition nextObjectProperty = null;
                foreach (DependencyPropertyDefinition property in declaringClass.EffectiveTypeTableProperties.Where(p => p.IsObjectProperty && p.AllowEnumeration))
                {
                    if (property == this)
                    {
                        foundObjectProperty = true;
                    }
                    else if (foundObjectProperty)
                    {
                        nextObjectProperty = property;
                        break;
                    }
                }

                if (nextObjectProperty != null)
                {
                    return nextObjectProperty;
                }

                if (declaringClass.BaseClass == null)
                {
                    return UnknownProperty;
                }

                return declaringClass.BaseClass.FirstObjectPropertyInTypeTable;
            }
        }

        public DependencyPropertyDefinition NextRenderPropertyInTypeTable
        {
            get
            {
                ClassDefinition declaringClass = (ClassDefinition)EffectiveTargetType.Type;
                bool foundRenderProperty = false;
                DependencyPropertyDefinition nextRenderProperty = null;
                foreach (DependencyPropertyDefinition property in declaringClass.EffectiveTypeTableProperties.Where(p => p.IsRenderProperty && p.AllowEnumeration))
                {
                    if (property == this)
                    {
                        foundRenderProperty = true;
                    }
                    else if (foundRenderProperty)
                    {
                        nextRenderProperty = property;
                        break;
                    }
                }

                if (nextRenderProperty != null)
                {
                    return nextRenderProperty;
                }

                if (declaringClass.BaseClass == null)
                {
                    return UnknownProperty;
                }

                return declaringClass.BaseClass.FirstRenderPropertyInTypeTable;
            }
        }

        public string RenderDirtyFlagCallbackPointer
        {
            get
            {
                if (HasRenderDirtyFlagCallback)
                {
                    return string.Format("&{0}::{1}", RenderDirtyFlagClassName, RenderDirtyFlagMethodName);
                }
                return string.Empty;
            }
        }

        public string RenderDirtyFlagClassName
        {
            get;
            set;
        }

        public string RenderDirtyFlagMethodName
        {
            get;
            set;
        }

        public string SimpleDeclaringClass
        {
            get
            {
                return DeclaringType.CoreName;
            }
        }

        public string SimpleDefaultValue
        {
            get;
            set;
        }

        public string SimpleGroupStorageClass
        {
            get;
            set;
        }

        public SimplePropertyStorage? SimpleStorage
        {
            get;
            set;
        }

        /// <summary>
        /// Type-specific index. Useful for bitfields to keep track of the state of a property for a type/object.
        /// </summary>
        public int Slot
        {
            get;
            set;
        }

        public int TypeTableIndex
        {
            get;
            set;
        }

        public string TypeTableName
        {
            get
            {
                if (this == UnknownProperty)
                {
                    return string.Empty;
                }
                return IdlDPInfo.Name;
            }
        }

        /// <summary>
        /// Index into the table with properties used during Enter/Leave.
        /// </summary>
        public int TypeTableEnterPropertyIndex
        {
            get;
            set;
        }

        /// <summary>
        /// Index into the table with properties used during ReferenceTrackerWalk.
        /// </summary>
        public int TypeTableObjectPropertyIndex
        {
            get;
            set;
        }

        /// <summary>
        /// Index into the table with properties used during CleanupDeviceRelatedResourcesRecursive/ResetReferencesFromChildren.
        /// </summary>
        public int TypeTableRenderPropertyIndex
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the underlying DP. This is currently only used by FrameworkElement.NameProperty, which 
        /// delegates to the internal DependencyObject.NameProperty.
        /// </summary>
        public DependencyPropertyDefinition UnderlyingDependencyProperty
        {
            get;
            set;
        }

        public DependencyPropertyDefinition()
        {
            DependencyPropertyModifier = OM.Modifier.Public;
            TypeTableIndex = -1;
            TypeTableObjectPropertyIndex = -1;
        }

        protected override Idl.IdlPropertyInfo CreateIdlPropertyInfo()
        {
            return new Idl.IdlDependencyPropertyInfo(this);
        }
    }
}

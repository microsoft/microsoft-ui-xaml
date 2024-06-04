// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Reflection;
using Microsoft.UI.Xaml.Markup;

namespace Microsoft.UI.Xaml.Markup
{
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
    internal class XamlReflectionMember : IXamlMember
    {
        string _memberName;
        bool _isDependencyProperty = false;
        bool _isReadOnly = false;
        Type _underlyingType = null; //The type of the property's value
        Type _declaringType = null; //The type the property was declared on

        // Fields for attachable properties
        bool _isAttachable = false;
        Type _targetType = null; // For attached properties, what type the property can be set on - otherwise null
        MethodInfo _attachableGetterInfo = null; // Method info for the getter method for an attachable property
        MethodInfo _attachableSetterInfo = null; // Method info for the setter method for an attachable property

        public static XamlReflectionMember Create(string memberName, Type declaringType)
        {
            bool isDependencyProperty = false;
            bool isReadOnly = false;
            Type underlyingType = null;

            bool isAttachable = false;
            Type targetType = null;
            MethodInfo attachableGetterInfo = null;
            MethodInfo attachableSetterInfo = null;

            PropertyInfo pi = declaringType.GetRuntimeProperty(memberName);

            // It's OK to not get the property - we could be dealing with an attached property,
            // which isn't a real property on the underlying type.
            // We do need to extract the underlying type and read-only based
            // on the existence of a getter and potentially setter later on
            // for attached properties
            if (pi != null)
            {
                underlyingType = pi.PropertyType;
                isReadOnly = !pi.CanWrite;
            }

            PropertyInfo piDP = declaringType.GetStaticProperty(memberName + "Property");
            if (piDP != null)
            {
                isDependencyProperty = true;
            }
            else
            {
                FieldInfo fiDP = declaringType.GetStaticField(memberName + "Property");
                if (fiDP != null)
                {
                    isDependencyProperty = true;
                }
            }

            // Attachable properties aren't necessarily backed by a DP, so we don't check for it
            bool foundGetter = false;
            bool foundSetter = false;
            Type getterTargetType = null;
            Type setterTargetType = null;
            // Assuming the property is attachable, we're looking for a "Set[MemberName]" method that takes two arguments (the object and the property value).
            // We're also looking for the "Get[MemberName]" method which takes the object and returns the property's value
            // While we know the type the attachable property was declared on and what its property value type is, we don't know what
            // object type it should be settable on.  So we need to dig through all of the type's methods to find an appropriate one
            IEnumerable<MethodInfo> typeMethods = declaringType.GetRuntimeMethods();
            foreach (MethodInfo mi in typeMethods)
            {
                if (foundGetter && foundSetter)
                {
                    break;
                }
                //The attachable getter and setter must be public and static
                if (mi.IsStatic && mi.IsPublic)
                {
                    // Name is what we expect for the setter
                    if (mi.Name.Equals("Set" + memberName))
                    {
                        //The second parameter's type should be the same as the type the property's value
                        ParameterInfo[] paramInfo = mi.GetParameters();
                        if (paramInfo.Length == 2)
                        {
                            if (underlyingType == null)
                            {
                                underlyingType = paramInfo[1].ParameterType;
                            }

                            setterTargetType = paramInfo[0].ParameterType;
                            foundSetter = true;
                            attachableSetterInfo = mi;
                        }
                    }
                    else if (mi.Name.Equals("Get" + memberName))
                    {
                        ParameterInfo[] paramInfo = mi.GetParameters();
                        if (paramInfo.Length == 1)
                        {
                            if (underlyingType == null)
                            {
                                underlyingType = mi.ReturnType;
                            }

                            getterTargetType = paramInfo[0].ParameterType;
                            foundGetter = true;
                            attachableGetterInfo = mi;
                        }
                    }
                }
            }

            // If we found a getter or setter following the attachable property convention, mark this member as attachable
            if (foundGetter || foundSetter)
            {
                isAttachable = true;
                isReadOnly = !foundSetter;

                if (foundGetter)
                {
                    targetType = getterTargetType;
                }
                else if (foundSetter)
                {
                    targetType = setterTargetType;
                }
            }

            // If underlyingType isn't null, we have enough valid info for a member and should create one
            // Otherwise, we couldn't resolve the member and should return null.
            if (underlyingType != null)
            {
                XamlReflectionMember memb = new XamlReflectionMember(memberName,
                    isDependencyProperty,
                    isReadOnly,
                    underlyingType,
                    declaringType,
                    isAttachable,
                    targetType,
                    attachableGetterInfo,
                    attachableSetterInfo);
                return memb;
            }

            return null;
        }

        protected XamlReflectionMember(string memberName,
            bool isDependencyProperty,
            bool isReadOnly,
            Type underlyingType,
            Type declaringType,
            bool isAttachable,
            Type targetType,
            MethodInfo attachableGetterInfo,
            MethodInfo attachableSetterInfo)
        {
            _memberName = memberName;
            _isDependencyProperty = isDependencyProperty;
            _isReadOnly = isReadOnly;
            _underlyingType = underlyingType;
            _declaringType = declaringType;
            _isAttachable = isAttachable;
            _targetType = targetType;
            _attachableGetterInfo = attachableGetterInfo;
            _attachableSetterInfo = attachableSetterInfo;
        }

        public string Name { get { return _memberName; } }

        public IXamlType Type
        {
            get
            {
                return ReflectionXamlMetadataProvider.getXamlType(_underlyingType);
            }
        }

        public IXamlType TargetType
        {
            get
            {
                return ReflectionXamlMetadataProvider.getXamlType(_targetType);
            }
        }

        public bool IsAttachable { get { return _isAttachable; } }

        public bool IsDependencyProperty { get { return _isDependencyProperty; } }

        public bool IsReadOnly { get { return _isReadOnly; } }

        public object GetValue(object instance)
        {
            if (!_isAttachable)
            {
                PropertyInfo pi = _declaringType.GetRuntimeProperty(_memberName);
                object objVal = pi.GetValue(instance);

                // When a native app sets a null/empty string (which are the same thing for native) on a managed object,
                // it's normally always converted into an empty string on the managed side.  If we were to actually set a null value
                // via reflection, the native app can't marshall the null value and will crash.  So we'll always interpet a null string
                // as being an empty string instead.
                if (objVal == null && pi.PropertyType.Equals(typeof(string)))
                {
                    objVal = String.Empty;
                }
                return objVal;
            }
            else
            {
                return _attachableGetterInfo.Invoke(null, new object[1] { instance });
            }
        }

        public void SetValue(object instance, object value)
        {
            if (!_isAttachable)
            {
                PropertyInfo pi = _declaringType.GetRuntimeProperty(_memberName);

                // When a native app sets a null/empty string (which are the same thing for native) on a managed object,
                // it's normally always converted into an empty string on the managed side.  If we were to actually set a null value
                // via reflection, the native app can't marshall the null value and will crash.  So we'll always interpet a null string
                // as being an empty string instead.
                if (value == null && pi.PropertyType.Equals(typeof(string)))
                {
                    value = String.Empty;
                }
                pi.SetValue(instance, value);
            }
            else
            {
                if (!_isReadOnly)
                {
                    _attachableSetterInfo.Invoke(null, new object[2] { instance, value });
                }
                else
                {
                    throw new ReflectionHelperException($"Attempted to write to read-only attachable property '{_declaringType.FullName}.{_memberName}'");
                }
            }
        }
    }
}
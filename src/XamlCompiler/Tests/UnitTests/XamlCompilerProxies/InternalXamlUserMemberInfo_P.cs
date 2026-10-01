// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Reflection;
using System.Text;

namespace Win8Xaml.CompilerProxies
{
    [DebuggerDisplay("{Type.Name}  {DeclaringType.Name}.{Name}")]
    public class InternalXamlUserMemberInfo
    {
        static ProxyHelper _internalXamlMemberType;
        static PropertyInfo _nameProperty;
        static PropertyInfo _typeProperty;
        static PropertyInfo _declaringTypeProperty;
        static PropertyInfo _isDependencyPropertyProperty;
        static PropertyInfo _isAttachableProperty;
        static PropertyInfo _targetTypeProperty;

        object _instance;

        static InternalXamlUserMemberInfo()
        {
            _internalXamlMemberType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.CodeGen.InternalXamlUserMemberInfo");
            _nameProperty = _internalXamlMemberType.GetProperty("Name");
            _typeProperty = _internalXamlMemberType.GetProperty("Type");
            _declaringTypeProperty = _internalXamlMemberType.GetProperty("DeclaringType");
            _isDependencyPropertyProperty = _internalXamlMemberType.GetProperty("IsDependencyProperty");
            _isAttachableProperty = _internalXamlMemberType.GetProperty("IsAttachable");
            _targetTypeProperty = _internalXamlMemberType.GetProperty("TargetType");
        }

        public InternalXamlUserMemberInfo(object instance)
        {
            _instance = instance;
        }

        public String Name
        {
            get { return (String)_nameProperty.GetValue(_instance, null); }
        }

        public InternalTypeEntry Type
        {
            get
            {
                object obj = _typeProperty.GetValue(_instance, null);
                return (obj == null) ? null : new InternalTypeEntry(obj);  // this is a proxied type.
            }
        }


        public InternalTypeEntry DeclaringType
        {
            get
            {
                object obj = _declaringTypeProperty.GetValue(_instance, null);
                return (obj == null) ? null : new InternalTypeEntry(obj);  // this is a proxied type.
            }
        }


        public bool IsDependencyProperty
        {
            get { return (bool)_isDependencyPropertyProperty.GetValue(_instance, null); }
        }

        public bool IsAttachable
        {
            get { return (bool)_isAttachableProperty.GetValue(_instance, null); }
        }

        public InternalTypeEntry TargetType
        {
            get
            {
                object obj = _targetTypeProperty.GetValue(_instance, null);
                return (obj == null) ? null : new InternalTypeEntry(obj);  // this is a proxied type.
            }
        }

    }
}

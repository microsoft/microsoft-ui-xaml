// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Internal Type subclass used during signature decoding to carry modreq/modopt information.
// Peeled off by SignatureUnwrap.Unwrap() after DecodeSignature returns.
// Never escapes into public reflection results.

using System;
using System.Globalization;
using System.Reflection;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Wraps a Type with an associated modifier (modreq or modopt).
    /// SRM's ISignatureTypeProvider.GetModifiedType returns TType, so we encode
    /// the modifier info into the Type itself and strip it later via SignatureUnwrap.
    /// </summary>
    internal sealed class SignatureModifiedType : Type
    {
        internal Type Underlying { get; }
        internal Type Modifier { get; }
        internal bool IsRequired { get; }

        internal SignatureModifiedType(Type underlying, Type modifier, bool isRequired)
        {
            Underlying = underlying ?? throw new ArgumentNullException(nameof(underlying));
            Modifier = modifier ?? throw new ArgumentNullException(nameof(modifier));
            IsRequired = isRequired;
        }

        // Forward all Type virtuals to Underlying so that callers who accidentally
        // use this Type before unwrapping get reasonable behavior.

        public override Assembly Assembly => Underlying.Assembly;
        public override string AssemblyQualifiedName => Underlying.AssemblyQualifiedName;
        public override Type BaseType => Underlying.BaseType;
        public override string FullName => Underlying.FullName;
        public override Guid GUID => Underlying.GUID;
        public override Module Module => Underlying.Module;
        public override string Namespace => Underlying.Namespace;
        public override Type UnderlyingSystemType => Underlying.UnderlyingSystemType;
        public override string Name => Underlying.Name;

        public override ConstructorInfo[] GetConstructors(BindingFlags bindingAttr) => Underlying.GetConstructors(bindingAttr);
        public override object[] GetCustomAttributes(bool inherit) => Underlying.GetCustomAttributes(inherit);
        public override object[] GetCustomAttributes(Type attributeType, bool inherit) => Underlying.GetCustomAttributes(attributeType, inherit);
        public override Type GetElementType() => Underlying.GetElementType();
        public override EventInfo GetEvent(string name, BindingFlags bindingAttr) => Underlying.GetEvent(name, bindingAttr);
        public override EventInfo[] GetEvents(BindingFlags bindingAttr) => Underlying.GetEvents(bindingAttr);
        public override FieldInfo GetField(string name, BindingFlags bindingAttr) => Underlying.GetField(name, bindingAttr);
        public override FieldInfo[] GetFields(BindingFlags bindingAttr) => Underlying.GetFields(bindingAttr);
        public override Type GetInterface(string name, bool ignoreCase) => Underlying.GetInterface(name, ignoreCase);
        public override Type[] GetInterfaces() => Underlying.GetInterfaces();
        public override MemberInfo[] GetMembers(BindingFlags bindingAttr) => Underlying.GetMembers(bindingAttr);
        public override MethodInfo[] GetMethods(BindingFlags bindingAttr) => Underlying.GetMethods(bindingAttr);
        public override Type GetNestedType(string name, BindingFlags bindingAttr) => Underlying.GetNestedType(name, bindingAttr);
        public override Type[] GetNestedTypes(BindingFlags bindingAttr) => Underlying.GetNestedTypes(bindingAttr);
        public override PropertyInfo[] GetProperties(BindingFlags bindingAttr) => Underlying.GetProperties(bindingAttr);
        public override object InvokeMember(string name, BindingFlags invokeAttr, Binder binder, object target, object[] args, ParameterModifier[] modifiers, CultureInfo culture, string[] namedParameters) => Underlying.InvokeMember(name, invokeAttr, binder, target, args, modifiers, culture, namedParameters);
        public override bool IsDefined(Type attributeType, bool inherit) => Underlying.IsDefined(attributeType, inherit);
        protected override TypeAttributes GetAttributeFlagsImpl() => Underlying.Attributes;
        protected override ConstructorInfo GetConstructorImpl(BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers) => Underlying.GetConstructor(bindingAttr, binder, callConvention, types, modifiers);
        protected override MethodInfo GetMethodImpl(string name, BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers) => Underlying.GetMethod(name, bindingAttr, binder, callConvention, types, modifiers);
        protected override PropertyInfo GetPropertyImpl(string name, BindingFlags bindingAttr, Binder binder, Type returnType, Type[] types, ParameterModifier[] modifiers) => Underlying.GetProperty(name, bindingAttr, binder, returnType, types, modifiers);
        protected override bool HasElementTypeImpl() => Underlying.HasElementType;
        protected override bool IsArrayImpl() => Underlying.IsArray;
        protected override bool IsByRefImpl() => Underlying.IsByRef;
        protected override bool IsCOMObjectImpl() => false;
        protected override bool IsPointerImpl() => Underlying.IsPointer;
        protected override bool IsPrimitiveImpl() => Underlying.IsPrimitive;

        public override bool Equals(object obj)
        {
            if (obj is SignatureModifiedType other)
                return Underlying.Equals(other.Underlying);
            return Underlying.Equals(obj);
        }

        public override int GetHashCode() => Underlying.GetHashCode();
    }
}

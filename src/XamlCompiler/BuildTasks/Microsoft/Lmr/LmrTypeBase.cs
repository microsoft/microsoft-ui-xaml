// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Diagnostics.CodeAnalysis;
using System.Globalization;
using System.Reflection;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// Common base class of every LMR implementation of <see cref="Type"/>.
    /// </summary>
    /// <remarks>
    /// The members of <see cref="Type"/> that are annotated for trimming (<see cref="DynamicallyAccessedMembersAttribute"/>
    /// on 'this', parameters or return values, <see cref="RequiresUnreferencedCodeAttribute"/>, <see cref="RequiresDynamicCodeAttribute"/>)
    /// are overridden exactly once, here, with the same annotations as the base member (which is what the trim analyzer
    /// requires from overrides), and they only forward to an unannotated internal "*Core" member that the LMR types implement.
    ///
    /// LMR types are never runtime types: they are read from metadata, and the reflection operations on them never
    /// touch any runtime type, method or field, so none of these annotations is relevant for them. Code that works with
    /// LMR types calls the "*Core" members, directly or through <see cref="LmrReflection"/>, which don't carry
    /// any annotation, instead of the annotated public members of <see cref="Type"/>.
    ///
    /// Don't put any class-level trimming annotation on LMR types: a class-level DynamicallyAccessedMembers keeps the
    /// members of all derived types (and warns about them), and a class-level RequiresUnreferencedCode applies to constructors.
    /// </remarks>
    internal abstract class LmrTypeBase : Type
    {
        // The annotations of the Type members, see Type.GetAllMembers etc. in the .NET sources.
        internal const DynamicallyAccessedMemberTypes AllConstructors =
            DynamicallyAccessedMemberTypes.PublicConstructors | DynamicallyAccessedMemberTypes.NonPublicConstructors;
        internal const DynamicallyAccessedMemberTypes AllEvents =
            DynamicallyAccessedMemberTypes.PublicEvents | DynamicallyAccessedMemberTypes.NonPublicEvents;
        internal const DynamicallyAccessedMemberTypes AllFields =
            DynamicallyAccessedMemberTypes.PublicFields | DynamicallyAccessedMemberTypes.NonPublicFields;
        internal const DynamicallyAccessedMemberTypes AllMethods =
            DynamicallyAccessedMemberTypes.PublicMethods | DynamicallyAccessedMemberTypes.NonPublicMethods;
        internal const DynamicallyAccessedMemberTypes AllNestedTypes =
            DynamicallyAccessedMemberTypes.PublicNestedTypes | DynamicallyAccessedMemberTypes.NonPublicNestedTypes;
        internal const DynamicallyAccessedMemberTypes AllProperties =
            DynamicallyAccessedMemberTypes.PublicProperties | DynamicallyAccessedMemberTypes.NonPublicProperties;
        internal const DynamicallyAccessedMemberTypes AllMembers =
            AllConstructors | AllEvents | AllFields | AllMethods | AllNestedTypes | AllProperties;
        internal const DynamicallyAccessedMemberTypes DefaultMembers =
            DynamicallyAccessedMemberTypes.PublicConstructors | DynamicallyAccessedMemberTypes.PublicMethods |
            DynamicallyAccessedMemberTypes.PublicFields | DynamicallyAccessedMemberTypes.PublicNestedTypes |
            DynamicallyAccessedMemberTypes.PublicProperties | DynamicallyAccessedMemberTypes.PublicEvents;
        internal const DynamicallyAccessedMemberTypes InvokeMembers =
            AllConstructors | AllMethods | AllFields | AllProperties;

        // The same messages as the ones on the members of Type
        internal const string MakeGenericTypeRequiresUnreferencedCodeMessage = "If some of the generic arguments are annotated (either with DynamicallyAccessedMembersAttribute, or generic constraints), trimming can't validate that the requirements of those annotations are met.";
        internal const string MakeGenericTypeRequiresDynamicCodeMessage = "The native code for this instantiation might not be available at runtime.";
        internal const string MakeArrayTypeRequiresDynamicCodeMessage = "The code for an array of the specified type might not be available.";

        // The message of the exceptions thrown by the implementations of Type that must be overridden
        internal const string SubclassOverrideMessage = "Derived classes must provide an implementation.";

        #region Constructors

        [DynamicallyAccessedMembers(AllConstructors)]
        public sealed override ConstructorInfo[] GetConstructors(BindingFlags bindingAttr)
        {
            return GetConstructorsCore(bindingAttr);
        }

        internal abstract ConstructorInfo[] GetConstructorsCore(BindingFlags bindingAttr);

        [DynamicallyAccessedMembers(AllConstructors)]
        protected sealed override ConstructorInfo GetConstructorImpl(BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
        {
            return GetConstructorImplCore(bindingAttr, binder, callConvention, types, modifiers);
        }

        internal abstract ConstructorInfo GetConstructorImplCore(BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers);

        #endregion

        #region Methods

        [DynamicallyAccessedMembers(AllMethods)]
        public sealed override MethodInfo[] GetMethods(BindingFlags bindingAttr)
        {
            return GetMethodsCore(bindingAttr);
        }

        internal abstract MethodInfo[] GetMethodsCore(BindingFlags bindingAttr);

        [DynamicallyAccessedMembers(AllMethods)]
        protected sealed override MethodInfo GetMethodImpl(string name, BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
        {
            return GetMethodImplCore(name, bindingAttr, binder, callConvention, types, modifiers);
        }

        internal abstract MethodInfo GetMethodImplCore(string name, BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers);

        #endregion

        #region Properties

        [DynamicallyAccessedMembers(AllProperties)]
        public sealed override PropertyInfo[] GetProperties(BindingFlags bindingAttr)
        {
            return GetPropertiesCore(bindingAttr);
        }

        internal abstract PropertyInfo[] GetPropertiesCore(BindingFlags bindingAttr);

        [DynamicallyAccessedMembers(AllProperties)]
        protected sealed override PropertyInfo GetPropertyImpl(string name, BindingFlags bindingAttr, Binder binder, Type returnType, Type[] types, ParameterModifier[] modifiers)
        {
            return GetPropertyImplCore(name, bindingAttr, binder, returnType, types, modifiers);
        }

        internal abstract PropertyInfo GetPropertyImplCore(string name, BindingFlags bindingAttr, Binder binder, Type returnType, Type[] types, ParameterModifier[] modifiers);

        #endregion

        #region Fields

        [DynamicallyAccessedMembers(AllFields)]
        public sealed override FieldInfo[] GetFields(BindingFlags bindingAttr)
        {
            return GetFieldsCore(bindingAttr);
        }

        internal abstract FieldInfo[] GetFieldsCore(BindingFlags bindingAttr);

        [DynamicallyAccessedMembers(AllFields)]
        public sealed override FieldInfo GetField(string name, BindingFlags bindingAttr)
        {
            return GetFieldCore(name, bindingAttr);
        }

        internal abstract FieldInfo GetFieldCore(string name, BindingFlags bindingAttr);

        #endregion

        #region Events

        [DynamicallyAccessedMembers(AllEvents)]
        public sealed override EventInfo[] GetEvents(BindingFlags bindingAttr)
        {
            return GetEventsCore(bindingAttr);
        }

        internal abstract EventInfo[] GetEventsCore(BindingFlags bindingAttr);

        [DynamicallyAccessedMembers(AllEvents)]
        public sealed override EventInfo GetEvent(string name, BindingFlags bindingAttr)
        {
            return GetEventCore(name, bindingAttr);
        }

        internal abstract EventInfo GetEventCore(string name, BindingFlags bindingAttr);

        #endregion

        #region Nested types

        [DynamicallyAccessedMembers(AllNestedTypes)]
        public sealed override Type[] GetNestedTypes(BindingFlags bindingAttr)
        {
            return GetNestedTypesCore(bindingAttr);
        }

        internal abstract Type[] GetNestedTypesCore(BindingFlags bindingAttr);

        [DynamicallyAccessedMembers(AllNestedTypes)]
        public sealed override Type GetNestedType(string name, BindingFlags bindingAttr)
        {
            return GetNestedTypeCore(name, bindingAttr);
        }

        internal abstract Type GetNestedTypeCore(string name, BindingFlags bindingAttr);

        #endregion

        #region Interfaces

        [DynamicallyAccessedMembers(DynamicallyAccessedMemberTypes.Interfaces)]
        public sealed override Type[] GetInterfaces()
        {
            return GetInterfacesCore();
        }

        internal abstract Type[] GetInterfacesCore();

        [DynamicallyAccessedMembers(DynamicallyAccessedMemberTypes.Interfaces)]
        [return: DynamicallyAccessedMembers(DynamicallyAccessedMemberTypes.Interfaces)]
        public sealed override Type GetInterface(string name, bool ignoreCase)
        {
            // The trim analyzer can't verify that the interface returned by GetInterfaceCore (an LMR type, whose "members" are
            // metadata rows, so the annotation is meaningless for it) satisfies the annotation of the return value of
            // Type.GetInterface: interfaces come from arrays or from metadata, which carry no annotation (dotnet/runtime has
            // the same limitation in TypeBuilderImpl.GetInterface). So in the Native AOT build, nothing returns through this
            // annotated override: the compiler calls GetInterfaceCore, through LmrReflection.GetInterfaceLmr. Nothing in the
            // native binary calls Type.GetInterface(string, bool) at all: ILC doesn't even compile any implementation of it
            // (including this one and RuntimeType's), as its map file shows. Should that ever change, this throws instead of
            // returning a value that doesn't satisfy the annotation.
            throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture,
                "Type.GetInterface isn't supported on '{0}' in the Native AOT build of the XAML compiler: use LmrReflection.GetInterfaceLmr instead.",
                this));
        }

        internal abstract Type GetInterfaceCore(string name, bool ignoreCase);

        #endregion

        #region Members

        [DynamicallyAccessedMembers(AllMembers)]
        public sealed override MemberInfo[] GetMembers(BindingFlags bindingAttr)
        {
            return GetMembersCore(bindingAttr);
        }

        internal abstract MemberInfo[] GetMembersCore(BindingFlags bindingAttr);

        [DynamicallyAccessedMembers(AllMembers)]
        public sealed override MemberInfo[] GetMember(string name, MemberTypes type, BindingFlags bindingAttr)
        {
            return GetMemberCore(name, type, bindingAttr);
        }

        /// <summary>
        /// Same as the implementation of <see cref="Type.GetMember(string, MemberTypes, BindingFlags)"/>, for the LMR types
        /// that don't override it.
        /// </summary>
        internal virtual MemberInfo[] GetMemberCore(string name, MemberTypes type, BindingFlags bindingAttr)
        {
            throw new NotSupportedException(SubclassOverrideMessage);
        }

        [DynamicallyAccessedMembers(DefaultMembers)]
        public sealed override MemberInfo[] GetDefaultMembers()
        {
            return GetDefaultMembersCore();
        }

        /// <summary>
        /// Same as the implementation of <see cref="Type.GetDefaultMembers"/>, for the LMR types that don't override it.
        /// </summary>
        internal virtual MemberInfo[] GetDefaultMembersCore()
        {
            throw new NotImplementedException();
        }

        [DynamicallyAccessedMembers(InvokeMembers)]
        public sealed override object InvokeMember(string name, BindingFlags invokeAttr, Binder binder, object target, object[] args, ParameterModifier[] modifiers, CultureInfo culture, string[] namedParameters)
        {
            return InvokeMemberCore(name, invokeAttr, binder, target, args, modifiers, culture, namedParameters);
        }

        internal abstract object InvokeMemberCore(string name, BindingFlags invokeAttr, Binder binder, object target, object[] args, ParameterModifier[] modifiers, CultureInfo culture, string[] namedParameters);

        #endregion

        #region Type algebra

        // These create LMR types (e.g. a generic instantiation over other LMR types), never runtime types, so no code is generated.

        [RequiresDynamicCode(MakeGenericTypeRequiresDynamicCodeMessage)]
        [RequiresUnreferencedCode(MakeGenericTypeRequiresUnreferencedCodeMessage)]
        public sealed override Type MakeGenericType(params Type[] typeArguments)
        {
            return MakeGenericTypeCore(typeArguments);
        }

        /// <summary>
        /// Same as the implementation of <see cref="Type.MakeGenericType"/>, for the LMR types that don't override it.
        /// </summary>
        internal virtual Type MakeGenericTypeCore(Type[] typeArguments)
        {
            throw new NotSupportedException(SubclassOverrideMessage);
        }

        [RequiresDynamicCode(MakeArrayTypeRequiresDynamicCodeMessage)]
        public sealed override Type MakeArrayType()
        {
            return MakeArrayTypeCore();
        }

        /// <summary>
        /// Same as the implementation of <see cref="Type.MakeArrayType()"/>, for the LMR types that don't override it.
        /// </summary>
        internal virtual Type MakeArrayTypeCore()
        {
            throw new NotSupportedException();
        }

        [RequiresDynamicCode(MakeArrayTypeRequiresDynamicCodeMessage)]
        public sealed override Type MakeArrayType(int rank)
        {
            return MakeArrayTypeCore(rank);
        }

        /// <summary>
        /// Same as the implementation of <see cref="Type.MakeArrayType(int)"/>, for the LMR types that don't override it.
        /// </summary>
        internal virtual Type MakeArrayTypeCore(int rank)
        {
            throw new NotSupportedException();
        }

        #endregion
    }
}

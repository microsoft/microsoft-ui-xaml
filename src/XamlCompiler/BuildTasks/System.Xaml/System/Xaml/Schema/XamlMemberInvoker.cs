// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Collections;
using System.Diagnostics.CodeAnalysis;
using System.Reflection;
using System.Security;
using System.Security.Permissions;
using System.Xaml;
using System.Xaml.MS.Impl;

namespace System.Xaml.Schema
{
    /// <SecurityNote>
    /// This class makes the assumption that any internal ShouldSerialize methods in System.Xaml are safe
    /// for public invocation. If this becomes untrue, then ShouldSerializeValue needs an IsSystemXamlNonPublic
    /// check, just like GetValue and SetValue have.
    /// </SecurityNote>
    public class XamlMemberInvoker
    {
        private static XamlMemberInvoker s_Directive;
        private static XamlMemberInvoker s_Unknown;
        private static object[] s_emptyObjectArray = new object[0];

        private XamlMember _member;

        protected XamlMemberInvoker()
        {
        }

        public XamlMemberInvoker(XamlMember member)
        {
            if (member == null)
            {
                throw new ArgumentNullException("member");
            }
            _member = member;
        }

        public static XamlMemberInvoker UnknownInvoker
        {
            get
            {
                if (s_Unknown == null)
                {
                    s_Unknown = new XamlMemberInvoker();
                }
                return s_Unknown;
            }
        }

        public MethodInfo UnderlyingGetter
        {
            get { return IsUnknown ? null : _member.Getter; }
        }

        public MethodInfo UnderlyingSetter
        {
            get { return IsUnknown ? null : _member.Setter; }
        }

        public virtual object GetValue(object instance)
        {
            if (instance == null)
            {
                throw new ArgumentNullException("instance");
            }
            ThrowIfUnknown();
            if (UnderlyingGetter == null)
            {
                throw new NotSupportedException(SR.Get(SRID.CantGetWriteonlyProperty, _member));
            }
            return GetValueSafeCritical(instance);
        }

        /// <SecurityNote>
        /// Critical: See explanation in SafeReflectionInvoker
        /// Safe: See explanation in SafeReflectionInvoker.
        /// </SecurityNote>
        [SecuritySafeCritical]
        private object GetValueSafeCritical(object instance)
        {
            if (UnderlyingGetter.IsStatic)
            {
                return SafeReflectionInvoker.InvokeMethod(UnderlyingGetter, null, new object[] { instance });
            }
            else
            {
                return SafeReflectionInvoker.InvokeMethod(UnderlyingGetter, instance, s_emptyObjectArray);
            }
        }

        public virtual void SetValue(object instance, object value)
        {
            if (instance == null)
            {
                throw new ArgumentNullException("instance");
            }
            ThrowIfUnknown();
            if (UnderlyingSetter == null)
            {
                throw new NotSupportedException(SR.Get(SRID.CantSetReadonlyProperty, _member));
            }
            SetValueSafeCritical(instance, value);
        }

        /// <SecurityNote>
        /// Critical: See explanation in SafeReflectionInvoker
        /// Safe: See explanation in SafeReflectionInvoker.
        /// </SecurityNote>
        [SecuritySafeCritical]
        private void SetValueSafeCritical(object instance, object value)
        {
            if (UnderlyingSetter.IsStatic)
            {
                SafeReflectionInvoker.InvokeMethod(UnderlyingSetter, null, new object[] { instance, value });
            }
            else
            {
                SafeReflectionInvoker.InvokeMethod(UnderlyingSetter, instance, new object[] { value });
            }
        }

        internal static XamlMemberInvoker DirectiveInvoker
        {
            get
            {
                if (s_Directive == null)
                {
                    s_Directive = new DirectiveMemberInvoker();
                }
                return s_Directive;
            }
        }

        // Returns true/false if ShouldSerialize method was invoked, null if no method was found
        public virtual ShouldSerializeResult ShouldSerializeValue(object instance)
        {
            // The XAML compiler never writes or reads object graphs (XamlObjectWriter/XamlObjectReader), and this needs runtime reflection.
            throw Microsoft.UI.Xaml.Markup.Compiler.NativeAotUnsupported.Create("XamlMemberInvoker.ShouldSerializeValue");
        }

        // vvvvv---- Unused members.  Servicing policy is to retain these anyway.  -----vvvvv
        /// <SecurityNote>
        /// Critical: Sets critical fields _getterIsSystemXamlNonPublic and _setterIsSystemXamlNonPublic
        /// Safe: Gets the result from SafeCritical method SafeReflectionInvoker.IsSystemXamlNonPublic.
        ///       The MethodInfo we're checking is exactly the one we're invoking, so even if the
        ///       MethodInfo lies about its visibility, there is no harm.
        /// Note: The [SecurityCritical] attribute isn't functionally necessary but flags the
        ///       method as security critical and changes should be reviewed.
        /// </SecurityNote>
        [SecuritySafeCritical]
        [SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Justification = "Retained per servicing policy.")]
        private static bool IsSystemXamlNonPublic(
            ref ThreeValuedBool methodIsSystemXamlNonPublic, MethodInfo method)
        {
            if (methodIsSystemXamlNonPublic == ThreeValuedBool.NotSet)
            {
                bool result = SafeReflectionInvoker.IsSystemXamlNonPublic(method);
                methodIsSystemXamlNonPublic = result ? ThreeValuedBool.True : ThreeValuedBool.False;
            }
            return methodIsSystemXamlNonPublic == ThreeValuedBool.True;
        }
        // ^^^^^----- End of unused members.  -----^^^^^

        private bool IsUnknown
        {
            get { return _member == null || _member.UnderlyingMember == null; }
        }

        private void ThrowIfUnknown()
        {
            if (IsUnknown)
            {
                throw new NotSupportedException(SR.Get(SRID.NotSupportedOnUnknownMember));
            }
        }

        private class DirectiveMemberInvoker : XamlMemberInvoker
        {
            public override object GetValue(object instance)
            {
                throw new NotSupportedException(SR.Get(SRID.NotSupportedOnDirective));
            }

            public override void SetValue(object instance, object value)
            {
                throw new NotSupportedException(SR.Get(SRID.NotSupportedOnDirective));
            }
        }
    }

    public enum ShouldSerializeResult
    {
        Default,
        True,
        False
    }
}

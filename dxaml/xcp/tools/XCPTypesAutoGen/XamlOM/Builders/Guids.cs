// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using XamlOM.NewBuilders;

namespace XamlOM
{
    /// <summary>
    /// Specifies the ClassGuid and other identifying parts of the type.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Interface | AttributeTargets.Delegate, AllowMultiple = true)]
    public class GuidsAttribute : Attribute, NewBuilders.ITypeBuilder, NewBuilders.IClassBuilder
    {
        public int Version
        {
            get;
            set;
        }

        // <summary>
        /// [Optional] Specifies the internal ClassGuid that is generated for this class. This Guid isn't public and is not required.
        /// </summary>
        public string ClassGuid
        {
            get;
            set;
        }

        /// <summary>
        /// [Optional] Specifies whether the interface for the type should be forwarded. The default for interfaces/types defined in RS5+
        /// is true.
        /// </summary>
        protected bool IsForwarded
        {
            get;
            set;
        }

        public bool ForwardEventArgs
        {
            get;
            set;
        }
        public GuidsAttribute()
        {
            Version = 1;
            IsForwarded = false;
            ForwardEventArgs = false;
        }

        bool ShouldInterfaceForwardByDefault(int version, Type source)
        {
            bool shouldForward = false;
            // We need to use good ole fashion C# reflection to correctly correlate this guid attribute, to any platform attribute
            // applied on the object. We can't be guaranteed at what order the attributes are applied on the model, so it's possible that 
            // they haven't been applied yet. However, they will still be on the actual C# model object (pointed to by 'source' parameter passed in)
            PlatformAttribute[] platforms = source.GetCustomAttributes(typeof(PlatformAttribute), false) as PlatformAttribute[];
            var thisSupportedPlatform = platforms.SingleOrDefault(p => p.Version == version);
            if (thisSupportedPlatform != null)
            {
                var contractRef = XamlOM.NewBuilders.ModelFactory.CreateContractReference(thisSupportedPlatform.ContractType, thisSupportedPlatform.ContractVersion);
                shouldForward = contractRef.SupportsModernIdl && !contractRef.IsPrivateApiContract;
            }

            return shouldForward;
        }
        public void BuildNewType(OM.TypeDefinition definition, Type source)
        {
            // For RS5+ interface forwarding is default
            if (ShouldInterfaceForwardByDefault(Version, source))
            {
                IsForwarded = true;
            }

            if (ClassGuid != null)
            {
                if (Version == 1)
                {
                    definition.Guids.ClassGuid = Guid.Parse(ClassGuid);
                }
                else
                {
                    throw new Exception($"Please remove the ClassGuid on version {Version} of {source.FullName}. Only the Version 1 ClassGuid is ever used.");
                }
            }
        }

        void NewBuilders.IClassBuilder.BuildNewClass(ClassDefinition definition, Type source)
        {
            // Event args classes don't support forwarding
            if (definition.IsAEventArgs && !ForwardEventArgs) return;

            if (definition.InterfaceForwardedVersions.ContainsKey(Version))
            {
                if (definition.InterfaceForwardedVersions[Version] != IsForwarded)
                {
                    throw new InvalidOperationException(string.Format("Type {0} already defines interface relationship", source.FullName));
                }
            }
            else
            {
                definition.InterfaceForwardedVersions.Add(Version, IsForwarded);
            }
        }
    }

    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Enum, Inherited = false, AllowMultiple = true)]
    public class VelocityAttribute : InterfaceDetailsAttribute
    {
        private string _feature;

        public VelocityAttribute()
        {
            // EventArgs templates don't support the forwarding semantics.  If you use an InterfaceDetailsAttribute
            // (as opposed to Guids) on them, you get into a weird state where you don't get the methods defined
            // with IFACEMETHOD/override (you instead get _check_return_ HRESULT STDMETHODCALLTYPE with no override).
            // This causes a problem with the VelocityFeatures because we need to control whether override is 
            // specified or not.  In additions, there are a few classes that have used InterfaceDetails with
            // EventArgs classes and so they are already in this odd state.  Don't want to mess with those, so
            // we will use a property on InterfaceDetailsAttribute to indicated that for Velocity that are
            // used on EventArgs, we don't want them forarded.
            ForwardEventArgs = false;
        }

        public new int Version
        {
            get { return base.Version; }
            set { throw new InvalidOperationException("Version may not be specified on Velocity attrubute - use Feature"); }
        }

        public string Feature
        {
            get { return _feature; }
            set
            {
                _feature = value;
                base.Version = VelocityFeatures.GetVersion(value);
            }
        }
    }

    /// <summary>
    /// Specifies the version of the interface should be forwarded. This is only required for compat reasons, and
    /// is unnecessary for types defined in RS5+.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, Inherited = false, AllowMultiple = true)]
    public class InterfaceDetailsAttribute : GuidsAttribute
    {

        public InterfaceDetailsAttribute()
        {
            IsForwarded = true;
            ForwardEventArgs = true;
        }
    }
}

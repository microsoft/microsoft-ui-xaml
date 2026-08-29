// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;

namespace OM
{
    public abstract class MemberDefinition : VersionedElement
    {
        private List<DeprecationDefinition> _deprecations = new List<DeprecationDefinition>();

        public bool AllowCrossThreadAccess
        {
            get;
            set;
        }

        public string Comment
        {
            get;
            set;
        }

        public string VelocityFeatureName
        {
            get;
            set;
        }

        public ClassDefinition DeclaringClass
        {
            get
            {
                return DeclaringType as ClassDefinition;
            }
        }

        public TypeDefinition DeclaringType
        {
            get;
            set;
        }

        public TypeReference DeclaringTypeReference
        {
            get
            {
                return new TypeReference(DeclaringType);
            }
        }

        public String DeclaringTypeFullInterfaceNameWithVersion
        {
            get
            {
                return String.Format("{0}.I{1}{2}", DeclaringType.DeclaringNamespace.Name, DeclaringType.Name, VersionString);
            }
        }

        public string DeclaringTypeVirtualMembersFullInterfaceNameWithVersion
        {
            get
            {
                return DeclaringClass.IdlClassInfo.FullVirtualMembersInterfaceName + VersionString;
            }
        }

        public ClassDefinition DeclaringVersion
        {
            get
            {
                return DeclaringClass.GetVersion(Version).GetProjection();
            }
        }

        public List<DeprecationDefinition> Deprecations
        {
            get
            {
                return _deprecations;
            }
        }

        /// <summary>
        /// Gets or sets whether to generate a default implementation.
        /// </summary>
        public bool GenerateDefaultBody
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets whether to generate a stub with parameter validation, thread checks, etc.
        /// </summary>
        public bool GenerateStub
        {
            get;
            set;
        }

        /// <summary>
        /// Gets whether this member has an "impl" method; a method which a developer implements to provide a custom 
        /// implementation for a generated member.
        /// </summary>
        public bool HasImplMethod
        {
            get
            {
                return GenerateStub && !GenerateDefaultBody && !IsAbstract;
            }
        }

        public string ImplName
        {
            get
            {
                if (IsVirtual)
                {
                    return IdlMemberInfo.VirtualName + "Impl";
                }
                return IdlMemberInfo.Name + "Impl";
            }
        }

        /// <summary>
        /// Gets or sets the interface member that the current member is implementing. Returns 
        /// null if this member isn't implementing an interface.
        /// </summary>
        public MemberDefinition InterfaceMember
        {
            get;
            set;
        }

        public abstract Idl.IdlMemberInfo IdlMemberInfo
        {
            get;
        }

        public bool IsAbstract
        {
            get;
            set;
        }

        public bool IsImplPureVirtual
        {
            get
            {
                return DeclaringClass.GeneratePartialClass && !IsStatic;
            }
        }

        public bool IsInterfaceImplementation
        {
            get
            {
                return !IdlMemberInfo.IsExcluded || InterfaceMember != null;
            }
        }

        public bool IsPureVirtual
        {
            get
            {
                if (!GenerateStub)
                {
                    return true;
                }

                // Default body for an abstract member would be no body. However, if we're not generating 
                // a default body, then we're calling out into a XImpl method, so by definition we're no 
                // longer pure virtual.
                if (!GenerateDefaultBody)
                {
                    return false;
                }

                if (!DeclaringClass.GeneratePartialClass && IsVirtual)
                {
                    return false;
                }

                return IsAbstract || !GenerateDefaultBody;
            }
        }

        public bool IsStatic
        {
            get;
            set;
        }

        public bool IsVirtual
        {
            get;
            set;
        }

        public Modifier Modifier
        {
            get;
            set;
        }

        public string Name
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets an order hint to help code-gen generate this member in the right location.
        /// </summary>
        public int? OrderHint
        {
            get;
            set;
        }

        public int PhoneMemberTableIndex
        {
            get;
            set;
        }

        public Strictness? Strictness
        {
            get;
            set;
        }

        public bool EmitStrictCheck
        {
            get;
            set;
        }

        public string VersionString
        {
            get
            {
                // Returns the version as it should be used in interface names and such.  For Velocity versions this
                // is the Velocity feature name and for everything else it is the actual Version (although null for
                // version 1).
                if (!string.IsNullOrEmpty(VelocityFeatureName)) return VelocityFeatureName;
                if (Version > 1) return Version.ToString();
                return "";
            }
        }

        public MemberDefinition()
        {
            GenerateDefaultBody = true;
            GenerateStub = true;
            Modifier = OM.Modifier.Public;
            PhoneMemberTableIndex = -1;
            Version = 1;
            GetterVersion = 1;
            SetterVersion = 1;
            VelocityFeatureName = string.Empty;
            EmitStrictCheck = true;
        }

        public override string ToString()
        {
            return base.ToString() + " - " + Name;
        }

        public bool SupportsV2CodeGen
        {
            get
            {
                if (SupportedContracts.Any())
                {
                    return SupportedContracts.SupportsV2CodeGen();
                }
                else
                {
                    return DeclaringType.SupportsV2CodeGen;
                }
            }
        }
    }
}

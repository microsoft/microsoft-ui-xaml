// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using OM;

namespace XamlOM
{
    /// <summary>
    /// Specifies an interface this type implements. For IVector or IMap, this will also implement IIterable for you.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Interface, AllowMultiple = true, Inherited = false)]
    public class ImplementsAttribute : Attribute, NewBuilders.IEndClassBuilder, NewBuilders.IOrderedBuilder
    {
        public Type InterfaceType
        {
            get;
            private set;
        }

        public int Order
        {
            get;
            set;
        }

        // Indicates when an interface implementation was introduced, when just the implementation is new.
        // E.g. a runtime class and interface are defined in Win8, but the runtinme class doesn't implement the interface.
        // Then in WinBlue, the runtime class does implement the interface.
        public int Version
        {
            get;
            set;
        }

        public string Velocity
        {
            get;
            set;
        }
        public bool IsStaticInterface
        {
            get;
            set;
        }

        public ImplementsAttribute(Type interfaceType)
        {
            InterfaceType = interfaceType;
            Version = 1;
        }

        public ImplementsAttribute(Type interfaceType, int version) : this(interfaceType)
        {
            Version = version;
        }

        public ImplementsAttribute(Type interfaceType, string velocity) : this(interfaceType)
        {
            Velocity = velocity;
        }


        public void BuildEndClass(OM.ClassDefinition definition, Type source)
        {
            int version = Version;
            if (!string.IsNullOrEmpty(Velocity))
            {
                version = VelocityFeatures.GetVersion(Velocity);
            }
            if (IsStaticInterface)
            {
                NewBuilders.Helper.ImplementStaticInterface(definition, InterfaceType, version);
            }
            else
            {
                NewBuilders.Helper.ImplementInterface(definition, InterfaceType, version);
            }
        }
    }
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Reflection;

namespace OM
{
    /// <summary>
    /// DEPRECATED: In Modern IDL, we don't need to supply GUIDS. This class is kept for our existing Guids.
    /// </summary>
    public class Guids
    {
        public static Guids CreateFromType(TypeDefinition def)
        {
            foreach (PropertyInfo property in typeof(NonIdlGuids).GetProperties())
            {
                if (def.Name == property.Name)
                {
                    return (Guids)property.GetValue(null, null);
                }
            }

            foreach (PropertyInfo property in typeof(EmptyInterfaceGuids).GetProperties())
            {
                if (def.Name == property.Name)
                {
                    return (Guids)property.GetValue(null, null);
                }
            }

            foreach (PropertyInfo property in typeof(ProjectionCompatGuids).GetProperties())
            {
                if (def.Name == property.Name)
                {
                    return (Guids)property.GetValue(null, null);
                }
            }

            return new Guids();
        }

        public Dictionary<int, Guid> InterfaceGuids
        {
            get;
            private set;
        }

        public Guid ClassGuid
        {
            get;
            set;
        }

        public Dictionary<int, Guid> OverrideGuids
        {
            get;
            private set;
        }

        public Dictionary<int, Guid> ProtectedGuids
        {
            get;
            private set;
        }

        public Dictionary<int, Guid> StaticsGuids
        {
            get;
            private set;
        }

        public Dictionary<int, Guid> FactoryGuids
        {
            get;
            private set;
        }

        public Dictionary<int, Guid> IReferenceGuids
        {
            get;
            private set;
        }

        public Guids()
        {
            InterfaceGuids = new Dictionary<int, Guid>();
            OverrideGuids = new Dictionary<int, Guid>();
            ProtectedGuids = new Dictionary<int, Guid>();
            StaticsGuids = new Dictionary<int, Guid>();
            FactoryGuids = new Dictionary<int, Guid>();
            IReferenceGuids = new Dictionary<int, Guid>();
        }

    }

    internal class InterfaceGuids : Guids
    {
        internal InterfaceGuids(string guid)
        {
            InterfaceGuids.Add(1, Guid.Parse(guid));
        }
    }

    internal class ClassGuids : InterfaceGuids
    {
        internal ClassGuids(string interfaceGuid, string factoryGuid) : base(interfaceGuid)
        {
            FactoryGuids.Add(1, Guid.Parse(factoryGuid));
        }
    }

    // These GUIDS are all internal and not part of IDL. This is a pattern that should
    // not be supported by code-gen in the future. Private interfaces should either
    // go into a private idl file or done by hand.
    public static class NonIdlGuids
    {
        public static Guids ISupportInitialize => new InterfaceGuids("e3baa6d9-a2ad-4bfa-a4ba-08477f98f5a4");

        public static Guids IDragOperationDeferralTarget => new InterfaceGuids("3fdacf14-c29f-4963-856e-ec390d1f1107");

        public static Guids IGeneratorHost => new InterfaceGuids("e1430e36-404a-4696-9f0f-7654bb708a1b");

        public static Guids IGroupHeaderMapping => new InterfaceGuids("785ff7cd-7787-45bd-bc64-5fa4b48d619e");

        public static Guids IItemContainerGenerator2 => new InterfaceGuids("b6076728-db1f-42ea-bb42-9b134dd59492");

        public static Guids IKeyboardNavigationPanel => new InterfaceGuids("cba39132-b6f1-4d2e-8669-4d2bb78bc8ac");

        public static Guids IKeyboardHeaderNavigationPanel => new InterfaceGuids("9e02e489-8ca7-4728-94c5-09c4d68d970e");

        public static Guids IItemLookupPanel => new InterfaceGuids("f50af115-dbfc-4d0a-8f03-9922510a21a8");

        public static Guids IOrientedPanel => new InterfaceGuids("e8b3f9b0-7efb-4e7a-9616-2b390cea2623");

        public static Guids ICustomGeneratorItemsHost => new InterfaceGuids("2741e724-81c2-4cae-8092-3b703aa1d93f");

        public static Guids IContainerContentChangingIterator => new InterfaceGuids("dbdc6853-84d3-48cf-a4ee-1831351344a7");

        public static Guids ITransitionContextProvider => new InterfaceGuids("fede5b34-83bd-4bd7-9930-02e6a9f48cff");

        public static Guids ITreeBuilder => new InterfaceGuids("a28ba838-4842-42bd-abdd-8d271c1b6d3e");

        public static Guids IXamlPredicate => new InterfaceGuids("9d571fda-ed2c-42c7-9f0f-e201180fbb96");

        public static Guids IPaginatedPanel => new InterfaceGuids("a0830eb1-e2d3-4438-ae98-419a85b74936");

        public static Guids IScrollInfo => new InterfaceGuids("7a0b0a1e-b4eb-4451-88ad-98721a8d5881");

        public static Guids IChildTransitionContextProvider => new InterfaceGuids("a50332aa-b8a8-4209-bb6e-55842a334c00");

        public static Guids IContainerRecyclingContext => new InterfaceGuids("2c7129ba-4044-40ed-bdbd-41513f58039c");
    }

    // The below empty marker interfaces need a GUID.
    // See https://docs.microsoft.com/en-us/uwp/midl-3/advanced#empty-interfaces for more info
    public static class EmptyInterfaceGuids
    {
        public static Guids IXamlDirectObject => new InterfaceGuids("f94d4ea9-e795-4425-a8c8-498990fce9c7");
    }

    // These GUIDS are needed for compat with the projection layers that hardcode our interfaces.
    public static class ProjectionCompatGuids
    {
        public static Guids IBindableIterable => new InterfaceGuids("036d2c08-df29-41af-8aa2-d774be62ba6f");

        public static Guids IBindableIterator => new InterfaceGuids("6a1d6c07-076d-49f2-8314-f52c9c9a8331");

        public static Guids IBindableObservableVector => new InterfaceGuids("fe1eb536-7e7f-4f90-ac9a-474984aae512");

        public static Guids IBindableVector => new InterfaceGuids("393de7de-6fd0-4c0d-bb71-47244a113e93");

        public static Guids IBindableVectorView => new InterfaceGuids("346dd6e7-976e-4bc3-815d-ece243bc0f33 ");

        public static Guids ICommand => new InterfaceGuids("e5af3542-ca67-4081-995b-709dd13792df");

        public static Guids ICustomProperty => new InterfaceGuids("30da92c0-23e8-42a0-ae7c-734a0e5d2782");

        public static Guids ICustomPropertyProvider => new InterfaceGuids("7c925755-3e48-42b4-8677-76372267033f");

        public static Guids BindableVectorChangedEventHandler => new InterfaceGuids("624cd4e1-d007-43b1-9c03-af4d3e6258c4");

        public static Guids NotifyCollectionChangedEventArgs => new ClassGuids("da049ff2-d2e0-5fe8-8c7b-f87f26060b6f", "5108eba4-4892-5a20-8374-a96815e0fd27");
    }
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CValue.h>
#include <TypeTableStructs.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { 
    namespace Metadata {

        class MockDependencyProperty : public CDependencyProperty
        {
        public:
            MockDependencyProperty()
                : CDependencyProperty(
                    KnownPropertyIndex::UnknownType_UnknownProperty,
                    KnownTypeIndex::UnknownType,
                    KnownTypeIndex::UnknownType,
                    KnownTypeIndex::UnknownType,
                    MetaDataPropertyInfoFlags::None)
                , offset(0)
                , groupCreator(nullptr)
                , groupOffset(0)
                , propertyMethod(nullptr)
                , renderChangedHandler(nullptr)
            {
            }

            MockDependencyProperty(_In_ KnownPropertyIndex index);

            void SetIndex(KnownPropertyIndex index);

            static bool IsMock(_In_ const CDependencyProperty* dp);

            std::function<HRESULT(CValue*)> createDefaultValue;
            UINT16 offset;
            CREATEGROUPPFN groupCreator;
            UINT16 groupOffset;
            METHODPFN propertyMethod;
            RENDERCHANGEDPFN renderChangedHandler;
        };

    }

} } } }

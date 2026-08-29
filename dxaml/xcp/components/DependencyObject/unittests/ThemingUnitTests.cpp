// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ThemingUnitTests.h"

#include <MetadataAPI.h>
#include <TypeTableStructs.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <CDependencyObject.h>
#include <Type.h>
#include <primitives.h>
#include <ModifiedValue.h>
#include <MockDependencyProperty.h>
#include <DependencyObjectMocks.h>
#include "theming\inc\Theme.h"
using namespace DirectUI;
using namespace Microsoft::UI::Xaml::Tests::Metadata;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace DependencyObject {

    void ThemingUnitTests::DoesNotifyThemeChangeForObjectSetOnSparseProperty()
    {
        CValue value;
        xref_ptr<MockDependencyObjectWithObjectProperty> obj;
        obj.attach(new MockDependencyObjectWithObjectProperty());

        xref_ptr<MockDependencyObjectTrackingThemeChanged> mock;
        mock.attach(new MockDependencyObjectTrackingThemeChanged());

        MockDependencyProperty dp;
        dp.SetFlags(MetaDataPropertyInfoFlags::IsSparse);
        dp.SetPropertyTypeIndex(KnownTypeIndex::DependencyObject);

        value.WrapObjectNoRef(mock.get());
        VERIFY_SUCCEEDED(obj->SetValue(&dp, value));

        VERIFY_SUCCEEDED(obj->NotifyThemeChanged(Theming::Theme::Light));
        VERIFY_IS_TRUE(mock->hasThemeChanged);
    }

} } } } } }

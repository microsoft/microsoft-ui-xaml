// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "MarkupExtensionTests.CustomMarkupExtension.h"
#include "MarkupExtensionTests.CustomButton.h"

using namespace Microsoft::UI::Xaml;

namespace Tests { namespace Native { namespace External { namespace Framework {
    namespace MarkupExtensions
    {
        DependencyProperty^ CustomButton::s_customPropertyProperty = nullptr;
        
        void CustomButton::RegisterDependencyProperties()
        {
            if (!s_customPropertyProperty)
            {
                s_customPropertyProperty = DependencyProperty::Register(
                    L"CustomProperty",
                    PewPewExtension::typeid,
                    CustomButton::typeid,
                    nullptr);
            }
        }

        void CustomButton::ClearDependencyProperties()
        {
            // The property itself will have already been cleared out of the metadata table,
            // however we also need to remove the reference from this static variable to
            // completely deallocate it.
            s_customPropertyProperty = nullptr;
        }
    }
} } } }
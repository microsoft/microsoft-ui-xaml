// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Tests { namespace External { namespace Controls { namespace CustomTypes {

    // Custom types MUST be sealed and have a public constructor
    // if you'd like them to be activatable.
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class CustomAppBarButton sealed 
        : public Microsoft::UI::Xaml::Controls::Button, public Microsoft::UI::Xaml::Controls::ICommandBarElement
    {
    public:
        CustomAppBarButton();

        property bool IsCompact {
            virtual bool get()
            {
                return m_isCompact;
            }
            virtual void set(bool value)
            {
                m_isCompact = value;
                return;
            }
        }

        property int DynamicOverflowOrder {
            virtual int get()
            {
                return m_dynamicOverflowOrder;
            }
            virtual void set(int value)
            {
                m_dynamicOverflowOrder = value;
                return;
            }
        }

        property bool IsInOverflow 
        {
            virtual bool get()
            {
                return m_IsInOverflow;
            }
        }

    private:
        bool m_isCompact;
        int m_dynamicOverflowOrder;
        bool m_IsInOverflow;
    };
}}}}
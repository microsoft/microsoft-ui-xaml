// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Tests { namespace External { namespace Controls { namespace CustomTypes {
    // Custom types MUST be sealed and have a public constructor
    // if you'd like them to be activatable. 
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class CustomListViewItemPresenter sealed
        : public Microsoft::UI::Xaml::Controls::ContentPresenter
    {
    public:
        void CustomListViewPresenter();
        static bool Get_PlaceholderStateReached();
        static void Set_PlaceholderStateReached(bool value);

    protected:
        bool GoToElementStateCore(Platform::String^ stateName, bool useTransitions) override;

    private:
        static bool m_PlaceHolderStateReached;
    };
}}}}
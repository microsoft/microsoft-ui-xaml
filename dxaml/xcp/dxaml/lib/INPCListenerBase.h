// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines the base class for the property access classes that 
//      will listen for INPC changes

#pragma once

namespace DirectUI
{
    class INPCListenerBase
    {
    protected:

        INPCListenerBase();
        ~INPCListenerBase();

        _Check_return_ HRESULT AddPropertyChangedHandler(_In_ IInspectable *pSource);
        _Check_return_ HRESULT UpdatePropertyChangedHandler(_In_opt_ IInspectable *pOldSource, _In_opt_ IInspectable *pNewSource);
        _Check_return_ HRESULT DisconnectPropertyChangedHandler(_In_ IInspectable *pSource);

        virtual _Check_return_ HRESULT OnPropertyChanged() = 0;
        virtual _Ret_notnull_ const wchar_t* GetPropertyName() = 0;

    private:

        _Check_return_ HRESULT OnPropertyChangedCallback(_In_ xaml_data::IPropertyChangedEventArgs *pArgs);

    private:    

        ctl::EventPtr<PropertyChangedEventCallback> m_epPropertyChangedHandler;
        UINT32 m_propertyNameLength;
    };
        

}

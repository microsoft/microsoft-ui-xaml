// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef ____x_ABI_CWindows_CFoundation_CIClosable_INTERFACE_DEFINED__
#define ____x_ABI_CWindows_CFoundation_CIClosable_INTERFACE_DEFINED__

// Privately declare the IClosable interface from windows.foundation.h
// Since this module was compiled with CX, the windows.foundation.h cannot be included without a lot of pain points.
namespace ABI {
    namespace Windows {
        namespace Foundation {

            MIDL_INTERFACE("30D5A829-7FA4-4026-83BB-D75BAE4EA99E")
            IClosable : public IInspectable
            {
            public:
                virtual HRESULT STDMETHODCALLTYPE Close(void) = 0;

            };

            extern const __declspec(selectany) IID & IID_IClosable = __uuidof(IClosable);


        }  /* end namespace */
    }  /* end namespace */
}  /* end namespace */

#endif // ____x_ABI_CWindows_CFoundation_CIClosable_INTERFACE_DEFINED__

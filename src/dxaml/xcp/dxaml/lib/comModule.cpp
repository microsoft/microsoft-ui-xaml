// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

using namespace DirectUI;
using namespace ctl;

// Do reverse string comparison.
bool wcsequals_reverse(const wchar_t* left, const wchar_t* right, size_t length)
{
    wchar_t* leftCursor = const_cast<wchar_t*>(left) + length;
    wchar_t* rightCursor = const_cast<wchar_t*>(right) + length;
    while (leftCursor > left && *leftCursor-- == *rightCursor--);
    return (leftCursor == left && *leftCursor == *rightCursor);
}

IActivationFactory* module::GetActivationFactory(HSTRING hClassID)
{
    IActivationFactory *pFound = NULL;
    XUINT32 nClassID = 0;
    LPCWSTR szClassID = NULL;

    szClassID = WindowsGetStringRawBuffer(hClassID, &nClassID);

    for (XUINT32 i = 0; i < __creationMapCount; i++)
    {
        const activation_factory_entry *pEntry = &__creationMap[i];

        if ((pEntry->m_strClassIDStorage.Count == nClassID) && wcsequals_reverse(pEntry->m_strClassIDStorage.Buffer, szClassID, nClassID))
        {
            pFound = __factories[i];

            if (pFound == NULL)
            {
                __factories[i] = pEntry->m_pfnCreator();
                pFound = __factories[i];
            }

            break;
        }
    }

    if (pFound)
    {
        pFound->AddRef();
    }

    return pFound;
}

void module::ClearFactoryCache()
{
    for (XUINT32 i = 0; i < __creationMapCount; i++)
    {
        if (__factories[i] != NULL)
        {
            __factories[i]->Release();
            __factories[i] = NULL;
        }
    }
}

void module::ResetFactories()
{
    for (XUINT32 i = 0; i < __creationMapCount; i++)
    {
        if (__factories[i] != NULL)
        {
            ctl::ComPtr<wf::IClosable> closableFactory;
            if (SUCCEEDED(__factories[i]->QueryInterface<wf::IClosable>(&closableFactory)))
            {
                VERIFYHR(closableFactory->Close());
            }
        }
    }
}


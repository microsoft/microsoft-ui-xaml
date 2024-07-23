// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <dopointercast.h>
#include <corep.h>
#include "ValidationErrorsCollection.h"
#include "ManagedObjectReference.h"
#include "DXamlServices.h"
#include "CValueBoxer.h"
#include "CControl.h"

using namespace DirectUI;

KnownTypeIndex CValidationErrorsCollection::GetTypeIndex() const
{ 
    return DependencyObjectTraits<CValidationErrorsCollection>::Index;
}

_Check_return_ HRESULT CValidationErrorsCollection::ValidateItem(_In_ CDependencyObject *object)
{
    wrl::ComPtr<xaml_controls::IInputValidationError> error;
    if (!TryGetValidationError(object, &error))
    {
        // We should never get here. API projections would prevent anything else from being added
        XAML_FAIL_FAST();
    }

    return S_OK;
}

_Check_return_ HRESULT CValidationErrorsCollection::OnAddToCollection(_In_ CDependencyObject *obj)
{
    wrl::ComPtr<xaml_controls::IInputValidationError> error;
    if (TryGetValidationError(obj, &error))
    {
        if (auto owner = do_pointer_cast<CControl>(GetOwner()))
        {
            owner->RaiseValidationErrorEvent(DirectUI::InputValidationErrorEventAction::Added, error.Get());
        }
    }
    return S_OK;
}
_Check_return_ HRESULT CValidationErrorsCollection::OnRemoveFromCollection(_In_ CDependencyObject *obj, _In_ int32_t previousIndex)
{
    wrl::ComPtr<xaml_controls::IInputValidationError> error;
    if (TryGetValidationError(obj, &error))
    {
        if (auto owner = do_pointer_cast<CControl>(GetOwner()))
        {
            owner->RaiseValidationErrorEvent(DirectUI::InputValidationErrorEventAction::Removed, error.Get());
        }
    }
    return S_OK;
}
_Check_return_ HRESULT CValidationErrorsCollection::Neat(bool)
{
    // Use remove so that we will fire notifications
    for (const auto& item : *this)
    {
        remove(item);
    }

    return S_OK;
}

bool CValidationErrorsCollection::TryGetValidationError(_In_ CDependencyObject* obj, _COM_Outptr_ xaml_controls::IInputValidationError** error) const
{
    // Initialize the output parameter to nullptr at the start
    *error = nullptr;
    
    bool gotObject = false;
    if (auto eor = do_pointer_cast<CManagedObjectReference>(obj))
    {
        wrl::ComPtr<IInspectable> peer;
        IFCFAILFAST(DXamlServices::GetPeer(eor, IID_PPV_ARGS(&peer)));

        wrl::ComPtr<IInspectable> unwrappedValue;
        CValueBoxer::UnwrapExternalObjectReferenceIfPresent(peer.Get(), &unwrappedValue);

        gotObject = SUCCEEDED(unwrappedValue.CopyTo(error));
    }
    return gotObject;
}

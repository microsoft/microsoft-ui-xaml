// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "VisualStateGroupCollection.h"
#include "VisualStateGroupCollection_Partial.h"

#include <CustomWriterRuntimeObjectCreator.h>
#include <VisualStateGroupCollectionCustomRuntimeData.h>
#include <StreamOffsetToken.h>
#include <ThemeResource.h>

class CustomWriterRuntimeData;

namespace DirectUI {

_Check_return_ HRESULT VisualStateGroupCollection::QueryInterfaceImpl(
    _In_ REFIID riid, _Outptr_ void** ppvObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(IVisualStateGroupCollectionTestHooks)))
    {
        *ppvObject = static_cast<IVisualStateGroupCollectionTestHooks*>(this);
    }
    else
    {
        return DirectUI::VisualStateGroupCollectionGenerated::QueryInterfaceImpl(riid, ppvObject);
    }

    AddRefOuter();
    return S_OK;
}

std::shared_ptr<std::vector<std::wstring>> VisualStateGroupCollection::GetVisualStateGroupNames()
{
    return GetHandle()->GetCustomRuntimeData() ? std::make_shared<std::vector<std::wstring>>(
        GetHandle()->GetCustomRuntimeData()->GetVisualStateGroupNames()) : nullptr;
}

std::shared_ptr<std::vector<std::wstring>> VisualStateGroupCollection::GetVisualStateNamesForGroup(_In_ unsigned int groupIdx)
{
    return std::make_shared<std::vector<std::wstring>>(
        GetHandle()->GetCustomRuntimeData()->GetVisualStateNamesForGroup(groupIdx));
}

bool VisualStateGroupCollection::DoesVisualStateGroupHaveTransitions(_In_ unsigned int groupIdx) const
{
    return GetHandle()->GetCustomRuntimeData()->HasVisualTransitions(groupIdx);
}

Microsoft::WRL::ComPtr<IInspectable> VisualStateGroupCollection::CreateStoryboard(
    _In_ unsigned int, _In_ unsigned int storyboardIdx)
{
    auto customRuntimeData = GetHandle()->GetCustomRuntimeData();
    auto customRuntimeContext = GetHandle()->GetCustomRuntimeContext();

    CustomWriterRuntimeObjectCreator creator(
        NameScopeRegistrationMode::SkipRegistration,
        customRuntimeContext);

    auto deferredToken = customRuntimeData->GetStoryboard(storyboardIdx);

    std::shared_ptr<CDependencyObject> createdCoreObject;
    xref_ptr<CThemeResource> unused;
    IFCFAILFAST(creator.CreateInstance(deferredToken, &createdCoreObject, &unused));

    ctl::ComPtr<DependencyObject> createdObject;
    VERIFYHR(DXamlCore::GetCurrent()->GetPeer(static_cast<CDependencyObject*>(createdCoreObject.get()), &createdObject));

    Microsoft::WRL::ComPtr<IInspectable> createdIObject;

    createdIObject.Attach(static_cast<IDependencyObject*>(createdObject.Detach()));
    return createdIObject;
}

CVisualStateGroupCollection* VisualStateGroupCollection::GetHandle() const
{
    return static_cast<CVisualStateGroupCollection*>(DependencyObject::GetHandle());
}

}

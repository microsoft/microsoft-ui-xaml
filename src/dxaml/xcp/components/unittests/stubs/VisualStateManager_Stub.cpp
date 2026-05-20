// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "vsm\inc\CVisualStateManager2.h"
#include "vsm\inc\VisualState.h"
#include "vsm\inc\VisualStateToken.h"
#include "qualifiers\inc\QualifierFactory.h"
#include "collection\inc\VisualStateGroupCollection.h"
#include "base\inc\xref_ptr.h"

xref_ptr<CVisualStateGroupCollection> CVisualStateManager2::GetGroupCollectionFromVisualState(_In_ const CVisualState*)
{
    return nullptr;
}

xref_ptr<CVisualStateGroupCollection> CVisualStateManager2::GetGroupCollectionFromControl(_In_ CControl*)
{
    return nullptr;
}

VisualStateToken::VisualStateToken()
{
}

bool VisualStateToken::operator==(const VisualStateToken& t)
{
    return false;
}

_Check_return_ HRESULT QualifierContext::RegisterChangedCallback(_In_ IQualifierContextCallback* callback, _In_ QualifierFlags flags)
{
    return E_NOTIMPL;
}

std::shared_ptr<IQualifier> QualifierFactory::Create(_In_ const int& width, _In_ const int& height)
{
    return nullptr;
}

std::shared_ptr<IQualifier> QualifierFactory::Create(_In_ const bool* pBool)
{
    return nullptr;
}

_Check_return_ VisualStateToken CVisualState::GetVisualStateToken()
{
    return VisualStateToken();
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <framework.h>
#include <resources.h>
#include <style.h>
#include <MetadataAPI.h>
#include <DOPointerCast.h>
#include "corep.h"
#include "resources\inc\ResourceResolver.h"
#include "TemplateBindingData.h"
#include "RuntimeEnabledFeatures.h"
#include <FxCallbacks.h>

using namespace DirectUI;

// NOTE: For some reason, using XFLOAT_INF here results in 0,
// so we have to do the cast.
const FrameworkElementGroupStorage CFrameworkElement::DefaultLayoutProperties = {
    0, // MinWidth
    static_cast<XFLOAT>(XDOUBLE_INF), // MaxWidth
    0, // MinHeight
    static_cast<XFLOAT>(XDOUBLE_INF), // MaxHeight
    DirectUI::HorizontalAlignment::Stretch, // HorizontalAlignment
    DirectUI::VerticalAlignment::Stretch, // VerticalAlignment
    { 0, 0, 0, 0 }, // Margin
    0, //Grid Row Index
    0, //Grid Column Index,
    1, //Grid Row Span
    1  //Grid Column Span
};

    XRECTF CFrameworkElement::GetEffectiveViewport() const
{
    CValue result;
    IFCFAILFAST(GetValueByIndex(KnownPropertyIndex::FrameworkElement_EffectiveViewport, &result));
    return *(result.AsRect());
}

XRECTF CFrameworkElement::GetMaxViewport() const
{
    CValue result;
    IFCFAILFAST(GetValueByIndex(KnownPropertyIndex::FrameworkElement_MaxViewport, &result));
    return *(result.AsRect());
}

DOUBLE CFrameworkElement::GetBringIntoViewDistanceX() const
{
    CValue result;
    IFCFAILFAST(GetValueByIndex(KnownPropertyIndex::FrameworkElement_BringIntoViewDistanceX, &result));
    return result.AsDouble();
}

DOUBLE CFrameworkElement::GetBringIntoViewDistanceY() const
{
    CValue result;
    IFCFAILFAST(GetValueByIndex(KnownPropertyIndex::FrameworkElement_BringIntoViewDistanceY, &result));
    return result.AsDouble();
}

void CFrameworkElement::SetEffectiveViewport(XRECTF value)
{
    CValue cValue;
    cValue.Set<valueRect>(new XRECTF{ value });
    IFCFAILFAST(SetValueByIndex(KnownPropertyIndex::FrameworkElement_EffectiveViewport, cValue));
}

void CFrameworkElement::SetMaxViewport(XRECTF value)
{
    CValue cValue;
    cValue.Set<valueRect>(new XRECTF{ value });
    IFCFAILFAST(SetValueByIndex(KnownPropertyIndex::FrameworkElement_MaxViewport, cValue));
}

void CFrameworkElement::SetBringIntoViewDistanceX(DOUBLE value)
{
    CValue cValue;
    cValue.Set<valueDouble>(value);
    IFCFAILFAST(SetValueByIndex(KnownPropertyIndex::FrameworkElement_BringIntoViewDistanceX, cValue));
}

void CFrameworkElement::SetBringIntoViewDistanceY(DOUBLE value)
{
    CValue cValue;
    cValue.Set<valueDouble>(value);
    IFCFAILFAST(SetValueByIndex(KnownPropertyIndex::FrameworkElement_BringIntoViewDistanceY, cValue));
}


_Check_return_ CDependencyObject* CFrameworkElement::GetLogicalParentNoRef()
{
    if (!IsPropertyDefault(MetadataAPI::GetPropertyByIndex(KnownPropertyIndex::FrameworkElement_Parent)))
    {
        //logical parent is set. return that.
        EnsureLayoutProperties(true);
        return m_pLogicalParent;
    }
    else
    {
        //logical parent is not set. if the visual parent treats this element
        //as its logical child, then return the visual parent. else return null.
        CDependencyObject* pParent = GetParent();
        if (pParent)
        {
            CUIElement* pVisualParent = do_pointer_cast<CUIElement>(pParent);
            if (pVisualParent && pVisualParent->AreChildrenInLogicalTree())
            {
                if (pVisualParent->OfTypeByIndex<KnownTypeIndex::Panel>())
                {
                    CDependencyObject* pVisualGrandParent = pVisualParent->GetParent();
                    if (pVisualGrandParent && pVisualGrandParent->OfTypeByIndex<KnownTypeIndex::ItemsPresenter>())
                    {
                        //panel inside itemspresenter is not the logical parent of its visual children.
                        return nullptr;
                    }
                }
                return pVisualParent;
            }
        }
        return nullptr;
    }
}


void CFrameworkElement::EnsureLayoutProperties(bool fDefaultOK)
{
    // Object will be created with m_pLayoutProperties pointing to the default properties instance.
    // If that's still OK, we'll just keep using it, otherwise, we'll make our own copy.
    if (!m_pLayoutProperties || (!fDefaultOK && (m_pLayoutProperties == &DefaultLayoutProperties)))
    {
        m_pLayoutProperties = new FrameworkElementGroupStorage;
        // Something is probably about to be set, so initialize entire thing with default values
        *m_pLayoutProperties = DefaultLayoutProperties;
    }
}

bool CFrameworkElement::AllowFocusWhenDisabled()
{
    const CDependencyProperty *pAllowFocusWhenDisabledProperty = GetPropertyByIndexInline(KnownPropertyIndex::FrameworkElement_AllowFocusWhenDisabled);
    CValue allowFocusWhenDisabledValue;
    if (SUCCEEDED(this->GetValueInherited(pAllowFocusWhenDisabledProperty, &allowFocusWhenDisabledValue)))
    {
        return allowFocusWhenDisabledValue.AsBool() == TRUE;
    }
    return false;
}

xref_ptr<CResourceDictionary> CFrameworkElement::GetResourcesNoCreate() const
{
    CValue result = CheckOnDemandProperty(KnownPropertyIndex::FrameworkElement_Resources);
    return checked_sp_cast<CResourceDictionary>(result.DetachObject());
}

xstring_ptr CFrameworkElement::GetClassName()
{
    VERIFYHR(EnsureClassName());
    return m_strClassName;
}

_Check_return_ HRESULT
CFrameworkElement::EnsureClassName()
{
    if (m_strClassName.IsNull())
    {
        // Get full name of class from managed if possible
        //$TODO: Use new type meta data when it is checked in to get
        //$TODO type name
        if (HasManagedPeer() && IsCustomType())
        {
            IFC_RETURN(FxCallbacks::FrameworkCallbacks_GetCustomTypeFullName(this, &m_strClassName));
        }

        // If class name could not be obtained from managed, this is a
        // core type
        if (m_strClassName.IsNull())
        {
            const CClassInfo* pClassInfo = DirectUI::MetadataAPI::GetClassInfoByIndex(GetTypeIndex());
            IFCEXPECT_RETURN(pClassInfo != NULL);
            m_strClassName = pClassInfo->GetFullName();
        }

        ASSERT(!m_strClassName.IsNull());
    }
    return S_OK;
}



//------------------------------------------------------------------------
//
//  Synopsis:
//      Ensures that implicit style was picked up from the tree
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CFrameworkElement::EnsureImplicitStyle(bool isLeavingParentStyle)
{
    if (!m_pImplicitStyle)
    {
        m_eImplicitStyleProvider = ImplicitStyleProvider::None;
        m_pImplicitStyleParentWeakRef.reset();

        // Since m_eImplicitStyleProvider is packed into a bitfield, we can't take the address of
        // it, so we'll use a temporary local.
        auto newProvider = ImplicitStyleProvider::None;

        // Get resource whose key is the full name of the class for the element passed in.

        // EnsureImplicitStyle() is called from ApplyStyle(), UpdateImplicitStyle() or Enter() after Framework's
        // CreateComplete() in general. ApplyBuiltInStyle() is also called in the Control's CreateComplete().
        IFC_RETURN(EnsureClassName());
        IFC_RETURN(Resources::ResourceResolver::ResolveImplicitStyleKey(
            this,
            isLeavingParentStyle,
            &m_pImplicitStyle,
            &newProvider,
            &m_pImplicitStyleParentWeakRef));

        m_eImplicitStyleProvider = newProvider;
    }

    return S_OK;
}

CDependencyObject *CFrameworkElement::GetTemplatedParent()
{
    CDependencyObject *pTemplatedParent = nullptr;

    if (m_pTemplateBindingData && m_pTemplateBindingData->m_pTemplatedParentWeakRef)
    {
        pTemplatedParent = m_pTemplateBindingData->m_pTemplatedParentWeakRef.lock_noref();
    }

    if (pTemplatedParent == nullptr)
    {
        // There isn't a valid parent so there shouldn't be any TemplateBindings but clear them out to be safe.
        ClearTemplateBindingData();
    }

    return pTemplatedParent;
}

void CFrameworkElement::ClearTemplateBindingData()
{
    m_pTemplateBindingData.reset();
}

void CFrameworkElement::RemoveLogicalChild(_Inout_opt_ CDependencyObject* pOldLogicalChild)
{
    const CDependencyProperty* pdp = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_Parent);

    CFrameworkElement* pChild = do_pointer_cast<CFrameworkElement>(pOldLogicalChild);

    if (pChild && !pChild->IsPropertyDefault(pdp) && pChild->GetLogicalParentNoRef() == this)
    {
        pChild->EnsureLayoutProperties(false);

        //set it back to the default value
        pChild->m_pLogicalParent = NULL;

        //clear the flags to indicate a value is not set for this property
        pChild->SetPropertyIsDefault(pdp);
    }
}
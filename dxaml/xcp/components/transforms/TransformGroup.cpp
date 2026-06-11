// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TransformGroup.h"
#include "TransformCollection.h"
#include "WinRTExpressionConversionContext.h"
#include <DependencyObjectDCompRegistry.h>
#include <ExpressionHelper.h>
#include <StringConversions.h>

_Check_return_ HRESULT CTransformGroup::FromString(_In_ CREATEPARAMETERS *pCreate)
{
    IFCEXPECT_RETURN(pCreate->m_value.GetType() == valueString);
    // Call the type converter
    XUINT32 cString = 0;
    const WCHAR* pString = pCreate->m_value.AsEncodedString().GetBufferAndCount(&cString);
    IFC_RETURN(MatrixFromString(cString, pString, &cString, &pString, &m_groupMatrix));
    // NOTE: We're not using SetValue to initialize this property because this is not
    // exposed to script, so we don't have to worry about properly setting the valid flag

    return S_OK;
}

void CTransformGroup::NWPropagateDirtyFlag(DirtyFlags flags)
{
    if (!m_isWinRTExpressionDirty
        && !flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        // If a child transform is dirty, then this parent TransformGroup needs to be dirtied as well. The next render walk
        // will regenerate the DComp transform for this transform group, and it will propagate to the child transforms. The
        // dirty ones will regenerate their DComp transforms in turn, while the clean ones will keep using the existing
        // DComp transform.
        SetDCompResourceDirty();
    }

    __super::NWPropagateDirtyFlag(flags);
}

void CTransformGroup::NWSetTransformsDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::TransformGroup>());

    CTransformGroup *pGroup = static_cast<CTransformGroup*>(pTarget);

    // It's important to set the dirty bit first, in case anything queries the transform during dirty flag propagation.
    // CUIElement::NWSetTransformDirty will do so, for example, to determine if the transform is
    // axis-alignment-preserving or not.
    pGroup->m_fNWTransformsDirty = TRUE;
    pGroup->m_isWinRTExpressionDirty = true;

    // Always propagate flags and let parent decide if to propagate further.
    //
    // Transforms may change bounds but the transform group is not bounds
    // aware. Terminating the dirty path here based on the render flag
    // may prevent the parent from updating bounds when required to due
    // to a transform change.
    pGroup->NWPropagateDirtyFlag(flags);
}

void CTransformGroup::MakeWinRTExpression(
    _Inout_ WinRTExpressionConversionContext* pWinRTContext,
    _Inout_opt_ WUComp::ICompositionPropertySet* pTransformGroupPS
    )
{
    // Even a transform that is clean might need to create an expression, if it's part of a transform group where some other
    // transform started animating, so check both the dirty flag and that we already have an expression. The corollary is that
    // calling this method on a clean transform will still create an expression for it.
    if (m_isWinRTExpressionDirty || m_spWinRTExpression == nullptr)
    {
        WinRTExpressionConversionContext myContext(pWinRTContext);
        Microsoft::WRL::ComPtr<WUComp::ICompositionPropertySet> propertySet;

        if (m_spWinRTExpression == nullptr)
        {
            pWinRTContext->CreateExpression(nullptr /* expressionString */, &m_spWinRTExpression, &propertySet);

            if (GetDCompObjectRegistry() != nullptr)
            {
                GetDCompObjectRegistry()->EnsureObjectWithDCompResourceRegistered(this);
            }
        }
        else
        {
            Microsoft::WRL::ComPtr<WUComp::ICompositionObject> spCO;
            IFCFAILFAST(m_spWinRTExpression.As(&spCO));
            IFCFAILFAST(spCO->get_Properties(&propertySet));
        }

        if (m_pChild != nullptr)
        {
            for (auto item : *m_pChild)
            {
                static_cast<CTransform*>(item)->MakeWinRTExpression(&myContext, propertySet.Get());
            }
        }

        const unsigned int expressionCount = myContext.GetExpressionCount();
        if (expressionCount > 0)
        {
            pWinRTContext->UpdateTransformGroupExpression(
                expressionCount,
                m_spWinRTExpression.Get()
                );
        }
        else
        {
            m_spWinRTExpression.Reset();
        }

        m_isWinRTExpressionDirty = false;
    }

    if (pTransformGroupPS)
    {
        pWinRTContext->AddExpressionToTransformGroupPropertySet(m_spWinRTExpression.Get(), pTransformGroupPS);
    }
}

void CTransformGroup::ClearWUCExpression()
{
    __super::ClearWUCExpression();

    if (m_pChild != nullptr)
    {
        for (auto item : *m_pChild)
        {
            static_cast<CTransform*>(item)->ClearWUCExpression();
        }
    }
}

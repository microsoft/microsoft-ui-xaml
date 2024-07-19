// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      Contains the Jupiter image definition

#ifndef JUPITER_IMAGE_H
#define JUPITER_IMAGE_H

#include "AlphaMask.h"
#include "core_media_native.h"
#include "XamlCastingSource.h"
#include "GraphicsUtility.h"

//------------------------------------------------------------------------
//
//  Class:  CImage
//
//  Synopsis:
//      Object created for <Image> tag on Jupiter.
//
//------------------------------------------------------------------------
class CImage final : public CImageBase, public IXcpImageSharingEngineNotify
{
protected:
    CImage(_In_ CCoreServices *pCore);

   ~CImage() override;

public:
   // Creation method
    DECLARE_CREATE(CImage);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CImage>::Index;
    }

    void Shutdown() override;

    _Check_return_ HRESULT EnsureBrush() override;
    _Check_return_ HRESULT FireImageFailed(_In_ XUINT32 iErrorCode) override;
    _Check_return_ HRESULT InvokeImpl(_In_ const CDependencyProperty *pdp, _In_opt_ CDependencyObject *pNamescopeOwner) final;

    _Check_return_ HRESULT GetCastingSource(_COM_Outptr_ wm::Casting::ICastingSource **ppCastingSource);

    _Check_return_ HRESULT OnConnected( ) override;
    _Check_return_ HRESULT OnDisconnected( ) override;

    _Check_return_ HRESULT UpdateSourceStreamForSharingService( );

    _Check_return_ HRESULT NotifyRenderContent(
        HWRenderVisibility visibility);

    _Check_return_ HRESULT GetAlphaMask(
        _Outptr_ WUComp::ICompositionBrush** ppReturnValue);

    void CleanupDeviceRelatedResourcesRecursive(
        _In_ bool cleanupDComp) override;

    void ClearPCRenderData() override;

    _Check_return_ HRESULT GetTitle(_Outptr_ HSTRING *output);
    _Check_return_ HRESULT GetDescription(_Outptr_ HSTRING *output);

private:
    _Check_return_ HRESULT UpdateCastingSource();

    IXcpImageSharingEngine *m_pImageSharingEngine;
    bool                    m_waitingForSoftwareSurface;
    bool                    m_hasReloadedSoftwareSurface;
    CXamlCastingSource<IXcpImageSharingEngine>* m_pCastingSource;

    AlphaMask m_alphaMask;
};
#endif

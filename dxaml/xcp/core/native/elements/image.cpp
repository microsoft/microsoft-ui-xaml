// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

using namespace Microsoft::WRL;

_Check_return_ HRESULT
CreateImageSharingEngine(
    _In_ IMediaServices* pMediaServices,
    _In_ IXcpImageSharingEngineNotify *pXcpImageSharingEngineNotify,
    _Outptr_ IXcpImageSharingEngine** ppXcpImageSharingEngine);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Constructor for Image object
//
//------------------------------------------------------------------------
CImage::CImage(_In_ CCoreServices *pCore)
        : CImageBase(pCore)
{
    m_pImageSharingEngine = NULL;
    m_waitingForSoftwareSurface = FALSE;
    m_hasReloadedSoftwareSurface = FALSE;
    m_pCastingSource = nullptr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Destructor for Image object
//
//------------------------------------------------------------------------
CImage::~CImage()
{
    // Shutdown and release IXcpImageSharingEngine before we invoke Shutdown(),
    // as we should not invoke Image_OnPlayToConnectionShutdown() when CImage has been destroyed.
    if (m_pImageSharingEngine)
    {
        m_pImageSharingEngine->Shutdown();
        ReleaseInterface( m_pImageSharingEngine );
    }

    ReleaseInterface(m_pCastingSource);

    Shutdown();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//
//      Handle the ImageFailed event in Image class level.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
 CImage::FireImageFailed(_In_ XUINT32 iErrorCode)
{
    IFC_RETURN(CImageBase::FireImageFailed(iErrorCode ));

    if (m_pImageSharingEngine != NULL)
    {
        m_waitingForSoftwareSurface = FALSE;
        m_hasReloadedSoftwareSurface = FALSE;
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Make sure our internal brush is current
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImage::EnsureBrush()
{
    IFC_RETURN(CImageBase::EnsureBrush( ));

    if ( m_pImageSharingEngine &&  m_waitingForSoftwareSurface && m_pImageSource )
    {
        //
        // If ImageSharingEngine is waiting for a software surface,
        // detect if the surface is ready or not, if it is ready,
        // pass it to the sharing engine and reset the flag.
        IFC_RETURN( UpdateSourceStreamForSharingService());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//        A private method to update source stream for sharing service.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImage::UpdateSourceStreamForSharingService( )
{
    ASSERT( m_pImageSource  != NULL );

    if ( m_pImageSource->GetSoftwareSurface() != NULL )
    {
        // Got the Software surface.
        // Pass it to the ImageSharingEngine.
        IFC_RETURN( m_pImageSharingEngine->SetSource( m_pImageSource->GetSoftwareSurface( ) ) );

        // Reset the flag, there is no need to wait for software surface any more.
        m_waitingForSoftwareSurface = FALSE;
    }
    else
    {
        //
        // If the source is ready, make sure it trigger the download
        // of the software surface just once.
        //
        if ( !m_hasReloadedSoftwareSurface )
        {
            IFC_RETURN (m_pImageSource->ReloadReleasedSoftwareImage( ) );
            m_hasReloadedSoftwareSurface = TRUE;
        }
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Image is shutdown from CoreSerivce.
//
//------------------------------------------------------------------------
void
CImage::Shutdown( )
{
    m_waitingForSoftwareSurface = FALSE;
    m_hasReloadedSoftwareSurface = FALSE;

    // Shutdown and release IXcpImageSharingEngine.
    if (m_pImageSharingEngine)
    {
        m_pImageSharingEngine->Shutdown();

        ReleaseInterface( m_pImageSharingEngine );

        // We've shutdown the sharing engine, so update the casting source object (if exists) to reflect that
        UpdateCastingSource();
    }

    CImageBase::Shutdown();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handle it when Image is connected to ImageSharingEngine.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImage::OnConnected( )
{
    HRESULT hr = S_OK;

    m_waitingForSoftwareSurface = TRUE;
    m_hasReloadedSoftwareSurface = FALSE;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handle it when Image is disconnected from ImageSharingEngine.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImage::OnDisconnected( )
{
    //
    // ImageSharingEngine is disconnected, there is no need to track for a software surface anymore.
    //
    //DEAD_CODE_REMOVAL ?
    m_waitingForSoftwareSurface = FALSE;

    RRETURN(S_OK);
}

_Check_return_ HRESULT CImage::GetTitle(_Outptr_ HSTRING *output)
{
    *output = nullptr;
    if (m_pImageSource)
    {
        IFC_RETURN(m_pImageSource->GetTitle(output));
    }
    return S_OK;
}

_Check_return_ HRESULT CImage::GetDescription(_Outptr_ HSTRING *output)
{
    *output = nullptr;
    if (m_pImageSource)
    {
        IFC_RETURN(m_pImageSource->GetDescription(output));
    }
    return S_OK;
}

// Gets/Creates the CastingSource for this Image
_Check_return_ HRESULT CImage::GetCastingSource(_COM_Outptr_ wm::Casting::ICastingSource **ppCastingSource)
{
    IFCEXPECT_RETURN(ppCastingSource);
    *ppCastingSource = nullptr;

    if (m_pCastingSource == nullptr)
    {
        if (m_pImageSharingEngine == nullptr)
        {
            IFC_RETURN(CreateImageSharingEngine(static_cast<IMediaServices*>(GetContext()),
                static_cast<IXcpImageSharingEngineNotify *>(this),
                &m_pImageSharingEngine));
        }

        IFC_RETURN(MakeAndInitialize<CXamlCastingSource<IXcpImageSharingEngine>>(&m_pCastingSource));
        IFC_RETURN(UpdateCastingSource());
    }

    IFC_RETURN(m_pCastingSource->QueryInterface(IID_PPV_ARGS(ppCastingSource)));

    return S_OK;
}

// Updates the casting source with the current media source information
_Check_return_ HRESULT
CImage::UpdateCastingSource()
{
    if (m_pCastingSource != nullptr)
    {
        IFC_RETURN(m_pCastingSource->SetElement(m_pImageSharingEngine));
        IFC_RETURN(m_pCastingSource->put_SupportedCastingTypes(wm::Casting::CastingPlaybackTypes_Picture));
        IFC_RETURN(m_pCastingSource->put_IsProtectedContent(false));
    }

    return S_OK;
}

// Overrides the base class implementation to update the image sharing engine whenever the Source
// property on the image changes
_Check_return_ HRESULT
CImage::InvokeImpl(_In_ const CDependencyProperty *pdp, _In_opt_ CDependencyObject *pNamescopeOwner)
{
    IFC_RETURN(CImageBase::InvokeImpl(pdp, pNamescopeOwner));

    if (pdp->GetIndex() == KnownPropertyIndex::Image_Source)
    {
        if ((m_pImageSharingEngine != nullptr) && (m_pImageSource != nullptr))
        {
            IFC_RETURN(UpdateSourceStreamForSharingService());
        }
    }

    return S_OK;
}

void CImage::CleanupDeviceRelatedResourcesRecursive(
    _In_ bool cleanupDComp
    )
{
    CFrameworkElement::CleanupDeviceRelatedResourcesRecursive(cleanupDComp);

    m_alphaMask.Hide();
}

void CImage::ClearPCRenderData()
{
    ASSERT(IsInPCScene_IncludingDeviceLost());

    __super::ClearPCRenderData();

    // Hide the alpha mask so that the composition surface is released.
    m_alphaMask.Hide();
}

_Check_return_ HRESULT CImage::NotifyRenderContent(
    HWRenderVisibility visibility
    )
{
    if (visibility == HWRenderVisibility::Invisible)
    {
        m_alphaMask.Hide();
    }
    else
    {
        // Update the alpha mask.  This will check if there is one created and only update it if necessary.
        IFC_RETURN(m_alphaMask.UpdateIfAvailable(this));
    }

    return S_OK;
}

_Check_return_ HRESULT CImage::GetAlphaMask(
    _Outptr_ WUComp::ICompositionBrush** ppReturnValue)
{
    IFC_RETURN(m_alphaMask.Ensure(this));
    *ppReturnValue = m_alphaMask.GetCompositionBrush().Detach();

    return S_OK;
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "core.h"
#include <cvalue.h>
#include "values.h"
#include "TypeTable.g.h"
#include "CompositionRequirement.h"
#include "IndependentAnimationType.h"
#include "DependencyObjectTraits.h"
#include "INameScope.h"
#include <vector_set.h>
#include <unordered_set>
#include <deque>
#include "ConnectedAnimationService.h"
#include <UIThreadScheduler.h>
#include <AKExport.h>
#include <namescope\inc\NameScopeRoot.h>
#include "FlyweightCoreState.h"
#include "MaxTextureSizeProvider.h"
#include "AtlasRequestProvider.h"
#include "FacadeStorage.h"
#include <SimpleProperties.h>
#include <Microsoft.UI.Xaml.coretypes.h>
#include <windows.graphics.display.h>
#include "InitializationType.h"
#include <ContentRootCoordinator.h>
#include "CircularMemoryLogger.h"
#include "ImageProvider.h"
#include "AsyncImageFactory.h"
#include "ResourceLookupLogger.h"

#include "AutomationEventsHelper.h"

#include <fwd/windows.ui.composition.h>
#include <fwd/windows.system.h>

#define  E_FOCUS_CHANGE_NOT_ALLOWED_DURING_FOCUS_CHANGING 32L

struct DeviceListener;
struct DisplayListener;
struct RenderStateListener;
class CSwapChainBackgroundPanel;
class CSwapChainPanel;
struct IXcpInputPaneHandler;
class CInputPaneHandler;
class CUIElement;
class CCustomClassInfo;
class CCustomDependencyProperty;
class CCustomProperty;
class CDependencyProperty;
class CEnterDependencyProperty;
class CObjectDependencyProperty;
class XamlSchemaContext;
class XamlNodeStreamCacheManager;
class XbfWriter;
class XamlServiceProviderContext;
class CTimeManager;
class CFocusManager;
class CEventManager;
class CInputServices;
class CGeometryBuilder;
class CGlyphPathBuilder;
class CDependencyObject;
class CEnumerated;
class CDouble;
class CInt32;
class CPoint;
class CColor;
class CTransform;
class CBrush;
class CSolidColorBrush;
class CCoreServices;
class CMediaQueue;
class CMediaQueueManager;
class CDeployment;
struct Property;
class CLayoutManager;
class CRootVisual;
class CUIElement;
class CDisplayMemberTemplate;
class CTextBlock;
class CItemsControl;
class CItemsPresenter;
class CItemCollection;
class CApplication;
class CStyle;
class CCustomTypeFactory;
class CAutomationPeer;
class CPopupRoot;
class CDataTemplate;
class CControlTemplate;
class CTileBrush;
class CDispatcherTimer;
class CStoryboard;
class CPrintRoot;
class CTransitionRoot;
class CFrameworkElement;
class XamlServiceProviderContext;
class HWWalk;
class HWCompTreeNode;
class CWriteableBitmap;
class CVirtualSurfaceImageSource;
class ResourceManager;
class CDeferredElement;
struct IAsyncImageFactory;
class CFlyoutBase;
class CMenuFlyout;
class CConnectedAnimationService;
class CDeferredAnimationOperation;

class CResourceDictionary;
class RootScale;
class CoreWindowRootScale;

// Text types
class CTextLine;
class CGlyphTypefaceCollection;
class CGlyphTypefaceCache;
class CTypefaceCollection;
class CCompositeFontFamily;
class CFontFamily;
class IFontAndScriptServices;
interface IDWriteFontDownloadQueue;
interface IDWriteFontCollection;
class TextFormatting;

struct ICustomResourceLoader;
class AsyncImageFactory;
class ImageProvider;
class ImageTaskDispatcher;
class CImageSource;

namespace RichTextServices
{
    class TextFormatterCache;
    class TextDrawingContext;
}

class CCompositorDirectManipulationViewport;
class CWindowRenderTarget;
class SurfaceCache;
class CompositorTreeHost;
struct IDCompositionDesktopDevicePartner;
class CRenderTargetBitmapManager;
class CWindowRenderTarget;
class CImageReloadManager;
class ImageSurfaceWrapper;
class XamlSchemaContext;
class FrameworkTheming;
class ThemeWalkResourceCache;
class CTransformGroup;
class WindowsPresentTarget;
class CXamlCompositionBrush;
class CLayoutTransitionElement;
class CFrameworkTemplate;
class CUIAWindow;
class CD3D11Device;

namespace Theming {
    enum class Theme : uint8_t;
}

#include "Indexes.g.h"
#include "TypeBits.h"
#include "enumdefs.h"
#include "refcounting.h"
#include "DataStructureFunctionSpecializations.h"
#include "DataStructureFunctionProvider.h"
#include "xmap.h"
#include "xlist.h"
#include "DataStructureFunctionProvider.h"
#include "DataStructureFunctionSpecializations.h"
#include "xref_ptr.h"

#include "xstring_ptr.h"
#include "XStringBuilder.h"

#include "memorysurface.h"

#include "DebugSource.h"
#include <Microsoft.UI.Xaml.hosting.referencetracker.h>
#include "ReferenceTrackerInterfaces.h"

#include "PropertyChangedParams.h"

#include "VisualTree.h"

#include "TextSurrogates.h"

#include "IParserCoreServices.h"

struct XSIZE_PHYSICAL : public XSIZE {};
struct XSIZE_LOGICAL : public XSIZE {};

// Returns the default text selection highlight color.  On windows, this
// returns 0xFF4617b4 (blueberry).  One phone, this returns the system
// accent color;
XUINT32 GetDefaultSelectionHighlightColor();

//------------------------------------------------------------------------
//
//  Method:   DecodeUtf16Character
//
//  Synopsis:
//
//  Takes a UTF16 buffer, a length and an index and extracts a Unicode
//  character, updating the index.
//
//------------------------------------------------------------------------

inline _Check_return_ HRESULT DecodeUtf16Character(
    _In_reads_(cCharacters) const WCHAR   *pCharacters,
    _In_                           XUINT32  cCharacters,
    _In_                           XUINT32  iCharacter,   // Offset into pCharacters
    _Deref_out_range_(1,2)         XUINT32 *pcCodeUnits,  // Number of UTF-16 code units used
    _Out_                          XUINT32 *pCharacter    // UTF-32 code unit
)
{
    IFCEXPECT_RETURN(iCharacter < cCharacters);

    // Start by assuming the simple non-surrogate case

    *pcCodeUnits = 1;
    *pCharacter  = pCharacters[iCharacter];

    // Test whether the UTF-16 code unit is in the
    // surrogate range D800 - DFFF.

    if (IS_SURROGATE(*pCharacter))
    {
        // Handle surrogate codepoints
        // The first of the pair must be in the range D800 - DBFF
        if (    IS_LEADING_SURROGATE(*pCharacter)
            &&  (iCharacter+1 < cCharacters)
            &&  IS_TRAILING_SURROGATE(pCharacters[iCharacter+1]))
        {
            // Caculate the UTF-32 code unit from the surrogate pair
            *pcCodeUnits = 2;
            *pCharacter  = ((*pCharacter & 0x3FF) << 10)        // High surrogate
                         + (pCharacters[iCharacter+1] & 0x3FF)  // Low surrogate
                         + 0x10000;

        }
        else
        {
            // Malformed surrogate
            *pcCodeUnits = 1;

            // TODO: Use the actual replacement character, U+FFFD
            *pCharacter  = 0;
        }
    }

    return S_OK;
}

#include "DependencyObjectTraits.h"

// Initially allocate this many slots in the metadata block.

#define INDEX_RESERVE                           256
#define MAX_INHERITED_PROPS                     16
#define MAX_SHARES_PER_PROP                     4 // At most TextBlock, RichTextBlock, TextElement and Control share inherited font properties.

#define MAX_ALLOWEDMEMORY_INCREASE              (52428800) // (50*1024*1024);

#define TRACKING_INTERVAL                       50
#define GCTRACKING_STATE_WAITING                0
#define GCTRACKING_STATE_LISTENING              1
#define GCTRACKING_STATE_COLLECTING             2

#define AGGRESSIVE_COLLECT_FACTOR               5

// Flags used to keep watch of changes that occur during enter/leave
#define WATCH_LOADED_EVENTS     0x00000002

// Define the order in which the various visual overlay layers appear, beginning
// witht he top most and working down.  The normal xaml content wil be at zero.

#define VISUALDIAGNOSTICSROOT_ZINDEX    XINT32_MAX - 1
#define CONNECTEDANIMATIONROOT_ZINDEX   VISUALDIAGNOSTICSROOT_ZINDEX - 1
#define POPUP_ZINDEX                    CONNECTEDANIMATIONROOT_ZINDEX - 1
#define FULLWINDOWMEDIAROOT_ZINDEX      POPUP_ZINDEX - 1
#define TRANSITIONROOT_ZINDEX           FULLWINDOWMEDIAROOT_ZINDEX - 1

class CDeferredMapping;

// Used in several places in the core to enable multum in parvo (MIP) mapping.

#if defined(_X86_) || defined(_AMD64_)
struct MIPDETAIL
{
    void   *pvScan0;        // +00 64bit
    XINT32  nStride;        // +04 (+08)
    XUINT32 nWidth;         // +08 (+0c)
    XUINT32 nHeight;        // +0c (+10)
};
#endif


// To optimize the graphics pipeline we have chose to use SIMD processor
// features. Since many different components in core directly reach into
// the software rasterizer it would be nearly impossible to contain this
// requirement to just core\sw.  So in order to avoid multiple, or worse,
// conflicting definitions of these types I'm placing them here.

#if defined(_X86_) || defined(_AMD64_)

#if !defined (_CRT_ALIGN)
#define _CRT_ALIGN(x) __declspec(align(x))
#endif  /* !defined (_CRT_ALIGN) */

#include <intrin.h>

#endif

#if defined(_X86_) && !defined(_ARM_)

typedef __m64   XX64;

typedef union
{
    __m128      flt128;
    __m128i     int128;
    __m128d     dbl128;
} XX128;
#endif

#include "PCRenderDataList.h"

//------------------------------------------------------------------------
//
//  Struct:  DREQUEST
//
//  Synopsis:
//      The data necessary for a download or streaming request.  These are
//  queued up during control load and processed after the control has finished
//  processing the OnLoaded event.
//
//------------------------------------------------------------------------

class CAbortableDownloadWrapper;

struct DREQUEST
{
    xstring_ptr                    strRelativeUri;  // Only valid for streams
    CAbortableDownloadWrapper      *pIAbortableDownload;
    IPALUri                         *pPreferredBaseUri;
    IPALDownloadRequest             *pDownloadRequest;  // Only valid for downloads

    DREQUEST()
    {
        pIAbortableDownload = NULL;
        pPreferredBaseUri = NULL;
        pDownloadRequest = NULL;
    }

    void ReleaseResources();
};

//------------------------------------------------------------------------
//
//  Class:  CTextCore
//
//  Abstract:
//
//      Contains the text components of the core services object.
//------------------------------------------------------------------------

template <class C> class SpanBase;
class WinTextCore;

// TODO: consider collapsing CTextCore with WinTextCore into a single class.
//       Right now it is not possible since core codebase cannot take dependency on Windows specific types.

class CTextCore
{
private:
    CCoreServices              *m_pCore;
    WinTextCore                *m_pWinTextCore;
    IFontAndScriptServices     *m_pFontAndScriptServices;
    RichTextServices::
    TextFormatterCache         *m_pTextFormatterCache;
    TextFormatting             *m_pDefaultTextFormatting;

    // Store the last Text Control that has some text selected.
    CUIElement                 *m_pLastSelectedTextElement;

public:
    CTextCore(_In_ CCoreServices *pCore);

    ~CTextCore();

    CCoreServices *GetCoreServices() { return m_pCore; }

    WinTextCore *GetWinTextCore() const { return m_pWinTextCore; }

    _Check_return_ HRESULT Initialize();

    _Check_return_ HRESULT GetFontAndScriptServices(
        _Outptr_ IFontAndScriptServices **ppFontAndScriptServices
        );

    _Check_return_ HRESULT GetTextFormatterCache(
        _Outptr_ RichTextServices::TextFormatterCache **ppTextFormatterCache
        );

    _Check_return_ HRESULT GetDefaultTextFormatting(
        _Outptr_ TextFormatting **ppDefaultTextFormatting
        );

    void ClearDefaultTextFormatting();
    void ReleaseUnusedTextFormatters();
    void SetLastSelectedTextElement(_In_ CUIElement *pLastSelectedTextElement);
    void ClearLastSelectedTextElement();
    HRESULT ConfigureNumberSubstitution();
    static bool IsTextControl(_In_ CDependencyObject* pDO);
    static bool IsTextSelectionEnabled(_In_ CDependencyObject* textControl);
    bool CanSelectText(_In_ CUIElement* pTextElement) const;
};

//------------------------------------------------------------------------
//
//  TextFormatting
//
//  A storage group which collects all the text core inherited properties.
//  Currently present as a storage group on CFramworkElement and CTextElement.
//  The generation counter on this object is compared to the global
//  counter on core, to decide whether non-locally set properties are up to
//  date.
//
//------------------------------------------------------------------------

class TextFormatting : public CReferenceCount
{
public:
    static _Check_return_ HRESULT Create(
        _In_        CCoreServices   *pCoreServices,
        _Outptr_ TextFormatting **ppTextFormatting
    );

    static _Check_return_ HRESULT CreateCopy(
        _In_          CCoreServices         *pCoreServices,
        _In_          const TextFormatting  *pTemplate,
        _Inout_ TextFormatting       **ppTextFormatting
    );

    static _Check_return_ HRESULT CreateDefault(
        _In_        CCoreServices   *pCoreServices,
        _Outptr_ TextFormatting **ppTextFormatting
    );

    _Check_return_ bool IsOld()
    {
        return m_cGenerationCounter != *m_pCoreInheritedPropGenerationCounter;
    }

    void SetIsUpToDate()
    {
        m_cGenerationCounter = *m_pCoreInheritedPropGenerationCounter;
    }

    XFLOAT GetScaledFontSize(XFLOAT fontScale) const;

    _Check_return_ HRESULT SetFontFamily(
        _In_ CDependencyObject *pdo,
        _In_opt_ CFontFamily *pFontFamily);

    _Check_return_ HRESULT SetForeground(
        _In_ CDependencyObject *pdo,
        _In_opt_ CBrush *pForeground);

    void SetFreezeForeground(_In_ bool freezeForeground);

    void SetLanguageString(_In_ xstring_ptr strLanguageString);
    void SetResolvedLanguageString(_In_ xstring_ptr strResolvedLanguageString);
    void SetResolvedLanguageListString(_In_ xstring_ptr strResolvedLanguageListString);

    xstring_ptr GetResolvedLanguageStringNoRef() const;
    xstring_ptr GetResolvedLanguageListStringNoRef() const;

    _Check_return_ HRESULT ResolveLanguageString(_In_ CCoreServices *pCore);
    _Check_return_ HRESULT ResolveLanguageListString(_In_ CCoreServices *pCore);

    CFontFamily                 *m_pFontFamily;
    CBrush                      *m_pForeground;
    XUINT32                     *m_pCoreInheritedPropGenerationCounter;
    xstring_ptr                  m_strLanguageString;
    XUINT32                      m_cGenerationCounter;
    XFLOAT                       m_eFontSize;
    XINT32                       m_nCharacterSpacing;
    DirectUI::CoreFontWeight     m_nFontWeight;
    DirectUI::FontStyle          m_nFontStyle;
    DirectUI::FontStretch        m_nFontStretch;
    DirectUI::TextDecorations    m_nTextDecorations;
    DirectUI::FlowDirection      m_nFlowDirection;
    bool                         m_isTextScaleFactorEnabled;

    // Don't pull foreground value from parent. Used for
    // mixing themes, where the subtree wants a different foreground
    // value than the parent.
    bool                         m_freezeForeground;

private:
    // Clients must use Create and ReleaseInterface.
    TextFormatting();
    ~TextFormatting() override;
    TextFormatting(_In_ CCoreServices *pCore);
    xstring_ptr m_strResolvedLanguageString;
    xstring_ptr m_strResolvedLanguageListString;
};

#include "InheritedProperties.h"

#include "IPLMListener.h"

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Implement this on any class that needs notifications of animation changes to show/hide
//      objects prior to the render walk. This behavior is required for text selection grippers.
//
//------------------------------------------------------------------------------
struct IGripper
{
    virtual _Check_return_ HRESULT UpdateVisibility() = 0;
};

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Struct to hold the various parameters defining an animation scenario
//      for power scenario, tracing & telemetry purposes . The structure is
//      expected to be zero initialized and then filled in before being passed
//      to tracking routines.
//
//------------------------------------------------------------------------------
struct AnimationTrackingScenarioInfo
{
    XUINT64 qpcInitiate;
    XUINT64 qpcInput;
    XUINT32 msIntendedDuration;
    XUINT16 priority;
    const WCHAR* scenarioName;
    const WCHAR* scenarioDetails;
};

enum class LayoutCompletedNeededReason
{
    WindowSizeChanged = 0,
    WindowMadeVisible
};

enum class ReentrancyBehavior
{
    CrashOnReentrancy = 0,
    BlockReentrancy,
    AllowReentrancy
};

// In perf traces we're seeing a noticeable amount of time spent in RoGetActicationFactory for the various types
// involved in initializing Xaml. For multi-UI-thread scenarios we are paying this cost with each new UI thread that's
// created. This class caches the various activation factories so that we only pay this cost once per process and not
// once per UI thread.
//
// There should only be one instance of this class, living in a process-wide DependencyLocator::Dependency. New islands
// (on separate UI threads) will use the cached activation factories. Note that we pay a small mutex cost in accessing
// DependencyLocator, but since initializing new islands isn't something that happens frequently this cost is going to
// be significantly cheaper than calling RoGetActivationFactory again.
class __declspec(uuid("e57cf768-857d-4a33-9579-169799a10ed4")) ActivationFactoryCache
{
public:
    static std::shared_ptr<ActivationFactoryCache> GetActivationFactoryCache();

    void ResetCache();

    HRESULT GetDispatcherQueueStatics(_Outptr_ msy::IDispatcherQueueStatics** statics);
    HRESULT GetDesktopChildSiteBridgeStatics(_Outptr_ ixp::IDesktopChildSiteBridgeStatics** statics);
    HRESULT GetDragDropManagerStatics(_Outptr_ mui::DragDrop::IDragDropManagerStatics** statics);

    ixp::ICompositionEasingFunctionStatics* GetCompositionEasingFunctionStatics();
    ixp::IInteropCompositorFactoryPartner* GetInteropCompositorFactoryPartner();
    ixp::ICompositionPathFactory* GetPathFactory();
    ixp::IInputSystemCursorStatics* GetInputSystemCursorStatics();
    ixp::IContentIslandStatics* GetContentIslandStatics();

private:
    // Multiple threads can be accessing the activation factories stored here.
    wil::critical_section m_lock;

    // FUTURE: Release these factory caches in DllCanUnloadNow if Xaml supports unloading the DLL. That behavior matches
    // cppwinrt's factory caches.
    // Note: We can cache more activation factories, but caching an out-of-proc factory (more accurately, an in-proc
    // proxy for an out-of-proc factory) involves watching for RPC errors and releasing/re-creating the in-proc proxy.
    // For simplicity we only cache in-proc factories.
    wrl::ComPtr<msy::IDispatcherQueueStatics> m_dispatcherQueueStatics;
    wrl::ComPtr<mui::DragDrop::IDragDropManagerStatics> m_dragDropManagerStatics;
    wrl::ComPtr<ixp::IDesktopChildSiteBridgeStatics> m_desktopChildSiteBridgeStatics;
    wrl::ComPtr<ixp::ICompositionEasingFunctionStatics> m_compositionEasingFunctionStatics;
    wrl::ComPtr<ixp::IInteropCompositorFactoryPartner> m_interopCompositorFactoryPartner;
    wrl::ComPtr<ixp::ICompositionPathFactory> m_compositionPathFactory;
    wrl::ComPtr<ixp::IInputSystemCursorStatics> m_inputSystemCursorStatics;
    wrl::ComPtr<ixp::IContentIslandStatics> m_contentIslandStatics;
};

//------------------------------------------------------------------------
//
//  Class:  CCoreServices
//
//  Synopsis:
//      The core services implement this class to provide the services.
//
//  This class is final from the point of view of Microsoft.UI.Xaml.dll
//  meaning that close to 400 method calls could be devirtualized by
//  marking it final. However the test code class XCbfCoreServices in
//  xcp\tools\GenXbfDLL does derive from CCoreServices.
//  In the short term just mark the most frequently-called methods of
//  CCoreServices that are not overridden by XCbfCoreServices as final,
//  rather than marking the whole class final.
//------------------------------------------------------------------------

class CRuntimeHost;

typedef containers::vector_map<CUIElement*, XRECTF> VisibleBoundsMap;
typedef std::map<xstring_ptr, const CDependencyProperty*> ResolvedDPMap;

class CCoreServices : public IMediaServices, public IParserCoreServices
{
    friend class DirectUI::DxamlCoreTestHooks;

public:
    CCoreServices();
    ~CCoreServices() noexcept;

    static  _Check_return_ HRESULT Create(_Out_ CCoreServices **ppCore);

#pragma region NameScope-Related Methods

    // Main entry-point
    xref_ptr<CDependencyObject> TryGetElementByName(
        const xstring_ptr_view& strName, _In_ CDependencyObject* referenceObject);

    // Called in EnterImpl for non-template namescopes and directly by the
    // parser for template namescopes.
    _Check_return_ HRESULT SetNamedObject(
        _In_ const xstring_ptr_view& strName,
        _In_ const CDependencyObject *pNamescopeOwner,
        _In_ Jupiter::NameScoping::NameScopeType nameScopeType,
        _In_ CDependencyObject* pObject);

    // Evil alternative way to call GetElementName. It looks like it's used
    // in Template scnearios when the pointer to the namescope owner might no longer
    // be valid, and we only want to use it as a token to find a namescope table. Evil
    // evil evil.
    _Check_return_ CDependencyObject* GetNamedObject(
        _In_ const xstring_ptr_view& strName,
        _In_opt_ const CDependencyObject *pNamescopeOwner,
        _In_ Jupiter::NameScoping::NameScopeType nameScopeType);

    // When nontemplate elements leave the visual tree they call this method to
    // remove themselves from the namescope table.
    _Check_return_ HRESULT ClearNamedObject(
        _In_ const xstring_ptr_view& strName,
        _In_opt_ const CDependencyObject *namescopeOwner,
        _In_opt_ CDependencyObject* originalEntry);

    _Check_return_ HRESULT RemoveNameScope(
        _In_ const CDependencyObject *pNamescopeOwner, Jupiter::NameScoping::NameScopeType nameScopeType);

    // As we're in a transitionary period between CCoreServices and a more civilized
    // NameScope implementation some NameScope methods go striaght to the underlying
    // NameScopeRoot, while in other cases hairy logic in CCoreServices has been left
    // in place until it can be better untangled.
    Jupiter::NameScoping::NameScopeRoot& GetNameScopeRoot();

    // Used in a couple places as an optimization to avoid nonlive enter walks.
    _Check_return_ bool HasRegisteredNames(_In_ const CDependencyObject *pNamescopeOwner) const;

    // The older namescope treewalk method. In most scenarios this code path is never executed,
    // instead the parser registers elements as it encounters them, but when parsing into a XAML root
    // that already has a namescope we'll instead use this less performant codepath.
    _Check_return_ HRESULT PostParseRegisterNames(_In_ CDependencyObject *pDependencyObject);

    // During parse this namescope is used when a call to put_Name from app code is made to
    // a DO that is owned by the parser.
    xref_ptr<INameScope> SetParserNamescope(_In_ xref_ptr<INameScope> namescope)
    {
        auto previous = m_parserNamescope;
        m_parserNamescope = namescope;
        return previous;
    }
    CDependencyObject* GetParserNamescopeOwner();

    // Check if element registered under this name exists and is deferred.
    CDeferredElement* GetDeferredElementIfExists(
        _In_ const xstring_ptr_view& strName,
        _In_ const CDependencyObject *pNamescopeOwner,
        _In_ Jupiter::NameScoping::NameScopeType nameScopeType);

    // Returns namescope and its type for a given reference object. The idea here is that when publically calling
    // FindName you always are passing in an object as a reference, and that the FindName call needs to be evaluated
    // within the context of that reference object. The trick is that a reference object could be a template namescope
    // member, it could be a UserControl, and it could be tied up in some of the edge cases our platform
    // has surrounding Popups and parse roots. This method contains the single source of logic to take a reference type and
    // make those decisions, finding the correct namescope owner and whether we should examine its template or standard namescope.
    static std::tuple<CDependencyObject*, Jupiter::NameScoping::NameScopeType> GetAdjustedReferenceObjectAndNamescopeType(
        _In_ CDependencyObject* referenceObject);

private:
    xref_ptr<INameScope> m_parserNamescope;
    Jupiter::NameScoping::NameScopeRoot m_nameScopeRoot;

#pragma endregion

public:
// Life time management
    XUINT8 IsCoreReady() {return m_fIsCoreReady;}

// Media Services

    XUINT32 Release() final;
    XUINT32 AddRef() final;

    _Check_return_ HRESULT CreateErrorService(
        _Outptr_ IErrorService **ppErrorService);

    _Check_return_ HRESULT CreateErrorServiceForXbfGenerator();

    HRESULT _Check_return_ CheckUri(
        _In_ const xstring_ptr& strRelativeUri,
        _In_ XUINT32 eUnsecureDownloadAction) final;

    // This member is used for cases where there are legitimate exceptions
    // to site-of-origin download restriction.  If you make a call to this
    // member, comment the call as to why the exception is valid and secure.
    _Check_return_ HRESULT UnsecureDownloadFromSite(
        _In_ const xstring_ptr& strRelativeUri,
        _In_opt_ IPALUri *pAbsoluteUri,
        _In_ IPALDownloadResponseCallback* pICallback,
        _In_ XUINT32 eUnsecureDownloadAction,
        _Outptr_opt_ IPALAbortableOperation **ppIAbortableDownload,
        _In_opt_ IPALUri *pPreferredBaseUri = NULL);

    IXcpBrowserHost *GetBrowserHost() final
    {
        return m_pBrowserHost;
    }

// Property services

    _Check_return_ HRESULT RegisterScriptCallback(
        _In_ void *pControl,
        _In_ EVENTPFN pfnAsync,
        _In_ EVENTPFN pfnSync);

    void TrackMemory();
    void ForceGCCollect();
    void StartMemoryTracking();

    _Check_return_ HRESULT GetDefaultInheritedPropertyValue(_In_  KnownPropertyIndex nUserIndex, _Out_ CValue  *pValue);

    _Check_return_ HRESULT GetDefaultFocusVisualSolidColorBrush(
        _In_ bool forFocusVisualSecondaryBrush,
        _In_ XUINT32 color,
        _Outptr_ CSolidColorBrush** ppSolidColorBrush);

    _Check_return_ HRESULT GetDefaultTextBrush(
        _Outptr_ CBrush **ppBrush);

    _Check_return_ HRESULT GetTextSelectionGripperBrush(
        _Outptr_ CBrush **ppFillBrush);

    _Check_return_ HRESULT GetSystemTextSelectionBackgroundBrush(
        _Outptr_ CSolidColorBrush **ppBrush);

    CSolidColorBrush* GetSystemColorWindowBrush();

    CSolidColorBrush* GetSystemColorWindowTextBrush();

    CSolidColorBrush* GetSystemColorDisabledTextBrush();

    CSolidColorBrush* GetSystemColorHotlightBrush();

    _Check_return_ HRESULT GetSystemTextSelectionForegroundBrush(
        _Outptr_ CSolidColorBrush **ppBrush);

    _Check_return_ HRESULT GetTransparentBrush(
        _Outptr_ CBrush **ppBrush);

    CSolidColorBrush* GetSystemColorWindowBrushNoRef();

    CSolidColorBrush* GetSystemColorWindowTextBrushNoRef();

    CSolidColorBrush* GetSystemColorDisabledTextBrushNoRef();

// Accessors

    DWORD GetThreadID() final { return m_nThreadID; }

    _Check_return_ HRESULT ParsePropertyPath(
        _Inout_ CDependencyObject **ppTarget,
        _Outptr_ const CDependencyProperty** ppDP,
        _In_ const xstring_ptr_view& strPath,
        _In_opt_ const std::map<xstring_ptr, const CDependencyProperty*>& resolvedPathTypes = ResolvedDPMap());

    _Check_return_ HRESULT GetPropertyIndexForPropertyPath(
            _In_ CDependencyObject *pTarget,
            _In_ XUINT32 bParen,
            _In_ XNAME *pNameClass,
            _In_ XNAME *pNameProperty,
            _In_ const CClassInfo* pClass,
            _Out_ KnownPropertyIndex* pnPropertyIndex,
            _In_ const std::map<xstring_ptr, const CDependencyProperty*>& resolvedPathTypes);

     _Check_return_ HRESULT CreateObject(
        _In_ const CClassInfo* pClass,
        _In_ const xstring_ptr& strString,
        _Outptr_result_maybenull_ CDependencyObject **ppDO);

     _Check_return_ HRESULT CreateObject(
        _In_ const CClassInfo* pClass,
        _In_ CDependencyObject *pInDO,
        _Outptr_result_maybenull_ CDependencyObject **ppDO);

// Remember wether we're setting values from managed or not
    void SetValueFromManaged(_In_ CDependencyObject *pdo, _Out_opt_ CDependencyObject** ppPrevious = nullptr)
    {
        if (ppPrevious != nullptr)
        {
            *ppPrevious = m_pdoInSetValueFromManaged;
        }
        m_pdoInSetValueFromManaged = pdo;
    }
    bool IsSettingValueFromManaged(_In_ CDependencyObject *pdo)
    {return ((pdo != NULL) && (pdo == m_pdoInSetValueFromManaged));}


    bool HasActiveAnimations();

    void SkipFrames(int amountOfFramesToSkip) { m_framesToSkip = amountOfFramesToSkip; };

// TransitionTargets can only be created during OnBegin of a DynamicTimeline
    void SetAllowTransitionTargetCreation(_In_ bool allow) { m_allowTransitionTargetsToBeCreated = allow; }
    bool IsAllowingTransitionTargetCreations() { return m_allowTransitionTargetsToBeCreated; }

// Parse the xaml text and instantiate a xaml object

    _Check_return_ HRESULT LoadXaml(
        _In_ XUINT32 cBuffer,
        _In_reads_opt_(cBuffer) const XUINT8 *pBuffer,
        _Outptr_result_maybenull_ CDependencyObject **ppDependencyObject);


    _Check_return_ HRESULT GetResourceManager(_Outptr_ IPALResourceManager **ppResourceManager) final;

    _Check_return_ HRESULT ParseXaml(
        _In_ const Parser::XamlBuffer& buffer,
        _In_ bool bForceUtf16,
        _In_ bool bCreatePermanentNamescope,
        _In_ bool bRequiresDefaultNamespace,
        _Outptr_ CDependencyObject **ppDependencyObject,
        _In_ const xstring_ptr_view& strSourceAssemblyName = xstring_ptr(),
        _In_ bool bExpandTemplatesDuringParse = false,
        _In_ const xstring_ptr_view& strXamlResourceUri = xstring_ptr());

    _Check_return_ HRESULT ParseXamlWithEventRoot(
        _In_ const Parser::XamlBuffer& buffer,
        _In_ bool bForceUtf16,
        _In_ bool bCreatePermanentNamescope,
        _In_ bool bRequireDefaultNamespace,
        _Outptr_ CDependencyObject **ppDependencyObject,
        _In_opt_ IPALUri *pBaseUri = nullptr,
        _In_ const xstring_ptr_view& strSourceAssemblyName = xstring_ptr(),
        _In_ bool bExpandTemplatesDuringParse = false);

    _Check_return_ HRESULT ParseXamlWithExistingFrameworkRoot(
        _In_ const Parser::XamlBuffer& buffer,
        _In_ CDependencyObject *pExistingFrameworkRoot,
        _In_ const xstring_ptr_view& strSourceAssemblyName,
        _In_ const xstring_ptr_view& strXamlResourceUri,
        _In_ const bool expandTemplatesDuringParse,
        _Outptr_ CDependencyObject **ppDependencyObject);

    _Check_return_ HRESULT ResetCoreWindowVisualTree();

    // Set the source Xaml object to be viewed
        _Check_return_ HRESULT putVisualRoot(_In_opt_ CDependencyObject *pDependencyObject);

    _Check_return_ CDependencyObject *getVisualRoot();
    _Check_return_ CDependencyObject *GetPublicOrFullScreenRootVisual();
    _Check_return_ CDependencyObject *getRootScrollViewer();

    // Some elements that can be updated asynchronously need to check and update their dirty state on each rendering pass.
     _Check_return_ HRESULT AddChildForUpdate(_In_ CDependencyObject *pChild)
    {
        RRETURN(m_rgpChildrenForUpdate.push_back(pChild));
    }

     _Check_return_ HRESULT RemoveChildForUpdate(_In_ CDependencyObject *pChild)
    {
        UpdateStateChildrenVector::const_reverse_iterator rend = m_rgpChildrenForUpdate.rend();
        for (UpdateStateChildrenVector::const_reverse_iterator it = m_rgpChildrenForUpdate.rbegin(); it != rend; ++it)
        {
            if ((*it) == pChild)
            {
                IFC_RETURN(m_rgpChildrenForUpdate.erase(it));
                break;
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT RegisterRedirectionElement(_In_ CUIElement *pRedirectionElement);
    _Check_return_ HRESULT UnregisterRedirectionElement(_In_ CUIElement *pRedirectionElement);

    _Check_return_ HRESULT RegisterGripper(_In_ IGripper *pGripper);
    _Check_return_ HRESULT UnregisterGripper(_In_ IGripper *pGripper);

    void GetWindowSize(
        _In_opt_ CDependencyObject *pObject,
        _Out_opt_ XSIZE_PHYSICAL* physicalSize,
        _Out_opt_ XSIZE_LOGICAL* logicalSize);

// Get Error Service instance for this core service.

    _Check_return_ HRESULT getErrorService(_Out_ IErrorService **ppErrorService);

    _Check_return_ HRESULT ReportUnhandledError(HRESULT errorHR);

// Have certain types of items entered the tree

    void BeginWatch()               { m_fWatch = 0; }
    void KeepWatch(XINT32 flag)     { m_fWatch |= flag; }
    XINT32 CheckWatch(XINT32 flag)  { return m_fWatch & flag; }

    _Check_return_ HRESULT NWDrawMainTree(
        _In_ CWindowRenderTarget* pIRenderTarget,
        bool forceRedraw,
        _Out_ bool *pFrameDawn);

    // Draw an element and its children with the supplied hw walk.
    _Check_return_ HRESULT NWDrawTree(
        _In_ HWWalk *pHWWalk,
        _In_ CWindowRenderTarget* pIRenderTarget,
        _In_opt_ VisualTree *pVisualTree,
        bool forceRedraw,
        _Out_ bool *pFrameDrawn);

    // Update internal state without drawing a frame.  tickForDrawing is TRUE if the frame will be drawn afterwards.
    _Check_return_ HRESULT Tick(
        bool tickForDrawing,
        _Out_opt_ bool *checkForAnimationComplete,
        _Out_opt_ bool *hasActiveFiniteAnimations
        );

    // Per-frame callback into user code (synchronous)
    _Check_return_ HRESULT CallPerFrameCallback(XFLOAT time);

    _Check_return_ HRESULT EnqueueSurfaceContentsLostEvent();
    _Check_return_ HRESULT RaiseQueuedSurfaceContentsLostEvent();

    _Check_return_ HRESULT AddSurfaceImageSource(CSurfaceImageSource *pItem);
    _Check_return_ HRESULT RemoveSurfaceImageSource(CSurfaceImageSource *pItem);

    _Check_return_ HRESULT AddVirtualSurfaceImageSource(CVirtualSurfaceImageSource *pItem);
    _Check_return_ HRESULT RemoveVirtualSurfaceImageSource(CVirtualSurfaceImageSource *pItem);
    _Check_return_ HRESULT VirtualSurfaceImageSourcePerFrameWork(_In_ const XRECTF_RB *pWindowBounds);

    VisibleBoundsMap* GetVisibleBoundsMap() const
    {
        return m_pVisibleBoundsMap;
    }

    // hit-testing
    _Check_return_ HRESULT HitTest(
        XPOINTF ptHit,
        _In_opt_ CDependencyObject *pHitTestRoot,
        _Outptr_result_maybenull_ CDependencyObject **ppVisualHit,
        bool hitDisabled = FALSE);

    _Check_return_ HRESULT GetTextElementBoundingRect(
        _In_ CDependencyObject *pElement,
        _Out_ XRECTF *pRectBound,
        _In_ bool ignoreClip = false);

    _Check_return_ HRESULT HitTestLinkFromTextControl(
        _In_ XPOINTF ptHit,
        _In_ CDependencyObject *pLinkContainer,
        _Outptr_ CDependencyObject **ppLink);

// InputManager support
    _Check_return_ HRESULT ProcessInput(
         _In_ InputMessage *pMessage,
         _In_opt_ CContentRoot* contentRoot,
         _Out_ XINT32 *fHandled);

    void RequestReplayPreviousPointerUpdate();
    void ReplayPreviousPointerUpdate();

    _Check_return_ HRESULT PauseDCompAnimationsOnSuspend();
    _Check_return_ HRESULT ResumeDCompAnimationsOnResume();

    void SetIsSuspended(bool isSuspended);
    void SetIsRenderEnabled(bool value);

    void ForceDisconnectRootOnSuspend(bool forceDisconnectRootOnSuspend)
    {
        m_forceDisconnectRootOnSuspend = forceDisconnectRootOnSuspend;
    }

    _Check_return_ HRESULT WaitForCommitCompletion();

    // Functions for telemetry data
    static const wchar_t* GetAppSessionId();
    static const wchar_t* GetAppId();
    static DWORD GetProcessId();

    _Check_return_ HRESULT ProcessTouchInteractionCallback(
        _In_ const xref_ptr<CUIElement> &element,
        _In_ TouchInteractionMsg *message);

    _Check_return_ HRESULT GetAdjustedPopupRootForElement(_In_opt_ CDependencyObject *pObject, _Outptr_result_maybenull_ CPopupRoot **ppPopupRoot);

    _Check_return_ HRESULT GetVisualDiagnosticsRoot(_Outptr_result_maybenull_ CGrid **ppVisualDiagnosticsCollection);

    _Check_return_ HRESULT IsObjectAnActiveRootVisual(_In_ CDependencyObject *pObject, _Out_ bool *pIsActiveRoot);
    _Check_return_ HRESULT IsObjectAnActivePublicRootVisual(_In_ CDependencyObject *pObject, _Out_ bool *pIsActiveRoot);
    _Check_return_ HRESULT IsObjectAnActivePopupRoot(_In_ CDependencyObject *pObject, _Out_ bool *pIsActivePopupRoot);
    _Check_return_ CRootVisual *GetMainRootVisual() { if (m_pMainVisualTree) return m_pMainVisualTree->GetRootVisual(); else return NULL;}
    CDependencyObject* GetRootForElement(_In_ CDependencyObject* dependencyObject);
    _Check_return_ CPopupRoot* GetMainPopupRoot() { if (m_pMainVisualTree) return m_pMainVisualTree->GetPopupRoot(); else return NULL; }
    _Check_return_ CFullWindowMediaRoot* GetMainFullWindowMediaRoot() { if (m_pMainVisualTree) return m_pMainVisualTree->GetFullWindowMediaRoot(); else return NULL; }
    _Check_return_ CRenderTargetBitmapRoot* GetMainRenderTargetBitmapRoot() { if (m_pMainVisualTree) return m_pMainVisualTree->GetRenderTargetBitmapRoot(); else return NULL; }
    CXamlIslandRootCollection* GetXamlIslandRootCollection() { if (m_pMainVisualTree) return m_pMainVisualTree->GetXamlIslandRootCollection(); else return nullptr; }
    VisualTree* GetMainVisualTree();

    _Check_return_ HRESULT StartApplication(_In_ CApplication *pApplication);
    _Check_return_ HRESULT RaisePendingLoadedRequests();

// Get the default template for ContentPresenter
    const xref_ptr<CDisplayMemberTemplate>& GetDefaultContentPresenterTemplate();

// Get time manager
    CTimeManager *GetTimeManager() const { return m_pTimeManager; }

// Get Input manager
    CInputServices *GetInputServices() const { return m_inputServices.get(); }
    _Check_return_ HRESULT TryGetPrimaryPointerLastPosition(_Out_ XPOINTF *pLastPosition, _Out_ bool *pSucceeded);

// Get event manager
    CEventManager *GetEventManager() const { return m_pEventManager; }

    // Get the Print Root
    _Check_return_ CPrintRoot* GetPrintRoot() const { if (m_pMainVisualTree) return m_pMainVisualTree->GetPrintRoot(); else return NULL; }
    bool IsPrinting();
    void SetIsPrinting(bool val);

    bool IsHolographic() const;
    void SetIsHolographicOverride(bool isHolographicOverrideSet) { m_isHolographicOverrideSet = isHolographicOverrideSet; }

    _Check_return_ CTransitionRoot* GetTransitionRootForElement(_In_ CDependencyObject *pDO) const { return m_pMainVisualTree->GetTransitionRootForElement(pDO); }

    void SetCustomResourceLoader(_In_opt_ ICustomResourceLoader *pResourceLoader);
    _Check_return_ ICustomResourceLoader* GetCustomResourceLoader();

    void SetIsUsingGenericXamlFilePath(_In_ bool bIsUsingGenericXamlFilePath)
    {
        m_bIsUsingGenericXamlFilePath = bIsUsingGenericXamlFilePath;
    }

    bool GetIsUsingGenericXamlFilePath()
    {
        return m_bIsUsingGenericXamlFilePath;
    }

    // For access to IXcpHostSite
    void RegisterHostSite(_In_ IXcpHostSite *pHostSite);
    IXcpHostSite *GetHostSite() final { return m_pHostSite; }
    void UnregisterHostSite();

// For access to IXcpBrowserHost
    _Check_return_ HRESULT RegisterBrowserHost(_In_ IXcpBrowserHost *pBrowserHost);

// Null out the reference to BH
    _Check_return_ HRESULT UnregisterBrowserHost();

// Download services

    _Check_return_ HRESULT RegisterDownloadSite(_In_ ICoreServicesSite *pSite);
    void UnregisterDownloadSite();

    HRESULT _Check_return_ GetSystemGlyphTypefaces(
        _Outptr_ CDependencyObject **pDo);

// Memory manager
    void AttachMemoryManagerEvents();
    void DetachMemoryManagerEvents();

    void DetachApartmentEvents();

// CLR services
    void CLR_AsyncReleaseNativeObject(_In_ CDependencyObject* const pDO);
    void CLR_CleanupNativePeers();

    _Check_return_ HRESULT CLR_FireEvent(
        _In_ CDependencyObject *pListener,
        _In_ EventHandle hEvent,
        _In_ CDependencyObject* pSender,
        _In_ CEventArgs* pArgs,
        _In_ XUINT32 flags = 0
        ) ;

    _Check_return_ HRESULT ShouldFireEvent(
        _In_ CDependencyObject *pListener,
        _In_ EventHandle hEvent,
        _In_ CDependencyObject* pSender,
        _In_ CEventArgs* pArgs,
        _In_ XINT32 flags,
        _Out_ XINT32* pfSupported);

    _Check_return_ HRESULT ApplicationStartupEventComplete(
        ) ;

// Uri caching for ResourceDictionary
    xstringmap<bool>& GetResourceDictionaryUriCache();

// Returns the Identity of current core
    XUINT32 GetIdentity(){ return m_objIdentity;}

    _Check_return_ HRESULT SetCurrentApplication(_In_ CApplication *pApplication);

    // Get/set the window render target currently attached to this object.
    void NWSetWindowRenderTarget(_In_opt_ CWindowRenderTarget *pWindowRenderTarget);

    _Ret_maybenull_ CWindowRenderTarget *NWGetWindowRenderTarget() final;

    _Ret_maybenull_ CD3D11Device* GetGraphicsDevice();

    void ClearCompositorTreeHostQueues();

    void SetDirtyState(bool fDirtyState) { m_fDirtyState = fDirtyState; }
    bool GetDirtyState() { return m_fDirtyState;}

    bool InRenderWalk() { return m_fInRenderWalk; }

    bool DbgIsThreadingAssertEnabled()
    {
        return m_fDbgEnableThreadingAssert;
    }

    void
    OnWindowSizeChanged(
        _In_ XUINT32 windowWidth,
        _In_ XUINT32 windowHeight,
        _In_ XUINT32 layoutBoundsWidth,
        _In_ XUINT32 layoutBoundsHeight
        );

    // Sets a callback function which is called after every UI thread tick (used by tests).
    void SetPostTickCallback(_In_opt_ std::function<void()> callback);

    _Check_return_ HRESULT ConfigureNumberSubstitution();

    _Check_return_ HRESULT OnDebugSettingsChanged();

    IPALWorkItemFactory* GetWorkItemFactory() const { return m_pWorkItemFactory; }

// UIAutomation Accessibility
    _Check_return_ HRESULT UIAClientsAreListening(_In_ UIAXcp::APAutomationEvents eAutomationEvent);
    _Check_return_ HRESULT UIARaiseAutomationEvent(_In_ CAutomationPeer *pAP,
                                                  _In_ UIAXcp::APAutomationEvents eAutomationEvent);
    _Check_return_ HRESULT UIARaiseAutomationPropertyChangedEvent(_In_ CAutomationPeer *pAP,
                                                                 _In_ UIAXcp::APAutomationProperties eAutomationProperty,
                                                                 _In_ const CValue& oldValue,
                                                                 _In_ const CValue& newValue);
    _Check_return_ HRESULT UIARaiseFocusChangedEventOnUIAWindow(_In_ CDependencyObject* sender);
    _Check_return_ HRESULT UIARaiseTextEditTextChangedEvent(_In_ CAutomationPeer *pAP,
        _In_ UIAXcp::AutomationTextEditChangeType eAutomationTextEditChangeType,
        _In_ CValue *pChangedData);
    _Check_return_ HRESULT UIARaiseNotificationEvent(
        _In_ CAutomationPeer* ap,
        UIAXcp::AutomationNotificationKind notificationKind,
        UIAXcp::AutomationNotificationProcessing notificationProcessing,
        _In_opt_ xstring_ptr displayString,
        _In_ xstring_ptr activityId);
    CUIAWindow* GetUIAWindowForElementRootNoRef(_In_ CDependencyObject* root);

    void RegisterForStructureChangedEvent(
        _In_ CAutomationPeer* automationPeer,
        _In_ CAutomationPeer* parent,
        AutomationEventsHelper::StructureChangedType type);

    _Check_return_ XUINT32 GetNextRuntimeId();

// Queues a commit to the main DComp device
    _Check_return_ HRESULT RequestMainDCompDeviceCommit(RequestFrameReason reason = RequestFrameReason::RequestCommit);

// Internal methods

    void ResetGeometryBuilder(XUINT32 nIndex)
    {
        if (nIndex < 2)
        {
            m_bBuilderReady[nIndex] = FALSE;
        }
    }

    _Check_return_ HRESULT GetGeometryBuilder(
        XUINT32 nIndex,
        _Outptr_ CGeometryBuilder **ppBuilder,
        bool fGetPathGeometryBuilder = false
        );

    _Check_return_ HRESULT GetGlyphPathBuilder(_Outptr_ CGlyphPathBuilder **ppBuilder);

    _Check_return_ HRESULT GetTextCore(_Outptr_result_maybenull_ CTextCore **ppTextCore);

    _Check_return_ CLayoutManager* GetMainLayoutManager()
    {
        if (m_pMainVisualTree)
        {
            return m_pMainVisualTree->GetLayoutManager();
        }
        else
        {
            return NULL;
        }
    }

    CUIElement *GetLastLayoutExceptionElement();
    void SetLastLayoutExceptionElement(_In_opt_ CUIElement *pElement);

    _Check_return_ HRESULT AddDownloadRequest(
            _In_ IPALDownloadRequest* pDownloadRequest,
            _Outptr_result_maybenull_ IPALAbortableOperation **ppIAbortableDownload,
            _In_opt_ IPALUri *pPreferredBaseUri = NULL);

// The following helpers process both queued download and streaming requests

    _Check_return_ HRESULT ProcessDownloadRequests(_In_ XINT32 bClear);

    _Check_return_ HRESULT RemoveDownloadRequest(_In_ DREQUEST* pRequest);

    void ReportAsyncErrorToBrowserHost( );

//  Helpers to access the deferred media event queue

    _Check_return_ HRESULT CreateMediaQueue(_In_ IMediaQueueClient *pClient, IMediaQueue** ppQueue) final;
    _Check_return_ HRESULT ProcessMediaEvents(bool fShuttingDown = false);

    _Check_return_ HRESULT SetWantsRendering(bool value);

    bool GetWantsRendering() const { return m_fWantsRenderingEvent; }

    void SetWantsCompositionTargetRenderedEvent(bool value)
    {
        m_fWantsCompositionTargetRenderedEvent = value;
    }

    bool GetWantsCompositionTargetRenderedEvent() const { return m_fWantsCompositionTargetRenderedEvent; }

    CDeployment* GetDeployment()
    {
        return m_pDeploymentTree;
    }

    // Capability to clear and re-initialize the new parser's XamlSchemaContext
    _Check_return_ HRESULT RefreshXamlSchemaContext();

    _Check_return_ HRESULT ClearDefaultLanguageString();

    _Check_return_ HRESULT static CallbackEventListener(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    IPALUri* GetBaseUriNoRef();

    _Check_return_ HRESULT GetFullClassName(
        _In_ const xstring_ptr_view& strClass,
        _Out_ xstring_ptr* pstrFullName);


    // Creates a compositor scheduler to be shared by the render targets
    _Check_return_ HRESULT CreateCompositorScheduler(
        _In_ WindowsGraphicsDeviceManager *pGraphicsDeviceManager,
        _Outptr_ CompositorScheduler** compositorScheduler);

    // Create a frame scheduler for the UI thread.
    void CreateUIThreadScheduler(
        _In_ IXcpDispatcher *pIDispatcher,
        _In_ CompositorScheduler* compositorScheduler,
        _Outptr_ ITickableFrameScheduler **ppUIThreadScheduler);

    // Creates a render target to be used by the compositor
    _Check_return_ HRESULT CreateWindowRenderTarget(
        _In_ WindowsGraphicsDeviceManager *pGraphicsDeviceManager,
        _In_ CompositorScheduler* compositorScheduler,
        _In_ WindowsPresentTarget *initialPresentTarget);

    // Register an object for suspend/resume notification
    // The implementation *does not* take a reference, the
    // caller should Unregister the listener before destroying it
    // (or the object can Unregister in its dtor)
    _Check_return_ HRESULT RegisterPLMListener(
        _In_ IPLMListener*pListener
        );

    // Unregister an object for suspend/resume notification
    _Check_return_ HRESULT UnregisterPLMListener(
        _In_ IPLMListener *pListenver
        );

    _Check_return_ HRESULT OnSuspend(bool isTriggeredByResourceTimer, bool allowOfferResources);
    _Check_return_ HRESULT OnResume();
    void OnLowMemory();

    _Check_return_ HRESULT DisconnectRoot(bool allowOfferResources, bool wasSuspend);
    _Check_return_ HRESULT ReclaimResources();

    static _Check_return_ HRESULT OnBackgroundResourceTimeout(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    _Check_return_ bool IsInResetVisualTree() { return m_bInResetVisualTree; }

    _Check_return_ HRESULT NotifyThemeChange();

    _Check_return_ HRESULT UpdateColorAndBrushResources(bool* pWasFirstCreateForResources);
    _Check_return_ HRESULT AddFocusVisualKindIsRevealToSystemColorsResources();
    _Check_return_ HRESULT SetFocusVisualKindIsReveal(bool isReveal);

    void EnsureBrush(xref_ptr<CSolidColorBrush>& brush, _In_ const xstring_ptr_view& key);
    void ResetThemedBrushes();

    _Check_return_ HRESULT GetSystemColorsResources(_Out_ CResourceDictionary** ppSystemColorsResources);

    void SetThemeResources(_In_opt_ CResourceDictionary *pThemeResources);

    CResourceDictionary* GetThemeResources();

    bool HasThemeEverChanged() const
    {
        return m_hasThemeEverChanged;
    }

    bool IsSwitchingTheme() const
    {
        return m_fIsSwitchingTheme;
    }

    void SetIsSwitchingTheme(bool isSwitchingTheme)
    {
        m_fIsSwitchingTheme = isSwitchingTheme;
    }

    bool IsLoadingGlobalThemeResources() const { return m_isLoadingGlobalThemeResources; }

    void SetIsLoadingGlobalThemeResources(bool value) { m_isLoadingGlobalThemeResources = value; }

    FrameworkTheming* GetFrameworkTheming()
    {
        return m_spTheming.get();
    }

    ThemeWalkResourceCache* GetThemeWalkResourceCache()
    {
        return m_themeWalkResourceCache.get();
    }

    Theming::Theme GetRequestedThemeForSubTree() const
    {
        return m_requestedThemeForSubTree;
    }

    void SetRequestedThemeForSubTree(_In_ Theming::Theme requestedTheme);

    bool IsThemeRequestedForSubTree() const;

    bool IsGeneratingBinaryXaml() const { return m_isGeneratingBinaryXaml; }

    void SetIsGeneratingBinaryXaml(bool value) { m_isGeneratingBinaryXaml = value; }

    _Check_return_ HRESULT ExecuteOnUIThread(_In_ IPALExecuteOnUIThread *pExecutor, const ReentrancyBehavior reentrancyBehavior);

    _Check_return_ HRESULT UpdateDirtyState();

    IAsyncImageFactory* GetImageFactory();
    ImageProvider* GetImageProvider();
    _Ret_maybenull_ SurfaceCache* GetSurfaceCache();
    _Check_return_ HRESULT ClearMainRootImageCaches();

    CResourceDictionary* GetApplicationResourceDictionary();

    _Check_return_ HRESULT SetLayoutCompletedNeeded(const LayoutCompletedNeededReason reason);

    bool IsAnimationTrackingEnabled();

    void AnimationTrackingScenarioBegin(_In_ AnimationTrackingScenarioInfo* pScenarioInfo);

    void AnimationTrackingScenarioReference(XUINT64 uniqueKey);

    void AnimationTrackingScenarioUnreference(XUINT64 uniqueKey);

    void SetSizeChangedNotification(bool value, XDWORD applicationViewState);

    bool IsXamlVisible() const;
    _Check_return_ HRESULT SetWindowVisibility(bool value, bool isStartingUp, bool freezeDWMSnapshotIfHidden);
    // Called in response to a visibility change, either on the core window or on an island.
    _Check_return_ HRESULT OnVisibilityChanged(bool isStartingUp, bool freezeDWMSnapshotIfHidden);

    // Note: Passing "false" forces Xaml to invisible and triggers soft suspend right away. Passing "true" removes the
    // override and defaults to normal product behavior.
    void ForceXamlInvisible_TestHook(bool isVisible);

    bool IsRenderEnabled()
    {
        return m_isRenderEnabled;
    }
    _Check_return_ HRESULT EnableRender(bool enable);

    _Check_return_ HRESULT LookupThemeResource(
        _In_ const xstring_ptr_view& strKey,
        _Outptr_ CDependencyObject **ppValue);

    _Check_return_ HRESULT LookupThemeResource(_In_ Theming::Theme theme, _In_ const xstring_ptr_view& key, _Outptr_ CDependencyObject **ppValue);

    void SetTransparentBackground(bool isTransparent);

    bool IsSuspended() { return m_isSuspended; }
    bool IsDeviceLost();

    bool IsRenderingFrames() const { return (m_isRenderEnabled || m_isFirstFrameAfterAppStart) && !m_isSuspended; }

    _Check_return_ HRESULT ReleaseDeviceResources(bool releaseDCompDevice, bool isDeviceLost);

    WUComp::ICompositor* GetCompositor() const;
    void EnsureCompositionIslandCreated(_In_ wuc::ICoreWindow* const coreWindow) const;

// CONTENT-TODO: Lifted IXP doesn't support OneCoreTransforms UIA yet.
#if false
    UINT64 GetCoreWindowCompositionIslandId();
#endif

    void AddXamlIslandRoot(_In_ CXamlIslandRoot* xamlIslandRoot);
    void RemoveXamlIslandRoot(_In_ CXamlIslandRoot* xamlIslandRoot);
    void UpdateXamlIslandRootTargetSize(_In_ CXamlIslandRoot* xamlIslandRoot);

#pragma region IXamlTestHooks

    _Check_return_ HRESULT SimulateDeviceLost(bool resetVisuals, bool resetDManip);
    void SetDCompDeviceLeakDetectionEnabled(bool enableLeakDetection) { m_isDCompLeakDetectionEnabled = enableLeakDetection; }

    void GetDCompDevice(
        _Outptr_ IDCompositionDesktopDevicePartner **ppDCompDevice
        ) const;

    _Check_return_ HRESULT SetWindowSizeOverride(
        _In_ const XSIZE *pWindowSize,
        _In_ XHANDLE hwnd,
        float testOverrideScale);

    void OverrideTrimImageResourceDelay(bool enabled);

    _Check_return_ HRESULT SetSystemFontCollectionOverride(_In_opt_ IDWriteFontCollection* pFontCollection);

    void SetTimeManagerClockOverrideConstant(double newTime);
    HRESULT CleanUpAfterTest();
    _Check_return_ HRESULT ShutdownToIdle();
    void InitializeContextObjects();
    void ClearContextObjects();
    _Check_return_ HRESULT InitializeFromIdle();
    _Check_return_ HRESULT EnsureBackgroundTimer();
    void CheckForLeaks();

    _Check_return_ HRESULT CheckMemoryUsage(bool simulateLowMemory);
    DCompTreeHost* GetDCompTreeHost();

    void SetThreadingAssertOverride(bool enable);

    xref_ptr<CLayoutTransitionElement> AddTestLTE(_In_ CUIElement *lteTarget, _In_ CUIElement *lteParent, bool parentIsRootVisual, bool parentIsPopupRoot, bool isAbsolutelyPositioned);
    void RemoveTestLTE(_In_ CUIElement *lte);
    void ClearTestLTEs();
    void ScheduleWaitForAnimatedFacadePropertyChanges(int count);

    void SetCanTickWithNoContent(bool canTickWithNoContent);

#pragma endregion

    _Ret_maybenull_ HWTextureManager *GetHWTextureManagerNoRef();

    uint32_t GetMaxTextureSize();
    MaxTextureSizeProvider& GetMaxTextureSizeProvider();

    bool AtlasRequest(uint32_t width, uint32_t height, PixelFormat pixelFormat);
    AtlasRequestProvider& GetAtlasRequestProvider();

    void PegNoRefCoreObjectWithoutPeer(_In_ CDependencyObject *pObject);
    void UnpegNoRefCoreObjectWithoutPeer(_In_ CDependencyObject *pObject);
    void ReferenceTrackerWalkOnCoreGCRoots(_In_ DirectUI::EReferenceTrackerWalkType walkType);

    void EnsureOleIntialized();

private:
    _Ret_maybenull_ HWWalk *GetHWWalk();

    _Ret_maybenull_ CSwapChainBackgroundPanel* FindSCBP() const;

    _Check_return_ HRESULT ResetState();
    _Check_return_ HRESULT Initialize();

    _Check_return_ HRESULT GetDirectManipulationChanges(
        _Inout_ xvector<CCompositorDirectManipulationViewport *>& directManipulationViewports,
        _Out_ bool *pViewportsChanged
        );

    void InvalidateRedirectionElements();

    void InitCoreWindowContentRoot();

    _Check_return_ HRESULT UpdateGripperVisibility();

    _Check_return_ HRESULT RenderWalk(
        _In_ HWWalk *pHWWalk,
        _In_ CWindowRenderTarget *pIRenderTarget,
        _In_ CUIElement *pVisualRoot,
        bool forceRedraw,
        bool hasUIAClientsListeningToStructure,
        _Out_ bool* canSubmitFrame);

    _Check_return_ HRESULT SubmitPrimitiveCompositionCommands(
        _In_ CWindowRenderTarget *pIRenderTarget,
        _In_opt_ CUIElement *pVisualRoot,
        _In_opt_ IPALClock *pClock,
        _In_ XFLOAT rFrameStartTime);

    _Check_return_ HRESULT SubmitRenderCommandsToCompositor(
        _In_opt_ HWCompTreeNode *pRootCompNode,
        _In_ CTimeManager *pTimeManager);

    _Check_return_ HRESULT CheckForLostSurfaceContent();

    void HandleVirtualSurfaceImageSourceLostResources();

    _Check_return_ HRESULT CleanupDeviceRelatedResources(_In_ bool cleanupDComp, _In_ bool isDeviceLost);
    _Check_return_ HRESULT BuildDeviceRelatedResources();
    void CleanupDeviceRelatedResourcesOnSurfaceWrappers();
    void TrimMemoryOnSurfaceWrappers();

public:
    CRenderTargetBitmapManager* GetRenderTargetBitmapManager() const { return m_pRenderTargetBitmapManager; }

public:

    void SetInitializationType(InitializationType newType)
    {
        FAIL_FAST_ASSERT(m_initializationType == InitializationType::Normal || newType == InitializationType::Normal);
        m_initializationType = newType;
    }
    InitializationType GetInitializationType() const { return m_initializationType; }

    bool UseUiaOnMainWindow() const { return GetInitializationType() != InitializationType::IslandsOnly; }
    bool HandleInputOnMainWindow() const { return GetInitializationType() != InitializationType::IslandsOnly; }
    bool BackButtonSupported() const { return GetInitializationType() != InitializationType::IslandsOnly; }

    // Perform deferred operation for each entry in  m_deferredAnimationOperationQueue
    _Check_return_ HRESULT FlushDeferredAnimationOperationQueue(bool doDeferredOperation = true);
    // Queue an animation operation to be performed at a later time
    void EnqueueDeferredAnimationOperation(_In_ std::shared_ptr<CDeferredAnimationOperation> deferredAnimationOperation);
    // Remove from the queue any animation operations that match the set of DO/DP pairs
    void RemoveMatchingDeferredAnimationOperations(_In_ CDependencyObject* targetObject, KnownPropertyIndex targetPropertyIndex);

    // Returns true when we are resetting the visual tree
    _Check_return_ bool IsShuttingDown() { return m_bIsShuttingDown;}
    bool IsTearingDownTree() { return m_bIsShuttingDown || m_isTearingDownIsland; }
    void SetShuttingDown(_In_ bool bState) { m_bIsShuttingDown = bState;}

    // Returns true when we are in the CoreServices destructor
    bool IsDestroyingCoreServices() { return m_bIsDestroyingCoreServices; }

    bool IsTransparentBackground() const { return m_isTransparentBackground; }
    bool GetIsTextPerformanceVisualizationEnabled() const;
    _Check_return_ HRESULT SetIsTextPerformanceVisualizationEnabled(bool enabled);

    bool IsInBackgroundTask() const { return GetInitializationType() == InitializationType::BackgroundTask; }

    void IncrementPendingDecodeCount()
    {
        if (PAL_InterlockedIncrement(&m_cPendingDecodes) == 1)
        {
            SetImageDecodingIdleEventSignaledStatus(FALSE);
        }
    }

    void DecrementPendingDecodeCount()
    {
        ASSERT(m_cPendingDecodes > 0);
        if (PAL_InterlockedDecrement(&m_cPendingDecodes) == 0)
        {
            SetImageDecodingIdleEventSignaledStatus(TRUE);
        }
    }
    XUINT32 GetPendingDecodeCount() { return m_cPendingDecodes; }

    void IncrementPendingFontDownloadCount()
    {
        if (PAL_InterlockedIncrement(&m_cPendingFontDownloads) == 1)
        {
            SetFontDownloadsIdleEventSignaledStatus(FALSE);
        }
    }

    void DecrementPendingFontDownloadCount()
    {
        ASSERT(m_cPendingFontDownloads > 0);
        if (PAL_InterlockedDecrement(&m_cPendingFontDownloads) == 0)
        {
            SetFontDownloadsIdleEventSignaledStatus(TRUE);
        }
    }

    void IncrementPendingImplicitShowHideCount();
    void DecrementPendingImplicitShowHideCount();

    void IncrementActiveFacadeAnimationCount();
    void DecrementActiveFacadeAnimationCount();

    void IncrementActiveBrushTransitionCount();
    void DecrementActiveBrushTransitionCount();

    void DecrementPendingAnimatedFacadePropertyChangeCount();

    bool InvisibleHitTestMode() { return m_bInvisibleHitTestMode; }
    void SetInvisibleHitTestMode(bool bState) { m_bInvisibleHitTestMode = bState; }

    std::shared_ptr<XamlSchemaContext> GetSchemaContext();

    // IParserCoreServices
    _Check_return_
    HRESULT GetXamlNodeStreamCacheManager(_Out_ std::shared_ptr<XamlNodeStreamCacheManager>& spXamlNodeStreamCacheManager) final;

    _Check_return_
        HRESULT GetParserErrorService(_Out_ IErrorService **ppErrorService) final;

    _Check_return_
        HRESULT LoadXamlResource(
        _In_            IPALUri       *pUri,
        _Out_           bool          *pfHasBinaryFile,
        _Outptr_        IPALMemory    **ppMemory,
        _Out_opt_       Parser::XamlBuffer* pBuffer,
        _Outptr_opt_ IPALUri          **ppPhysicalUri = nullptr
        );

    _Check_return_
        HRESULT TryLoadXamlResource(
        _In_            IPALUri       *pUri,
        _Out_           bool          *pfHasBinaryFile,
        _Outptr_        IPALMemory    **ppMemory,
        _Out_opt_       Parser::XamlBuffer* pBuffer,
        _Outptr_opt_ IPALUri          **ppPhysicalUri = nullptr
        );

    _Check_return_
        HRESULT TryLoadXamlResourceHelper(
        _In_            IPALUri       *pUri,
        _Out_           bool          *pfHasBinaryFile,
        _Outptr_        IPALMemory    **ppMemory,
        _Out_opt_       Parser::XamlBuffer* pBuffer,
        _Outptr_opt_ IPALUri          **ppPhysicalUri = nullptr
        );

    _Check_return_ bool IsVisibilityToggled() { return m_bVisibilityToggled; }
    void SetVisibilityToggled(_In_ bool bState) { m_bVisibilityToggled = bState; }

    _Check_return_ HRESULT EnsureDeviceLostListener();
    void ReleaseDeviceLostListener();
    _Check_return_ HRESULT DetermineDeviceLost(_Out_opt_ bool *pIsDeviceLost);
    void HandleDeviceLost(_Inout_ HRESULT *pResult);
    _Check_return_ HRESULT RecoverFromDeviceLost();

    void RegisterSurfaceWrapperForDeviceCleanup(_In_ ImageSurfaceWrapper *pSurfaceWrapper);
    void UnregisterSurfaceWrapperForDeviceCleanup(_In_ ImageSurfaceWrapper *pSurfaceWrapper);

    void AddDeviceListener(_In_ DeviceListener *deviceListener);
    void RemoveDeviceListener(_In_ DeviceListener *deviceListener);

    void AddDisplayListener(_In_ DisplayListener *displayListener);
    void RemoveDisplayListener(_In_ DisplayListener *displayListener);

    void AddRenderStateListener(_In_ RenderStateListener *renderStateListener);
    void RemoveRenderStateListener(_In_ RenderStateListener *renderStateListener);

    _Check_return_ HRESULT InvalidateImplicitStylesOnRoots(_In_opt_ CResourceDictionary *pOldResources);
    _Check_return_ HRESULT UpdateImplicitStylesOnRoots(_In_opt_ CStyle *pOldStyle, _In_opt_ CStyle *pNewStyle, _In_ bool forceUpdate);

    // Note: this is just a shim measure until the Jupiter ResourceManager is implemented.  I would
    // expect that when the true ResourceManager is implemented, this will not be called directly.
    _Check_return_ HRESULT ShimGetResourcePropertyBagRaw(
        _In_ const xstring_ptr& spKeyString,
        _In_ const xref_ptr<IPALUri>& spBaseUri,
        _Out_ std::vector<std::pair<xstring_ptr, xstring_ptr>>& propertyBag);

    // Used for testing. Overrides the resource property bag normally retrieved from the resource manager.
    void OverrideResourcePropertyBag(_In_opt_ std::map<std::wstring, std::vector<std::pair<std::wstring, std::wstring>>>* propertyBag);

    _Check_return_ HRESULT EnqueueImageDecodeRequest(_In_ CImageSource* pBitmapImage, XUINT32 width, XUINT32 height, bool retainPlaybackState);
    _Check_return_ HRESULT RemoveImageDecodeRequest(_In_ CImageSource* pBitmapImage);
    _Check_return_ HRESULT FlushImageDecodeRequests();

    _Check_return_ HRESULT StartTrackingImageForRenderWalk(_In_ CImageSource* pBitmapImage);
    _Check_return_ HRESULT StopTrackingImageForRenderWalk(_In_ CImageSource* pBitmapImage);
    _Check_return_ HRESULT ProcessTrackedImages();

    void StartTrackingAnimatedImage(_In_ CImageSource* bitmapImage);
    void StopTrackingAnimatedImage(_In_ CImageSource* bitmapImage);

    void EnqueueImageUpdateRequest(_In_ ImageSurfaceWrapper* pImageSurfaceWrapper);
    void RemoveImageUpdateRequest(_In_ ImageSurfaceWrapper* pImageSurfaceWrapper);
    void FlushPendingImageUpdates();
    void ClearPendingImageUpdates();

    static HINSTANCE GetInstanceHandle();

    // Mostly useful for optimizations.  If there are no XamlIslands now, there could still
    // be some in the future.
    bool HasXamlIslands() const;

    _Check_return_ HRESULT GetPointerInfoFromPointerPoint(
            _In_ ixp::IPointerPoint* pointerPoint,
            _Out_ PointerInfo* pointerInfoResult);

    const bool IsFrameAfterVisualTreeReset() { return m_isFrameAfterVisualTreeReset; }
    const bool HasRenderedFrame() { return m_uFrameNumber > 1; }

    Diagnostics::ResourceLookupLogger* GetResourceLookupLogger();

public:
    XUINT32                  m_uFrameNumber;
    int                      m_framesToSkip = 0;

    // If the render walk encountered an error while rendering some element, it can choose to request a subsequent
    // frame to try rendering the element again. In that case, we don't want to submit the frame where we encountered
    // an error, because the problematic element has no content and will flicker on screen. We also don't want to
    // repeatedly skip submitting frames because an element is constantly failing to render, so we remember that we
    // already skipped submitting a frame here. If we've already skipped a frame, we're not going to skip submitting
    // the next one.
    //
    // This mechanism is not necessary if the render walk was smarter about releasing old content. See
    // Task 26308130: Render walk should better tolerate errors - don't release old content until new content is ready
    bool                     m_skippedSubmittingFrameDueToRenderingError { false };

    bool                     m_allowTransitionTargetsToBeCreated;

    // Is m_pdoInSetValueFromManaged currently having its value set from managed?
    CDependencyObject       *m_pdoInSetValueFromManaged;

    CDeferredMapping* GetDeferredMapping();

protected:
    std::shared_ptr<XamlSchemaContext> m_spXamlSchemaContext;
    wil::critical_section m_AppVerificationIdLock;

private:
    _Check_return_ HRESULT UpdateLockScreenState();

    static bool IsPresentationModeHolographic();

    class FontDownloadListener;

// Member data
private:

    XUINT32                     m_cRef;
    DWORD                       m_nThreadID;        // Thread that created this core

    std::shared_ptr<XamlNodeStreamCacheManager>
                                m_spXamlNodeStreamCacheManager;

    VisualTree                 *m_pMainVisualTree;
    CDeployment                *m_pDeploymentTree;
    CTimeManager               *m_pTimeManager;
    CEventManager              *m_pEventManager;
    xref_ptr<CInputServices>    m_inputServices;

    CMediaQueueManager         *m_pmqm;

    IXcpHostSite               *m_pHostSite;
    IXcpBrowserHost            *m_pBrowserHost;
    IErrorService              *m_pErrorServiceForXbf;
    ICoreServicesSite          *m_pSite;
    XINT32                      m_bBuilderReady[2]; // We keep 2 scratch builders for use in widening, parsing, or other path building
    CGeometryBuilder           *m_pBuilder[2];
    CGlyphPathBuilder          *m_pGlyphPathBuilder;
    CTextCore                  *m_pTextCore;
    xref_ptr<CBrush>            m_defaultTextBrush;
    xref_ptr<CBrush>            m_textSelectionGripperFillBrush;
    xref_ptr<CSolidColorBrush>  m_textSelectionBackgroundBrush;
    xref_ptr<CSolidColorBrush>  m_textSelectionForegroundBrush;
    xref_ptr<CSolidColorBrush>  m_systemColorWindowBrush;
    xref_ptr<CSolidColorBrush>  m_systemColorWindowTextBrush;
    xref_ptr<CSolidColorBrush>  m_systemColorDisabledTextBrush;
    xref_ptr<CSolidColorBrush>  m_systemColorHotlightBrush;
    xref_ptr<CSolidColorBrush>  m_defaultFocusVisualSecondarySolidColorBrush; // Default solid color brushes used for the focus rect when the
    xref_ptr<CSolidColorBrush>  m_defaultFocusVisualPrimarySolidColorBrush; // SystemControlFocusVisualPrimaryBrush/SystemControlFocusVisualSecondaryBrush dictionary resources cannot be found.
    xref_ptr<CBrush>            m_transparentBrush;
    IPALQueue                  *m_preqQueue;
    XINT32                      m_fWatch;
    XUINT8                      m_fIsCoreReady;
    XUINT32                     m_objIdentity;
    CCoreServices              *m_pDrawReentrancyCheck; // used to induce an early crash when we re-enter draw.

    xref_ptr<CDisplayMemberTemplate>
                                m_pDefaultContentPresenterTemplate;

    xstringmap<bool>           m_resourceDictionaryUriCache; // Uri cache for cycle checking in ResourceDictionary

    XUINT8                      m_bProcessingDownloads;
    xref_ptr<IDWriteFontDownloadQueue>
                                m_pFontDownloadQueue;
    std::unique_ptr<FontDownloadListener>
                                m_pFontDownloadListener; // listener object for the download queue callback
    IPALQueue                  *m_pNativeManagedPeerQ;

    // Use the leak ignoring allocator for the deque, and we'll ensure that this is empty when we shut down. Deque is an interesting
    // beast where it re-allocates some sentinal node structure when you call shrink_to_fit. std::vector doesn't do this, but in
    // some controlled tests, std::deque is much quicker at popping from the front than std::vector when the list get's large.
    // This list probably wouldn't have more than 200 or so, in which case deque and vector would be pretty similar, but what's the point?
#if XCP_MONITOR
    using DeferredAnimationQueue = std::deque<std::shared_ptr<CDeferredAnimationOperation>, ::XcpAllocation::LeakIgnoringAllocator<std::shared_ptr<CDeferredAnimationOperation>>>;
#else
    using DeferredAnimationQueue = std::deque<std::shared_ptr<CDeferredAnimationOperation>>;
#endif

    DeferredAnimationQueue      m_deferredAnimationOperationQueue;

    bool                        m_isMainTreeLoading;
    bool                        m_fWantsRenderingEvent; // Is anyone listening to the render event
    bool                        m_fWantsCompositionTargetRenderedEvent = false; // Is anyone listening to the Composition.Target Rendered event
    LONG                        m_fSurfaceContentsLostEnqueued;   // Indicates we have hit a Device Removed (adapter changed, etc) situation and need to signal the application

    // A list of all the SurfaceImageSources associated with this Core.
    // Used to clear out pending updates on device lost.
    CXcpList<CSurfaceImageSource>
                               *m_pAllSurfaceImageSources;

    // A list of all the VirtualSurfaceImageSources associated with this Core.
    // Used to fire notifications at the proper time.
    CXcpList<CVirtualSurfaceImageSource>
                               *m_pAllVirtualSurfaceImageSources;

    // A cache, mapping CUIElement to its visible bounds, used only while in the
    // context of CCoreServices::VirtualSurfaceImageSourcePerFrameWork().
    VisibleBoundsMap           *m_pVisibleBoundsMap = nullptr;

    XUINT32                     m_uRuntimeId;

    // Used for testing. Override of the resource property bag normally retrieved from the resource manager.
    using PropertyBagOverride = std::shared_ptr<std::map<xstring_ptr, std::shared_ptr<PropertyBag>>>;
    PropertyBagOverride         m_propertyBagOverride;

    typedef xvector<CDependencyObject*> UpdateStateChildrenVector;
    UpdateStateChildrenVector   m_rgpChildrenForUpdate;

    xvector<CUIElement*>        m_redirectionElementsNoRef;
    xvector<IGripper*>          m_grippersNoRef;

//  TRUE when ResetVisualTree is processing
    bool                        m_bInResetVisualTree;
    // Set to true after resetting the visual tree. Causes NWDrawTree to fire an event used in a test hook.
    bool m_isFrameAfterVisualTreeReset;

    bool                        m_bIsShuttingDown;
    bool                        m_bVisibilityToggled;
    bool                        m_isTransparentBackground;
    bool                        m_isTearingDownIsland { false };

    XINT32                      m_cPendingDecodes;
    INT32                       m_cPendingFontDownloads;
    LONG                        m_pendingImplicitShowHideCount;

//  When this is true, hit testing APIs will hit test elements with no fill/background as if they
//  have a solid color fill. This is particularly useful for Designer tools which need to be able
//  to select stuff like Grid elements (no background by default).
//  TODO:  In the future refactor to pass this parameter down the hit-test call stack.
    bool                       m_bInvisibleHitTestMode;

    // set to true when destructor is called.  This will give info on the state
    // of the coreservice obj, and will help to avoid reentrance problems on cleanup
    bool                       m_bIsDestroyingCoreServices;

    bool                        m_bIsUsingGenericXamlFilePath;

    ICustomResourceLoader      *m_pCustomResourceLoader;

    xref_ptr<AsyncImageFactory> m_imageFactory;
    xref_ptr<ImageTaskDispatcher> m_imageTaskDispatcher;
    xref_ptr<ImageProvider> m_imageProvider;

    IPALWorkItemFactory        *m_pWorkItemFactory;

    IPALResourceManager        *m_pResourceManager;

    // Opaque context stored here for the framework
    XHANDLE                     m_frameworkContext;

    IPALMemory                 *m_pAppVerificationId;

    CUIElement *m_pLastLayoutExceptionElement;

    typedef xvector<IPLMListener *> PLMListenerVector;
    PLMListenerVector           m_plmListeners;

    // Both layout and rendering need to be completely invalidated when the zoom scale changes. Track them with separate flags,
    // because layout still happens even if we're not rendering.
    bool                       m_zoomScaleChanged_ForceRelayout : 1;
    bool                       m_zoomScaleChanged_ForceRerender : 1;
    bool                       m_localeSettingChanged :1;
    bool                       m_debugSettingsChanged : 1;
    bool                       m_commitRequested : 1;

    // Is Binary XAML being generated (by Visual Studio build)?
    bool                       m_isGeneratingBinaryXaml : 1;

    bool                       m_fLayoutCompletedNeeded;
    bool                       m_isRenderEnabled;
    bool                       m_renderStateChanged;
    bool                       m_isSuspended;

    // Not good enough in XamlIslands mode. If any island is visible, we need to keep ticking.
    bool                        m_isWindowVisible;
    bool                        m_wasWindowEverMadeVisible;    // For debugging/post-mortem purposes

    enum class SuspendReason
    {
        NotSuspended = 0,
        PLMSuspend,
        WindowHiddenTimeout
    };
    SuspendReason               m_currentSuspendReason; // For debugging/post-mortem purposes

    // Bug 33863304: Setting up a fresh VM on co_refresh_dash_wcx 6/19 and go missing Start content (just window shadow outline) again.
    // There are bugs where Xaml is left in a state where we don't render, and we can't tell what went wrong with just
    // a process dump. Add some logging around significant events so we can tell what got us into a bad state.
    enum class CoreServicesEvent
    {
        TestHook = 0,
        WindowShown,
        WindowHidden,
        RenderEnabled,
        RenderDisabled,
        LayoutCompletedNeeded_SizeChanged,
        LayoutCompletedNeeded_Visible,
        StateReset,
        HardwareResourcesReleased,
        HardwareResourcesRebuilt,
        OnVisibilityChangedDuringShutdown,
        D2DWrongTargetError,
    };

    struct CoreServicesEventLog
    {
        unsigned int frameNumber;
        CoreServicesEvent coreServicesEvent;
    };

    CircularMemoryLogger<32, CoreServicesEventLog> m_coreServicesEventLog;

    // Note: this object is on a very hot code path (250k+ logger accesses just opening a new File Explorer window).
    // One of these exists for each UI thread.
    std::unique_ptr<Diagnostics::ResourceLookupLogger> m_resourceLookupLogger;

    // The DComp page rotation manager has a policy that skips the animation for the next rotation change after the
    // window goes from invisible to visible in order to prevent showing a stale frame. Due to timing variations,
    // sometimes the rotation notification comes after the window is made visible, which we correctly ignore, but
    // other times the rotation notification comes before the window is made visible, which leaves the page rotation
    // manager in a bad state. It's going to skip the animation for the next rotation, except the rotation that it
    // meant to skip has already happened, which means the next legitimate rotation will be skipped.
    //
    // To get the rotation manager out of this bad state, we reset the "skip next animation" flag whenever we tell it
    // we rendered a frame. We only tell it that we rendered a frame after detecting a rotation update, which will
    // not be enough if the phone was in portrait mode all along, so we need a second check - this flag tells us to
    // update the page rotation manager even if the rotation hasn't changed. We set it after going from invisible to
    // visible.
    bool                        m_shouldUpdateRotationManagerAfterWindowMadeVisible;

    // Track the lost device state
    enum class DeviceLostState
    {
        None,                   // No device lost encountered
        HardwareOnly,           // Device lost encountered, reset graphics hardware objects only
        HardwareAndVisuals,     // Device lost encountered, reset graphics hardware objects and DComp visuals.
                                // Used only by tests via CCoreServices::SimulateDeviceLost
        HardwareReleased        // Graphics hardware was manually shutdown
    }                           m_deviceLost;

    wil::unique_handle          m_deviceLostEvent;
    wil::unique_threadpool_wait m_deviceLostWaiter;
    DWORD                       m_deviceLostEventCookie = 0;

    std::unordered_set<DeviceListener*>
                                m_deviceListeners;

    std::unordered_set<RenderStateListener*>
                                m_renderStateListeners;

    bool                        m_attachMemoryManagerEvents = true;
    Microsoft::WRL::ComPtr<wsy::IMemoryManagerStatics>
                                m_memoryManager;
    EventRegistrationToken      m_appMemoryUsageLimitChangingToken = EventRegistrationToken();
    EventRegistrationToken      m_appMemoryUsageIncreasedToken = EventRegistrationToken();

    CDispatcherTimer           *m_pBackgroundResourceTimer;

    CRenderTargetBitmapManager *m_pRenderTargetBitmapManager;

    // On device loss all the DComp surfaces must be deleted. If the element
    // owning the surface is off the tree then the surface is inaccessible for
    // deletion. There are two ways to solve this problem.
    // 1) Delete all the hardware resources whenever the owning element
    // leaves the tree.
    // 2) Track the surfaces whose owning elements are off the tree in a list
    // and use this list when device loss happens.
    // Using solution (1) for images causes behavioral regressions since
    // reloading the images is expensive and usually takes multiple frames.
    //
    // The trade-off was resolved by delaying the destruction of hardware resources
    // by sc_imageSurfaceWrapperReleaseDelayMilliseconds time (currently 1s) so that
    // it will be cleaned up at a future time using the surface tracker.  This resolved
    // the behavioral regressions from solution (1)
    IntrusiveList<ImageSurfaceWrapper>
                                m_imageSurfaceWrapperCleanupList;

    struct DecodeRequestEntry
    {
        DecodeRequestEntry()
            : width(0)
            , height(0)
            , retainPlaybackState(false)
        {
        }

        XUINT32 width;
        XUINT32 height;
        bool retainPlaybackState;
    };

    // Controls the delay after an image has left the tree for it's resources to be released
    static const UINT64         sc_imageSurfaceWrapperReleaseDelayMilliseconds = 1000;

    // This should only be used by tests to override waiting for the timeout to expire
    bool                        m_testOverrideImageSurfaceWrapperReleaseDelay;
    bool m_forceWindowInvisible_TestHook = false;

    // Map for all image decode requests
    typedef xchainedmap<CImageSource*, DecodeRequestEntry> DecodeRequestMap;
    DecodeRequestMap            m_decodeRequests;

    // Holds a map of CImageSource's we're tracking for render walk culling,
    // map is only used for fast lookup of CImageSource ptrs, value it maps to is unused.
    xchainedmap<CImageSource*, bool>
                                m_trackedImages;

    // Holds animated CImageSource's we are tracking for render walk culling
    std::unordered_set<CImageSource*>
                                m_animatedImages;

    // Holds a vector of all hardware surfaces that require updates.  Most will be updated
    // during the render walk.  However, at the end of the render walk, if an image was not
    // updated because it was culled, it will be updated using this vector.
    typedef std::vector<ImageSurfaceWrapper*> ImageSurfaceWrapperList;
    ImageSurfaceWrapperList     m_pendingImageUpdates;

    bool                        m_pendingFirstFrameTraceLoggingEvent;

    std::unique_ptr<CDeferredMapping>
                                m_deferredMapping;

    std::function<void()>       m_postTickCallback = nullptr;

    // IPALClock used by the FrameRateCounter and CompositionTarget events in NWDrawTree
    xref_ptr<IPALClock> m_pPALClock;

    InitializationType m_initializationType = InitializationType::Normal;

public:
    void SetFrameworkContext( XHANDLE context ) { m_frameworkContext = context; }
    XHANDLE GetFrameworkContext() { return m_frameworkContext; }

    static bool GetIsCoreServicesReady();
    static void SetIsCoreServicesReady(bool value);

    void MarkRootScaleTransformDirty()
    {
        m_zoomScaleChanged_ForceRelayout = TRUE;
        m_zoomScaleChanged_ForceRerender = TRUE;
    }

    _Check_return_ HRESULT InitWaitForIdleEvents();
    bool HasAnimationEvents();
    _Check_return_ HRESULT SetHasAnimationsEventSignaledStatus(_In_ bool bSignaled);
    _Check_return_ HRESULT SetAnimationsCompleteEvent();
    _Check_return_ HRESULT SetHasDeferredAnimationOperationsEventSignaledStatus(_In_ bool bSignaled);
    _Check_return_ HRESULT SetDeferredAnimationOperationsCompleteEvent();
    _Check_return_ HRESULT SetRootVisualResetEventSignaledStatus(_In_ bool bSignaled);
    _Check_return_ HRESULT SetImageDecodingIdleEventSignaledStatus(_In_ bool bSignaled);
    _Check_return_ HRESULT SetFontDownloadsIdleEventSignaledStatus(_In_ bool bSignaled);
    _Check_return_ HRESULT SetPopupMenuCommandInvokedEvent();
    bool HasBuildTreeWorkEvents();
    _Check_return_ HRESULT SetHasBuildTreeWorksEventSignaledStatus(_In_ bool bSignaled);
    _Check_return_ HRESULT SetBuildTreeServiceDrainedEvent();

    _Check_return_ HRESULT SetKeyboardInputEvent();
    _Check_return_ HRESULT EnableKeyboardInputEvent();
    void DisableKeyboardInputEvent();
    bool CanFireKeyboardInputEvent() const;

    _Check_return_ HRESULT SetPointerInputEvent();
    _Check_return_ HRESULT EnablePointerInputEvent();
    void DisablePointerInputEvent();
    bool CanFirePointerInputEvent() const;

    void RequestAdditionalFrame(RequestFrameReason reason);

    _Check_return_ HRESULT UpdateFontScale(_In_ XFLOAT newFontScale);
    XFLOAT GetFontScale();

    _Check_return_ HRESULT CancelAllConnectedAnimationsAndResetDefaults();

    XUINT32                     m_cInheritedPropGenerationCounter;
    unsigned char               m_cIsRightToLeftGenerationCounter;

    XUINT32                     m_cTrackingInterval;
    XUINT64                     m_cMaxAllowedMemoryincrease;
    XUINT8                      m_State; //Tracking

    XUINT64                     m_lPrvMemCount;

    // The window render target currently attached to this core services object.
    _Maybenull_ CWindowRenderTarget
                               *m_pNWWindowRenderTarget;

    bool                       m_fDirtyState;
    bool                       m_fInRenderWalk;
    bool                        m_fDbgEnableThreadingAssert;

    bool                       m_fPrinting;

    bool                        m_calledOleInitialize;

    xstring_ptr                 m_strSystemColorsResourcesXaml;
    CResourceDictionary        *m_pSystemColorsResources;

    bool                        m_fIsSwitchingTheme = false;
    bool                        m_hasThemeEverChanged = false;
    bool                        m_isLoadingGlobalThemeResources = false;
    bool                        m_isApplicationFocusVisualKindReveal = false;

    std::unique_ptr<FrameworkTheming>
                                m_spTheming;
    std::unique_ptr<ThemeWalkResourceCache>
                                m_themeWalkResourceCache;

    // Corresponds to FrameworkElement.RequestedTheme. Such a framework element
    // is the root of a theme subtree, and is used for theme mixing, where a
    // subtree can have a different theme than the rest of the app. This theme
    // is pushed/popped during a tree walk to change theme.
    Theming::Theme              m_requestedThemeForSubTree;

    // This is set when starting to process a frame for the main visual tree and
    // is cleared to zero when processing is complete.
    XUINT64                     m_qpcDrawMainTreeStart;

    bool                        m_isFirstFrameAfterAppStart;

    // Core objects which are GC roots. These objects don't have FX peers
    // so GC cannot walk them as roots from the FX peers. Instead GC will
    // walk these core objects as GC roots. These correspond to peers which
    // are GC roots because of PegNoRef. They are used for GC walks when
    // during visual tree constrution, when they have not yet been protected
    // by a parent.

#if XCP_MONITOR
    // Ideally, we wouldn't need to mark this with the leak ignoring allocator.
    // However, the application object will be added to this list when first queried for
    // it's resource dictionary. If it doesn't have one and it's created on-demand, it will
    // end up getting pegged in SetValue.
    using PegNoRefVector = containers::vector_set<CDependencyObject*, ::std::less<>, ::XcpAllocation::LeakIgnoringAllocator<CDependencyObject*>>;
#else
    using PegNoRefVector = containers::vector_set<CDependencyObject*>;
#endif
    PegNoRefVector              m_PegNoRefCoreObjectsWithoutPeers;

public:
    XINT64& GetAnimationSlowFactor()
    {
        return m_animationSlowFactor;
    }

    void SetAnimationSlowFactor(const XINT64& factor)
    {
        m_animationSlowFactor = factor;
    }

    void ForceDisableTSF3() { m_isTsf3Disabled = true; }
    bool IsTSF3Enabled() const;

private:
    XINT64                      m_animationSlowFactor;
    UINT64                      m_lastPointerReplayTime = 0;

    IPALEvent                  *m_pHasAnimationsEvent;
    IPALEvent                  *m_pAnimationsCompleteEvent;
    IPALEvent                  *m_pHasDeferredAnimationOperationsEvent;
    IPALEvent                  *m_pDeferredAnimationOperationsCompleteEvent;
    IPALEvent                  *m_pRootVisualResetEvent;
    IPALEvent                  *m_pImageDecodingIdleEvent;
    IPALEvent                  *m_pFontDownloadsIdleEvent;
    IPALEvent                  *m_pPopupMenuCommandInvokedEvent;
    IPALEvent                  *m_pHasBuildTreeWorksEvent;
    IPALEvent                  *m_pBuildTreeServiceDrainedEvent;
    IPALEvent                  *m_pKeyboardInputEvent;
    IPALEvent                  *m_pPointerInputEvent;
    IPALEvent                  *m_pImplicitShowHideCompleteEvent;
    IPALEvent                  *m_hasFacadeAnimationsEvent = nullptr;
    IPALEvent                  *m_facadeAnimationsCompleteEvent = nullptr;
    IPALEvent                  *m_animatedFacadePropertyChangesCompleteEvent = nullptr;
    IPALEvent                  *m_hasBrushTransitionsEvent = nullptr;
    IPALEvent                  *m_brushTransitionsCompleteEvent = nullptr;

    int                         m_pendingAnimatedFacadePropertyChanges = 0;
    LONG                        m_facadeAnimationCount = 0;

    LONG                        m_brushTransitionCount = 0;

    XFLOAT                      m_fontScale;

    bool                        m_forceDisconnectRootOnSuspend;
    bool                        m_replayPointerUpdateAfterTick = false;

    bool                        m_fireKeyboardInputEvent;
    bool                        m_firePointerInputEvent;

    // True if the associated CoreWindow is simulating (for test purposes) presenting in Holographic space
    bool                        m_isHolographicOverrideSet;

    // Has TSF3 been explicitly disabled?  Set by DesktopWindowXamlSource
    bool                        m_isTsf3Disabled = false;

    bool                        m_isDCompLeakDetectionEnabled = true;
    CResourceDictionary        *m_pThemeResources;

    AutomationEventsHelper m_automationEventsHelper;

public:
    void AddThemeChangedListener(_In_ CFrameworkElement* pFE);
    void RemoveThemeChangedListener(_In_ CFrameworkElement* pFE);

    CoreState& GetFlyweightState()
    {
        return m_flyweightState;
    }

private:
    containers::vector_map<CFrameworkElement*, unsigned int> m_elementsWithThemeChangedListener;
    CoreState                           m_flyweightState;

    // Connected Animation members/methods
public:
    void EnsureConnectedAnimationService();
    _Check_return_ CConnectedAnimationService* GetConnectedAnimationServiceNoRef();
    _Check_return_ CConnectedAnimationRoot* GetConnectedAnimationRoot() const
    {
        return m_pMainVisualTree ? m_pMainVisualTree->GetConnectedAnimationRoot() : nullptr;
    }

private:
    xref_ptr<CConnectedAnimationService>
                                m_connectedAnimationService;

// Default theme transition
public:
    void SetDefaultNavigationTransition(xaml::IDependencyObject* transition) { m_defaultNavigationTransition = transition; }
    xaml::IDependencyObject* GetDefaultNavigationTransition() { return m_defaultNavigationTransition.Get(); }
private:
    Microsoft::WRL::ComPtr<xaml::IDependencyObject> m_defaultNavigationTransition;

private:
    std::unique_ptr<AccessKeys::AccessKeyExport> m_akExport;

private:
    Microsoft::WRL::ComPtr<wf::IPropertyValueStatics> m_spPropertyValueFactory;
    MaxTextureSizeProvider m_maxTextureSizeProvider;
    AtlasRequestProvider m_atlasRequestProvider;

public:
     wf::IPropertyValueStatics* GetPropertyValueStatics();

public:
    FacadeStorage& GetFacadeStorage() { return m_facadeStorage; }

    SimpleProperty::sparsetables& GetSimplePropertySparseTables()
    {
        return m_sparseTables;
    }

    SimpleProperty::changehandlerstables& GetSimplePropertyHandlersTables()
    {
        return m_changeHandlersTables;
    }

    void AddMutableStyleValueChangedListener(
        _In_ CStyle* const pStyle,
        xref::weakref_ptr<CFrameworkElement>& elementWeakRef);
    void RemoveMutableStyleValueChangedListener(
        _In_ CStyle* const pStyle,
        xref::weakref_ptr<CFrameworkElement>& elementWeakRef);
    const std::vector<xref::weakref_ptr<CFrameworkElement>>& GetMutableStyleValueChangedListeners(_In_ CStyle* const pStyle);
    void NotifyMutableStyleValueChangedListeners(
        _In_ CStyle* const pStyle,
        KnownPropertyIndex propertyIndex);

private:
    FacadeStorage m_facadeStorage;

    SimpleProperty::sparsetables m_sparseTables;
    SimpleProperty::changehandlerstables m_changeHandlersTables;

    Microsoft::WRL::ComPtr<wgrd::IDisplayInformation2> m_displayInformation;

    containers::vector_map<xref::weakref_ptr<CStyle>, std::vector<xref::weakref_ptr<CFrameworkElement>>> m_appliedStyleTables;

    // Used by tests to tick the UI thread with no content in it. This is done so the UI thread can recover from device
    // lost and create its WUC compositor. Xaml islands needs to do this before creating the Xaml islands, otherwise
    // the Xaml island will be closed as soon as the UI thread ticks and releases its compositor to handle the device
    // lost from MockDComp injection.
    bool m_canTickWithNoContent = false;

public:
    void AddValueWithExpectedReference(_In_ CModifiedValue* value);
    void RemoveValueWithExpectedReference(_In_ CModifiedValue* value);
    void ClearValuesWithExpectedReference();

private:
    std::vector<CModifiedValue*> m_valuesWithExpectedReference;

public:
    ContentRootCoordinator* GetContentRootCoordinator() { return &m_contentRootCoordinator; }
    bool GetShouldReevaluateIsAnimationEnabled() const { return m_shouldReevaluateIsAnimationEnabled; }
    void SetShouldReevaluateIsAnimationEnabled(bool value) { m_shouldReevaluateIsAnimationEnabled = value; }

    wf::Size GetContentRootMaxSize();

private:
    ContentRootCoordinator m_contentRootCoordinator;
    bool m_shouldReevaluateIsAnimationEnabled = true;

private:
    void LogCoreServicesEvent(CoreServicesEvent coreServicesEvent);
};


//------------------------------------------------------------------------
//
//  Class:  CAbortableDownloadWrapper
//
//  Synopsis:
//      An abortable download to hand out when a download is
//      being placed on the queue.
//
//------------------------------------------------------------------------

class CAbortableDownloadWrapper final : public IPALAbortableOperation
{
public:
    _Check_return_ static HRESULT Create(
        _In_ DREQUEST* pDownloadRequest,
        _In_ CCoreServices* pcs,
        _Outptr_ CAbortableDownloadWrapper** ppAbortableDownload);

    void Abort() override;

    XUINT32 AddRef() override;
    XUINT32 Release() override;

    _Check_return_ HRESULT SetAbortable(_In_ IPALAbortableOperation* pAbortable);


protected:
    CAbortableDownloadWrapper()
    : m_cRef(1), m_pAbortable(NULL)
    {
        XCP_WEAK(&m_pcs);
    }

private:
    ~CAbortableDownloadWrapper() override
    {
        ReleaseInterface(m_pAbortable);
    }
    DREQUEST*                m_pRequest;
    CCoreServices*           m_pcs;
    XUINT32                  m_cRef;
    IPALAbortableOperation*  m_pAbortable;
};

static_assert(sizeof(MetaDataPropertyNonAggregate) == sizeof(MetaDataProperty), "Types need to be kept in sync.");


class AppMemoryUsageLimitChangingCallback :
    public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::RuntimeClassType::ClassicCom>,
                        wf::IEventHandler<wsy::AppMemoryUsageLimitChangingEventArgs*>,
                        Microsoft::WRL::FtmBase>
{
public:
    IFACEMETHOD(Invoke)(_In_ IInspectable * sender, _In_ wsy::IAppMemoryUsageLimitChangingEventArgs * args) override;
};

class AppMemoryUsageIncreasedCallback :
    public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::RuntimeClassType::ClassicCom>,
                        wf::IEventHandler<IInspectable*>,
                        Microsoft::WRL::FtmBase>
{
public:
    IFACEMETHOD(Invoke)(_In_ IInspectable * sender, _In_ IInspectable * args) override;
};

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include "BackingEtwRule.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {
// The ImageDecodingRule is a rule that detects if developers are not using correct programming
// techniques when rendering images on the screen. If the image control is not in the live tree
// before the source is set on the bitmap, then Decode to Render size feature can not be used.

class ImageDecodingRule
    : public EtwRuleImpl<ImageDecodingRule, appanalysis::RuleCategories_Performance>
{

public:

    ImageDecodingRule()
    {
    }

    virtual ~ImageDecodingRule()
    {
    }

    HRESULT ImageDecodingRule::ProcessImageDecodedInfo(
        _In_ appanalysis::IEtwEventRecord* pEvent
        )
    {
        ImageSize decodedSize = { 0 };
        ImageSize naturalSize = { 0 };
        IFC_RETURN(pEvent->GetUInt32Property(StringRef(L"DecodeWidth"), &decodedSize.Width));
        IFC_RETURN(pEvent->GetUInt32Property(StringRef(L"DecodeHeight"), &decodedSize.Height));
        IFC_RETURN(pEvent->GetUInt32Property(StringRef(L"NaturalWidth"), &naturalSize.Width));
        IFC_RETURN(pEvent->GetUInt32Property(StringRef(L"NaturalHeight"), &naturalSize.Height));

        // if the decoded size is zero, then that means we fell back to the natural size
        if (decodedSize.Width == 0 && naturalSize.Width == 0)
        {
            decodedSize = naturalSize;
        }

        InstanceHandle imageId = 0;
        IFC_RETURN(pEvent->GetUInt64Property(StringRef(L"ImageId"), &imageId));

        // If this image was decoded due to a synchronous decode. Then we will not
        // have gotten the RequestDecode event, that will come at a later time. From
        // there we can get the requested size.
        if (IsImageRequestDeffered(imageId))
        {
            IFCSTL_RETURN(m_pendingDecodes.emplace(imageId, decodedSize));
        }
        else
        {
            // find the request for this image. We should have a request as the only time
            // we wouldn't would be in the case for a synchronous decode and in that case
            // we cache the decoded size for a later call to ProcessImageDecodeRequest.
            auto imageRequest = m_requestCache.find(imageId);
            ASSERT(imageRequest != m_requestCache.end());

            INT64 timestamp = 0;
            IFC_RETURN(pEvent->get_Timestamp(&timestamp));
            IFC_RETURN(FireNotificationForImage(imageId, imageRequest->second, decodedSize, timestamp));
        }

        return S_OK;
    }

    HRESULT ImageDecodingRule::ProcessDecodeToRenderSizeDisabledInfoEvent(
        _In_ appanalysis::IEtwEventRecord* pEvent
        )
    {
        InstanceHandle imageId = 0;
        IFC_RETURN(pEvent->GetUInt64Property(StringRef(L"ImageId"), &imageId));

        UINT32 reason = 0;
        IFC_RETURN(pEvent->GetUInt32Property(StringRef(L"ReasonKey"), &reason));

        // Only place this in the map if one doesn't currently exist
        if (!HasReasonForImage(imageId, s_reasons[reason]))
        {
            IFCSTL_RETURN(m_reasonCache.emplace(imageId, reason));
        }

        return S_OK;
    }

    HRESULT ImageDecodingRule::ProcessImageRelationInfoEvent(
        _In_ appanalysis::IEtwEventRecord* pEvent
        )
    {
        InstanceHandle imageId = 0;
        IFC_RETURN(pEvent->GetUInt64Property(StringRef(L"ElementId"), &imageId));

        InstanceHandle parentId = 0;
        IFC_RETURN(pEvent->GetUInt64Property(StringRef(L"ParentId"), &parentId));

        wil::shared_hstring name;
        IFC_RETURN(pEvent->GetUnicodeStringProperty(StringRef(L"Name"), &name));

        IFCSTL_RETURN(m_parentCache[imageId].emplace(parentId, std::move(name)));

        return S_OK;
    }

    HRESULT ImageDecodingRule::ProcessImageDecodeRequest(
        _In_ appanalysis::IEtwEventRecord* pEvent
    )
    {
        InstanceHandle imageId = 0;
        IFC_RETURN(pEvent->GetUInt64Property(StringRef(L"ImageId"), &imageId));

        ImageSize requestSize = { 0 };
        IFC_RETURN(pEvent->GetUInt32Property(StringRef(L"Width"), &requestSize.Width));
        IFC_RETURN(pEvent->GetUInt32Property(StringRef(L"Height"), &requestSize.Height));

        if (requestSize.Width == 0 || requestSize.Height == 0)
        {
            return S_OK;
        }

        // If this image request was deffered, then we have stored the results of the decoded
        // image and now know the actual requested size, so we can fire! Otherwise, we'll storing this
        // request for a later decode
        auto iter = m_pendingDecodes.find(imageId);
        if (iter != m_pendingDecodes.end())
        {
            INT64 timestamp = 0;
            IFC_RETURN(pEvent->get_Timestamp(&timestamp));

            HRESULT hr = FireNotificationForImage(imageId, requestSize, iter->second, timestamp);
            m_pendingDecodes.erase(iter);
            IFC_RETURN(hr);
        }
        else
        {
            // std::map::emplace returns a pair representing the iterator to the location in the map
            // of the insertion and whether the insertion happened
            std::pair<ImageSizeCache::iterator, bool> insertIter;
            IFCSTL_RETURN(insertIter = m_requestCache.emplace(imageId, requestSize));

            // If there already exists a previous request, and this one is larger, then update it. We
            // only update it here because the framework will only redecode the image in this scenario.
            // We won't update the size if uses SoftwareRendering because the framework
            // doesn't request a decode at the full size of the image since we can't use DTRS.
            if (!insertIter.second &&
                !HasReasonForImage(imageId, IMAGE_RULE_SOFTWARERENDERING) &&
                (requestSize.Width > insertIter.first->second.Width ||
                 requestSize.Height > insertIter.first->second.Height))
            {
                insertIter.first->second = requestSize;
            }

        }

        return S_OK;
    }

private:
    struct ImageSize
    {
        unsigned int Width;
        unsigned int Height;
    };

    using ParentCache = std::map<InstanceHandle, std::map<InstanceHandle, wil::shared_hstring>>;
    using ReasonCache = std::multimap<InstanceHandle, unsigned int>;
    using ImageSizeCache = std::map<InstanceHandle, ImageSize>;

    // There are two cases where we decode immediately: Synchronous decode,
    // and when the decode size is specified. When decode size is specified,
    // we don't decode synchronously, but we do know the size to decode to
    // so we don't wait for the request to kick off the decode. If there are
    // other reasons in the cache where shouldn't defer, then we don't want to defer.
    // The reason we defer is that if only DecodePixelSiz/SynchronousDecode are received,
    // We won't get the Request until after the Decode.
    bool IsImageRequestDeffered(InstanceHandle imageId) const
    {
        auto range = m_reasonCache.equal_range(imageId);
        for (auto iter = range.first; iter != range.second; ++iter)
        {
            if (s_reasons[iter->second] != IMAGE_RULE_SYNCDECODE &&
                s_reasons[iter->second] != IMAGE_RULE_DECODESIZESPECIFIED)
            {
                return false;
            }
        }

        return true;
    }

    bool HasReasonForImage(InstanceHandle imageId, int reason) const
    {
        auto range = m_reasonCache.equal_range(imageId);
        for (auto iter = range.first; iter != range.second; ++iter)
        {
            if (s_reasons[iter->second] == reason)
            {
                return true;
            }
        }

        return false;
    }

    void ResetImage(InstanceHandle image)
    {
        m_parentCache.erase(image);
        m_reasonCache.erase(image);
    }

    HRESULT FireNotificationForImage(InstanceHandle imageId, const ImageSize& requestedSize, const ImageSize& decodedSize, INT64 timestamp)
    {
        // We want to protect ourselves and clean up even if there is a failure
        // because the same bitmap image could be reused and our data would be stale
        // if there was a DecodeStream event fired in the future
        auto callback = wil::scope_exit([&]() { ResetImage(imageId); });

        UINT widthDelta = decodedSize.Width - requestedSize.Width;
        UINT heightDelta = decodedSize.Height - requestedSize.Height;

        // As of now there is no threshold, always fire if image is too large
        if (widthDelta > 0 || heightDelta > 0)
        {
            wrl::ComPtr<appanalysis::ISourceInfoRuleService> sourceInfoService;
            IFC_RETURN(GetRuleService(&sourceInfoService));

            // Create RuleTriggeredEventArgs objects detailing why we couldn't decode this image to
            // render size
            auto reasonRange = m_reasonCache.equal_range(imageId);
            for (auto reasonIter = reasonRange.first; reasonIter != reasonRange.second; ++reasonIter)
            {
                // Find elements in the live tree that this is related to
                auto parentsIter = m_parentCache.find(imageId);
                if (parentsIter != m_parentCache.end())
                {
                    for (const auto& parentIdNamePair: parentsIter->second)
                    {
                        RuleTriggeredEventArgs::CreateParams params;

                        params.timeline.Start = timestamp;
                        params.timeline.Stop = timestamp;

                        IFC_RETURN(wrl::MakeAndInitialize<ResourceString>(&params.description, s_reasons[reasonIter->second]));
                        IFC_RETURN(params.description->Append(parentIdNamePair.second.get()));
                        IFC_RETURN(sourceInfoService->GetSourceInfo(parentIdNamePair.first, &params.sourceInfo));
                        IFC_RETURN(sourceInfoService->GetVisualTreeId(parentIdNamePair.first, &params.elementId));

                        params.measurement.Value = BYTES_TO_KB(4 * widthDelta*heightDelta);
                        params.measurement.Unit = appanalysis::MeasurementUnit_Kilobytes;

                        wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs> imageRuleRuleTriggeredEventArgs;
                        IFC_RETURN(CreateRuleTriggeredEventArgs(params, &imageRuleRuleTriggeredEventArgs));
                        FireNotification(imageRuleRuleTriggeredEventArgs.Get());
                    }
                }
            }
        }

        return S_OK;
    }

    static const int s_reasons[];

    ParentCache m_parentCache;
    ReasonCache m_reasonCache;
    ImageSizeCache m_requestCache;
    ImageSizeCache m_pendingDecodes;
};

const int ImageDecodingRule::s_reasons[] = {
    IMAGE_RULE_SYNCDECODE,
    IMAGE_RULE_NOTLIVE,
    IMAGE_RULE_BITMAPICON,
    IMAGE_RULE_SOFTWARERENDERING,
    IMAGE_RULE_USESIMAGETILING,
    IMAGE_RULE_EMPTYBOUNDS,
    IMAGE_RULE_NINEGRID,
    IMAGE_RULE_DRAGDROP,
    IMAGE_RULE_DECODESIZESPECIFIED,
};

BEGIN_PROVIDERS(ImageDecodingRule)
    DECLARE_MANIFEST_PROVIDER(WINDOWS_UI_XAML_ETW_PROVIDER, L"Microsoft-Windows-XAML-ETW.man")
    DECLARE_MANIFEST_PROVIDER(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, L"Microsoft-Windows-XAML-ETW.man")
END_PROVIDERS()

BEGIN_CALLBACKS(ImageDecodingRule)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_ETW_PROVIDER, DecodeStreamForImageInfo_value, EventVersion_0, &ImageDecodingRule::ProcessImageDecodedInfo)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_ETW_PROVIDER, ImageRequestDecodeInfo_value, EventVersion_0, &ImageDecodingRule::ProcessImageDecodeRequest)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_ETW_PROVIDER, DecodeToRenderSizeDisabledInfo_value, EventVersion_1, &ImageDecodingRule::ProcessDecodeToRenderSizeDisabledInfoEvent)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, ImageSourceRelationInfo_value, EventVersion_0, &ImageDecodingRule::ProcessImageRelationInfoEvent)
END_CALLBACKS()

////////////////////////////////////////////////////////////////////////////////
//
HRESULT ImageDecodingRule_CreateInstance(
    _COM_Outptr_ appanalysis::IEtwRule** ppInstance
    )
{
    wrl::ComPtr<ImageDecodingRule> rule;
    IFC_RETURN(ImageDecodingRule::CreateInstance(
        IMAGE_RULE_ID, IMAGE_RULE_TITLE, IMAGE_RULE_IMPACT,
        IMAGE_RULE_LINK_TITLE, IMAGE_RULE_LINK_URL,
        &rule));

    wrl::ComPtr<appanalysis::IEtwEventWatcher> watcher;
    IFC_RETURN(rule->RegisterEvents(&watcher));

    IFC_RETURN(wrl::MakeAndInitialize<EtwRule>(ppInstance, rule.Get(), watcher.Get()));

    return S_OK;
}
} } }

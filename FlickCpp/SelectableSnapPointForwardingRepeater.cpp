#include "pch.h"
#include "SelectableSnapPointForwardingRepeater.h"

using namespace FlickCpp;
using namespace Platform;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;

/*static*/ DependencyProperty^ SelectableSnapPointForwardingRepeater::s_repeatCountProperty = nullptr;

/*static*/ void SelectableSnapPointForwardingRepeater::RegisterDependencyProperties()
{
    if (s_repeatCountProperty == nullptr)
    {
        s_repeatCountProperty =
            DependencyProperty::Register(
                "RepeatCount",
                int::typeid,
                SelectableSnapPointForwardingRepeater::typeid,
                ref new PropertyMetadata(500));
    }
}

SelectableSnapPointForwardingRepeater::SelectableSnapPointForwardingRepeater()
{
    RegisterDependencyProperties();
}

SelectableSnapPointForwardingRepeater::~SelectableSnapPointForwardingRepeater()
{
}

IVectorView<float>^ SelectableSnapPointForwardingRepeater::GetIrregularSnapPoints(Orientation /* orientation */, SnapPointsAlignment /* alignment */)
{
    return nullptr;
}

float SelectableSnapPointForwardingRepeater::GetRegularSnapPoints(Orientation orientation, SnapPointsAlignment alignment, _Out_ float *offset)
{
    if (alignment == SnapPointsAlignment::Center && orientation == Orientation::Horizontal)
    {
        /*VirtualizingUniformCarouselStackLayout^ layout = dynamic_cast<VirtualizingUniformCarouselStackLayout^>(Layout);

        if (layout != nullptr)
        {
            *offset = layout->FirstSnapPointOffset;
            return static_cast<float>(layout->ItemWidth + layout->Spacing);
        }*/
    }

    *offset = 0.0f;
    return 0.0f;
}

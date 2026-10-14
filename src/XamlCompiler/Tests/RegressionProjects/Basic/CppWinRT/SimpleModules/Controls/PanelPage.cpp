#include <unknwn.h>
#ifdef WINRT_IMPORT_MODULE
import Simple.Application_Xaml.Controls.PanelPage;
#endif
#include "PanelPage.h"
#include "Controls/PanelPage.g.cpp"
namespace winrt::Simple::Controls::implementation
{
    PanelPage::PanelPage() { InitializeComponent(); }
}

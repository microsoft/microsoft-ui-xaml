#include <unknwn.h>
#ifdef WINRT_IMPORT_MODULE
import Simple.Application_Xaml.Views.PanelPage;
#endif
#include "PanelPage.h"
#include "Views/PanelPage.g.cpp"
namespace winrt::Simple::Views::implementation
{
    PanelPage::PanelPage() { InitializeComponent(); }
}

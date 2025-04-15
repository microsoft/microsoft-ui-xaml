# NavigationView Overview

NavigationView has multiple modes and various features. Please refer to the [public documentation linked here](https://learn.microsoft.com/en-us/windows/apps/design/controls/navigationview) for this information.

[Link to internal NavigationView API Spec](https://microsoft.sharepoint.com/:w:/r/teams/specstore/Developer%20Platform%20Team%20DEP/Redstone/NavigationView_API%20Spec.docx?d=w7550b1936a5c4d4188da05720cf091f9&csf=1&web=1&e=rrM9HT)
[Link to internal NavigationView Updates API Spec](https://microsoft.sharepoint.com/:w:/r/teams/specstore/Developer%20Platform%20Team%20DEP/Redstone/NavigationView_updates_API%20Spec.docx?d=w96af3c61351a42a099cd3016ea0fe4f0&csf=1&web=1&e=kQRyFd)

## Some History

### Listview to ItemsRepeater Transition

[Link to relevant Pull Request.](https://github.com/microsoft/microsoft-ui-xaml/pull/1683)

Originally NavigationView used ListView in order to display the list of items. ListView handled various functionality
internally (such as selection) which required NavigationView to adapt its internal implementation to account for 
ListView behaviour. As more features got added to NavigationView, the fighting with ListView became more difficult and 
the code written became less legible and straightforward. 

When implementing Heirarchical NavigationView, the decision was made to replace ListView internally with
ItemsRepeater and SelectionModel. This made it easier to implement Heirarchical functionality and later
add the FooterMenuItems functionality.

### Heirarchical NavigationView Addition

[Link to revelant functional spec.](https://microsoft.sharepoint.com/:w:/r/teams/specstore/Developer%20Platform%20Team%20DEP/Redstone/NavigationView_Hierarchy_FunctionalSpec.docx?d=w72ac844b58134381a558fc8d29bdeb79&csf=1&web=1&e=GDTAhN)

[Link to relevant Pull Request.](https://github.com/microsoft/microsoft-ui-xaml/pull/2004)

This was the first feature addition to NavigationView which made use of the, at the time, new addition
of ItemsRepeater and SelectionModel. SelectionModel simplified the implementation of multi-level selection
and ItemsRepeater simplified the ability to display nested items, whether inline or in a flyout.

### FooterMenuItems Addition

[Link to relevant Pull Request.](https://github.com/microsoft/microsoft-ui-xaml/pull/1997) 

FooterMenuItems functionality was contributed by an open source community member.
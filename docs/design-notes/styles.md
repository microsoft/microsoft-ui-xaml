# Xaml Styles

This doc supplements the information at the [XAML styles](https://learn.microsoft.com/en-us/windows/apps/develop/platform/xaml/xaml-styles) doc.

[[_TOC_]]


## Property System

The property system has multiple layers. One is `BaseValueSourceStyle`, and another is `BaseValueSourceBuiltInStyle`. Values can be applied at both layers, in which case the BVSStyle layer takes precedence.

Styles can be applied at both the BVSStyle and the BVSBuiltInStyle layers. This is the only way that multiple styles can coexist. Within the same layer, setting one style will overwrite the previous style. Styles applied at the two layers will coexist if they have Setters that set different properties. If both styles have a Setter that sets the same property, the style at the BVSStyle layer wins.


## Implicit Styles

Implicit styles are applied in multiple places. CControl applies it at the `BVSBuiltInStyle` layer, but it also calls the base CFrameworkElement (in both CreationComplete and EnterImpl) to apply it at the `BVSStyle` layer.

CControl finds the implicit style in `CControl::GetBuiltInStyle`, which uses `DefaultStyles::GetDefaultStyleByKey`. The key that it looks up comes from the `Control.DefaultStyleKey` property, and is resolved by `DefaultStyles::ResolveTypeName`.

The abbreviated call stack is:
* CControl::ApplyBuiltInStyle
    * CControl::GetBuiltInStyle
        * DirectUI::DefaultStyles::GetDefaultStyleByKey
            * DirectUI::DefaultStyles::GetDefaultStyleByTypeInfo
                * ...which uses the FrameworkStyles, which includes Application.Resources
    * CControl::OnStyleChanged

> Note that CControl is the _only_ place that can set a style at the BaseValueSourceBuiltInStyle layer. All other styles, implicit or explicit, defined in MUXC or the rest of the Application, can only apply style values at the BaseValueSourceStyle layer.

CFrameworkElement finds the implicit style in `CFrameworkElement::EnsureImplicitStyle`, which uses `ResourceResolver::ResolveImplicitStyleKeyImpl`, using the full class name as the style key.

The abbreviated call stack is:
* CFrameworkElement::ApplyStyle
    * CFrameworkElement::EnsureImplicitStyle
        * Resources::ResourceResolver::ResolveImplicitStyleKeyImpl
            * ...which uses Application.Resources directly
    * CFrameworkElement::OnStyleChanged


## Search Paths

Note that CControl and CFrameworkElement search through different ResourceDictionaries. CFrameworkElement's search goes directly to Application.Resources, which first checks any RDs provided by the app, then checks MUXC's themeresources_perf2026.xaml. Once a hit is found, we stop. CControl's search goes to FrameworkStyles, which first includes the "global theme resources", MUX's generic.xaml. If a hit is found we _do not stop_ there. We search all of Application.Resources for an override (see `CResourceDictionary::GetKeyFromGlobalThemeResourceNoRef`), which will come back to MUXC and find another hit. Altogether we can end up looking up a style 3 times
    a. CFrameworkElement::ApplyStyle
    b. CControl::GetBuiltInStyle
    c. CControl::GetBuiltInStyle (looking for an override in Application.Resources)

In cases like CommandBar where MUXC defines an implicit style (e.g. `DefaultCommandBarStyle`) meant to _completely replace_ MUX's built-in style (e.g. `CommandBarRevealStyle`), we can end up doing wasted work by loading the MUX style which won't actually apply any properties. As an optimization we want to skip this work.

There are other instances of controls providing implicit styles meant to _supplement_ MUX's built-in style, like in an "EmptyStateHyperlinkStyle" style for HyperlinkButton, which defines only four alignment/font/background properties and does _not_ contain a setter for Control.Template. That means it's relying on the template from MUX's built-in style in generic.xaml.


## Implicit Style Optimizations

We want to limit the work done loading a built-in style when an implicit style will just override it entirely, but the use cases above limit our options for optimizing the styles themselves:

* One option is to remove the MUX style and leave only the MUXC style, but this breaks implicit styles meant to supplement MUX's built-in styles. If we did this, the "EmptyStateHyperlinkStyle" HyperlinkButtons would stop rendering because their Templates would be null. This also breaks apps that only reference MUX without including MUXC.
* The other option is to remove the MUXC style and rely only on the MUX style, but this requires pushing the MUXC style into MUX. MUXC has styles that reference types defined in MUXC (e.g. `AnimatedIcon`) which MUX knows nothing about. These styles cannot be pushed into MUX without merging MUXC in first.

So we optimize by defer evaluating the Setter.Value of a style until something asks for the value. For example, for the Control.Template property, we'll load the MUX style and find the setter, applying it at the BVSBuiltInStyle layer without evaluating the Setter.Value. Then we load the MUXC style and find the setter, applying it at the BVSStyle layer also without evaluating Setter.Value. When the control asks for the Template property, the MUXC style's setter is the one at the higher priority so it's the one that gets realized and returned. The MUX setter for Control.Template is never realized, and we don't pay the cost of creating that ControlTemplate or doing all the StaticResource/ThemeResource lookups inside.


## Other Places Applying Styles

There's a potential _third pass_ to applying styles, in DirectUI::ItemsControl::PrepareContainerForItemOverrideImpl. `DirectUI::ItemsControl::ApplyItemContainerStyle` directly calls put_Style on the item container, passing in the ItemContainerStyle. This eventually calls CFrameworkElement::OnStyleChanged as well.

This one is notable because it's not an "implicit style". ItemsControl.ItemsContainerStyle is a way to explicitly specify a style on child item. This happens as the ItemsControl creates its children, which is after CreationComplete and after EnterImpl. This applies a style at the BVSStyle layer, and will overwrite the style that CFrameworkElement applied.

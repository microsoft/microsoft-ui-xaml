// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;

namespace Microsoft.UI.Xaml.Tests.Framework.ApiSurface
{
    // Operation kinds the sweep performs on a member. Expected failures are keyed per operation so that, for
    // example, a setter-only failure can be allow-listed without affecting the getter of the same property.
    internal static class SweepOperation
    {
        public const string Activate = "activate";
        public const string Get = "get";
        public const string GetValue = "getvalue";
        public const string Set = "set";
        public const string SetSample = "set-sample";
        public const string Restore = "restore";
        public const string ClearValue = "clearvalue";
        public const string AddRemove = "add-remove";
        public const string Parse = "parse";
        public const string CreatePeer = "create-peer";
    }

    // A single member operation that is known to fail with one exact exception type and HRESULT in the sweep's
    // detached (not-in-tree) context. The sweep treats only an exactly matching failure as expected; a different
    // exception type, a different HRESULT or an unexpected success is a test failure, so an entry can never hide a
    // behavior change.
    internal sealed class ExpectedFailure
    {
        public ExpectedFailure(Type exceptionType, int hresult, string reason)
        {
            ExceptionType = exceptionType ?? throw new ArgumentNullException(nameof(exceptionType));
            HResult = hresult;
            Reason = reason;
        }

        public Type ExceptionType { get; }
        public int HResult { get; }
        public string Reason { get; }

        public bool Matches(Exception error) => error != null && error.GetType() == ExceptionType && error.HResult == HResult;

        public override string ToString() => $"{ExceptionType.Name} 0x{HResult:X8} ({Reason})";
    }

    // The exact outcome of writing one specific distinct sample to a property when the product does not simply store
    // it: either an intended coercion (source-documented) or a reported product concern. The sweep asserts the recorded
    // getter and GetValue results exactly, and fails with "no longer reproduces" once the sample is observed unchanged.
    internal sealed class KnownSampleOutcome
    {
        public KnownSampleOutcome(string sample, string getterValue, string storedValue, string reason)
        {
            Sample = sample;
            GetterValue = getterValue;
            StoredValue = storedValue;
            Reason = reason;
        }

        public string Sample { get; }
        public string GetterValue { get; }
        public string StoredValue { get; }
        public string Reason { get; }
    }

    internal enum GetterStoreDivergenceKind
    {
        // The getter returns an object of GetterType while GetValue returns null (the store is empty).
        EmptyStore,
        // The getter and GetValue both return a GetterType object, but not the same instance.
        FreshInstanceSameType,
    }

    // A DependencyProperty-backed property whose getter result was observed to differ from GetValue in a recorded way.
    // GetterType is compared with the runtime type's Type.ToString(). The sweep asserts the recorded divergence exactly,
    // fails with "no longer reproduces" once getter and GetValue agree, and fails if the entry is never exercised.
    internal sealed class KnownGetterStoreDivergence
    {
        public KnownGetterStoreDivergence(GetterStoreDivergenceKind kind, string getterType)
        {
            Kind = kind;
            GetterType = getterType;
        }

        public GetterStoreDivergenceKind Kind { get; }
        public string GetterType { get; }
    }

    // A DependencyProperty-backed instance property whose hand-written accessor reads a private field instead of the
    // DependencyProperty store, so a value set through the DependencyProperty (for example from XAML) is not returned by
    // the getter. The sweep asserts the split exactly: the DependencyProperty holds the XAML value while the getter
    // keeps returning GetterValue; it fails with "no longer reproduces" once the getter observes the XAML value.
    internal sealed class KnownPropertyStoreSplit
    {
        public KnownPropertyStoreSplit(string getterValue, string concernId, string description)
        {
            GetterValue = getterValue;
            ConcernId = concernId;
            Description = description;
        }

        public string GetterValue { get; }
        public string ConcernId { get; }
        public string Description { get; }
    }

    internal enum IidDefectKind
    {
        // GetIids reports the IID but QueryInterface for it fails.
        ReportedButNotQueryable,
        // QueryInterface succeeds for a projected statics/factory interface that GetIids does not report.
        QueryableButNotReported,
    }

    internal sealed class KnownIidDefect
    {
        public KnownIidDefect(string subject, string iid, IidDefectKind kind, int queryHResult, string concernId, string description)
        {
            Subject = subject;
            Iid = new Guid(iid);
            Kind = kind;
            QueryHResult = queryHResult;
            ConcernId = concernId;
            Description = description;
        }

        public string Subject { get; }
        public Guid Iid { get; }
        public IidDefectKind Kind { get; }
        public int QueryHResult { get; }
        public string ConcernId { get; }
        public string Description { get; }
        public string Key => $"{ConcernId} {Subject} {{{Iid}}}";
    }

    // Explicit, reasoned scoping for the generated API-surface sweep. Every entry names one type or member and why
    // it is excluded or expected to fail; there are no wildcard or blanket exclusions.
    internal static class ApiSurfaceAllowList
    {
        // Runtime class names that are never passed to Microsoft.UI.Xaml.dll!DllGetActivationFactory. Their factory
        // creators deliberately terminate the process with IFCFAILFAST(E_NOTIMPL) instead of returning an error
        // (dxaml\xcp\components\UIBridgeFocus\NavigationFocusEventArgs.cpp, "Bug 17347047: CodeGen should not
        // generate factory code for non activatable event args classes"), so they cannot be probed in-process.
        // Neither name is a registered activatable class (absent from LiftedWinRTClassRegistrations.xml).
        public static readonly IReadOnlyDictionary<string, string> FactoryProbeExclusions = new Dictionary<string, string>
        {
            ["Microsoft.UI.Xaml.Hosting.DesktopWindowXamlSourceGotFocusEventArgs"] = "Factory creator fail-fasts with E_NOTIMPL (NavigationFocusEventArgs.cpp, Bug 17347047).",
            ["Microsoft.UI.Xaml.Hosting.DesktopWindowXamlSourceTakeFocusRequestedEventArgs"] = "Factory creator fail-fasts with E_NOTIMPL (NavigationFocusEventArgs.cpp, Bug 17347047).",
        };
        // Runtime classes the sweep never default-activates. These are still covered by the activation-factory sweep.
        public static readonly IReadOnlyDictionary<string, string> ActivationExclusions = new Dictionary<string, string>
        {
            ["Microsoft.UI.Xaml.Application"] = "Process singleton; the test host already owns the Application instance.",
            ["Microsoft.UI.Xaml.Window"] = "Creates a top-level desktop window; not part of the detached object surface.",
            ["Microsoft.UI.Xaml.Hosting.DesktopWindowXamlSource"] = "Creates a XAML island bound to a host HWND.",
        };

        // Activatable types whose instance properties/events are not swept, because mutating them changes
        // process-wide test infrastructure state rather than the state of the instance. (DxamlCoreTestHooks is not
        // part of the public projection and is therefore already out of scope.)
        public static readonly IReadOnlyDictionary<string, string> InstanceSweepExclusions = new Dictionary<string, string>();

        // Types whose instance properties are not read by the generic property sweep because doing so crashes the test
        // host on the current product (reported product concern). Each type has a dedicated strict test method that is
        // marked Ignore because it crashes the host; re-enable that method and remove the entry when the defect is fixed.
        public static readonly IReadOnlyDictionary<string, string> ProductConcernTypes = new Dictionary<string, string>
        {
            ["Microsoft.UI.Xaml.Input.AccessKeyDisplayRequestedEventArgs"] =
                "PC-4: default-constructed args get a plain CEventArgs core peer (FrameworkEventArgs.cpp CreateCorePeer) that " +
                "get_PressedKeys static_casts to CAccessKeyDisplayRequestedEventArgs (access violation); see VerifyDefaultConstructedAccessKeyDisplayRequestedEventArgs.",
            ["Microsoft.UI.Xaml.Input.AccessKeyInvokedEventArgs"] =
                "PC-4: same core-peer type mismatch for get_/put_Handled (CAccessKeyInvokedEventArgs); see VerifyDefaultConstructedAccessKeyInvokedEventArgs.",
            ["Microsoft.UI.Xaml.Media.Imaging.RenderTargetBitmap"] =
                "PC-13: before RenderAsync the core m_impl is null and the PixelWidth/PixelHeight property methods dereference it " +
                "(CRenderTargetBitmap::PixelWidth/PixelHeight, access violation); see VerifyDefaultConstructedRenderTargetBitmapPixelSize.",
            ["Microsoft.UI.Xaml.Markup.ProvideValueTargetProperty"] =
                "PC-7: default-constructed instance has a null m_xamlProperty that get_DeclaringTypeImpl/get_NameImpl dereference " +
                "(ProvideValueTargetProperty_Partial.cpp, access violation); see VerifyDefaultConstructedProvideValueTargetProperty.",
        };

        // Writable properties whose setter is not invoked by the generic round-trip, keyed "DeclaringType.Property".
        public static readonly IReadOnlyDictionary<string, string> PropertyWriteExclusions = BuildPropertyWriteExclusions();

        // Known product defects in IInspectable::GetIids vs QueryInterface consistency, recorded exactly so the sweep
        // stays strict: a listed mismatch must still occur exactly as recorded (otherwise the test fails and asks for
        // the entry to be removed), and any mismatch that is not listed fails. Subjects are the runtime class name for
        // instances and "<runtime class name> factory" for activation factories. QueryHResult is the QueryInterface
        // result for a ReportedButNotQueryable entry; E_NOINTERFACE follows from the QueryInterfaceImpl fall-through
        // to the base implementation for an IID the class does not handle.
        public static readonly IReadOnlyList<KnownIidDefect> KnownProductDefects = new[]
        {
            new KnownIidDefect("Microsoft.UI.Xaml.BrushTransition", "E7BEAEE7-160E-50F7-8789-D63463F979FA", IidDefectKind.ReportedButNotQueryable, InspectableAbiHelper.E_NOINTERFACE,
                "PC-2", "GetIids reports IDependencyObject but BrushTransition is not a DependencyObject."),
            new KnownIidDefect("Microsoft.UI.Xaml.ScalarTransition", "E7BEAEE7-160E-50F7-8789-D63463F979FA", IidDefectKind.ReportedButNotQueryable, InspectableAbiHelper.E_NOINTERFACE,
                "PC-2", "GetIids reports IDependencyObject but ScalarTransition is not a DependencyObject."),
            new KnownIidDefect("Microsoft.UI.Xaml.Vector3Transition", "E7BEAEE7-160E-50F7-8789-D63463F979FA", IidDefectKind.ReportedButNotQueryable, InspectableAbiHelper.E_NOINTERFACE,
                "PC-2", "GetIids reports IDependencyObject but Vector3Transition is not a DependencyObject."),
            new KnownIidDefect("Microsoft.UI.Xaml.UIElementWeakCollection", "E7BEAEE7-160E-50F7-8789-D63463F979FA", IidDefectKind.ReportedButNotQueryable, InspectableAbiHelper.E_NOINTERFACE,
                "PC-2", "GetIids reports IDependencyObject but UIElementWeakCollection is not a DependencyObject."),
            new KnownIidDefect("Microsoft.UI.Xaml.Automation.AutomationElementIdentifiers factory", "2FB51A33-B0CF-5A4C-9ED3-267ECA7AEEFC", IidDefectKind.ReportedButNotQueryable, InspectableAbiHelper.E_NOINTERFACE,
                "PC-3", "Factory GetIids reports the instance interface IAutomationElementIdentifiers."),
            new KnownIidDefect("Microsoft.UI.Xaml.Automation.AutomationProperties factory", "525C6A71-DD8A-52A0-977B-DB1B02F8E896", IidDefectKind.ReportedButNotQueryable, InspectableAbiHelper.E_NOINTERFACE,
                "PC-3", "Factory GetIids reports the instance interface IAutomationProperties."),
            new KnownIidDefect("Microsoft.UI.Xaml.Controls.Primitives.LayoutInformation factory", "CEEA0A8C-5A4F-5D7A-8FEA-77B5E0E0230C", IidDefectKind.ReportedButNotQueryable, InspectableAbiHelper.E_NOINTERFACE,
                "PC-3", "Factory GetIids reports the instance interface ILayoutInformation."),
            new KnownIidDefect("Microsoft.UI.Xaml.Hosting.XamlSourceFocusNavigationResult factory", "F533F53B-5C00-5C88-9A41-3888CB86E495", IidDefectKind.ReportedButNotQueryable, InspectableAbiHelper.E_NOINTERFACE,
                "PC-3", "Factory GetIids reports IXamlSourceFocusNavigationResultFactory but the factory does not implement it."),
            new KnownIidDefect("Microsoft.UI.Xaml.Media.CompositionTarget factory", "12A4BE6F-6DB1-5165-B622-D57AB782745B", IidDefectKind.QueryableButNotReported, 0,
                "PC-3", "Factory implements ICompositionTargetStatics but GetIids omits it."),
        };

        public static bool TryGetKnownIidDefect(string subject, Guid iid, IidDefectKind kind, out KnownIidDefect defect)
        {
            defect = KnownProductDefects.FirstOrDefault(d => d.Subject == subject && d.Iid == iid && d.Kind == kind);
            return defect != null;
        }

        // Member operations that fail with a specific HRESULT on a detached instance, keyed
        // "DeclaringType.Member:operation" where operation is one of the SweepOperation constants.
        public static readonly IReadOnlyDictionary<string, ExpectedFailure> ExpectedFailures = BuildExpectedFailures();

        // Keyed by "<declaring type full name>.<property>". Observed on WinUiTestVM1 (validation attempt 8b) on
        // default-constructed, detached instances.
        public static readonly IReadOnlyDictionary<string, KnownGetterStoreDivergence> KnownGetterStoreDivergences = new Dictionary<string, KnownGetterStoreDivergence>
        {
            ["Microsoft.UI.Xaml.Printing.PrintDocument.DocumentSource"] = new KnownGetterStoreDivergence(GetterStoreDivergenceKind.EmptyStore, "WinRT.IInspectable"),
            ["Microsoft.UI.Xaml.UIElement.RenderTransform"] = new KnownGetterStoreDivergence(GetterStoreDivergenceKind.EmptyStore, "Microsoft.UI.Xaml.Media.MatrixTransform"),
            ["Microsoft.UI.Xaml.Controls.Frame.BackStack"] = new KnownGetterStoreDivergence(GetterStoreDivergenceKind.EmptyStore, "System.Collections.Generic.IListImpl`1[Microsoft.UI.Xaml.Navigation.PageStackEntry]"),
            ["Microsoft.UI.Xaml.Controls.Frame.ForwardStack"] = new KnownGetterStoreDivergence(GetterStoreDivergenceKind.EmptyStore, "System.Collections.Generic.IListImpl`1[Microsoft.UI.Xaml.Navigation.PageStackEntry]"),
            ["Microsoft.UI.Xaml.Controls.ScrollViewer.ZoomSnapPoints"] = new KnownGetterStoreDivergence(GetterStoreDivergenceKind.EmptyStore, "System.Collections.Generic.IListImpl`1[System.Single]"),
            ["Microsoft.UI.Xaml.Controls.FontIconSource.FontFamily"] = new KnownGetterStoreDivergence(GetterStoreDivergenceKind.FreshInstanceSameType, "Microsoft.UI.Xaml.Media.FontFamily"),
            ["Microsoft.UI.Xaml.Media.Brush.Transform"] = new KnownGetterStoreDivergence(GetterStoreDivergenceKind.FreshInstanceSameType, "Microsoft.UI.Xaml.Media.MatrixTransform"),
            ["Microsoft.UI.Xaml.Media.Brush.RelativeTransform"] = new KnownGetterStoreDivergence(GetterStoreDivergenceKind.FreshInstanceSameType, "Microsoft.UI.Xaml.Media.MatrixTransform"),
        };

        // Keyed by "<declaring type full name>.<property>"; values are enum member names or "True"/"False".
        public static readonly IReadOnlyDictionary<string, KnownSampleOutcome> KnownSampleOutcomes = new Dictionary<string, KnownSampleOutcome>
        {
            // Intended coercion: FlyoutBase::UpdateStateToShowMode (FlyoutBase_partial.cpp) rewrites Auto to Standard.
            ["Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase.ShowMode"] = new KnownSampleOutcome(
                "Auto", "Standard", "Standard", "Coercion: FlyoutBase::UpdateStateToShowMode stores Standard when Auto is set."),
        };

        // Keyed by "<declaring type full name>.<property>".
        public static readonly IReadOnlyDictionary<string, KnownPropertyStoreSplit> KnownPropertyStoreSplits = new Dictionary<string, KnownPropertyStoreSplit>
        {
            // PC-10: AutoSuggestBoxTextChangedEventArgs_Partial.h overrides get_/put_Reason with a private m_reason field,
            // while the generated ReasonProperty DependencyProperty is a separate store written by XAML and SetValue.
            ["Microsoft.UI.Xaml.Controls.AutoSuggestBoxTextChangedEventArgs.Reason"] = new KnownPropertyStoreSplit(
                "ProgrammaticChange", "PC-10", "Reason getter reads private m_reason, not the ReasonProperty value set from XAML."),
        };

        // Every entry below was observed on WinUiTestVM1 (validation attempt 3, amd64chk @ 82b7a2f6) with exactly this HRESULT.
        private static Dictionary<string, ExpectedFailure> BuildExpectedFailures()
        {
            var failures = new Dictionary<string, ExpectedFailure>();

            // PC-5: CTextElement does not override CDependencyObject::AddEventListener (which returns E_NOTIMPL; only
            // CHyperlink does), so subscribing to TextElement access-key events fails on non-Hyperlink text elements.
            foreach (string eventName in new[] { "AccessKeyDisplayDismissed", "AccessKeyDisplayRequested", "AccessKeyInvoked" })
            {
                failures[OperationKey("Microsoft.UI.Xaml.Documents.TextElement." + eventName, SweepOperation.AddRemove)] = new ExpectedFailure(
                    typeof(NotImplementedException), InspectableAbiHelper.E_NOTIMPL, "PC-5: TextElement access-key event registration is not implemented for non-Hyperlink text elements.");
            }

            // PC-6: the XAML VirtualKey value table (satVirtualKey in components\metadata\inc\EnumValueTable.g.h) ends at
            // RightMenu and lacks these Windows.System.VirtualKey names, so they cannot be used as XAML text.
            foreach (string keyName in new[] {
                "Favorites", "GamepadA", "GamepadB", "GamepadDPadDown", "GamepadDPadLeft", "GamepadDPadRight", "GamepadDPadUp",
                "GamepadLeftShoulder", "GamepadLeftThumbstickButton", "GamepadLeftThumbstickDown", "GamepadLeftThumbstickLeft",
                "GamepadLeftThumbstickRight", "GamepadLeftThumbstickUp", "GamepadLeftTrigger", "GamepadMenu", "GamepadRightShoulder",
                "GamepadRightThumbstickButton", "GamepadRightThumbstickDown", "GamepadRightThumbstickLeft", "GamepadRightThumbstickRight",
                "GamepadRightThumbstickUp", "GamepadRightTrigger", "GamepadView", "GamepadX", "GamepadY", "GoBack", "GoForward", "GoHome",
                "ImeOff", "ImeOn", "NavigationAccept", "NavigationCancel", "NavigationDown", "NavigationLeft", "NavigationMenu",
                "NavigationRight", "NavigationUp", "NavigationView", "Refresh", "Search", "Stop" })
            {
                failures[OperationKey("Microsoft.UI.Xaml.Input.KeyboardAccelerator.Key=" + keyName, SweepOperation.Parse)] = new ExpectedFailure(
                    typeof(Microsoft.UI.Xaml.Markup.XamlParseException), InspectableAbiHelper.E_XAMLPARSEFAILED, "PC-6: VirtualKey name missing from the XAML enum value table.");
            }

            // PC-9: Typography attached properties set on the sweep's DependencyObject target cannot be cleared
            // (ClearValue fails with E_INVALIDARG although Set/Get succeed).
            foreach (string name in new[] {
                "AnnotationAlternates", "Capitals", "CapitalSpacing", "CaseSensitiveForms", "ContextualAlternates", "ContextualLigatures",
                "ContextualSwashes", "DiscretionaryLigatures", "EastAsianExpertForms", "EastAsianLanguage", "EastAsianWidths", "Fraction",
                "HistoricalForms", "HistoricalLigatures", "Kerning", "MathematicalGreek", "NumeralAlignment", "NumeralStyle", "SlashedZero",
                "StandardLigatures", "StandardSwashes", "StylisticAlternates", "StylisticSet1", "StylisticSet10", "StylisticSet11",
                "StylisticSet12", "StylisticSet13", "StylisticSet14", "StylisticSet15", "StylisticSet16", "StylisticSet17", "StylisticSet18",
                "StylisticSet19", "StylisticSet2", "StylisticSet20", "StylisticSet3", "StylisticSet4", "StylisticSet5", "StylisticSet6",
                "StylisticSet7", "StylisticSet8", "StylisticSet9", "Variants" })
            {
                failures[OperationKey("Microsoft.UI.Xaml.Documents.Typography." + name, SweepOperation.ClearValue)] = new ExpectedFailure(
                    typeof(ArgumentException), InspectableAbiHelper.E_INVALIDARG, "PC-9: ClearValue of a Typography attached property fails after a successful Set.");
            }

            // Observed, contract not established: these classes are rejected as the root of XamlReader.Load with a
            // regular XAML parse error.
            foreach (string rootType in new[] { "Microsoft.UI.Xaml.Automation.Peers.AutomationPeerAnnotation", "Microsoft.UI.Xaml.Printing.PrintDocument" })
            {
                failures[OperationKey(rootType + " object element", SweepOperation.Parse)] = new ExpectedFailure(
                    typeof(Microsoft.UI.Xaml.Markup.XamlParseException), InspectableAbiHelper.E_XAMLPARSEFAILED,
                    "Observed: not creatable as a standalone XamlReader.Load root object.");
            }

            // PC-11: loading these classes as the XamlReader.Load root lets a native structured exception (E_FAIL) escape
            // to the caller as SEHException instead of a XamlParseException.
            foreach (string rootType in new[] {
                "Microsoft.UI.Xaml.Data.Binding", "Microsoft.UI.Xaml.Data.RelativeSource",
                "Microsoft.UI.Xaml.Media.Imaging.RenderTargetBitmap", "Microsoft.UI.Xaml.Style" })
            {
                failures[OperationKey(rootType + " object element", SweepOperation.Parse)] = new ExpectedFailure(
                    typeof(global::System.Runtime.InteropServices.SEHException), InspectableAbiHelper.E_FAIL,
                    "PC-11: native SEH exception escapes XamlReader.Load for this root object.");
            }
            AddInstancePropertyFailures(failures);
            return failures;
        }

        // Observed on WinUiTestVM1 (validation attempt 8b) in the instance property sweep, each with exactly this exception
        // type and HRESULT on a default-constructed, detached instance.
        private static void AddInstancePropertyFailures(Dictionary<string, ExpectedFailure> failures)
        {
            void Add(string member, string operation, Type exceptionType, int hresult, string reason) =>
                failures[OperationKey(member, operation)] = new ExpectedFailure(exceptionType, hresult, reason);

            // Observed, contract not established: these members fail on a default-constructed or detached instance.
            foreach (string name in new[] { "Cumulative", "Delta" })
            {
                Add("Microsoft.UI.Xaml.Input.ManipulationDeltaRoutedEventArgs." + name, SweepOperation.Get, typeof(NullReferenceException),
                    InspectableAbiHelper.E_POINTER, "Observed: default-constructed event args carry no manipulation data.");
            }
            foreach (string name in new[] { "ContentEnd", "ContentStart", "ElementEnd", "ElementStart" })
            {
                Add("Microsoft.UI.Xaml.Documents.TextElement." + name, SweepOperation.Get, typeof(global::System.Runtime.InteropServices.COMException),
                    unchecked((int)0x80131515), "Observed: text pointers are not available for a text element outside a text container.");
            }
            Add("Microsoft.UI.Xaml.Controls.VirtualizingPanel.ItemContainerGenerator", SweepOperation.Get, typeof(global::System.Runtime.InteropServices.COMException),
                unchecked((int)0x8000FFFF), "Observed: a panel that is not an items host has no item container generator.");
            Add("Microsoft.UI.Xaml.Controls.Control.IsFocusEngaged", SweepOperation.SetSample, typeof(global::System.Runtime.InteropServices.COMException),
                unchecked((int)0x800F1000), "Observed: focus engagement cannot be set on a control that is not focusable in the live tree.");
            Add("Microsoft.UI.Xaml.Controls.Primitives.Selector.IsSynchronizedWithCurrentItem", SweepOperation.SetSample, typeof(ArgumentException),
                InspectableAbiHelper.E_INVALIDARG, "Documented: setting IsSynchronizedWithCurrentItem to true is not supported.");
            Add("Microsoft.UI.Xaml.Style.TargetType", SweepOperation.Set, typeof(ArgumentNullException),
                InspectableAbiHelper.E_POINTER, "Projection: CsWinRT cannot marshal a null System.Type (ArgumentNullException 'key'); the call does not reach Microsoft.UI.Xaml.");

            // PC-17: reading ProofingMenuFlyout on a detached text box lets a native exception escape as SEHException.
            foreach (string owner in new[] { "Microsoft.UI.Xaml.Controls.RichEditBox", "Microsoft.UI.Xaml.Controls.TextBox" })
            {
                Add(owner + ".ProofingMenuFlyout", SweepOperation.Get, typeof(global::System.Runtime.InteropServices.SEHException),
                    InspectableAbiHelper.E_FAIL, "PC-17: native exception escapes get_ProofingMenuFlyout on a detached text control.");
            }

            // PC-18: writing back the value the getter returned (the default) is rejected.
            foreach (string name in new[] { "ExpansionBehavior", "RotationBehavior", "TranslationBehavior" })
            {
                Add("Microsoft.UI.Xaml.Input.ManipulationInertiaStartingRoutedEventArgs." + name, SweepOperation.Set, typeof(ArgumentException),
                    InspectableAbiHelper.E_INVALIDARG, "PC-18: setter rejects the value returned by its getter.");
            }
            foreach (string name in new[] { "RasterizePixelHeight", "RasterizePixelWidth" })
            {
                Add("Microsoft.UI.Xaml.Media.Imaging.SvgImageSource." + name, SweepOperation.Set, typeof(ArgumentException),
                    InspectableAbiHelper.E_INVALIDARG, "PC-18: setter rejects the value returned by its getter.");
            }
            Add("Microsoft.UI.Xaml.Setter.Property", SweepOperation.Set, typeof(NullReferenceException),
                InspectableAbiHelper.E_POINTER, "PC-18: setter rejects the value returned by its getter (null).");
            Add("Microsoft.UI.Xaml.Documents.Glyphs.FontUri", SweepOperation.Set, typeof(ArgumentException),
                InspectableAbiHelper.E_INVALIDARG, "PC-18: setter rejects the value returned by its getter (null).");

            // PC-19: writing the default (null) color lets a native exception escape as SEHException.
            foreach (string name in new[] {
                "Accent", "AltHigh", "AltLow", "AltMedium", "AltMediumHigh", "AltMediumLow", "BaseHigh", "BaseLow", "BaseMedium",
                "BaseMediumHigh", "BaseMediumLow", "ChromeAltLow", "ChromeBlackHigh", "ChromeBlackLow", "ChromeBlackMedium",
                "ChromeBlackMediumLow", "ChromeDisabledHigh", "ChromeDisabledLow", "ChromeGray", "ChromeHigh", "ChromeLow",
                "ChromeMedium", "ChromeMediumLow", "ChromeWhite", "ErrorText", "ListLow", "ListMedium" })
            {
                Add("Microsoft.UI.Xaml.ColorPaletteResources." + name, SweepOperation.Set, typeof(global::System.Runtime.InteropServices.SEHException),
                    InspectableAbiHelper.E_FAIL, "PC-19: native exception escapes when the default (null) color is written back.");
            }
        }

        private static Dictionary<string, string> BuildPropertyWriteExclusions()
        {
            var exclusions = new Dictionary<string, string>
            {
                ["Microsoft.UI.Xaml.Controls.Primitives.Popup.IsOpen"] = "Opening a popup creates a windowed popup outside the swept object.",
                ["Microsoft.UI.Xaml.Controls.MediaPlayerElement.AutoPlay"] =
                    "PC-20: writing AutoPlay creates a default MediaPlayer; when the element is later released during a render tick, " +
                    "releasing that MediaPlayer pumps messages and trips the dispatcher reentrancy assertion (0xC0000420); " +
                    "PC-16 and PC-20 are covered by VerifyMediaPlayerElementAutoPlayOnDetachedElement.",
                ["Microsoft.UI.Xaml.UIElement.CanDrag"] =
                    "PC-14: CanDrag = true on an element outside the live tree reaches CInputServices::GetIslandInputSiteRegistrationForUIElement, " +
                    "which fail-fasts when the element's island input site matches no registration; see VerifyDetachedElementCanDragAssignment.",
                ["Microsoft.UI.Xaml.Controls.PasswordBox.InputScope"] =
                    "PC-15: PasswordBox::put_InputScopeImpl calls get_Names on the InputScope argument without a null check, so writing " +
                    "the default (null) value crashes the host; see VerifyPasswordBoxInputScopeNullAssignment.",
                ["Microsoft.UI.Xaml.EventTrigger.RoutedEvent"] =
                    "PC-12: EventTrigger::put_RoutedEventImpl dereferences the RoutedEvent argument without a null check, so writing " +
                    "the default (null) value crashes the host; see VerifyEventTriggerRoutedEventNullAssignment.",
                ["Microsoft.UI.Xaml.Controls.ToolTip.IsOpen"] = "Opening a tooltip creates a windowed popup outside the swept object.",
                ["Microsoft.UI.Xaml.Controls.ComboBox.IsDropDownOpen"] = "Opening the drop-down creates a popup outside the swept object.",
                ["Microsoft.UI.Xaml.Controls.MediaPlayerElement.IsFullWindow"] = "Enters full-window mode for the whole test window.",
                ["Microsoft.UI.Xaml.Controls.Frame.SourcePageType"] = "Setting the page type starts a navigation.",
            };

            // Strict/non-strict ("facade") properties lock the object into one strictness mode on first write, so a
            // generic write order would make later writes fail by design. They are covered explicitly by
            // VerifyParserObjectElementsAndSimpleProperties.
            const string strictnessReason = "Strictness-governed property; covered by the explicit strict-mode scenario.";
            foreach (string name in new[] { "CenterPoint", "Rotation", "RotationAxis", "Scale", "TransformMatrix", "Translation",
                                            "OpacityTransition", "RotationTransition", "ScaleTransition", "TranslationTransition",
                                            "Projection", "RenderTransform", "RenderTransformOrigin", "Transform3D" })
            {
                exclusions["Microsoft.UI.Xaml.UIElement." + name] = strictnessReason;
            }
            foreach (string name in new[] { "CenterPoint", "Rotation", "Scale", "TransformMatrix", "Translation" })
            {
                exclusions["Microsoft.UI.Xaml.Media.LinearGradientBrush." + name] = strictnessReason;
            }
            foreach (string name in new[] { "RelativeTransform", "Transform" })
            {
                exclusions["Microsoft.UI.Xaml.Media.Brush." + name] = strictnessReason;
            }
            foreach (string owner in new[] { "Microsoft.UI.Xaml.Controls.Panel", "Microsoft.UI.Xaml.Controls.Border", "Microsoft.UI.Xaml.Controls.ContentPresenter" })
            {
                exclusions[owner + ".BackgroundTransition"] = strictnessReason;
            }
            return exclusions;
        }

        public static string OperationKey(string memberKey, string operation) => memberKey + ":" + operation;

        public static bool TryGetExpectedFailure(string memberKey, string operation, out ExpectedFailure expected)
        {
            return ExpectedFailures.TryGetValue(OperationKey(memberKey, operation), out expected);
        }

        public static bool HasExpectedFailure(string memberKey, string operation)
        {
            return ExpectedFailures.ContainsKey(OperationKey(memberKey, operation));
        }
    }
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Linq;

using MUXControlsTestApp.Utilities;

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.AnimatedVisuals;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.XamlTypeInfo;
using Windows.UI.Text;
using Windows.Foundation.Metadata;
using Common;
using Color = Windows.UI.Color;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class IconSourceApiTests : ApiTestBase
    {
        [TestMethod]
        public void ImageIconSourceTest()
        {
            ImageIconSource iconSource = null;
            ImageIcon imageIcon = null;
            var uri = new Uri("ms-appx:///Assets/Nuclear_symbol.svg");

            RunOnUIThread.Execute(() =>
            {
                iconSource = new ImageIconSource();
                imageIcon = iconSource.CreateIconElement() as ImageIcon;

                // IconSource.Foreground should be null to allow foreground inheritance from
                // the parent to work.
                Verify.AreEqual(iconSource.Foreground, null);
                //Verify.AreEqual(imageIcon.Foreground, null);

                Log.Comment("Validate the defaults match BitmapIcon.");

                var icon = new ImageIcon();
                Verify.AreEqual(icon.Source, iconSource.ImageSource);
                Verify.AreEqual(imageIcon.Source, iconSource.ImageSource);

                Log.Comment("Validate that you can change the properties.");

                iconSource.Foreground = new SolidColorBrush(Microsoft.UI.Colors.Red);
                iconSource.ImageSource = new SvgImageSource(uri);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(iconSource.Foreground is SolidColorBrush);
                Verify.IsTrue(imageIcon.Foreground is SolidColorBrush);
                Verify.AreEqual(Microsoft.UI.Colors.Red, (iconSource.Foreground as SolidColorBrush).Color);
                Verify.AreEqual(Microsoft.UI.Colors.Red, (imageIcon.Foreground as SolidColorBrush).Color);
                Verify.AreEqual(uri, ((SvgImageSource)iconSource.ImageSource).UriSource);
                Verify.AreEqual(uri, ((SvgImageSource)imageIcon.Source).UriSource);
            });
        }

        [TestMethod]
        public void AnimatedIconSourceTest()
        {
            AnimatedIconSource iconSource = null;
            IAnimatedVisualSource2 source = null;
            AnimatedIcon animatedIcon = null;

            RunOnUIThread.Execute(() =>
            {
                iconSource = new AnimatedIconSource();
                source = new AnimatedChevronDownSmallVisualSource();
                animatedIcon = iconSource.CreateIconElement() as AnimatedIcon;

                // IconSource.Foreground should be null to allow foreground inheritance from
                // the parent to work.
                Verify.AreEqual(iconSource.Foreground, null);
                //Verify.AreEqual(animatedIcon.Foreground, null);
                Verify.AreEqual(iconSource.MirroredWhenRightToLeft, false);
                Verify.AreEqual(animatedIcon.MirroredWhenRightToLeft, false);

                Log.Comment("Validate the defaults match BitmapIcon.");

                var icon = new AnimatedIcon();
                Verify.AreEqual(icon.Source, iconSource.Source);
                Verify.AreEqual(animatedIcon.Source, iconSource.Source);
                Verify.AreEqual(icon.MirroredWhenRightToLeft, iconSource.MirroredWhenRightToLeft);

                Log.Comment("Validate that you can change the properties.");

                iconSource.Foreground = new SolidColorBrush(Microsoft.UI.Colors.Red);
                iconSource.Source = source;
                iconSource.MirroredWhenRightToLeft = true;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(iconSource.Foreground is SolidColorBrush);
                Verify.IsTrue(animatedIcon.Foreground is SolidColorBrush);
                Verify.AreEqual(Microsoft.UI.Colors.Red, (iconSource.Foreground as SolidColorBrush).Color);
                Verify.AreEqual(Microsoft.UI.Colors.Red, (animatedIcon.Foreground as SolidColorBrush).Color);
                Verify.AreEqual(source, iconSource.Source);
                Verify.AreEqual(source, animatedIcon.Source);
                Verify.IsTrue(iconSource.MirroredWhenRightToLeft);
                Verify.IsTrue(animatedIcon.MirroredWhenRightToLeft);
            });
        }

        // Scenario: Creates an AnimatedIconSource with AnimatedBackVisualSource, a Back fallback, a green Foreground
        // and MirroredWhenRightToLeft true, then calls CreateIconElement.
        // Expected: the result is an AnimatedIcon with a green Foreground, the same Source and FallbackIconSource
        // objects (not copies) and MirroredWhenRightToLeft true.
        // Failure means: CreateIconElement does not copy AnimatedIconSource's properties to the created AnimatedIcon.
        [TestMethod]
        public void AnimatedIconSourceCopiesPropertiesToCreatedIcon()
        {
            var source = new AnimatedBackVisualSource();
            var iconSource = CreateAnimatedIconSource(source, Symbol.Back, Microsoft.UI.Colors.Green, mirroredWhenRightToLeft: true);

            var animatedIcon = CreateIconElement(iconSource) as AnimatedIcon;

            Verify.IsNotNull(animatedIcon);
            Verify.AreEqual(Microsoft.UI.Colors.Green, GetSolidForegroundColor(animatedIcon));
            Verify.IsTrue(HasSameSourceAndFallback(iconSource, animatedIcon));
            Verify.IsTrue(GetMirroredWhenRightToLeft(animatedIcon));
        }

        // Scenario: Creates an icon from an AnimatedIconSource with no fallback, then sets the source's
        // FallbackIconSource to a SymbolIconSource(Accept).
        // Expected: the icon has no fallback at first, then has the same Accept SymbolIconSource object as the
        // AnimatedIconSource.
        // Failure means: later FallbackIconSource changes on AnimatedIconSource are not pushed to icons it already
        // created.
        [TestMethod]
        public void AnimatedIconSourceFallbackIconSourcePropagatesToCreatedIcon()
        {
            var iconSource = CreateAnimatedIconSource(null, null, null, mirroredWhenRightToLeft: false);
            var animatedIcon = CreateIconElement(iconSource) as AnimatedIcon;
            Verify.IsNotNull(animatedIcon);
            Verify.AreEqual("null", DescribeFallbackIconSource(animatedIcon));

            SetFallbackSymbol(iconSource, Symbol.Accept);
            IdleSynchronizer.Wait();

            Verify.AreEqual("SymbolIconSource:" + Symbol.Accept, DescribeFallbackIconSource(animatedIcon));
            Verify.IsTrue(HasSameSourceAndFallback(iconSource, animatedIcon));
        }

        // Scenario: Reads the AnimatedIconSource Source, FallbackIconSource and MirroredWhenRightToLeft property
        // identifiers, constructs an AnimatedIconSource, then reads them again along with AnimatedIcon's same-named
        // ones.
        // Expected: the 3 identifiers are non-null, distinct and unchanged after construction, and separate from
        // AnimatedIcon's identifiers (6 distinct in total).
        // Failure means: AnimatedIconSource property registration is missing, is redone when an instance is
        // constructed, or is shared with AnimatedIcon.
        [TestMethod]
        public void AnimatedIconSourceDependencyPropertyIdentifiersAreRegisteredDistinctAndStable()
        {
            var identifiers = ReadAnimatedIconSourcePropertyIds();

            // Constructing an AnimatedIconSource runs EnsureProperties again; it must keep the registered identifiers.
            var iconSource = CreateEmptyAnimatedIconSource();
            Verify.IsNotNull(iconSource);
            var identifiersReadAgain = ReadAnimatedIconSourcePropertyIds();
            var animatedIconIdentifiers = ReadAnimatedIconPropertyIds();

            Verify.AreEqual(3, CountDistinctNonNull(identifiers, new DependencyProperty[0]));
            Verify.AreEqual(identifiers[0], identifiersReadAgain[0]);
            Verify.AreEqual(identifiers[1], identifiersReadAgain[1]);
            Verify.AreEqual(identifiers[2], identifiersReadAgain[2]);

            // The same-named AnimatedIcon properties are separate registrations owned by AnimatedIcon.
            Verify.AreEqual(6, CountDistinctNonNull(identifiers, animatedIconIdentifiers));
        }

        // Scenario: Checks the registered defaults, sets the three AnimatedIconSource properties through SetValue and
        // reads them through the CLR properties, then the reverse, then clears them.
        // Expected: defaults are null, null and false; a new instance has no local values; both access paths see the
        // same values; ClearValue restores the defaults and leaves no local values.
        // Failure means: AnimatedIconSource's CLR properties and dependency properties use different storage, or its
        // defaults or ClearValue behavior are wrong.
        [TestMethod]
        public void AnimatedIconSourceClrPropertiesAndDependencyPropertiesShareStorage()
        {
            // Source, FallbackIconSource, MirroredWhenRightToLeft.
            var ids = ReadAnimatedIconSourcePropertyIds();
            var defaults = GetMetadataDefaultValues(ids, typeof(AnimatedIconSource));
            Verify.IsNull(defaults[0]);
            Verify.IsNull(defaults[1]);
            Verify.AreEqual(false, defaults[2]);

            var iconSource = CreateEmptyAnimatedIconSource();
            Verify.AreEqual(3, CountUnsetLocalValues(iconSource, ids));

            // Values set through the identifiers are what the CLR properties return.
            var source = new AnimatedBackVisualSource();
            var fallback = CreateSymbolIconSource(Symbol.Accept);
            SetValueOf(iconSource, ids[0], source);
            SetValueOf(iconSource, ids[1], fallback);
            SetValueOf(iconSource, ids[2], true);
            Verify.AreEqual(source, GetIconSourceSource(iconSource));
            Verify.AreEqual(fallback, GetIconSourceFallback(iconSource));
            Verify.AreEqual(true, GetIconSourceMirrored(iconSource));

            // Values set through the CLR properties are stored under the identifiers.
            var otherSource = new AnimatedSettingsVisualSource();
            var otherFallback = CreateSymbolIconSource(Symbol.Back);
            SetIconSourceClrProperties(iconSource, otherSource, otherFallback, false);
            Verify.AreEqual(otherSource, GetValueOf(iconSource, ids[0]));
            Verify.AreEqual(otherFallback, GetValueOf(iconSource, ids[1]));
            Verify.AreEqual(false, GetValueOf(iconSource, ids[2]));

            // Clearing the local values restores the registered defaults.
            SetIconSourceClrProperties(iconSource, otherSource, otherFallback, true);
            ClearValuesOf(iconSource, ids);
            Verify.AreEqual(3, CountUnsetLocalValues(iconSource, ids));
            Verify.IsNull(GetIconSourceSource(iconSource));
            Verify.IsNull(GetIconSourceFallback(iconSource));
            Verify.AreEqual(false, GetIconSourceMirrored(iconSource));
        }

        // Scenario: Creates an icon from an empty AnimatedIconSource, then sets Source, FallbackIconSource and
        // MirroredWhenRightToLeft on the source through SetValue, sets mirroring back to false, and clears Source.
        // Expected: each change is immediately visible on the created icon; clearing Source makes the icon's Source
        // null while its FallbackIconSource is unchanged.
        // Failure means: AnimatedIconSource does not push property changes, or cleared values, to icons it already
        // created.
        [TestMethod]
        public void AnimatedIconSourceDependencyPropertyChangesPropagateToCreatedIcon()
        {
            var ids = ReadAnimatedIconSourcePropertyIds();
            var iconSource = CreateEmptyAnimatedIconSource();
            var animatedIcon = CreateIconElement(iconSource) as AnimatedIcon;
            Verify.IsNotNull(animatedIcon);

            // IconSource pushes each changed value into the icons it created, synchronously.
            var source = new AnimatedBackVisualSource();
            var fallback = CreateSymbolIconSource(Symbol.Accept);
            SetValueOf(iconSource, ids[0], source);
            SetValueOf(iconSource, ids[1], fallback);
            SetValueOf(iconSource, ids[2], true);
            Verify.AreEqual(source, GetAnimatedIconSourceValue(animatedIcon));
            Verify.AreEqual(fallback, GetAnimatedIconFallback(animatedIcon));
            Verify.AreEqual(true, GetMirroredWhenRightToLeft(animatedIcon));

            SetValueOf(iconSource, ids[2], false);
            Verify.AreEqual(false, GetMirroredWhenRightToLeft(animatedIcon));

            // Clearing a reference-typed value pushes its null default; the other values are untouched.
            ClearValuesOf(iconSource, ids[0]);
            Verify.IsNull(GetAnimatedIconSourceValue(animatedIcon));
            Verify.AreEqual(fallback, GetAnimatedIconFallback(animatedIcon));
        }

        // Scenario: Repro for a product bug. Creates an icon from an AnimatedIconSource, sets the source's
        // MirroredWhenRightToLeft to true, then calls ClearValue(MirroredWhenRightToLeftProperty) on the source. The
        // icon's getter is read inside a try/catch so an exception fails the test instead of crashing the process.
        // Expected: the icon reads true after the set and the registered default false after the clear, exactly as when
        // the value is cleared on the icon itself.
        // Failure means: the product bug is still present: the already-created icon's MirroredWhenRightToLeft getter
        // throws InvalidCastException (E_NOINTERFACE unboxing the pushed Boolean) instead of returning false. The test
        // is ignored until the bug is fixed; if it passes, the bug is fixed and the test should be re-enabled.
        [TestMethod]
        [TestProperty("Ignore", "True")] // Repro for the AnimatedIconSource ClearValue(MirroredWhenRightToLeftProperty) product bug. Re-enable when fixed.
        public void AnimatedIconSourceClearedMirroredWhenRightToLeftPropagatesDefaultToCreatedIcon()
        {
            var ids = ReadAnimatedIconSourcePropertyIds();
            var iconSource = CreateEmptyAnimatedIconSource();
            var animatedIcon = CreateIconElement(iconSource) as AnimatedIcon;
            Verify.IsNotNull(animatedIcon);

            SetValueOf(iconSource, ids[2], true);
            Verify.AreEqual("True", DescribeMirroredWhenRightToLeft(animatedIcon));

            ClearValuesOf(iconSource, ids[2]);
            Verify.AreEqual("False", DescribeMirroredWhenRightToLeft(animatedIcon));
        }

        // Scenario: Loads an AnimatedIconSource from XAML with MirroredWhenRightToLeft="True", an
        // AnimatedBackVisualSource Source and a SymbolIconSource(Back) fallback, then calls CreateIconElement.
        // Expected: the parsed source has those values, and the created AnimatedIcon shares the same Source and
        // fallback objects and has mirroring true.
        // Failure means: XAML parsing of AnimatedIconSource, or copying its parsed properties to the created icon, is
        // broken.
        [TestMethod]
        public void AnimatedIconSourceXamlMarkupCreatesConfiguredIcon()
        {
            var iconSource = LoadAnimatedIconSourceMarkup(c_animatedIconSourceMarkup);

            Verify.AreEqual(true, GetIconSourceMirrored(iconSource));
            Verify.AreEqual(typeof(AnimatedBackVisualSource).FullName, GetIconSourceSourceTypeName(iconSource));
            Verify.AreEqual("SymbolIconSource:" + Symbol.Back, DescribeIconSourceFallback(iconSource));

            var animatedIcon = CreateIconElement(iconSource) as AnimatedIcon;
            Verify.IsNotNull(animatedIcon);
            Verify.AreEqual(typeof(AnimatedBackVisualSource).FullName, GetAnimatedIconSourceTypeName(animatedIcon));
            Verify.AreEqual("SymbolIconSource:" + Symbol.Back, DescribeFallbackIconSource(animatedIcon));
            Verify.IsTrue(HasSameSourceAndFallback(iconSource, animatedIcon));
            Verify.AreEqual(true, GetMirroredWhenRightToLeft(animatedIcon));
        }

        // Scenario: Gets the AnimatedIconSource type from the controls XAML metadata provider, inspects its members,
        // activates an instance and sets and gets values through the metadata members.
        // Expected: the full name matches, there is no content property, Source, FallbackIconSource and
        // MirroredWhenRightToLeft are writable dependency properties, State is not a member, and member get/set reaches
        // the real properties.
        // Failure means: the generated XAML type information for AnimatedIconSource is wrong, which breaks XAML parsing
        // or binding of AnimatedIconSource.
        [TestMethod]
        public void AnimatedIconSourceXamlMetadataDescribesMembers()
        {
            var type = GetControlsXamlType(c_animatedIconSourceTypeName);

            Verify.AreEqual(c_animatedIconSourceTypeName, GetFullNameOf(type));
            // Unlike AnimatedIcon, AnimatedIconSource declares no content property.
            Verify.IsNull(GetContentPropertyName(type));
            Verify.AreEqual(
                "Source=DP,RW; FallbackIconSource=DP,RW; MirroredWhenRightToLeft=DP,RW; State=missing",
                DescribeMembers(type, new[] { "Source", "FallbackIconSource", "MirroredWhenRightToLeft", "State" }));

            var instance = ActivateXamlType(type);
            Verify.AreEqual(typeof(AnimatedIconSource), instance.GetType());
            var iconSource = (AnimatedIconSource)instance;

            // Member values go through the registered dependency properties.
            var source = new AnimatedBackVisualSource();
            SetMemberValue(type, "Source", iconSource, source);
            SetMemberValue(type, "MirroredWhenRightToLeft", iconSource, true);
            Verify.AreEqual(source, GetIconSourceSource(iconSource));
            Verify.AreEqual(true, GetIconSourceMirrored(iconSource));

            var fallback = CreateSymbolIconSource(Symbol.Find);
            SetFallbackIconSource(iconSource, fallback);
            Verify.AreEqual(fallback, GetMemberValue(type, "FallbackIconSource", iconSource));
        }

        private const string c_animatedIconSourceTypeName = "Microsoft.UI.Xaml.Controls.AnimatedIconSource";

        private const string c_animatedIconSourceMarkup =
            @"<controls:AnimatedIconSource xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                           xmlns:controls='using:Microsoft.UI.Xaml.Controls'
                                           xmlns:visuals='using:Microsoft.UI.Xaml.Controls.AnimatedVisuals'
                                           MirroredWhenRightToLeft='True'>
                <controls:AnimatedIconSource.Source>
                    <visuals:AnimatedBackVisualSource/>
                </controls:AnimatedIconSource.Source>
                <controls:AnimatedIconSource.FallbackIconSource>
                    <controls:SymbolIconSource Symbol='Back'/>
                </controls:AnimatedIconSource.FallbackIconSource>
            </controls:AnimatedIconSource>";

        // Source, FallbackIconSource, MirroredWhenRightToLeft.
        private static DependencyProperty[] ReadAnimatedIconSourcePropertyIds()
        {
            DependencyProperty[] ids = null;
            RunOnUIThread.Execute(() =>
            {
                ids = new[] { AnimatedIconSource.SourceProperty, AnimatedIconSource.FallbackIconSourceProperty, AnimatedIconSource.MirroredWhenRightToLeftProperty };
            });
            return ids;
        }

        // Source, FallbackIconSource, MirroredWhenRightToLeft.
        private static DependencyProperty[] ReadAnimatedIconPropertyIds()
        {
            DependencyProperty[] ids = null;
            RunOnUIThread.Execute(() =>
            {
                ids = new[] { AnimatedIcon.SourceProperty, AnimatedIcon.FallbackIconSourceProperty, AnimatedIcon.MirroredWhenRightToLeftProperty };
            });
            return ids;
        }

        private static int CountDistinctNonNull(DependencyProperty[] first, DependencyProperty[] second)
        {
            var all = first.Concat(second).Where(property => property != null).ToArray();
            return all.Where((property, index) => Array.FindIndex(all, other => ReferenceEquals(other, property)) == index).Count();
        }

        // "True"/"False", or "threw <exception type> 0x<HRESULT>" when the getter throws.
        private static string DescribeMirroredWhenRightToLeft(AnimatedIcon animatedIcon)
        {
            string description = null;
            RunOnUIThread.Execute(() =>
            {
                try
                {
                    description = animatedIcon.MirroredWhenRightToLeft.ToString();
                }
                catch (Exception e)
                {
                    description = "threw " + e.GetType().Name + " 0x" + e.HResult.ToString("X8");
                }
            });
            return description;
        }

        private static AnimatedIconSource CreateEmptyAnimatedIconSource()
        {
            AnimatedIconSource iconSource = null;
            RunOnUIThread.Execute(() => iconSource = new AnimatedIconSource());
            return iconSource;
        }

        private static object[] GetMetadataDefaultValues(DependencyProperty[] properties, Type ownerType)
        {
            object[] defaults = null;
            RunOnUIThread.Execute(() =>
            {
                defaults = properties.Select(property => property.GetMetadata(ownerType).DefaultValue).ToArray();
            });
            return defaults;
        }

        private static int CountUnsetLocalValues(DependencyObject element, DependencyProperty[] properties)
        {
            int count = -1;
            RunOnUIThread.Execute(() =>
            {
                count = properties.Count(property => element.ReadLocalValue(property) == DependencyProperty.UnsetValue);
            });
            return count;
        }

        private static object GetValueOf(DependencyObject element, DependencyProperty property)
        {
            object value = null;
            RunOnUIThread.Execute(() => value = element.GetValue(property));
            return value;
        }

        private static void SetValueOf(DependencyObject element, DependencyProperty property, object value)
        {
            RunOnUIThread.Execute(() => element.SetValue(property, value));
        }

        private static void ClearValuesOf(DependencyObject element, params DependencyProperty[] properties)
        {
            RunOnUIThread.Execute(() =>
            {
                foreach (var property in properties)
                {
                    element.ClearValue(property);
                }
            });
        }

        private static SymbolIconSource CreateSymbolIconSource(Symbol symbol)
        {
            SymbolIconSource iconSource = null;
            RunOnUIThread.Execute(() => iconSource = new SymbolIconSource { Symbol = symbol });
            return iconSource;
        }

        private static IAnimatedVisualSource2 GetIconSourceSource(AnimatedIconSource iconSource)
        {
            IAnimatedVisualSource2 source = null;
            RunOnUIThread.Execute(() => source = iconSource.Source);
            return source;
        }

        private static IconSource GetIconSourceFallback(AnimatedIconSource iconSource)
        {
            IconSource fallback = null;
            RunOnUIThread.Execute(() => fallback = iconSource.FallbackIconSource);
            return fallback;
        }

        private static bool GetIconSourceMirrored(AnimatedIconSource iconSource)
        {
            bool mirrored = false;
            RunOnUIThread.Execute(() => mirrored = iconSource.MirroredWhenRightToLeft);
            return mirrored;
        }

        private static string GetIconSourceSourceTypeName(AnimatedIconSource iconSource)
        {
            string name = null;
            RunOnUIThread.Execute(() => name = iconSource.Source?.GetType().FullName);
            return name;
        }

        private static string DescribeIconSourceFallback(AnimatedIconSource iconSource)
        {
            string description = null;
            RunOnUIThread.Execute(() =>
            {
                var fallback = iconSource.FallbackIconSource as SymbolIconSource;
                description = iconSource.FallbackIconSource == null ? "null" : "SymbolIconSource:" + fallback?.Symbol;
            });
            return description;
        }

        private static void SetIconSourceClrProperties(AnimatedIconSource iconSource, IAnimatedVisualSource2 source, IconSource fallback, bool mirrored)
        {
            RunOnUIThread.Execute(() =>
            {
                iconSource.Source = source;
                iconSource.FallbackIconSource = fallback;
                iconSource.MirroredWhenRightToLeft = mirrored;
            });
        }

        private static void SetFallbackIconSource(AnimatedIconSource iconSource, IconSource fallback)
        {
            RunOnUIThread.Execute(() => iconSource.FallbackIconSource = fallback);
        }

        private static IAnimatedVisualSource2 GetAnimatedIconSourceValue(AnimatedIcon animatedIcon)
        {
            IAnimatedVisualSource2 source = null;
            RunOnUIThread.Execute(() => source = animatedIcon.Source);
            return source;
        }

        private static string GetAnimatedIconSourceTypeName(AnimatedIcon animatedIcon)
        {
            string name = null;
            RunOnUIThread.Execute(() => name = animatedIcon.Source?.GetType().FullName);
            return name;
        }

        private static IconSource GetAnimatedIconFallback(AnimatedIcon animatedIcon)
        {
            IconSource fallback = null;
            RunOnUIThread.Execute(() => fallback = animatedIcon.FallbackIconSource);
            return fallback;
        }

        private static AnimatedIconSource LoadAnimatedIconSourceMarkup(string markup)
        {
            AnimatedIconSource iconSource = null;
            RunOnUIThread.Execute(() => iconSource = (AnimatedIconSource)XamlReader.Load(markup));
            Verify.IsNotNull(iconSource);
            return iconSource;
        }

        private static IXamlType GetControlsXamlType(string typeName)
        {
            IXamlType type = null;
            RunOnUIThread.Execute(() => type = new XamlControlsXamlMetaDataProvider().GetXamlType(typeName));
            Verify.IsNotNull(type, "XAML type " + typeName);
            return type;
        }

        private static string GetFullNameOf(IXamlType type)
        {
            string name = null;
            RunOnUIThread.Execute(() => name = type.FullName);
            return name;
        }

        private static string GetContentPropertyName(IXamlType type)
        {
            string name = "not read";
            RunOnUIThread.Execute(() => name = type.ContentProperty?.Name);
            return name;
        }

        // "Name=DP,RW" for a writable dependency-property member, "Name=missing" when the type has no such member.
        private static string DescribeMembers(IXamlType type, string[] memberNames)
        {
            string description = null;
            RunOnUIThread.Execute(() =>
            {
                description = string.Join("; ", memberNames.Select(name =>
                {
                    var member = type.GetMember(name);
                    return member == null ? name + "=missing" :
                        member.Name + "=" + (member.IsDependencyProperty ? "DP" : "CLR") + "," + (member.IsReadOnly ? "RO" : "RW");
                }));
            });
            return description;
        }

        private static object ActivateXamlType(IXamlType type)
        {
            object instance = null;
            RunOnUIThread.Execute(() => instance = type.ActivateInstance());
            Verify.IsNotNull(instance, "Activated " + type.FullName);
            return instance;
        }

        private static void SetMemberValue(IXamlType type, string memberName, object instance, object value)
        {
            RunOnUIThread.Execute(() => type.GetMember(memberName).SetValue(instance, value));
        }

        private static object GetMemberValue(IXamlType type, string memberName, object instance)
        {
            object value = null;
            RunOnUIThread.Execute(() => value = type.GetMember(memberName).GetValue(instance));
            return value;
        }

        private static AnimatedIconSource CreateAnimatedIconSource(IAnimatedVisualSource2 source, Symbol? fallbackSymbol, Color? foreground, bool mirroredWhenRightToLeft)
        {
            AnimatedIconSource iconSource = null;
            RunOnUIThread.Execute(() =>
            {
                iconSource = new AnimatedIconSource { Source = source, MirroredWhenRightToLeft = mirroredWhenRightToLeft };
                if (fallbackSymbol.HasValue)
                {
                    iconSource.FallbackIconSource = new SymbolIconSource { Symbol = fallbackSymbol.Value };
                }
                if (foreground.HasValue)
                {
                    iconSource.Foreground = new SolidColorBrush(foreground.Value);
                }
            });
            return iconSource;
        }

        private static IconElement CreateIconElement(IconSource iconSource)
        {
            IconElement element = null;
            RunOnUIThread.Execute(() => element = iconSource.CreateIconElement());
            return element;
        }

        private static void SetFallbackSymbol(AnimatedIconSource iconSource, Symbol symbol)
        {
            RunOnUIThread.Execute(() => iconSource.FallbackIconSource = new SymbolIconSource { Symbol = symbol });
        }

        private static Color GetSolidForegroundColor(IconElement element)
        {
            var color = default(Color);
            RunOnUIThread.Execute(() => color = ((SolidColorBrush)element.Foreground).Color);
            return color;
        }

        private static bool GetMirroredWhenRightToLeft(AnimatedIcon animatedIcon)
        {
            bool mirrored = false;
            RunOnUIThread.Execute(() => mirrored = animatedIcon.MirroredWhenRightToLeft);
            return mirrored;
        }

        private static string DescribeFallbackIconSource(AnimatedIcon animatedIcon)
        {
            string description = null;
            RunOnUIThread.Execute(() =>
            {
                var fallback = animatedIcon.FallbackIconSource as SymbolIconSource;
                description = animatedIcon.FallbackIconSource == null ? "null" : "SymbolIconSource:" + fallback?.Symbol;
            });
            return description;
        }

        // The created icon must share the configured objects, not copies of them.
        private static bool HasSameSourceAndFallback(AnimatedIconSource iconSource, AnimatedIcon animatedIcon)
        {
            bool same = false;
            RunOnUIThread.Execute(() =>
            {
                same = ReferenceEquals(iconSource.Source, animatedIcon.Source) &&
                    ReferenceEquals(iconSource.FallbackIconSource, animatedIcon.FallbackIconSource);
            });
            return same;
        }

        [TestMethod]
        public void CreateIconElementReturnsCorrectTypeTest()
        {
            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Verify BitmapIconSource creates BitmapIcon");
                var bitmapIconSource = new BitmapIconSource();
                var bitmapIcon = bitmapIconSource.CreateIconElement();
                Verify.IsNotNull(bitmapIcon);
                Verify.IsTrue(bitmapIcon is BitmapIcon);

                Log.Comment("Verify FontIconSource creates FontIcon");
                var fontIconSource = new FontIconSource();
                var fontIcon = fontIconSource.CreateIconElement();
                Verify.IsNotNull(fontIcon);
                Verify.IsTrue(fontIcon is FontIcon);

                Log.Comment("Verify SymbolIconSource creates SymbolIcon");
                var symbolIconSource = new SymbolIconSource();
                var symbolIcon = symbolIconSource.CreateIconElement();
                Verify.IsNotNull(symbolIcon);
                Verify.IsTrue(symbolIcon is SymbolIcon);

                Log.Comment("Verify PathIconSource creates PathIcon");
                var pathIconSource = new PathIconSource();
                var pathIcon = pathIconSource.CreateIconElement();
                Verify.IsNotNull(pathIcon);
                Verify.IsTrue(pathIcon is PathIcon);
            });
        }

        [TestMethod]
        public void CreateIconElementForegroundTest()
        {
            FontIconSource iconSource1 = null;
            FontIconSource iconSource2 = null;
            FontIcon icon1 = null;
            FontIcon icon2 = null;

            RunOnUIThread.Execute(() =>
            {
                iconSource1 = new FontIconSource()
                {
                    Foreground = new SolidColorBrush(Microsoft.UI.Colors.Blue)
                };

                iconSource2 = new FontIconSource();
                
                Log.Comment("Create first icon element with foreground already set");
                icon1 = iconSource1.CreateIconElement() as FontIcon;
                Verify.IsNotNull(icon1);
                
                Log.Comment("Create second icon element with foreground not set");
                icon2 = iconSource2.CreateIconElement() as FontIcon;
                Verify.IsNotNull(icon2);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Verify foreground is applied to both icon elements");
                Verify.IsTrue(icon1.Foreground is SolidColorBrush);
                Verify.IsTrue(icon2.Foreground is SolidColorBrush);
                Verify.AreEqual(Microsoft.UI.Colors.Blue, (icon1.Foreground as SolidColorBrush).Color);
            });
        }

        [TestMethod]
        public void PropertyChangePropagationToCreatedElements()
        {
            FontIconSource iconSource = null;
            FontIcon icon = null;

            RunOnUIThread.Execute(() =>
            {
                iconSource = new FontIconSource();
                
                Log.Comment("Create icon element before setting properties");
                icon = iconSource.CreateIconElement() as FontIcon;
                
                Verify.IsNotNull(icon);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Verify foreground is not null before setting");
                Verify.IsNotNull(icon.Foreground);
                
                Log.Comment("Change foreground on IconSource");
                iconSource.Foreground = new SolidColorBrush(Microsoft.UI.Colors.Red);
            });
            
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Verify foreground propagates to created element");
                Verify.IsTrue(icon.Foreground is SolidColorBrush);
                Verify.AreEqual(Microsoft.UI.Colors.Red, (icon.Foreground as SolidColorBrush).Color);
            });
        }

        [TestMethod]
        public void CreateIconElementPreservesIconSourceProperties()
        {
            FontIconSource fontIconSource = null;
            FontIcon fontIcon = null;

            RunOnUIThread.Execute(() =>
            {
                fontIconSource = new FontIconSource();
                fontIconSource.Glyph = "\uE001";
                fontIconSource.FontSize = 24;
                fontIconSource.FontFamily = new Microsoft.UI.Xaml.Media.FontFamily("Segoe UI Symbol");
                fontIconSource.Foreground = new SolidColorBrush(Microsoft.UI.Colors.Purple);
                
                Log.Comment("Create icon element from configured source");
                fontIcon = fontIconSource.CreateIconElement() as FontIcon;
                Verify.IsNotNull(fontIcon);
            });
            
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Verify all properties are transferred to the icon element");
                Verify.AreEqual("\uE001", fontIcon.Glyph);
                Verify.AreEqual(24.0, fontIcon.FontSize);
                Verify.AreEqual("Segoe UI Symbol", fontIcon.FontFamily.Source);
                Verify.AreEqual(Microsoft.UI.Colors.Purple, (fontIcon.Foreground as SolidColorBrush).Color);
            });
        }
    }
}

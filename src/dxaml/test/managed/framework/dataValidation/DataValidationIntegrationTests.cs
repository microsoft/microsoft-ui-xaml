// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Microsoft.UI.Xaml.Controls.Maps;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using MockDataAnnotations;
using Microsoft.UI.Xaml.Controls;
using System.ComponentModel;
namespace Microsoft.UI.Xaml.Tests.Framework.DataValidation
{
    public class DataValidationTestHelper : IDisposable
    {
        private static string DefaultMarkup = @"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                          xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Page.Resources>
                            <DataTemplate x:Key='MyCustomErrorTemplate'>
                                <TextBlock Text='{{Binding Path=ValidationErrors[0].ErrorMessage}}' />
                            </DataTemplate>
                            <DataTemplate x:Key='DefaultInputValidationErrorTemplate'>
                                <ItemsControl ItemsSource='{{Binding ValidationErrors}}'>
                                    <ItemsControl.ItemTemplate>
                                        <DataTemplate>
                                            <TextBlock Text='{{Binding ErrorMessage}}' />
                                        </DataTemplate>
                                    </ItemsControl.ItemTemplate>
                                </ItemsControl>
                            </DataTemplate>
                            <Style TargetType='TextBox'>
                                <Setter Property='ErrorTemplate' Value='{{StaticResource DefaultInputValidationErrorTemplate}}' />
                                  <Setter Property='Template'>
                                    <Setter.Value>
                                        <ControlTemplate TargetType='TextBox'>
                                            <Grid>
                                                <Grid.Resources>
                                                    <Style x:Name='DeleteButtonStyle' TargetType='Button'>
                                                        <Setter Property='Template'>
                                                            <Setter.Value>
                                                                <ControlTemplate TargetType='Button'>
                                                                    <Grid x:Name='ButtonLayoutGrid'
                                                                        BorderBrush='{{ThemeResource TextControlButtonBorderBrush}}'
                                                                        BorderThickness='{{TemplateBinding BorderThickness}}'
                                                                        Background='{{ThemeResource TextControlButtonBackground}}'>

                                                                        <VisualStateManager.VisualStateGroups>
                                                                            <VisualStateGroup x:Name='CommonStates'>
                                                                                <VisualState x:Name='Normal' />
                                                                                <VisualState x:Name='PointerOver'>
                                                                                    <Storyboard>
                                                                                        <ObjectAnimationUsingKeyFrames Storyboard.TargetName='ButtonLayoutGrid' Storyboard.TargetProperty='Background'>
                                                                                            <DiscreteObjectKeyFrame KeyTime='0' Value='{{ThemeResource TextControlButtonBackgroundPointerOver}}' />
                                                                                        </ObjectAnimationUsingKeyFrames>
                                                                                        <ObjectAnimationUsingKeyFrames Storyboard.TargetName='ButtonLayoutGrid' Storyboard.TargetProperty='BorderBrush'>
                                                                                            <DiscreteObjectKeyFrame KeyTime='0' Value='{{ThemeResource TextControlButtonBorderBrushPointerOver}}' />
                                                                                        </ObjectAnimationUsingKeyFrames>
                                                                                        <ObjectAnimationUsingKeyFrames Storyboard.TargetName='GlyphElement' Storyboard.TargetProperty='Foreground'>
                                                                                            <DiscreteObjectKeyFrame KeyTime='0' Value='{{ThemeResource TextControlButtonForegroundPointerOver}}' />
                                                                                        </ObjectAnimationUsingKeyFrames>
                                                                                    </Storyboard>
                                                                                </VisualState>
                                                                                <VisualState x:Name='Pressed'>
                                                                                    <Storyboard>
                                                                                        <ObjectAnimationUsingKeyFrames Storyboard.TargetName='ButtonLayoutGrid' Storyboard.TargetProperty='Background'>
                                                                                            <DiscreteObjectKeyFrame KeyTime='0' Value='{{ThemeResource TextControlButtonBackgroundPressed}}' />
                                                                                        </ObjectAnimationUsingKeyFrames>
                                                                                        <ObjectAnimationUsingKeyFrames Storyboard.TargetName='ButtonLayoutGrid' Storyboard.TargetProperty='BorderBrush'>
                                                                                            <DiscreteObjectKeyFrame KeyTime='0' Value='{{ThemeResource TextControlButtonBorderBrushPressed}}' />
                                                                                        </ObjectAnimationUsingKeyFrames>
                                                                                        <ObjectAnimationUsingKeyFrames Storyboard.TargetName='GlyphElement' Storyboard.TargetProperty='Foreground'>
                                                                                            <DiscreteObjectKeyFrame KeyTime='0' Value='{{ThemeResource TextControlButtonForegroundPressed}}' />
                                                                                        </ObjectAnimationUsingKeyFrames>
                                                                                    </Storyboard>
                                                                                </VisualState>
                                                                                <VisualState x:Name='Disabled'>
                                                                                    <Storyboard>
                                                                                        <DoubleAnimation Storyboard.TargetName='ButtonLayoutGrid'
                                                                                            Storyboard.TargetProperty='Opacity'
                                                                                            To='0'
                                                                                            Duration='0' />
                                                                                    </Storyboard>
                                                                                </VisualState>
                                                                            </VisualStateGroup>
                                                                        </VisualStateManager.VisualStateGroups>
                                                                        <TextBlock x:Name='GlyphElement'
                                                                            Foreground='{{ThemeResource TextControlButtonForeground}}'
                                                                            VerticalAlignment='Center'
                                                                            HorizontalAlignment='Center'
                                                                            FontStyle='Normal'
                                                                            FontSize='12'
                                                                            Text='&#xE10A;'
                                                                            FontFamily='{{ThemeResource SymbolThemeFontFamily}}'
                                                                            AutomationProperties.AccessibilityView='Raw' />
                                                                    </Grid>
                                                                </ControlTemplate>
                                                            </Setter.Value>
                                                        </Setter>
                                                    </Style>
                                                </Grid.Resources>
                                                <VisualStateManager.VisualStateGroups>
                                                    <VisualStateGroup x:Name='CommonStates'>
                                                        <VisualState x:Name='Normal' />
                                                        <VisualState x:Name='Disabled'>
                                                            <Storyboard>
                                                                <ObjectAnimationUsingKeyFrames Storyboard.TargetName='HeaderContentPresenter' Storyboard.TargetProperty='Foreground'>
                                                                    <DiscreteObjectKeyFrame KeyTime='0' Value='{{ThemeResource TextControlHeaderForegroundDisabled}}' />
                                                                </ObjectAnimationUsingKeyFrames>
                                                                <ObjectAnimationUsingKeyFrames Storyboard.TargetName='BorderElement' Storyboard.TargetProperty='Background'>
                                                                    <DiscreteObjectKeyFrame KeyTime='0' Value='{{ThemeResource TextControlBackgroundDisabled}}' />
                                                                </ObjectAnimationUsingKeyFrames>
                                                                <ObjectAnimationUsingKeyFrames Storyboard.TargetName='BorderElement' Storyboard.TargetProperty='BorderBrush'>
                                                                    <DiscreteObjectKeyFrame KeyTime='0' Value='{{ThemeResource TextControlBorderBrushDisabled}}' />
                                                                </ObjectAnimationUsingKeyFrames>
                                                                <ObjectAnimationUsingKeyFrames Storyboard.TargetName='ContentElement' Storyboard.TargetProperty='Foreground'>
                                                                    <DiscreteObjectKeyFrame KeyTime='0' Value='{{ThemeResource TextControlForegroundDisabled}}' />
                                                                </ObjectAnimationUsingKeyFrames>
                                                                <ObjectAnimationUsingKeyFrames Storyboard.TargetName='PlaceholderTextContentPresenter' Storyboard.TargetProperty='Foreground'>
                                                                    <DiscreteObjectKeyFrame KeyTime='0' Value='{{Binding PlaceholderForeground, RelativeSource={{RelativeSource TemplatedParent}}, TargetNullValue={{ThemeResource TextControlPlaceholderForegroundDisabled}}}}' />
                                                                </ObjectAnimationUsingKeyFrames>
                                                            </Storyboard>
                                                        </VisualState>

                                                        <VisualState x:Name='PointerOver'>
                                                            <Storyboard>
                                                                <ObjectAnimationUsingKeyFrames Storyboard.TargetName='BorderElement' Storyboard.TargetProperty='BorderBrush'>
                                                                    <DiscreteObjectKeyFrame KeyTime='0' Value='{{ThemeResource TextControlBorderBrushPointerOver}}' />
                                                                </ObjectAnimationUsingKeyFrames>
                                                                <ObjectAnimationUsingKeyFrames Storyboard.TargetName='BorderElement' Storyboard.TargetProperty='Background'>
                                                                    <DiscreteObjectKeyFrame KeyTime='0' Value='{{ThemeResource TextControlBackgroundPointerOver}}' />
                                                                </ObjectAnimationUsingKeyFrames>
                                                                <ObjectAnimationUsingKeyFrames Storyboard.TargetName='PlaceholderTextContentPresenter' Storyboard.TargetProperty='Foreground'>
                                                                    <DiscreteObjectKeyFrame KeyTime='0' Value='{{Binding PlaceholderForeground, RelativeSource={{RelativeSource TemplatedParent}}, TargetNullValue={{ThemeResource TextControlPlaceholderForegroundPointerOver}}}}' />
                                                                </ObjectAnimationUsingKeyFrames>
                                                                <ObjectAnimationUsingKeyFrames Storyboard.TargetName='ContentElement' Storyboard.TargetProperty='Foreground'>
                                                                    <DiscreteObjectKeyFrame KeyTime='0' Value='{{ThemeResource TextControlForegroundPointerOver}}' />
                                                                </ObjectAnimationUsingKeyFrames>
                                                            </Storyboard>
                                                        </VisualState>
                                                        <VisualState x:Name='Focused'>
                                                            <Storyboard>
                                                                <ObjectAnimationUsingKeyFrames Storyboard.TargetName='PlaceholderTextContentPresenter' Storyboard.TargetProperty='Foreground'>
                                                                    <DiscreteObjectKeyFrame KeyTime='0' Value='{{Binding PlaceholderForeground, RelativeSource={{RelativeSource TemplatedParent}}, TargetNullValue={{ThemeResource TextControlPlaceholderForegroundFocused}}}}' />
                                                                </ObjectAnimationUsingKeyFrames>
                                                                <ObjectAnimationUsingKeyFrames Storyboard.TargetName='BorderElement' Storyboard.TargetProperty='Background'>
                                                                    <DiscreteObjectKeyFrame KeyTime='0' Value='{{ThemeResource TextControlBackgroundFocused}}' />
                                                                </ObjectAnimationUsingKeyFrames>
                                                                <ObjectAnimationUsingKeyFrames Storyboard.TargetName='BorderElement' Storyboard.TargetProperty='BorderBrush'>
                                                                    <DiscreteObjectKeyFrame KeyTime='0' Value='{{ThemeResource TextControlBorderBrushFocused}}' />
                                                                </ObjectAnimationUsingKeyFrames>
                                                                <ObjectAnimationUsingKeyFrames Storyboard.TargetName='ContentElement' Storyboard.TargetProperty='Foreground'>
                                                                    <DiscreteObjectKeyFrame KeyTime='0' Value='{{ThemeResource TextControlForegroundFocused}}' />
                                                                </ObjectAnimationUsingKeyFrames>
                                                                <ObjectAnimationUsingKeyFrames Storyboard.TargetName='ContentElement' Storyboard.TargetProperty='RequestedTheme'>
                                                                    <DiscreteObjectKeyFrame KeyTime='0' Value='Light' />
                                                                </ObjectAnimationUsingKeyFrames>
                                                            </Storyboard>
                                                        </VisualState>
                                                    </VisualStateGroup>
                                                    <VisualStateGroup x:Name='ButtonStates'>
                                                        <VisualState x:Name='ButtonVisible'>
                                                            <Storyboard>
                                                                <ObjectAnimationUsingKeyFrames Storyboard.TargetName='DeleteButton' Storyboard.TargetProperty='Visibility'>
                                                                    <DiscreteObjectKeyFrame KeyTime='0'>
                                                                        <DiscreteObjectKeyFrame.Value>
                                                                            <Visibility>Visible</Visibility>
                                                                        </DiscreteObjectKeyFrame.Value>
                                                                    </DiscreteObjectKeyFrame>
                                                                </ObjectAnimationUsingKeyFrames>
                                                            </Storyboard>
                                                        </VisualState>
                                                        <VisualState x:Name='ButtonCollapsed' />
                                                    </VisualStateGroup>
                                                    <VisualStateGroup x:Name='HeaderStates'>
                                                        <VisualState x:Name='TopHeader' />
                                                        <VisualState x:Name='LeftHeader'>
                                                            <VisualState.Setters>
                                                                <Setter Target='RequiredHeaderPresenter.(Grid.Row)' Value='1' />
                                                                <Setter Target='RequiredHeaderPresenter.(Grid.Column)' Value='0' />
                                                                <Setter Target='RequiredHeaderPresenter.(Grid.ColumnSpan)' Value='1' />
                                                                <Setter Target='HeaderContentPresenter.(Grid.Row)' Value='1' />
                                                                <Setter Target='HeaderContentPresenter.(Grid.Column)' Value='1' />
                                                                <Setter Target='HeaderContentPresenter.(Grid.ColumnSpan)' Value='1' />
                                                                <Setter Target='HeaderContentPresenter.Margin' Value='{{StaticResource TextBoxLeftHeaderMargin}}' />
                                                                <Setter Target='HeaderContentPresenter.MaxWidth' Value='{{StaticResource TextBoxLeftHeaderMaxWidth}}' />
                                                            </VisualState.Setters>
                                                        </VisualState>
                                                    </VisualStateGroup>
                                                    <VisualStateGroup x:Name='InputValidationEnabledStates'>
                                                        <VisualState x:Name='CompactValidationEnabled' />
                                                        <VisualState x:Name='InlineValidationEnabled'>
                                                            <VisualState.Setters>
                                                                <Setter Target='ErrorPresenterRow.MinHeight' Value='20' />
                                                            </VisualState.Setters>
                                                        </VisualState>
                                                        <VisualState x:Name='ValidationDisabled' />
                                                    </VisualStateGroup>
                                                    <VisualStateGroup x:Name='InputValidationErrorStates'>
                                                        <VisualState x:Name='CompactErrors'>
                                                            <VisualState.Setters>
                                                                <Setter Target='ErrorIconColumn.MaxWidth' Value='20' />
                                                                <Setter Target='ErrorPresenter.Visibility' Value='Visible' />
                                                                <Setter Target='BorderElement.BorderBrush' Value='{{StaticResource SystemControlErrorTextForegroundBrush}}' />
                                                            </VisualState.Setters>
                                                        </VisualState>
                                                        <VisualState x:Name='InlineErrors'>
                                                            <VisualState.Setters>
                                                                <Setter Target='ErrorPresenter.(Grid.Row)' Value='2' />
                                                                <Setter Target='ErrorPresenter.(Grid.Column)' Value='2' />
                                                                <Setter Target='ErrorPresenter.(Grid.ColumnSpan)' Value='3' />
                                                                <Setter Target='DescriptionPresenter.Visibility' Value='Collapsed' />
                                                                <Setter Target='ErrorPresenter.Visibility' Value='Visible' />
                                                                <Setter Target='BorderElement.BorderBrush' Value='{{StaticResource SystemControlErrorTextForegroundBrush}}' />
                                                            </VisualState.Setters>
                                                        </VisualState>
                                                        <VisualState x:Name='ErrorsCleared' />
                                                    </VisualStateGroup>
                                                </VisualStateManager.VisualStateGroups>
                                                <Grid.ColumnDefinitions>
                                                    <ColumnDefinition Width='Auto' />
                                                    <ColumnDefinition Width='Auto' />
                                                    <ColumnDefinition Width='*' />
                                                    <ColumnDefinition Width='Auto' />
                                                    <ColumnDefinition x:Name='ErrorIconColumn' MaxWidth='0' />
                                                </Grid.ColumnDefinitions>
                                                <Grid.RowDefinitions>
                                                    <RowDefinition Height='Auto' />
                                                    <RowDefinition Height='*' />
                                                    <RowDefinition x:Name='ErrorPresenterRow' Height='Auto' MinHeight='0' />
                                                </Grid.RowDefinitions>
                                                <Border x:Name='BorderElement'
                                                    Grid.Row='1'
                                                    Grid.Column='2'
                                                    Grid.RowSpan='1'
                                                    Grid.ColumnSpan='2'
                                                    Background='{{TemplateBinding Background}}'
                                                    BorderBrush='{{TemplateBinding BorderBrush}}'
                                                    BorderThickness='{{TemplateBinding BorderThickness}}'
                                                    CornerRadius='{{TemplateBinding CornerRadius}}'
                                                    Control.IsTemplateFocusTarget='True'
                                                    MinWidth='{{ThemeResource TextControlThemeMinWidth}}'
                                                    MinHeight='{{ThemeResource TextControlThemeMinHeight}}' />
                                                <ContentPresenter x:Name='RequiredHeaderPresenter'
                                                    Content='{{StaticResource RequiredHeaderContent}}'
                                                    x:DeferLoadStrategy='Lazy'
                                                    Visibility='Collapsed'
                                                    Grid.Column='1'
                                                    AutomationProperties.AccessibilityView='Raw'
                                                    Foreground='{{ThemeResource SystemControlErrorTextForegroundBrush}}' />
                                                <ContentPresenter x:Name='HeaderContentPresenter'
                                                    x:DeferLoadStrategy='Lazy'
                                                    Grid.Row='0'
                                                    Grid.Column='2'
                                                    Grid.ColumnSpan='2'
                                                    Content='{{TemplateBinding Header}}'
                                                    ContentTemplate='{{TemplateBinding HeaderTemplate}}'
                                                    FontWeight='Normal'
                                                    Foreground='{{ThemeResource TextControlHeaderForeground}}'
                                                    Margin='{{StaticResource TextBoxTopHeaderMargin}}'
                                                    TextWrapping='Wrap'
                                                    VerticalAlignment='Top'
                                                    Visibility='Collapsed' />
                                                <ScrollViewer x:Name='ContentElement'
                                                    Grid.Row='1'
                                                    Grid.Column='2'
                                                    HorizontalScrollMode='{{TemplateBinding ScrollViewer.HorizontalScrollMode}}'
                                                    HorizontalScrollBarVisibility='{{TemplateBinding ScrollViewer.HorizontalScrollBarVisibility}}'
                                                    VerticalScrollMode='{{TemplateBinding ScrollViewer.VerticalScrollMode}}'
                                                    VerticalScrollBarVisibility='{{TemplateBinding ScrollViewer.VerticalScrollBarVisibility}}'
                                                    IsHorizontalRailEnabled='{{TemplateBinding ScrollViewer.IsHorizontalRailEnabled}}'
                                                    IsVerticalRailEnabled='{{TemplateBinding ScrollViewer.IsVerticalRailEnabled}}'
                                                    IsDeferredScrollingEnabled='{{TemplateBinding ScrollViewer.IsDeferredScrollingEnabled}}'
                                                    Margin='{{TemplateBinding BorderThickness}}'
                                                    Padding='{{TemplateBinding Padding}}'
                                                    IsTabStop='False'
                                                    AutomationProperties.AccessibilityView='Raw'
                                                    ZoomMode='Disabled' />
                                                <TextBlock x:Name='PlaceholderTextContentPresenter'
                                                    Grid.Row='1'
                                                    Grid.Column='2'
                                                    Grid.ColumnSpan='2'
                                                    Foreground='{{Binding PlaceholderForeground, RelativeSource={{RelativeSource TemplatedParent}}, TargetNullValue={{ThemeResource TextControlPlaceholderForeground}}}}'
                                                    Margin='{{TemplateBinding BorderThickness}}'
                                                    Padding='{{TemplateBinding Padding}}'
                                                    Text='{{TemplateBinding PlaceholderText}}'
                                                    TextAlignment='{{TemplateBinding TextAlignment}}'
                                                    TextWrapping='{{TemplateBinding TextWrapping}}'
                                                    IsHitTestVisible='False' />
                                                <Button x:Name='DeleteButton'
                                                    Grid.Row='1'
                                                    Grid.Column='3'
                                                    Style='{{StaticResource DeleteButtonStyle}}'
                                                    BorderThickness='{{TemplateBinding BorderThickness}}'
                                                    Margin='{{ThemeResource HelperButtonThemePadding}}'
                                                    IsTabStop='False'
                                                    Visibility='Collapsed'
                                                    AutomationProperties.AccessibilityView='Raw'
                                                    FontSize='{{TemplateBinding FontSize}}'
                                                    MinWidth='34'
                                                    VerticalAlignment='Stretch' />
                                                <ContentPresenter x:Name='ErrorPresenter'
                                                    x:Load='False'
                                                    Grid.Row='1'
                                                    Grid.Column='4'
                                                    Foreground='{{ThemeResource SystemControlErrorTextForegroundBrush}}'
                                                    AutomationProperties.AccessibilityView='Raw' />
                                                <ContentPresenter x:Name='DescriptionPresenter'
                                                    Grid.Row='2'
                                                    Grid.ColumnSpan='2'
                                                    Grid.Column='2'
                                                    x:Load='False'
                                                    Content='{{TemplateBinding Description}}'
                                                    Foreground='{{ThemeResource SystemControlDescriptionTextForegroundBrush}}'
                                                    AutomationProperties.AccessibilityView='Raw' />
                                            </Grid>
                                        </ControlTemplate>
                                    </Setter.Value>
                            </Style>
                        </Page.Resources>
                        <StackPanel Margin='10'>
                            <TextBox x:Name='NameBox' Header='Name:' Text='{{Binding Name, Mode=TwoWay, UpdateSourceTrigger=PropertyChanged}}' />
                            <TextBox x:Name='PhoneBox' Header='Phone:' Text='{{Binding PhoneNumber, Mode=TwoWay, UpdateSourceTrigger=PropertyChanged}}' {0}/>
                        </StackPanel>
                    </Page>";

        public DataValidationTestHelper(string phoneStringProperty = null)
        {
            TestServices.WindowHelper.InitializeXaml();
            Model = new PersonModel();
            Model.Name = "Billy";
            UIExecutor.Execute(() =>
            {
                var pageString = String.Format(DefaultMarkup, phoneStringProperty);
                try
                {
                    Root = (Page)Microsoft.UI.Xaml.Markup.XamlReader.Load(pageString);
                }
                catch (XamlParseException ex)
                {
                    Log.Error(ex.Message);
                    throw;
                }
                Root.DataContext = Model;

                Name = Root.FindName<TextBox>("NameBox");

                Phone = Root.FindName<TextBox>("PhoneBox");
                Phone.ValidationContext = new InputValidationContext("Phone", true);
                TestServices.WindowHelper.WindowContent = Root;

                 var defaultCompactTemplate = (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
            @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                <FontIcon Foreground='{StaticResource SystemControlErrorTextForegroundBrush}' Glyph='&#xE946;' FontSize='16' Width='20' Height='20' Margin='3,0,0,0'>
                    <ToolTipService.ToolTip>
                        <ToolTip Placement='Right' />
                    </ToolTipService.ToolTip>
                </FontIcon>
            </DataTemplate>");
                Application.Current.Resources.Add("DefaultCompactErrorIconTemplate",defaultCompactTemplate );
            });

            TestServices.WindowHelper.WaitForIdle();

            Verify.IsNull(GetErrorPresenter(Phone));
            Verify.IsNull(GetErrorPresenter(Name));
        }

        public ContentPresenter GetErrorPresenter(Control elem)
        {
            ContentPresenter ep = null;
            UIExecutor.Execute(() =>
            {
                ep = VisualTreeUtils.FindNameInSubtree(elem, "ErrorPresenter") as ContentPresenter;
            });

            return ep;
        }

        public ContentPresenter GetDescriptionPresenter(Control elem)
        {
            ContentPresenter ep = null;
            UIExecutor.Execute(() =>
            {
                ep = VisualTreeUtils.FindNameInSubtree(elem, "DescriptionPresenter") as ContentPresenter;
            });

            return ep;
        }

        public ContentPresenter GetRequiredHeaderPresenter(Control elem)
        {
            ContentPresenter ep = null;
            UIExecutor.Execute(() =>
            {
                ep = VisualTreeUtils.FindNameInSubtree(elem, "RequiredHeaderPresenter") as ContentPresenter;
            });

            return ep;
        }

        public ContentPresenter GetHeaderPresenter(Control elem)
        {
            ContentPresenter ep = null;
            UIExecutor.Execute(() =>
            {
                ep = VisualTreeUtils.FindNameInSubtree(elem, "HeaderContentPresenter") as ContentPresenter;
            });

            return ep;
        }

        public void AddError(IInputValidationControl control, string error)
        {
            using (var eventTester = new EventTester<IInputValidationControl, InputValidationErrorEventArgs>(control, "ValidationError"))
            {
                Log.Comment("Adding error");
                UIExecutor.Execute(() =>
                {
                    // Simulate what x:Bind will do and add errors to the InputValidation.ValidationErrors collection
                    control.ValidationErrors.Add(new InputValidationError(error));
                });
                eventTester.Wait();
            }
        }

        public void ValidateError(IInputValidationControl control, int errorIndex, string errorMessage)
        {
            UIExecutor.Execute(() =>
            {
                var errors = control.ValidationErrors;
                var error = errors[errorIndex];
                Verify.AreEqual(errorMessage, error.ErrorMessage);

                Verify.IsTrue(control.HasValidationErrors);

                // Verify the Error Presenter is in the tree
                var errorPresenter = GetErrorPresenter((Control)control);
                Verify.IsNotNull(errorPresenter);

                Verify.IsNotNull(errorPresenter.Content);
                var kind = control.InputValidationKind;
                var content = errorPresenter.Content as DependencyObject;
                if (kind != InputValidationKind.Inline)
                {
                    // Get the content from the tool tip
                    var tooltip = ToolTipService.GetToolTip(content) as ContentControl;
                    Verify.IsNotNull(tooltip);
                    content = tooltip.Content as DependencyObject;
                }
                var itemsControl = content as ItemsControl;
                string displayedErrorMessage = null;
                if (itemsControl != null)
                {
                    Verify.AreEqual(errors.Count, itemsControl.Items.Count);
                    var item = itemsControl.Items[errorIndex] as InputValidationError;
                    displayedErrorMessage = item.ErrorMessage;
                    Verify.AreSame(error, item);
                }
                else if (content is TextBlock)
                {
                    displayedErrorMessage = (content as TextBlock).Text;
                }

                Verify.AreEqual(errorMessage, displayedErrorMessage);
            });
        }


        public void ValidateDescription(Control control, string description)
        {
            UIExecutor.Execute(() =>
            {
                // Verify the Error Presenter is in the tree
                var descriptionPresenter = GetDescriptionPresenter(control);
                Verify.IsNotNull(descriptionPresenter);

                Verify.IsNotNull(descriptionPresenter.Content);
                string displayedDescription = descriptionPresenter.Content as String;
                Verify.AreEqual(description, displayedDescription);
            });
        }

        public void SetInvalidText(TextBox textBox, string invalidText)
        {
            using (var eventTester = new EventTester<object, Microsoft.UI.Xaml.Data.DataErrorsChangedEventArgs>(Model, "ErrorsChanged"))
            {
                Log.Comment($"Setting incorrect text: {invalidText}");
                UIExecutor.Execute(() =>
                {
                    textBox.Text = invalidText;
                });
                eventTester.Wait();
                Log.Comment("Error happened");
            }
        }
        public TextBox Name
        {
            get;
            private set;
        }

        public TextBox Phone
        {
            get;
            private set;
        }

        public Page Root
        {
            get;
            private set;
        }

        private PersonModel Model;
        #region IDisposable Support

        private bool disposedValue = false; // To detect redundant calls
        private bool isDisposing = false;

        private void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                this.isDisposing = disposing;
                if (disposing)
                {
                    UIExecutor.Execute(() =>
                    {
                        // Clear the error collections so that we don't leak. In normal app scenarios,
                        // this collection being populated will prevent the user from doing anything else, so
                        // it's more likely than not that this will always wind up cleared and we won't see actual
                        // leaks in apps. Clearing the collections will result in the errorpresenter be re-deferred
                        // and the leak goes away. The leak is due to x:Load.
                        this.Phone.ValidationErrors.Clear();
                        this.Name.ValidationErrors.Clear();

                        this.Root = null;
                        this.Phone = null;
                        this.Name = null;
                        TestServices.WindowHelper.WindowContent = null;

                        Application.Current.Resources.Remove("DefaultCompactErrorIconTemplate");
                    });
                    TestServices.WindowHelper.WaitForIdle();

                    TestServices.WindowHelper.ShutdownXaml();
                    TestServices.WindowHelper.VerifyTestCleanup();
                }
                disposedValue = true;
            }
        }

        // This code added to correctly implement the disposable pattern.
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(true);
            // TODO: uncomment the following line if the finalizer is overridden above.
            // GC.SuppressFinalize(this);
        }

        #endregion
    }

    [TestClass]
    public class DataValidationIntegrationTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        public static void Setup(TestContext context)
        {
            TestServices.EnsureInitialized();
            XamlTestsBase.SetupBase(context);
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        public void CanSetAndRetrieveErrors()
        {
            using (var testHelper = new DataValidationTestHelper())
            {
                testHelper.SetInvalidText(testHelper.Phone, "hi there");
                testHelper.AddError(testHelper.Phone, "Text");
                testHelper.ValidateError(testHelper.Phone, 0, "Text");
            }
        }

        [TestMethod]
        public void RespectUseSystemValidationVisualsProperty()
        {
            using (var testHelper = new DataValidationTestHelper("InputValidationMode='Disabled'"))
            {
                testHelper.SetInvalidText(testHelper.Phone, "hi there");
                testHelper.AddError(testHelper.Phone, "Text");

                UIExecutor.Execute(() =>
                {
                    var errors = testHelper.Phone.ValidationErrors;
                    Verify.AreEqual(1, errors.Count);

                    var error = errors[0];
                    Verify.AreEqual("Text", error.ErrorMessage);

                    Verify.IsTrue(testHelper.Phone.HasValidationErrors);

                    // Verify the Error Presenter is not in the tree
                    Verify.IsNull(testHelper.GetErrorPresenter(testHelper.Phone));
                });

                UIExecutor.Execute(() =>
                {
                    testHelper.Phone.InputValidationMode = InputValidationMode.Auto;
                });
                TestServices.WindowHelper.WaitForIdle();

                // Verify the Error Presenter is now in the tree
                Verify.IsNotNull(testHelper.GetErrorPresenter(testHelper.Phone));
            }
        }

        [TestMethod]
        public void VisualizeMultipleErrors()
        {
            using (var testHelper = new DataValidationTestHelper())
            {
                testHelper.SetInvalidText(testHelper.Phone, "hi there");

                testHelper.AddError(testHelper.Phone, "Text");
                testHelper.AddError(testHelper.Phone, "Required");

                testHelper.ValidateError(testHelper.Phone, 0, "Text");
                testHelper.ValidateError(testHelper.Phone, 1, "Required");
            }
        }

        [TestMethod]
        public void CanBindToIndexInValidationErrorsCollection()
        {
            using (var testHelper = new DataValidationTestHelper("ErrorTemplate='{StaticResource MyCustomErrorTemplate}'"))
            {
                testHelper.SetInvalidText(testHelper.Phone, "hi there");
                testHelper.AddError(testHelper.Phone, "Text is invalid");
                testHelper.ValidateError(testHelper.Phone, 0, "Text is invalid");
            }
        }

        [TestMethod]
        public void MultipleControlsWithErrors()
        {
            using (var testHelper = new DataValidationTestHelper())
            {
                testHelper.SetInvalidText(testHelper.Phone, "hi there");
                testHelper.AddError(testHelper.Phone, "Text");

                testHelper.SetInvalidText(testHelper.Name, String.Empty);
                testHelper.AddError(testHelper.Name, "Name is required");

                testHelper.ValidateError(testHelper.Phone, 0, "Text");
                testHelper.ValidateError(testHelper.Name, 0, "Name is required");

                UIExecutor.Execute(() =>
                {
                    Verify.AreSame(testHelper.Phone.ErrorTemplate, testHelper.Name.ErrorTemplate);
                });
            }
        }

        [TestMethod]
        public void DontCrashWhenErrorsCollectionHasntBeenCreated()
        {
            using (var testHelper = new DataValidationTestHelper("InputValidationMode='Default'"))
            {
                Log.Comment("yay we didn't crash!");
            }
        }

        [TestMethod]
        public void ValidateDescription()
        {
            using (var testHelper = new DataValidationTestHelper("Description='Hello good sir, do things my way or the highway'"))
            {
                testHelper.ValidateDescription(testHelper.Phone, "Hello good sir, do things my way or the highway");

                Log.Comment("Validate that the errors override the description");
                testHelper.SetInvalidText(testHelper.Phone, "hi there");
                testHelper.AddError(testHelper.Phone, "Text");
                testHelper.ValidateError(testHelper.Phone, 0, "Text");
            }
        }

        [TestMethod]
        public void ValidateVisualStates()
        {
            using (var testHelper = new DataValidationTestHelper())
            {
                Log.Comment("Set the description while in compact mode, make sure it displays properly");
                UIExecutor.Execute(() =>
                {
                    testHelper.Name.Description = "Enter something nice";
                });

                TestServices.WindowHelper.WaitForIdle();
                testHelper.ValidateDescription(testHelper.Name, "Enter something nice");

                Log.Comment("Add an error and validate it works properly");
                testHelper.SetInvalidText(testHelper.Name, String.Empty);
                testHelper.AddError(testHelper.Name, "Name is required");
                testHelper.ValidateError(testHelper.Name, 0, "Name is required");

                Log.Comment("Validate the binding is hooked up properly");
                UIExecutor.Execute(() =>
                {
                    var presenter = testHelper.GetErrorPresenter(testHelper.Name);
                    var parent = VisualTreeHelper.GetParent(presenter) as Grid;

                    Verify.AreEqual(Grid.GetColumn(presenter), 4.0);
                    Verify.AreEqual(Grid.GetRow(presenter), 1.0);

                    var column = parent.ColumnDefinitions[Grid.GetColumn(presenter)];
                    Verify.AreEqual(20.0, column.MaxWidth);
                });

                Log.Comment("Changing InputValidationKind and validating changes");
                UIExecutor.Execute(() =>
                {
                    testHelper.Name.InputValidationKind = InputValidationKind.Inline;
                });

                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    var presenter = testHelper.GetErrorPresenter(testHelper.Name);
                    var parent = VisualTreeHelper.GetParent(presenter) as Grid;

                    Verify.AreEqual(Grid.GetColumn(presenter), 2.0);
                    Verify.AreEqual(Grid.GetRow(presenter), 2.0);

                    var row = parent.RowDefinitions[Grid.GetRow(presenter)];
                    Verify.AreEqual(20.0, row.MinHeight);
                });
            }
        }

#if false
        [TestMethod]
        public void ValidateRequiredHeaderPlacement()
        {
            using (var testHelper = new DataValidationTestHelper("HeaderPlacement='Left'"))
            {
                UIExecutor.Execute(() =>
                {
                    var requiredHeaderPresenter = testHelper.GetRequiredHeaderPresenter(testHelper.Phone);
                    Verify.IsNotNull(requiredHeaderPresenter);

                    Verify.AreEqual(Grid.GetColumn(requiredHeaderPresenter), 0.0);
                    Verify.AreEqual(Grid.GetRow(requiredHeaderPresenter), 1.0);

                    testHelper.Phone.HeaderPlacement = ControlHeaderPlacement.Top;
                });

                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    var requiredHeaderPresenter = testHelper.GetRequiredHeaderPresenter(testHelper.Phone);
                    Verify.IsNotNull(requiredHeaderPresenter);

                    Verify.AreEqual(Grid.GetColumn(requiredHeaderPresenter), 1.0);
                    Verify.AreEqual(Grid.GetRow(requiredHeaderPresenter), 0.0);
                });
            }
        }
#endif
    }
}

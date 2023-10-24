// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using Microsoft.UI.Xaml.Markup;
using XamlOM;

namespace Microsoft.UI.Xaml.Controls
{
    //
    // Notice:
    //  This file is already too large. If possible please add new metadata into its own file.
    //  Ideally into a folder in ..\..\Modules\Controls\ or in a new module under ..\..\Modules
    //
    //


    public interface INavigate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Boolean Navigate(Windows.UI.Xaml.Interop.TypeName sourcePageType);
    }

    [Platform(typeof(PrivateApiContract), 1)]
    [DXamlIdlGroup("Controls2")]
    public interface IFramePrivate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void GetNavigationTransitionInfoOverride(out Microsoft.UI.Xaml.Media.Animation.NavigationTransitionInfo infoOverride, out Windows.Foundation.Boolean isBackNavigation, out Windows.Foundation.Boolean isInitialPage);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void SetNavigationTransitionInfoOverride([Optional] Microsoft.UI.Xaml.Media.Animation.NavigationTransitionInfo infoOverride);
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.INavigate))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IFramePrivate))]
    [Guids(ClassGuid = "6dd83456-8b6f-4c19-9e09-501a6de803f6")]
    public class Frame
     : Microsoft.UI.Xaml.Controls.ContentControl
    {
        public Windows.Foundation.Int32 CacheSize
        {
            get;
            set;
        }

        [FieldBacked]
        public Windows.Foundation.Boolean CanGoBack
        {
            get;
            internal set;
        }

        [FieldBacked]
        public Windows.Foundation.Boolean CanGoForward
        {
            get;
            internal set;
        }

        [FieldBacked]
        public Windows.UI.Xaml.Interop.TypeName CurrentSourcePageType
        {
            get;
            internal set;
        }

        [FieldBacked]
        public Windows.UI.Xaml.Interop.TypeName SourcePageType
        {
            get;
            set;
        }

        [FieldBacked]
        public Windows.Foundation.Int32 BackStackDepth
        {
            get;
            internal set;
        }

        public event Microsoft.UI.Xaml.Navigation.NavigatedEventHandler Navigated;

        public event Microsoft.UI.Xaml.Navigation.NavigatingCancelEventHandler Navigating;

        public event Microsoft.UI.Xaml.Navigation.NavigationFailedEventHandler NavigationFailed;

        public event Microsoft.UI.Xaml.Navigation.NavigationStoppedEventHandler NavigationStopped;

        public Frame() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void GoBack()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("GoBackWithTransitionInfo")]
        [DXamlOverloadName("GoBack")]
        public void GoBackWithTransitionInfo([Optional] Microsoft.UI.Xaml.Media.Animation.NavigationTransitionInfo transitionInfoOverride)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void GoForward()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("Navigate")]
        public Windows.Foundation.Boolean Navigate(Windows.UI.Xaml.Interop.TypeName sourcePageType, [Optional] Windows.Foundation.Object parameter)
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("NavigateWithTransitionInfo")]
        [DXamlOverloadName("Navigate")]
        public Windows.Foundation.Boolean NavigateWithTransitionInfo(Windows.UI.Xaml.Interop.TypeName sourcePageType, [Optional] Windows.Foundation.Object parameter, [Optional] Microsoft.UI.Xaml.Media.Animation.NavigationTransitionInfo infoOverride)
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("NavigateToType")]
        public Windows.Foundation.Boolean NavigateToType(Windows.UI.Xaml.Interop.TypeName sourcePageType, [Optional] Windows.Foundation.Object parameter, [Optional] Microsoft.UI.Xaml.Navigation.FrameNavigationOptions navigationOptions)
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.String GetNavigationState()
        {
            return default(Windows.Foundation.String);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("SetNavigationState")]
        [DXamlOverloadName("SetNavigationState")]
        public void SetNavigationState(Windows.Foundation.String navigationState)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("SetNavigationStateWithNavigationControl")]
        [DXamlOverloadName("SetNavigationState")]
        public void SetNavigationState(Windows.Foundation.String navigationState, bool suppressNavigate)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [CollectionType(CollectionKind.Vector, NewCodeGenCollectionType = typeof(Windows.Foundation.Collections.IVector<Microsoft.UI.Xaml.Navigation.PageStackEntry>))]
        [ReadOnly]
        public Microsoft.UI.Xaml.Controls.PageStackEntryCollection BackStack
        {
            get;
            internal set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [CollectionType(CollectionKind.Vector, NewCodeGenCollectionType = typeof(Windows.Foundation.Collections.IVector<Microsoft.UI.Xaml.Navigation.PageStackEntry>))]
        [ReadOnly]
        public Microsoft.UI.Xaml.Controls.PageStackEntryCollection ForwardStack
        {
            get;
            internal set;
        }

        public Windows.Foundation.Boolean IsNavigationStackEnabled
        {
            get;
            set;
        }
    }

    [CodeGen(CodeGenLevel.LookupOnly, Partial = true)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Modifier(Modifier.Internal)]
    [HideFromNewCodeGen]
    [Guids(ClassGuid = "755317e0-b2a2-4e92-a394-d3f7300c9e82")]
    public sealed class PageStackEntryCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<Microsoft.UI.Xaml.Navigation.PageStackEntry>
    {

        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.Navigation.PageStackEntry ContentProperty
        {
            get;
            set;
        }

        public PageStackEntryCollection() { }
    }

    internal interface IPaginatedPanel
    {
        [CodeGen(CodeGenLevel.Idl)]
        Windows.Foundation.Int32 GetLastItemIndexInViewport(Microsoft.UI.Xaml.Controls.Primitives.IScrollInfo scrollInfo);

        [CodeGen(CodeGenLevel.Idl)]
        Windows.Foundation.Double GetItemsPerPage(Microsoft.UI.Xaml.Controls.Primitives.IScrollInfo scrollInfo);
    }

    [TypeTable(IsExcludedFromCore = true)]
    internal interface IContainerRecyclingContext
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void PrepareForItemRecycling([Optional] Windows.Foundation.Object item);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        bool IsCompatible(Microsoft.UI.Xaml.UIElement candidate);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        Microsoft.UI.Xaml.UIElement SelectedContainer { get; set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void ConfigureSelectedContainer(Microsoft.UI.Xaml.UIElement container);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void StopRecycling();
    }

    [DXamlIdlGroup("Controls2")]
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "33ac1788-ad8a-43d8-9d3d-2eba28017d39")]
    internal sealed class ApplicationBarService
     : Microsoft.UI.Xaml.DependencyObject
    {
        internal ApplicationBarService() { }
    }

    [DXamlIdlGroup("Controls2")]
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [InstanceCountTelemetry]
    [Platform("Feature_HeaderPlacement", typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "5f8e05c8-4ea3-4ee1-a5a0-a35719a32671")]
    [ContentProperty("Header")]
    public class DatePicker
     : Microsoft.UI.Xaml.Controls.Control
    {
        #region Properties

        // Version 1
        [PropertyFlags(AffectsMeasure = true)]
        public Windows.Foundation.Object Header { get; set; }

        [PropertyFlags(AffectsMeasure = true)]
        public Microsoft.UI.Xaml.DataTemplate HeaderTemplate { get; set; }

        public Windows.Foundation.String CalendarIdentifier { get; set; }

        public Windows.Foundation.DateTime Date { get; set; }

        public Windows.Foundation.Boolean DayVisible { get; set; }

        public Windows.Foundation.Boolean MonthVisible { get; set; }

        public Windows.Foundation.Boolean YearVisible { get; set; }

        public Windows.Foundation.String DayFormat { get; set; }

        public Windows.Foundation.String MonthFormat { get; set; }

        public Windows.Foundation.String YearFormat { get; set; }

        public Windows.Foundation.DateTime MinYear { get; set; }

        public Windows.Foundation.DateTime MaxYear { get; set; }

        public Microsoft.UI.Xaml.Controls.Orientation Orientation { get; set; }

        public Microsoft.UI.Xaml.Controls.LightDismissOverlayMode LightDismissOverlayMode { get; set; }

        public Windows.Foundation.DateTime? SelectedDate { get; set; }

        // Velocity
        [VelocityFeature("Feature_HeaderPlacement")]
        [NativeStorageType(ValueType.valueEnum)]
        public Microsoft.UI.Xaml.Controls.ControlHeaderPlacement HeaderPlacement { get; set; }

        #endregion

        #region Events

        // Version 1
        public event Windows.Foundation.EventHandler<DatePickerValueChangedEventArgs> DateChanged;

        public event Windows.Foundation.TypedEventHandler<DatePicker,DatePickerSelectedValueChangedEventArgs> SelectedDateChanged;

        #endregion

        public DatePicker() { }
    }

    [DXamlIdlGroup("Controls2")]
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "480b852f-078a-48a0-bfad-8a66a7f53fcc")]
    public sealed class DatePickerValueChangedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        public Windows.Foundation.DateTime OldDate
        {
            get;
            internal set;
        }

        public Windows.Foundation.DateTime NewDate
        {
            get;
            internal set;
        }

        internal DatePickerValueChangedEventArgs() { }
    }

    [DXamlIdlGroup("Controls2")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "53a9634e-0f76-462b-87da-bfc35469e4e8")]
    public sealed class DatePickerSelectedValueChangedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        public Windows.Foundation.DateTime? OldDate
        {
            get;
            internal set;
        }

        public Windows.Foundation.DateTime? NewDate
        {
            get;
            internal set;
        }

        internal DatePickerSelectedValueChangedEventArgs() { }
    }

    [Comment("Commonly-used symbols in the Segoe Symbol UI font.")]
    [DXamlIdlGroup("Controls2")]
    [NativeName("KnownSymbol")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [NativeComment("Commonly-used symbols in the Segoe Symbol UI font.")]
    public enum Symbol
    {
        [NativeValueName("KnownSymbol_Previous")]
        Previous = 0xE100,
        [NativeValueName("KnownSymbol_Next")]
        Next = 0xE101,
        [NativeValueName("KnownSymbol_Play")]
        Play = 0xE102,
        [NativeValueName("KnownSymbol_Pause")]
        Pause = 0xE103,
        [NativeValueName("KnownSymbol_Edit")]
        Edit = 0xE104,
        [NativeValueName("KnownSymbol_Save")]
        Save = 0xE105,
        [NativeValueName("KnownSymbol_Clear")]
        Clear = 0xE106,
        [NativeValueName("KnownSymbol_Delete")]
        Delete = 0xE107,
        [NativeValueName("KnownSymbol_Remove")]
        Remove = 0xE108,
        [NativeValueName("KnownSymbol_Add")]
        Add = 0xE109,
        [NativeValueName("KnownSymbol_Cancel")]
        Cancel = 0xE10A,
        [NativeValueName("KnownSymbol_Accept")]
        Accept = 0xE10B,
        [NativeValueName("KnownSymbol_More")]
        More = 0xE10C,
        [NativeValueName("KnownSymbol_Redo")]
        Redo = 0xE10D,
        [NativeValueName("KnownSymbol_Undo")]
        Undo = 0xE10E,
        [NativeValueName("KnownSymbol_Home")]
        Home = 0xE10F,
        [NativeValueName("KnownSymbol_Up")]
        Up = 0xE110,
        [NativeValueName("KnownSymbol_Forward")]
        Forward = 0xE111,
        [NativeValueName("KnownSymbol_Back")]
        Back = 0xE112,
        [NativeValueName("KnownSymbol_Favorite")]
        Favorite = 0xE113,
        [NativeValueName("KnownSymbol_Camera")]
        Camera = 0xE114,
        [NativeValueName("KnownSymbol_Setting")]
        Setting = 0xE115,
        [NativeValueName("KnownSymbol_Video")]
        Video = 0xE116,
        [NativeValueName("KnownSymbol_Sync")]
        Sync = 0xE117,
        [NativeValueName("KnownSymbol_Download")]
        Download = 0xE118,
        [NativeValueName("KnownSymbol_Mail")]
        Mail = 0xE119,
        [NativeValueName("KnownSymbol_Find")]
        Find = 0xE11A,
        [NativeValueName("KnownSymbol_Help")]
        Help = 0xE11B,
        [NativeValueName("KnownSymbol_Upload")]
        Upload = 0xE11C,
        [NativeValueName("KnownSymbol_Emoji")]
        Emoji = 0xE11D,
        [NativeValueName("KnownSymbol_TwoPage")]
        TwoPage = 0xE11E,
        [NativeValueName("KnownSymbol_LeaveChat")]
        LeaveChat = 0xE11F,
        [NativeValueName("KnownSymbol_MailForward")]
        MailForward = 0xE120,
        [NativeValueName("KnownSymbol_Clock")]
        Clock = 0xE121,
        [NativeValueName("KnownSymbol_Send")]
        Send = 0xE122,
        [NativeValueName("KnownSymbol_Crop")]
        Crop = 0xE123,
        [NativeValueName("KnownSymbol_RotateCamera")]
        RotateCamera = 0xE124,
        [NativeValueName("KnownSymbol_People")]
        People = 0xE125,
        [NativeValueName("KnownSymbol_OpenPane")]
        OpenPane = 0xE126,
        [NativeValueName("KnownSymbol_ClosePane")]
        ClosePane = 0xE127,
        [NativeValueName("KnownSymbol_World")]
        World = 0xE128,
        [NativeValueName("KnownSymbol_Flag")]
        Flag = 0xE129,
        [NativeValueName("KnownSymbol_PreviewLink")]
        PreviewLink = 0xE12A,
        [NativeValueName("KnownSymbol_Globe")]
        Globe = 0xE12B,
        [NativeValueName("KnownSymbol_Trim")]
        Trim = 0xE12C,
        [NativeValueName("KnownSymbol_AttachCamera")]
        AttachCamera = 0xE12D,
        [NativeValueName("KnownSymbol_ZoomIn")]
        ZoomIn = 0xE12E,
        [NativeValueName("KnownSymbol_Bookmarks")]
        Bookmarks = 0xE12F,
        [NativeValueName("KnownSymbol_Document")]
        Document = 0xE130,
        [NativeValueName("KnownSymbol_ProtectedDocument")]
        ProtectedDocument = 0xE131,
        [NativeValueName("KnownSymbol_Page")]
        Page = 0xE132,
        [NativeValueName("KnownSymbol_Bullets")]
        Bullets = 0xE133,
        [NativeValueName("KnownSymbol_Comment")]
        Comment = 0xE134,
        [NativeValueName("KnownSymbol_MailFilled")]
        MailFilled = 0xE135,
        [NativeValueName("KnownSymbol_ContactInfo")]
        ContactInfo = 0xE136,
        [NativeValueName("KnownSymbol_HangUp")]
        HangUp = 0xE137,
        [NativeValueName("KnownSymbol_ViewAll")]
        ViewAll = 0xE138,
        [NativeValueName("KnownSymbol_MapPin")]
        MapPin = 0xE139,
        [NativeValueName("KnownSymbol_Phone")]
        Phone = 0xE13A,
        [NativeValueName("KnownSymbol_VideoChat")]
        VideoChat = 0xE13B,
        [NativeValueName("KnownSymbol_Switch")]
        Switch = 0xE13C,
        [NativeValueName("KnownSymbol_Contact")]
        Contact = 0xE13D,
        [NativeValueName("KnownSymbol_Rename")]
        Rename = 0xE13E,
        [NativeValueName("KnownSymbol_Pin")]
        Pin = 0xE141,
        [NativeValueName("KnownSymbol_MusicInfo")]
        MusicInfo = 0xE142,
        [NativeValueName("KnownSymbol_Go")]
        Go = 0xE143,
        [NativeValueName("KnownSymbol_Keyboard")]
        Keyboard = 0xE144,
        [NativeValueName("KnownSymbol_DockLeft")]
        DockLeft = 0xE145,
        [NativeValueName("KnownSymbol_DockRight")]
        DockRight = 0xE146,
        [NativeValueName("KnownSymbol_DockBottom")]
        DockBottom = 0xE147,
        [NativeValueName("KnownSymbol_Remote")]
        Remote = 0xE148,
        [NativeValueName("KnownSymbol_Refresh")]
        Refresh = 0xE149,
        [NativeValueName("KnownSymbol_Rotate")]
        Rotate = 0xE14A,
        [NativeValueName("KnownSymbol_Shuffle")]
        Shuffle = 0xE14B,
        [NativeValueName("KnownSymbol_List")]
        List = 0xE14C,
        [NativeValueName("KnownSymbol_Shop")]
        Shop = 0xE14D,
        [NativeValueName("KnownSymbol_SelectAll")]
        SelectAll = 0xE14E,
        [NativeValueName("KnownSymbol_Orientation")]
        Orientation = 0xE14F,
        [NativeValueName("KnownSymbol_Import")]
        Import = 0xE150,
        [NativeValueName("KnownSymbol_ImportAll")]
        ImportAll = 0xE151,
        [NativeValueName("KnownSymbol_BrowsePhotos")]
        BrowsePhotos = 0xE155,
        [NativeValueName("KnownSymbol_WebCam")]
        WebCam = 0xE156,
        [NativeValueName("KnownSymbol_Pictures")]
        Pictures = 0xE158,
        [NativeValueName("KnownSymbol_SaveLocal")]
        SaveLocal = 0xE159,
        [NativeValueName("KnownSymbol_Caption")]
        Caption = 0xE15A,
        [NativeValueName("KnownSymbol_Stop")]
        Stop = 0xE15B,
        [NativeValueName("KnownSymbol_ShowResults")]
        ShowResults = 0xE15C,
        [NativeValueName("KnownSymbol_Volume")]
        Volume = 0xE15D,
        [NativeValueName("KnownSymbol_Repair")]
        Repair = 0xE15E,
        [NativeValueName("KnownSymbol_Message")]
        Message = 0xE15F,
        [NativeValueName("KnownSymbol_Page2")]
        Page2 = 0xE160,
        [NativeValueName("KnownSymbol_CalendarDay")]
        CalendarDay = 0xE161,
        [NativeValueName("KnownSymbol_CalendarWeek")]
        CalendarWeek = 0xE162,
        [NativeValueName("KnownSymbol_Calendar")]
        Calendar = 0xE163,
        [NativeValueName("KnownSymbol_Character")]
        Character = 0xE164,
        [NativeValueName("KnownSymbol_MailReplyAll")]
        MailReplyAll = 0xE165,
        [NativeValueName("KnownSymbol_Read")]
        Read = 0xE166,
        [NativeValueName("KnownSymbol_Link")]
        Link = 0xE167,
        [NativeValueName("KnownSymbol_Account")]
        Account = 0xE168,
        [NativeValueName("KnownSymbol_ShowBcc")]
        ShowBcc = 0xE169,
        [NativeValueName("KnownSymbol_HideBcc")]
        HideBcc = 0xE16A,
        [NativeValueName("KnownSymbol_Cut")]
        Cut = 0xE16B,
        [NativeValueName("KnownSymbol_Attach")]
        Attach = 0xE16C,
        [NativeValueName("KnownSymbol_Paste")]
        Paste = 0xE16D,
        [NativeValueName("KnownSymbol_Filter")]
        Filter = 0xE16E,
        [NativeValueName("KnownSymbol_Copy")]
        Copy = 0xE16F,
        [NativeValueName("KnownSymbol_Emoji2")]
        Emoji2 = 0xE170,
        [NativeValueName("KnownSymbol_Important")]
        Important = 0xE171,
        [NativeValueName("KnownSymbol_MailReply")]
        MailReply = 0xE172,
        [NativeValueName("KnownSymbol_SlideShow")]
        SlideShow = 0xE173,
        [NativeValueName("KnownSymbol_Sort")]
        Sort = 0xE174,
        [NativeValueName("KnownSymbol_Manage")]
        Manage = 0xE178,
        [NativeValueName("KnownSymbol_AllApps")]
        AllApps = 0xE179,
        [NativeValueName("KnownSymbol_DisconnectDrive")]
        DisconnectDrive = 0xE17A,
        [NativeValueName("KnownSymbol_MapDrive")]
        MapDrive = 0xE17B,
        [NativeValueName("KnownSymbol_NewWindow")]
        NewWindow = 0xE17C,
        [NativeValueName("KnownSymbol_OpenWith")]
        OpenWith = 0xE17D,
        [NativeValueName("KnownSymbol_ContactPresence")]
        ContactPresence = 0xE181,
        [NativeValueName("KnownSymbol_Priority")]
        Priority = 0xE182,
        [NativeValueName("KnownSymbol_GoToToday")]
        GoToToday = 0xE184,
        [NativeValueName("KnownSymbol_Font")]
        Font = 0xE185,
        [NativeValueName("KnownSymbol_FontColor")]
        FontColor = 0xE186,
        [NativeValueName("KnownSymbol_Contact2")]
        Contact2 = 0xE187,
        [NativeValueName("KnownSymbol_Folder")]
        Folder = 0xE188,
        [NativeValueName("KnownSymbol_Audio")]
        Audio = 0xE189,
        [NativeValueName("KnownSymbol_Placeholder")]
        Placeholder = 0xE18A,
        [NativeValueName("KnownSymbol_View")]
        View = 0xE18B,
        [NativeValueName("KnownSymbol_SetLockScreen")]
        SetLockScreen = 0xE18C,
        [NativeValueName("KnownSymbol_SetTile")]
        SetTile = 0xE18D,
        [NativeValueName("KnownSymbol_ClosedCaption")]
        ClosedCaption = 0xE190,
        [NativeValueName("KnownSymbol_StopSlideShow")]
        StopSlideShow = 0xE191,
        [NativeValueName("KnownSymbol_Permissions")]
        Permissions = 0xE192,
        [NativeValueName("KnownSymbol_Highlight")]
        Highlight = 0xE193,
        [NativeValueName("KnownSymbol_DisableUpdates")]
        DisableUpdates = 0xE194,
        [NativeValueName("KnownSymbol_UnFavorite")]
        UnFavorite = 0xE195,
        [NativeValueName("KnownSymbol_UnPin")]
        UnPin = 0xE196,
        [NativeValueName("KnownSymbol_OpenLocal")]
        OpenLocal = 0xE197,
        [NativeValueName("KnownSymbol_Mute")]
        Mute = 0xE198,
        [NativeValueName("KnownSymbol_Italic")]
        Italic = 0xE199,
        [NativeValueName("KnownSymbol_Underline")]
        Underline = 0xE19A,
        [NativeValueName("KnownSymbol_Bold")]
        Bold = 0xE19B,
        [NativeValueName("KnownSymbol_MoveToFolder")]
        MoveToFolder = 0xE19C,
        [NativeValueName("KnownSymbol_LikeDislike")]
        LikeDislike = 0xE19D,
        [NativeValueName("KnownSymbol_Dislike")]
        Dislike = 0xE19E,
        [NativeValueName("KnownSymbol_Like")]
        Like = 0xE19F,
        [NativeValueName("KnownSymbol_AlignRight")]
        AlignRight = 0xE1A0,
        [NativeValueName("KnownSymbol_AlignCenter")]
        AlignCenter = 0xE1A1,
        [NativeValueName("KnownSymbol_AlignLeft")]
        AlignLeft = 0xE1A2,
        [NativeValueName("KnownSymbol_Zoom")]
        Zoom = 0xE1A3,
        [NativeValueName("KnownSymbol_ZoomOut")]
        ZoomOut = 0xE1A4,
        [NativeValueName("KnownSymbol_OpenFile")]
        OpenFile = 0xE1A5,
        [NativeValueName("KnownSymbol_OtherUser")]
        OtherUser = 0xE1A6,
        [NativeValueName("KnownSymbol_Admin")]
        Admin = 0xE1A7,
        [NativeValueName("KnownSymbol_Street")]
        Street = 0xE1C3,
        [NativeValueName("KnownSymbol_Map")]
        Map = 0xE1C4,
        [NativeValueName("KnownSymbol_ClearSelection")]
        ClearSelection = 0xE1C5,
        [NativeValueName("KnownSymbol_FontDecrease")]
        FontDecrease = 0xE1C6,
        [NativeValueName("KnownSymbol_FontIncrease")]
        FontIncrease = 0xE1C7,
        [NativeValueName("KnownSymbol_FontSize")]
        FontSize = 0xE1C8,
        [NativeValueName("KnownSymbol_CellPhone")]
        CellPhone = 0xE1C9,
        [NativeValueName("KnownSymbol_ReShare")]
        ReShare = 0xE1CA,
        [NativeValueName("KnownSymbol_Tag")]
        Tag = 0xE1CB,
        [NativeValueName("KnownSymbol_RepeatOne")]
        RepeatOne = 0xE1CC,
        [NativeValueName("KnownSymbol_RepeatAll")]
        RepeatAll = 0xE1CD,
        [NativeValueName("KnownSymbol_OutlineStar")]
        OutlineStar = 0xE1CE,
        [NativeValueName("KnownSymbol_SolidStar")]
        SolidStar = 0xE1CF,
        [NativeValueName("KnownSymbol_Calculator")]
        Calculator = 0xE1D0,
        [NativeValueName("KnownSymbol_Directions")]
        Directions = 0xE1D1,
        [NativeValueName("KnownSymbol_Target")]
        Target = 0xE1D2,
        [NativeValueName("KnownSymbol_Library")]
        Library = 0xE1D3,
        [NativeValueName("KnownSymbol_PhoneBook")]
        PhoneBook = 0xE1D4,
        [NativeValueName("KnownSymbol_Memo")]
        Memo = 0xE1D5,
        [NativeValueName("KnownSymbol_Microphone")]
        Microphone = 0xE1D6,
        [NativeValueName("KnownSymbol_PostUpdate")]
        PostUpdate = 0xE1D7,
        [NativeValueName("KnownSymbol_BackToWindow")]
        BackToWindow = 0xE1D8,
        [NativeValueName("KnownSymbol_FullScreen")]
        FullScreen = 0xE1D9,
        [NativeValueName("KnownSymbol_NewFolder")]
        NewFolder = 0xE1DA,
        [NativeValueName("KnownSymbol_CalendarReply")]
        CalendarReply = 0xE1DB,
        [NativeValueName("KnownSymbol_UnSyncFolder")]
        UnSyncFolder = 0xE1DD,
        [NativeValueName("KnownSymbol_ReportHacked")]
        ReportHacked = 0xE1DE,
        [NativeValueName("KnownSymbol_SyncFolder")]
        SyncFolder = 0xE1DF,
        [NativeValueName("KnownSymbol_BlockContact")]
        BlockContact = 0xE1E0,
        [NativeValueName("KnownSymbol_SwitchApps")]
        SwitchApps = 0xE1E1,
        [NativeValueName("KnownSymbol_AddFriend")]
        AddFriend = 0xE1E2,
        [NativeValueName("KnownSymbol_TouchPointer")]
        TouchPointer = 0xE1E3,
        [NativeValueName("KnownSymbol_GoToStart")]
        GoToStart = 0xE1E4,
        [NativeValueName("KnownSymbol_ZeroBars")]
        ZeroBars = 0xE1E5,
        [NativeValueName("KnownSymbol_OneBar")]
        OneBar = 0xE1E6,
        [NativeValueName("KnownSymbol_TwoBars")]
        TwoBars = 0xE1E7,
        [NativeValueName("KnownSymbol_ThreeBars")]
        ThreeBars = 0xE1E8,
        [NativeValueName("KnownSymbol_FourBars")]
        FourBars = 0xE1E9,
        [NativeValueName("KnownSymbol_Scan")]
        Scan = 0xE294,
        [NativeValueName("KnownSymbol_Preview")]
        Preview = 0xE295,
        [NativeValueName("KnownSymbol_GlobalNavigationButton")]
        GlobalNavigationButton = 0xE700,
        [NativeValueName("KnownSymbol_Share")]
        Share = 0xE72D,
        [NativeValueName("KnownSymbol_Print")]
        Print = 0xE749,
        [NativeValueName("KnownSymbol_XboxOneConsole")]
        XboxOneConsole = 0xE990,
    }

    [DXamlIdlGroup("Controls2")]
    [NativeName("CIconElement")]
    [ClassFlags(HasTypeConverter = true)]
    [Guids(ClassGuid = "696359e5-f50c-4784-8f88-64312655ba5b")]
    public abstract class IconElement
     : Microsoft.UI.Xaml.FrameworkElement
    {
        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTextFormatting")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_pForeground")]
        public Microsoft.UI.Xaml.Media.Brush Foreground
        {
            get;
            set;
        }

        internal IconElement() { }
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [NativeName("CSymbolIcon")]
    [ClassFlags(HasTypeConverter = true)]
    [Guids(ClassGuid = "610584aa-9a0f-4a60-93bc-52bb48367145")]
    public sealed class SymbolIcon
     : Microsoft.UI.Xaml.Controls.IconElement
    {
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_nSymbol")]
        public Microsoft.UI.Xaml.Controls.Symbol Symbol
        {
            get;
            set;
        }

        public SymbolIcon() { }

        [FactoryMethodName("CreateInstanceWithSymbol")]
        public SymbolIcon(Microsoft.UI.Xaml.Controls.Symbol symbol) { }
    }

    [DXamlIdlGroup("Controls2")]
    [NativeName("CFontIcon")]
    [Guids(ClassGuid = "e552d78b-fadf-4e3f-b438-e14b1fab23ea")]
    public class FontIcon
     : Microsoft.UI.Xaml.Controls.IconElement
    {
        [NativeStorageType(ValueType.valueString)]
        [OffsetFieldName("m_strGlyph")]
        public Windows.Foundation.String Glyph
        {
            get;
            set;
        }

        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_eFontSize")]
        public Windows.Foundation.Double FontSize
        {
            get;
            set;
        }

        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_pFontFamily")]
        public Microsoft.UI.Xaml.Media.FontFamily FontFamily
        {
            get;
            set;
        }

        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nFontWeight")]
        [CoreType(typeof(Windows.UI.Text.CoreFontWeight), NewCodeGenPropertyType = typeof(Windows.UI.Text.FontWeight))]
        public Windows.UI.Text.FontWeight FontWeight
        {
            get;
            set;
        }

        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nFontStyle")]
        public Windows.UI.Text.FontStyle FontStyle
        {
            get;
            set;
        }

        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_isTextScaleFactorEnabled")]
        public Windows.Foundation.Boolean IsTextScaleFactorEnabled
        {
            get;
            set;
        }

        public Windows.Foundation.Boolean MirroredWhenRightToLeft
        {
            get;
            set;
        }

        public FontIcon() { }
    }

    [DXamlIdlGroup("Controls2")]
    [NativeName("CPathIcon")]
    [ClassFlags(HasTypeConverter = true)]
    [Guids(ClassGuid = "21aa1b55-3a07-4c16-a333-70484c4cca73")]
    public class PathIcon
     : Microsoft.UI.Xaml.Controls.IconElement
    {
        [RequiresMultipleAssociationCheck]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pData")]
        public Microsoft.UI.Xaml.Media.Geometry Data
        {
            get;
            set;
        }

        public PathIcon() { }
    }

    [DXamlIdlGroup("Controls2")]
    [NativeName("CBitmapIcon")]
    [Guids(ClassGuid = "c7031ab4-1abf-4bbc-980f-88d87c7938f3")]
    public class BitmapIcon
     : Microsoft.UI.Xaml.Controls.IconElement
    {
        [NativeStorageType(ValueType.valueString)]
        [OffsetFieldName("m_strSource")]
        public Windows.Foundation.Uri UriSource
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_showAsMonochrome")]
        public Windows.Foundation.Boolean ShowAsMonochrome
        {
            get;
            set;
        }

        public BitmapIcon() { }
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [FrameworkTypePattern]
    [InstanceCountTelemetry]
    [Platform("Feature_HeaderPlacement", typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "a7c4ae46-6dae-4d6e-a957-eb1fa53f3049")]
    [ContentProperty("Header")]
    public class TimePicker
     : Microsoft.UI.Xaml.Controls.Control
    {
        #region Properties

        // Version 1
        [PropertyFlags(AffectsMeasure = true)]
        public Windows.Foundation.Object Header { get; set; }

        [PropertyFlags(AffectsMeasure = true)]
        public Microsoft.UI.Xaml.DataTemplate HeaderTemplate { get; set; }

        public Windows.Foundation.String ClockIdentifier { get; set; }

        public Windows.Foundation.Int32 MinuteIncrement { get; set; }

        public Windows.Foundation.TimeSpan Time { get; set; }

        public Microsoft.UI.Xaml.Controls.LightDismissOverlayMode LightDismissOverlayMode { get; set; }

        public Windows.Foundation.TimeSpan? SelectedTime { get; set; }

        // Velocity
        [VelocityFeature("Feature_HeaderPlacement")]
        [NativeStorageType(ValueType.valueEnum)]
        public Microsoft.UI.Xaml.Controls.ControlHeaderPlacement HeaderPlacement { get; set; }

        #endregion

        #region Events

        // Version 1
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public event Microsoft.UI.Xaml.Controls.TimeChangedEventHandler TimeChanged;

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.SelectedTimeChangedEventHandler SelectedTimeChanged;

        #endregion

        public TimePicker() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "37904516-a83d-465c-9751-02278e68b966")]
    public sealed class TimePickerValueChangedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        public Windows.Foundation.TimeSpan OldTime
        {
            get;
            internal set;
        }

        public Windows.Foundation.TimeSpan NewTime
        {
            get;
            internal set;
        }

        internal TimePickerValueChangedEventArgs() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "659e212f-d777-403a-b7bf-ea65744b42d6")]
    public sealed class TimePickerSelectedValueChangedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        public Windows.Foundation.TimeSpan? OldTime
        {
            get;
            internal set;
        }

        public Windows.Foundation.TimeSpan? NewTime
        {
            get;
            internal set;
        }

        internal TimePickerSelectedValueChangedEventArgs() { }
    }

    [CodeGen(partial: true)]
    [InstanceCountTelemetry]
    [ContentProperty("Header")]
    [Platform("Feature_HeaderPlacement", typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "4a16430c-8a1b-435a-9100-17795a709d05")]
    public sealed class ToggleSwitch
     : Microsoft.UI.Xaml.Controls.Control
    {
        #region Properties

        // Version 1
        [PropertyFlags(AffectsMeasure = true)]
        public Windows.Foundation.Boolean IsOn { get; set; }

        [PropertyFlags(AffectsMeasure = true)]
        public Windows.Foundation.Object Header { get; set; }

        [PropertyFlags(AffectsMeasure = true)]
        public Microsoft.UI.Xaml.DataTemplate HeaderTemplate { get; set; }

        public Windows.Foundation.Object OnContent { get; set; }

        [PropertyFlags(AffectsMeasure = true)]
        public Microsoft.UI.Xaml.DataTemplate OnContentTemplate { get; set; }

        [PropertyFlags(AffectsMeasure = true)]
        public Windows.Foundation.Object OffContent { get; set; }

        [PropertyFlags(AffectsMeasure = true)]
        public Microsoft.UI.Xaml.DataTemplate OffContentTemplate { get; set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Controls.Primitives.ToggleSwitchTemplateSettings TemplateSettings { get; private set; }

        // Velocity
        [VelocityFeature("Feature_HeaderPlacement")]
        [NativeStorageType(ValueType.valueEnum)]
        public Microsoft.UI.Xaml.Controls.ControlHeaderPlacement HeaderPlacement { get; set; }

        #endregion

        #region Events

        // Version 1
        public event Microsoft.UI.Xaml.RoutedEventHandler Toggled;

        #endregion

        public ToggleSwitch() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ForceVirtual]
        protected void OnToggled()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ForceVirtual]
        protected void OnOnContentChanged([Optional] Windows.Foundation.Object oldContent, [Optional] Windows.Foundation.Object newContent)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ForceVirtual]
        protected void OnOffContentChanged([Optional] Windows.Foundation.Object oldContent, [Optional] Windows.Foundation.Object newContent)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ForceVirtual]
        protected void OnHeaderChanged([Optional] Windows.Foundation.Object oldContent, [Optional] Windows.Foundation.Object newContent)
        {
        }
    }

    [CodeGen(partial: true)]
    [NativeName("CButton")]
    [VersionPostfix(2, "WithFlyout")]
    [Guids(ClassGuid = "17182bd4-1444-4239-bca2-8b91ee62e565")]
    public class Button
     : Microsoft.UI.Xaml.Controls.Primitives.ButtonBase
    {
        public Button() { }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase Flyout
        {
            get;
            set;
        }
    }

    [CodeGen(partial: true)]
    [InstanceCountTelemetry]
    [Guids(ClassGuid = "3ffc61bb-63e7-4f0c-9153-2650261e4bd8")]
    public class CheckBox
     : Microsoft.UI.Xaml.Controls.Primitives.ToggleButton
    {
        public CheckBox() { }
    }

    [CodeGen(partial: true)]
    [Implements(typeof(Microsoft.UI.Xaml.Media.Animation.ITransitionContextProvider))]
    [Guids(ClassGuid = "161af7e4-07ac-4b1a-bcef-8c7a05a557ef")]
    public class GroupItem
     : Microsoft.UI.Xaml.Controls.ContentControl
    {
        public GroupItem() { }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "26c5d894-49a7-4d33-aad5-cb6437e4937c")]
    public class StyleSelector
    {
        public StyleSelector() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public virtual Microsoft.UI.Xaml.Style SelectStyle([Optional] Windows.Foundation.Object item, Microsoft.UI.Xaml.DependencyObject container)
        {
            return default(Microsoft.UI.Xaml.Style);
        }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "37dcb56e-5a34-499d-974d-d0a00b1c5188")]
    [Implements(typeof(Microsoft.UI.Xaml.IElementFactory), 1)]
    public class DataTemplateSelector
     : Windows.Foundation.Object
    {
        public DataTemplateSelector() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public virtual Microsoft.UI.Xaml.DataTemplate SelectTemplate([Optional] Windows.Foundation.Object item, Microsoft.UI.Xaml.DependencyObject container)
        {
            return default(Microsoft.UI.Xaml.DataTemplate);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlOverloadName("SelectTemplate")]
        public virtual Microsoft.UI.Xaml.DataTemplate SelectTemplateForItem([Optional] Windows.Foundation.Object item)
        {
            return default(Microsoft.UI.Xaml.DataTemplate);
        }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "1b467be6-6bef-4910-b6f8-a44db1e078c9")]
    public class GroupStyleSelector
     : Windows.Foundation.Object
    {
        public GroupStyleSelector() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public virtual Microsoft.UI.Xaml.Controls.GroupStyle SelectGroupStyle([Optional] Windows.Foundation.Object group, Windows.Foundation.UInt32 level)
        {
            return default(Microsoft.UI.Xaml.Controls.GroupStyle);
        }
    }

    [CodeGen(partial: true)]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [FrameworkTypePattern]
    [Implements(typeof(Microsoft.UI.Xaml.Data.INotifyPropertyChanged))]
    [Guids(ClassGuid = "13e262c0-7e62-48f5-85af-294ffc3748b7")]
    public class GroupStyle
     : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Internal)]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Controls.ItemsPanelTemplate Panel
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        [Deprecated("ContainerStyle may be altered or unavailable for releases after Windows 8.1, and is not supported for ItemsControl.GroupStyle.")]
        public Microsoft.UI.Xaml.Style ContainerStyle
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        public Microsoft.UI.Xaml.Style HeaderContainerStyle
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        [Deprecated("ContainerStyleSelector may be altered or unavailable for releases after Windows 8.1, and is not supported for ItemsControl.GroupStyle.")]
        public Microsoft.UI.Xaml.Controls.StyleSelector ContainerStyleSelector
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        public Microsoft.UI.Xaml.DataTemplate HeaderTemplate
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        public Microsoft.UI.Xaml.Controls.DataTemplateSelector HeaderTemplateSelector
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        public Windows.Foundation.Boolean HidesIfEmpty
        {
            get;
            set;
        }

        public GroupStyle() { }
    }

    [CodeGen(partial: true)]
    [Guids(ClassGuid = "187a957b-5090-4a2f-80e2-69cf2b3e7b51")]
    public class ListBoxItem
     : Microsoft.UI.Xaml.Controls.Primitives.SelectorItem
    {
        public ListBoxItem() { }
    }

    [CodeGen(partial: true)]
    [Guids(ClassGuid = "1fc644a0-39bd-4d94-8539-7c46c888a875")]
    public class ComboBoxItem
     : Microsoft.UI.Xaml.Controls.Primitives.SelectorItem
    {
        public ComboBoxItem() { }
    }

    [CodeGen(partial: true)]
    [InstanceCountTelemetry]
    [Guids(ClassGuid = "aa49fc10-7014-44ff-9c11-d86fcac208d6")]
    public class FlipView
     : Microsoft.UI.Xaml.Controls.Primitives.Selector
    {
        public FlipView() { }

        public Windows.Foundation.Boolean UseTouchAnimationsForAllNavigation
        {
            get;
            set;
        }
    }

    [CodeGen(partial: true)]
    [Guids(ClassGuid = "7e6bb3ef-9561-4d87-a17c-b1b4cc6a8be0")]
    public class FlipViewItem
     : Microsoft.UI.Xaml.Controls.Primitives.SelectorItem
    {
        public FlipViewItem() { }
    }

    [DXamlIdlGroup("Controls2")]
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [InstanceCountTelemetry]
    [Guids(ClassGuid = "2b32fc60-69d1-4716-be30-3f2b925bbee4")]
    [Implements(typeof(Microsoft.Internal.FrameworkUdk.IBackButtonPressedListener))]
    [TemplatePart("Container", typeof(Microsoft.UI.Xaml.Controls.Border))]
    [TemplatePart("LayoutRoot", typeof(Microsoft.UI.Xaml.Controls.Grid))]
    [TemplatePart("BackgroundElement", typeof(Microsoft.UI.Xaml.Controls.Border))]
    [TemplatePart("Title", typeof(Microsoft.UI.Xaml.Controls.ContentControl))]
    [TemplatePart("Content", typeof(Microsoft.UI.Xaml.Controls.ContentPresenter))]
    [TemplatePart("ContentPanel", typeof(Microsoft.UI.Xaml.Controls.Grid))]
    [TemplatePart("Button1Host", typeof(Microsoft.UI.Xaml.Controls.Border))]
    [TemplatePart("Button2Host", typeof(Microsoft.UI.Xaml.Controls.Border))]
    [TemplatePart("CommandSpace", typeof(Microsoft.UI.Xaml.Controls.Grid))]
    [TemplatePart("DialogSpace", typeof(Microsoft.UI.Xaml.Controls.Grid))]
    [TemplatePart("ContentScrollViewer", typeof(Microsoft.UI.Xaml.Controls.ScrollViewer))]
    [TemplatePart("PrimaryButton", typeof(Microsoft.UI.Xaml.Controls.Primitives.ButtonBase))]
    [TemplatePart("SecondaryButton", typeof(Microsoft.UI.Xaml.Controls.Primitives.ButtonBase))]
    [TemplatePart("CloseButton", typeof(Microsoft.UI.Xaml.Controls.Primitives.ButtonBase))]
    [TemplatePart("ScaleTransform", typeof(Microsoft.UI.Xaml.Media.ScaleTransform))]
    public class ContentDialog : Microsoft.UI.Xaml.Controls.ContentControl
    {
        public Windows.Foundation.Object Title { get; set; }

        public Microsoft.UI.Xaml.DataTemplate TitleTemplate { get; set; }

        public Windows.Foundation.Boolean FullSizeDesired { get; set; }

        public Windows.Foundation.String PrimaryButtonText { get; set; }

        public Windows.Foundation.String SecondaryButtonText { get; set; }

        public Windows.Foundation.String CloseButtonText { get; set; }

        public Microsoft.UI.Xaml.Input.ICommand PrimaryButtonCommand { get; set; }

        public Microsoft.UI.Xaml.Input.ICommand SecondaryButtonCommand { get; set; }

        public Microsoft.UI.Xaml.Input.ICommand CloseButtonCommand { get; set; }

        public Windows.Foundation.Object PrimaryButtonCommandParameter { get; set; }

        public Windows.Foundation.Object SecondaryButtonCommandParameter { get; set; }

        public Windows.Foundation.Object CloseButtonCommandParameter { get; set; }

        public Windows.Foundation.Boolean IsPrimaryButtonEnabled { get; set; }

        public Windows.Foundation.Boolean IsSecondaryButtonEnabled { get; set; }

        public Microsoft.UI.Xaml.Style PrimaryButtonStyle { get; set; }

        public Microsoft.UI.Xaml.Style SecondaryButtonStyle { get; set; }

        public Microsoft.UI.Xaml.Style CloseButtonStyle { get; set; }

        public Microsoft.UI.Xaml.Controls.ContentDialogButton DefaultButton { get; set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void Hide() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("ShowAsync")]
        [DXamlOverloadName("ShowAsync")]
        public Windows.Foundation.IAsyncOperation<ContentDialogResult> ShowAsync()
        {
            return default(Windows.Foundation.IAsyncOperation<ContentDialogResult>);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("ShowAsyncWithPlacement")]
        [DXamlOverloadName("ShowAsync")]
        public Windows.Foundation.IAsyncOperation<ContentDialogResult> ShowAsync(ContentDialogPlacement placement)
        {
            return default(Windows.Foundation.IAsyncOperation<ContentDialogResult>);
        }

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.ContentDialogClosingEventHandler Closing;

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.ContentDialogClosedEventHandler Closed;

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.ContentDialogOpenedEventHandler Opened;

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.ContentDialogButtonClickEventHandler PrimaryButtonClick;

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.ContentDialogButtonClickEventHandler SecondaryButtonClick;

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.ContentDialogButtonClickEventHandler CloseButtonClick;
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "65d9d049-7881-4686-8915-bfe383d5b591")]
    public sealed class ContentDialogClosingDeferral
    {
        internal ContentDialogClosingDeferral() { }

        public void Complete()
        { }
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "82669f0c-6c28-4bb4-98b5-34b4f9e15bde")]
    public sealed class ContentDialogButtonClickDeferral
    {
        internal ContentDialogButtonClickDeferral() { }

        public void Complete()
        { }
    }

    [DXamlIdlGroup("Controls2")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum ContentDialogResult
    {
        None = 0,
        Primary = 1,
        Secondary = 2,
    }

    [DXamlIdlGroup("Controls2")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum ContentDialogButton
    {
        None = 0,
        Primary = 1,
        Secondary = 2,
        Close = 3,
    }

    [Platform("Feature_ExperimentalApi", typeof(Microsoft.UI.Xaml.WinUIContract), Microsoft.UI.Xaml.WinUIContract.LatestVersion)]
    [DXamlIdlGroup("Controls2")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum ContentDialogPlacement
    {
        Popup = 0,
        InPlace = 1,
        [VelocityFeature("Feature_ExperimentalApi")]
        UnconstrainedPopup = 2,
    }

    [FrameworkTypePattern]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "ead885ca-f241-4af1-9427-b95685ad888b")]
    public sealed class ContentDialogOpenedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        internal ContentDialogOpenedEventArgs() { }
    }

    [FrameworkTypePattern]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "5d5f32fe-7223-4771-9e70-07c942f9f57c")]
    public sealed class ContentDialogClosedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        internal ContentDialogClosedEventArgs() { }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public ContentDialogResult Result
        {
            get;
            internal set;
        }
    }

    [FrameworkTypePattern]
    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "d724a064-7359-4188-9bdd-65c2bf14854a")]
    public sealed class ContentDialogClosingEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        internal ContentDialogClosingEventArgs() { }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public ContentDialogResult Result
        {
            get;
            internal set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean Cancel
        {
            get;
            set;
        }

        public ContentDialogClosingDeferral GetDeferral()
        {
            return null;
        }
    }

    [FrameworkTypePattern]
    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "2285d599-0188-4d91-8a57-a1fdd488219c")]
    public sealed class ContentDialogButtonClickEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        internal ContentDialogButtonClickEventArgs() { }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean Cancel
        {
            get;
            set;
        }

        public ContentDialogButtonClickDeferral GetDeferral()
        {
            return null;
        }
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [NativeName("CFlyout")]
    [ContentProperty("Content")]
    [InstanceCountTelemetry]
    [Guids(ClassGuid = "5c847e10-cf37-4efa-a9c9-51970b5d5941")]
    public class Flyout
     : Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase
    {
        public Flyout() { }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.UIElement Content
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Style FlyoutPresenterStyle
        {
            get;
            set;
        }
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "ba7a1dd8-cc37-4140-8b20-56e4f5c8d2a1")]
    public class FlyoutPresenter
     : Microsoft.UI.Xaml.Controls.ContentControl
    {
        public FlyoutPresenter() { }

        public Windows.Foundation.Boolean IsDefaultShadowEnabled { get; set; }
    }

    [CodeGen(partial: true)]
    [InstanceCountTelemetry]
    [Guids(ClassGuid = "773ee7eb-b2f9-4e42-bcac-42d7ec8b92d2")]
    public class HyperlinkButton
     : Microsoft.UI.Xaml.Controls.Primitives.ButtonBase
    {
        public Windows.Foundation.Uri NavigateUri
        {
            get;
            set;
        }

        public HyperlinkButton() { }
    }

    [CodeGen(partial: true)]
    [Guids(ClassGuid = "d4d4df83-bdb1-4bbe-82f4-09c2fdd15754")]
    public class ListBox
     : Microsoft.UI.Xaml.Controls.Primitives.Selector
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DependencyPropertyModifier(Modifier.Private)]
        [CollectionType(CollectionKind.Vector)]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Controls.ItemCollection SelectedItems
        {
            get;
            private set;
        }

        public Microsoft.UI.Xaml.Controls.SelectionMode SelectionMode
        {
            get;
            set;
        }

        public ListBox() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void ScrollIntoView([Optional] Windows.Foundation.Object item)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SelectAll()
        {
        }

        [FieldBacked]
        public Windows.Foundation.Boolean SingleSelectionFollowsFocus
        {
            get;
            set;
        }

    }

    [CodeGen(partial: true)]
    [InstanceCountTelemetry]
    [Guids(ClassGuid = "55b2e7ce-5da4-49af-86e7-d8bd98ef0748")]
    public class RadioButton
     : Microsoft.UI.Xaml.Controls.Primitives.ToggleButton
    {
        public Windows.Foundation.String GroupName
        {
            get;
            set;
        }

        public RadioButton() { }
    }

    [DXamlIdlGroup("Controls2")]
    [CodeGen(partial: true)]
    [Comment("Also inherits from IScrollInfo - this is done manually in the partial class.")]
    [FrameworkTypePattern]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.Primitives.IScrollInfo))]
    [Guids(ClassGuid = "0d51bece-96e7-4469-bdfe-6fbe071c3d7c")]
    public sealed class ScrollContentPresenter
     : Microsoft.UI.Xaml.Controls.ContentPresenter
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean CanVerticallyScroll
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean CanHorizontallyScroll
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Double ExtentWidth
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Double ExtentHeight
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Double ViewportWidth
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Double ViewportHeight
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Double HorizontalOffset
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Double VerticalOffset
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Object ScrollOwner
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        public Windows.Foundation.Boolean CanContentRenderOutsideBounds
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyFlags(AffectsMeasure = true)]
        public Windows.Foundation.Boolean SizesContentToTemplatedParent
        {
            get;
            set;
        }

        public ScrollContentPresenter() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void LineUp()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void LineDown()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void LineLeft()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void LineRight()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void PageUp()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void PageDown()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void PageLeft()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void PageRight()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void MouseWheelUp()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void MouseWheelDown()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void MouseWheelLeft()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void MouseWheelRight()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetHorizontalOffset(Windows.Foundation.Double offset)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetVerticalOffset(Windows.Foundation.Double offset)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Rect MakeVisible(Microsoft.UI.Xaml.UIElement visual, Windows.Foundation.Rect rectangle)
        {
            return default(Windows.Foundation.Rect);
        }
    }

    [DXamlIdlGroup("Controls2")]
    [Comment("Encapsulates three read-only properties representing the horizontal & vertical offsets and zoom factor of a ScrollViewer.")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "2c093ebc-8a2c-4454-bc76-2cae494bc0ac")]
    public sealed class ScrollViewerView
    {
        [Comment("Gets the HorizontalOffset property.")]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Double HorizontalOffset
        {
            get;
            internal set;
        }

        [Comment("Gets the VerticalOffset property.")]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Double VerticalOffset
        {
            get;
            internal set;
        }

        [Comment("Gets the ZoomFactor property.")]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Float ZoomFactor
        {
            get;
            internal set;
        }

        internal ScrollViewerView() { }
    }

    [TypeTable(ForceInclude = true)]
    public interface IScrollAnchorProvider
    {
        void RegisterAnchorCandidate(Microsoft.UI.Xaml.UIElement element);
        void UnregisterAnchorCandidate(Microsoft.UI.Xaml.UIElement element);

        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        Microsoft.UI.Xaml.UIElement CurrentAnchor { get; }
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface IScrollViewerPrivate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [Comment("Prevents overpan effect so that panning will hard-stop at the boundaries of the scrollable region.")]
        void DisableOverpan();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [Comment("Reenables overpan.")]
        void EnableOverpan();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void SetIsNearVerticalAlignmentForced(Windows.Foundation.Boolean enabled);

        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        [Comment("Returns whether a direct manipulation is controlling the offset.")]
        Windows.Foundation.Boolean IsInDirectManipulation
        {
            get;
            set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [Comment("Ignore pointer wheel events and let them bubble up the visual tree. Default value is false.")]
        Windows.Foundation.Boolean ArePointerWheelEventsIgnored
        {
            get;
            set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [Comment("Causes us to ignore bring-into-view requests. Default value is false.")]
        Windows.Foundation.Boolean IsRequestBringIntoViewIgnored
        {
            get;
            set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        [Comment("Returns whether a direct manipulation has started.")]
        Windows.Foundation.Boolean IsInActiveDirectManipulation
        {
            get;
            set;
        }
    }

    [CodeGen(partial: true)]
    [ForceSealed]
    [DXamlIdlGroup("Controls2")]
    [InstanceCountTelemetry]
    [Guids(ClassGuid = "71d529d6-3a4e-4a1f-8b40-6f1e6cdce52c")]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IScrollViewerPrivate))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IScrollAnchorProvider))]
    [NativeName("CScrollViewer")]
    public class ScrollViewer
     : Microsoft.UI.Xaml.Controls.ScrollContentControl
    {
        [Attached(SetterParameterName = "horizontalScrollBarVisibility", TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        public static Microsoft.UI.Xaml.Controls.ScrollBarVisibility AttachedHorizontalScrollBarVisibility
        {
            get;
            set;
        }

        [Attached(SetterParameterName = "verticalScrollBarVisibility", TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        public static Microsoft.UI.Xaml.Controls.ScrollBarVisibility AttachedVerticalScrollBarVisibility
        {
            get;
            set;
        }

        [Attached(SetterParameterName = "isHorizontalRailEnabled", TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        public static Windows.Foundation.Boolean AttachedIsHorizontalRailEnabled
        {
            get;
            set;
        }

        [Attached(SetterParameterName = "isVerticalRailEnabled", TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        public static Windows.Foundation.Boolean AttachedIsVerticalRailEnabled
        {
            get;
            set;
        }

        [Attached(SetterParameterName = "isHorizontalScrollChainingEnabled", TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        public static Windows.Foundation.Boolean AttachedIsHorizontalScrollChainingEnabled
        {
            get;
            set;
        }

        [Attached(SetterParameterName = "isVerticalScrollChainingEnabled", TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        public static Windows.Foundation.Boolean AttachedIsVerticalScrollChainingEnabled
        {
            get;
            set;
        }

        [Attached(SetterParameterName = "isZoomChainingEnabled", TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        public static Windows.Foundation.Boolean AttachedIsZoomChainingEnabled
        {
            get;
            set;
        }

        [Attached(SetterParameterName = "isScrollInertiaEnabled", TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        public static Windows.Foundation.Boolean AttachedIsScrollInertiaEnabled
        {
            get;
            set;
        }

        [Attached(SetterParameterName = "isZoomInertiaEnabled", TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        public static Windows.Foundation.Boolean AttachedIsZoomInertiaEnabled
        {
            get;
            set;
        }

        [Attached(SetterParameterName = "horizontalScrollMode", TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        public static Microsoft.UI.Xaml.Controls.ScrollMode AttachedHorizontalScrollMode
        {
            get;
            set;
        }

        [Attached(SetterParameterName = "verticalScrollMode", TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        public static Microsoft.UI.Xaml.Controls.ScrollMode AttachedVerticalScrollMode
        {
            get;
            set;
        }

        [Attached(SetterParameterName = "zoomMode", TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        public static Microsoft.UI.Xaml.Controls.ZoomMode AttachedZoomMode
        {
            get;
            set;
        }

        [OrderHint(1)]
        [Attached(SetterParameterName = "isDeferredScrollingEnabled", TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        public static Windows.Foundation.Boolean AttachedIsDeferredScrollingEnabled
        {
            get;
            set;
        }

        [OrderHint(2)]
        [Attached(SetterParameterName = "bringIntoViewOnFocusChange", TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        public static Windows.Foundation.Boolean AttachedBringIntoViewOnFocusChange
        {
            get;
            set;
        }

        [Attached(SetterParameterName = "canContentRenderOutsideBounds", TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        public static Windows.Foundation.Boolean AttachedCanContentRenderOutsideBounds
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Controls.ScrollBarVisibility HorizontalScrollBarVisibility
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Controls.ScrollBarVisibility VerticalScrollBarVisibility
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean IsHorizontalRailEnabled
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean IsVerticalRailEnabled
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean IsHorizontalScrollChainingEnabled
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean IsVerticalScrollChainingEnabled
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean IsZoomChainingEnabled
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean IsScrollInertiaEnabled
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean IsZoomInertiaEnabled
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Controls.ScrollMode HorizontalScrollMode
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Controls.ScrollMode VerticalScrollMode
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Controls.ZoomMode ZoomMode
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [OrderHint(1)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean IsDeferredScrollingEnabled
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [OrderHint(2)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean BringIntoViewOnFocusChange
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Controls.Primitives.SnapPointsAlignment HorizontalSnapPointsAlignment
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Controls.Primitives.SnapPointsAlignment VerticalSnapPointsAlignment
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Controls.SnapPointsType HorizontalSnapPointsType
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Controls.SnapPointsType VerticalSnapPointsType
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Controls.SnapPointsType ZoomSnapPointsType
        {
            get;
            set;
        }

        [PropertyFlags(IsSetterImplVirtual = true)]
        public Windows.Foundation.Double HorizontalOffset
        {
            get;
            internal set;
        }

        [FieldBacked]
        [PropertyFlags(IsSetterImplVirtual = true)]
        public Windows.Foundation.Double ViewportWidth
        {
            get;
            internal set;
        }

        [FieldBacked]
        public Windows.Foundation.Double ScrollableWidth
        {
            get;
            internal set;
        }

        public Microsoft.UI.Xaml.Visibility ComputedHorizontalScrollBarVisibility
        {
            get;
            internal set;
        }

        [FieldBacked]
        [PropertyFlags(IsSetterImplVirtual = true)]
        public Windows.Foundation.Double ExtentWidth
        {
            get;
            internal set;
        }

        [PropertyFlags(IsSetterImplVirtual = true)]
        public Windows.Foundation.Double VerticalOffset
        {
            get;
            internal set;
        }

        [FieldBacked]
        [PropertyFlags(IsSetterImplVirtual = true)]
        public Windows.Foundation.Double ViewportHeight
        {
            get;
            internal set;
        }

        [FieldBacked]
        public Windows.Foundation.Double ScrollableHeight
        {
            get;
            internal set;
        }

        public Microsoft.UI.Xaml.Visibility ComputedVerticalScrollBarVisibility
        {
            get;
            internal set;
        }

        [FieldBacked]
        [PropertyFlags(IsSetterImplVirtual = true)]
        public Windows.Foundation.Double ExtentHeight
        {
            get;
            internal set;
        }

        [CoreType(typeof(Windows.Foundation.Double))]
        public Windows.Foundation.Float MinZoomFactor
        {
            get;
            set;
        }

        [CoreType(typeof(Windows.Foundation.Double))]
        public Windows.Foundation.Float MaxZoomFactor
        {
            get;
            set;
        }

        [CoreType(typeof(Windows.Foundation.Double))]
        [PropertyFlags(IsSetterImplVirtual = true)]
        public Windows.Foundation.Float ZoomFactor
        {
            get;
            internal set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [CollectionType(CollectionKind.Vector, NewCodeGenCollectionType = typeof(Windows.Foundation.Collections.IVector<Windows.Foundation.Float>))]
        [ReadOnly]
        public Microsoft.UI.Xaml.Media.FloatCollection ZoomSnapPoints
        {
            get;
            private set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.UIElement TopLeftHeader
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.UIElement LeftHeader
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.UIElement TopHeader
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueBool)]
        public Windows.Foundation.Boolean ReduceViewportForCoreInputViewOcclusions
        {
            get;
            set;
        }

        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        public Windows.Foundation.Double HorizontalAnchorRatio { get; set; }

        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        public Windows.Foundation.Double VerticalAnchorRatio { get; set; }

        public event Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.Controls.ScrollViewer, Microsoft.UI.Xaml.Controls.AnchorRequestedEventArgs> AnchorRequested;

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean CanContentRenderOutsideBounds
        {
            get;
            set;
        }

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public event Microsoft.UI.Xaml.Controls.ScrollViewerViewChangingEventHandler ViewChanging;

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public event Microsoft.UI.Xaml.Controls.ScrollViewerViewChangedEventHandler ViewChanged;

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public event Microsoft.UI.Xaml.EventHandler DirectManipulationStarted;

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public event Microsoft.UI.Xaml.EventHandler DirectManipulationCompleted;

        public ScrollViewer() { }

        [Deprecated("ScrollToHorizontalOffset may be altered or unavailable for releases after Windows 8.1. Instead, use ChangeView.")]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void ScrollToHorizontalOffset(Windows.Foundation.Double offset)
        {
        }

        [Deprecated("ScrollToVerticalOffset may be altered or unavailable for releases after Windows 8.1. Instead, use ChangeView.")]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void ScrollToVerticalOffset(Windows.Foundation.Double offset)
        {
        }

        [Deprecated("ZoomToFactor may be altered or unavailable for releases after Windows 8.1. Instead, use ChangeView.")]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void ZoomToFactor([CoreType(typeof(Windows.Foundation.Double))] Windows.Foundation.Float factor)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("ChangeView")]
        [DXamlOverloadName("ChangeView")]
        public Windows.Foundation.Boolean ChangeView([Optional] Windows.Foundation.Double? horizontalOffset, [Optional] Windows.Foundation.Double? verticalOffset, [Optional] Windows.Foundation.Float? zoomFactor)
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("ChangeViewWithOptionalAnimation")]
        [DXamlOverloadName("ChangeView")]
        public Windows.Foundation.Boolean ChangeView([Optional] Windows.Foundation.Double? horizontalOffset, [Optional] Windows.Foundation.Double? verticalOffset, [Optional] Windows.Foundation.Float? zoomFactor, Windows.Foundation.Boolean disableAnimation)
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void InvalidateScrollInfo()
        {
        }
    }

    [CodeGen(partial: true)]
    [ClassFlags(ForceCoreFieldInitializer = true)]
    [InstanceCountTelemetry]
    [Platform("Feature_HeaderPlacement", typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "22d330ef-bfdf-41d7-ae0b-72d42784adf3")]
    public class Slider
     : Microsoft.UI.Xaml.Controls.Primitives.RangeBase
    {
        #region Properties

        public Windows.Foundation.Double IntermediateValue { get; set; }

        public Windows.Foundation.Double StepFrequency { get; set; }

        public Microsoft.UI.Xaml.Controls.Primitives.SliderSnapsTo SnapsTo { get; set; }

        public Windows.Foundation.Double TickFrequency { get; set; }

        public Microsoft.UI.Xaml.Controls.Primitives.TickPlacement TickPlacement { get; set; }

        public Microsoft.UI.Xaml.Controls.Orientation Orientation { get; set; }

        public Windows.Foundation.Boolean IsDirectionReversed { get; set; }

        public Windows.Foundation.Boolean IsThumbToolTipEnabled { get; set; }

        public Microsoft.UI.Xaml.Data.IValueConverter ThumbToolTipValueConverter { get; set; }

        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Object Header { get; set; }

        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.DataTemplate HeaderTemplate { get; set; }

        // Velocity
        [VelocityFeature("Feature_HeaderPlacement")]
        [NativeStorageType(ValueType.valueEnum)]
        public Microsoft.UI.Xaml.Controls.ControlHeaderPlacement HeaderPlacement { get; set; }

        #endregion

        public Slider() { }

        protected override void OnMinimumChanged(Windows.Foundation.Double oldMinimum, Windows.Foundation.Double newMinimum)
        {
        }

        protected override void OnMaximumChanged(Windows.Foundation.Double oldMaximum, Windows.Foundation.Double newMaximum)
        {
        }

        protected override void OnValueChanged(Windows.Foundation.Double oldValue, Windows.Foundation.Double newValue)
        {
        }
    }

    [CodeGen(partial: true)]
    [Platform("Feature_WUXCPreviewTypes", typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [NativeName("CPanel")]
    [ContentProperty("Children")]
    [Guids(ClassGuid = "357584cc-0daf-4784-990a-00d725888abb")]
    public abstract class Panel
     : Microsoft.UI.Xaml.FrameworkElement
    {
        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CUIElement", "GetChildren")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetSubgraphDirty")]
        public Microsoft.UI.Xaml.Controls.UIElementCollection Children
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pBackground")]
        [RenderDirtyFlagClassName("CPanel")]
        [RenderDirtyFlagMethodName("NWSetBackgroundDirty")]
        public Microsoft.UI.Xaml.Media.Brush Background
        {
            get;
            set;
        }

        [ReadOnly]
        public Windows.Foundation.Boolean IsItemsHost
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Boolean IsIgnoringTransitions
        {
            get;
            set;
        }

        [PropertyFlags(HadFieldInBlue = true, IsValueCreatedOnDemand = true)]
        public Microsoft.UI.Xaml.Media.Animation.TransitionCollection ChildrenTransitions
        {
            get;
            set;
        }

        protected Panel() { }

        [VelocityFeature("Feature_WUXCPreviewTypes")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CPanel")]
        [RenderDirtyFlagMethodName("NWSetBackgroundDirty")]
        [DependencyPropertyModifier(Modifier.Private)]
        protected Microsoft.UI.Xaml.Media.Brush BorderBrushProtected
        {
            get;
            set;
        }

        [VelocityFeature("Feature_WUXCPreviewTypes")]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueThickness)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        [DependencyPropertyModifier(Modifier.Private)]
        protected Microsoft.UI.Xaml.Thickness BorderThicknessProtected
        {
            get;
            set;
        }

        [VelocityFeature("Feature_WUXCPreviewTypes")]
        [NativeStorageType(ValueType.valueCornerRadius)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [DependencyPropertyModifier(Modifier.Private)]
        protected Microsoft.UI.Xaml.CornerRadius CornerRadiusProtected
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.BrushTransition BackgroundTransition
        {
            get;
            set;
        }
    }

    [Guids(ClassGuid = "c4d89a08-d1fc-4c96-9e52-8033e4803cf2")]
    [CodeGen(partial: true)]
    [NativeName("CImage")]
    public sealed class Image
     : Microsoft.UI.Xaml.FrameworkElement
    {
        [PropertyFlags(AffectsMeasure = true, NeedsInvoke = true)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pImageSource")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Media.ImageSource Source
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_Stretch")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Media.Stretch Stretch
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueThickness)]
        [OffsetFieldName("m_ninegridPrivate")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Thickness NineGrid
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.ExceptionRoutedEventHandler ImageFailed;

        [NativeStorageType(ValueType.valueObject)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.RoutedEventHandler ImageOpened;

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Media.Casting.WindowsMediaCastingSource GetAsCastingSource()
        {
            return default(Windows.Media.Casting.WindowsMediaCastingSource);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Microsoft.UI.Composition.CompositionBrush GetAlphaMask()
        {
            return default(Microsoft.UI.Composition.CompositionBrush);
        }
        public Image() { }
    }

    [DXamlIdlGroup("Main")]
    [NativeName("BackgroundSizing")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    public enum BackgroundSizing
    {
        [NativeValueName("BackgroundSizingInnerBorderEdge")]
        InnerBorderEdge = 0,
        [NativeValueName("BackgroundSizingOuterBorderEdge")]
        OuterBorderEdge = 1,
    }

    [DXamlIdlGroup("Main")]
    [VelocityFeature("Feature_HeaderPlacement")]
    public enum ControlHeaderPlacement
    {
        Top = 0,
        Left = 1,
    }

    [NativeName("CRelativePanel")]
    [ClassFlags(AreMeasureAndArrangeSealed = true)]
    [Guids(ClassGuid = "c5501c0d-6852-46f3-a28e-57c4aa63f503")]
    public class RelativePanel
        : Microsoft.UI.Xaml.Controls.Panel
    {
        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueObject)]
        public static Windows.Foundation.Object AttachedLeftOf
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueObject)]
        public static Windows.Foundation.Object AttachedAbove
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueObject)]
        public static Windows.Foundation.Object AttachedRightOf
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueObject)]
        public static Windows.Foundation.Object AttachedBelow
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueObject)]
        public static Windows.Foundation.Object AttachedAlignHorizontalCenterWith
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueObject)]
        public static Windows.Foundation.Object AttachedAlignVerticalCenterWith
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueObject)]
        public static Windows.Foundation.Object AttachedAlignLeftWith
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueObject)]
        public static Windows.Foundation.Object AttachedAlignTopWith
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueObject)]
        public static Windows.Foundation.Object AttachedAlignRightWith
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueObject)]
        public static Windows.Foundation.Object AttachedAlignBottomWith
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueBool)]
        public static Windows.Foundation.Boolean AttachedAlignLeftWithPanel
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueBool)]
        public static Windows.Foundation.Boolean AttachedAlignTopWithPanel
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueBool)]
        public static Windows.Foundation.Boolean AttachedAlignRightWithPanel
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueBool)]
        public static Windows.Foundation.Boolean AttachedAlignBottomWithPanel
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueBool)]
        public static Windows.Foundation.Boolean AttachedAlignHorizontalCenterWithPanel
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueBool)]
        public static Windows.Foundation.Boolean AttachedAlignVerticalCenterWithPanel
        {
            get;
            set;
        }

        public RelativePanel() { }

        [NativeStorageType(ValueType.valueEnum)]
        [RenderDirtyFlagClassName("CPanel")]
        [RenderDirtyFlagMethodName("NWSetBackgroundDirty")]
        public Microsoft.UI.Xaml.Controls.BackgroundSizing BackgroundSizing
        {
            get;
            set;
        }

        [OrderHint(1)]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CPanel")]
        [RenderDirtyFlagMethodName("NWSetBorderBrushDirty")]
        public Microsoft.UI.Xaml.Media.Brush BorderBrush
        {
            get;
            set;
        }

        [OrderHint(2)]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueThickness)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Thickness BorderThickness
        {
            get;
            set;
        }

        [OrderHint(3)]

        [NativeStorageType(ValueType.valueCornerRadius)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.CornerRadius CornerRadius
        {
            get;
            set;
        }

        [OrderHint(4)]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueThickness)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Thickness Padding
        {
            get;
            set;
        }

    }

    [NativeName("CCanvas")]
    [ClassFlags(AreMeasureAndArrangeSealed = true)]
    [Guids(ClassGuid = "9478cc51-d00b-47fe-8cca-f845cc070b7a")]
    public class Canvas
     : Microsoft.UI.Xaml.Controls.Panel
    {
        [Attached(SetterParameterName = "length", TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(IsIndependentlyAnimatable = true)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetTransformDirty")]
        public static Windows.Foundation.Double AttachedLeft
        {
            get;
            set;
        }

        [Attached(SetterParameterName = "length", TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(IsIndependentlyAnimatable = true)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetTransformDirty")]
        public static Windows.Foundation.Double AttachedTop
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]

        [NativeMethod("CUIElement", "ZIndex")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetTransformDirty")]
        public static Windows.Foundation.Int32 AttachedZIndex
        {
            get;
            set;
        }

        public Canvas() { }
    }

    [CodeGen(partial: true)]
    [Implements(typeof(Microsoft.Internal.FrameworkUdk.IBackButtonPressedListener))]
    [Guids(ClassGuid = "34b383f2-569a-4b9c-840b-d8869a8ccdbc")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 3)]
    public class MediaTransportControls
     : Microsoft.UI.Xaml.Controls.Control
    {
        // TODO 39683362: RI Full Screen work to WinAppSDK 1.x master
        // public Windows.Foundation.Boolean IsFullWindowButtonVisible { get; set; }

        // public Windows.Foundation.Boolean IsFullWindowEnabled { get; set; }

        public Windows.Foundation.Boolean IsZoomButtonVisible { get; set; }

        public Windows.Foundation.Boolean IsZoomEnabled { get; set; }

        public Windows.Foundation.Boolean IsFastForwardButtonVisible { get; set; }

        public Windows.Foundation.Boolean IsFastForwardEnabled { get; set; }

        public Windows.Foundation.Boolean IsFastRewindButtonVisible { get; set; }

        public Windows.Foundation.Boolean IsFastRewindEnabled { get; set; }

        public Windows.Foundation.Boolean IsStopButtonVisible { get; set; }

        public Windows.Foundation.Boolean IsStopEnabled { get; set; }

        public Windows.Foundation.Boolean IsVolumeButtonVisible { get; set; }

        public Windows.Foundation.Boolean IsVolumeEnabled { get; set; }

        public Windows.Foundation.Boolean IsPlaybackRateButtonVisible { get; set; }

        public Windows.Foundation.Boolean IsPlaybackRateEnabled { get; set; }

        public Windows.Foundation.Boolean IsSeekBarVisible { get; set; }

        public Windows.Foundation.Boolean IsSeekEnabled { get; set; }

        public Windows.Foundation.Boolean IsCompact { get; set; }

        public MediaTransportControls() { }

        public Windows.Foundation.Boolean IsSkipForwardButtonVisible { get; set; }

        public Windows.Foundation.Boolean IsSkipForwardEnabled { get; set; }

        public Windows.Foundation.Boolean IsSkipBackwardButtonVisible { get; set; }

        public Windows.Foundation.Boolean IsSkipBackwardEnabled { get; set; }

        public Windows.Foundation.Boolean IsNextTrackButtonVisible { get; set; }

        public Windows.Foundation.Boolean IsPreviousTrackButtonVisible { get; set; }

        public Microsoft.UI.Xaml.Media.FastPlayFallbackBehaviour FastPlayFallbackBehaviour { get; set; }

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [EventFlags(IsImplVirtual = true)]
        public event Microsoft.UI.Xaml.Controls.ThumbnailRequestedEventHandler ThumbnailRequested;

        public void Show() { }

        public void Hide() { }

        public Windows.Foundation.Boolean ShowAndHideAutomatically { get; set; }

        public Windows.Foundation.Boolean IsRepeatEnabled { get; set; }

        public Windows.Foundation.Boolean IsRepeatButtonVisible { get; set; }
        // TODO 39683362: RI Full Screen work to WinAppSDK 1.x master
        // public Windows.Foundation.Boolean IsCompactOverlayButtonVisible { get; set; }

        // public Windows.Foundation.Boolean IsCompactOverlayEnabled { get; set; }
    }

    [StubDelegate]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 3)]
    public delegate void ThumbnailRequestedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Media.MediaTransportControlsThumbnailRequestedEventArgs e);

    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 3, ForcePrimaryInterfaceGeneration = true )]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Guids(ClassGuid = "2f79d65e-45c9-42b8-8e87-f58b85ce2f53")]
    public static class MediaTransportControlsHelper
    {
        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        public static Windows.Foundation.Int32? AttachedDropoutOrder
        {
            get;
            set;
        }
    }

    // There was no IUIElementCollection in Windows 8, so use that name instead.
    [VersionPostfix(2, "")]
    [DXamlIdlGroup("Main")]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CUIElementCollection")]
    [Platform(2, typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "afdb2a69-86a2-4fac-8be7-2daa120c090d")]
    public sealed class UIElementCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<UIElement>
    {
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public Microsoft.UI.Xaml.UIElement ContentProperty
        {
            get;
            set;
        }

        internal UIElementCollection() { }

        [PInvoke]
        [Version(2)]
        public void Move([CoreType(typeof(Windows.Foundation.Int32))] Windows.Foundation.UInt32 oldIndex, [CoreType(typeof(Windows.Foundation.Int32))] Windows.Foundation.UInt32 newIndex)
        {
        }
    }

    [Comment("Determines whether a control requires mouse pointer input.")]
    [FrameworkTypePattern]
    [DXamlIdlGroup("Main")]
    [NativeName("RequiresPointer")]
    public enum RequiresPointer
    {
        Never = 0,
        WhenEngaged = 1,
        WhenFocused = 2,
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Main")]
    [ControlPattern]
    [NativeName("CControl")]
    [Guids(ClassGuid = "7ef20697-b515-4ddc-903d-5605d6dfae81")]
    public abstract class Control
     : Microsoft.UI.Xaml.FrameworkElement
    {

        [NativeStorageType(ValueType.valueBool)]
        public Windows.Foundation.Boolean IsFocusEngagementEnabled
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueBool)]
        public Windows.Foundation.Boolean IsFocusEngaged
        {
            get;
            set;
        }

        public void RemoveFocusEngagement() { }

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.FocusEngagedEventHandler FocusEngaged;

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.FocusDisengagedEventHandler FocusDisengaged;


        public Microsoft.UI.Xaml.Controls.RequiresPointer RequiresPointer
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.FrameworkElement))]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public static Windows.Foundation.Boolean IsTemplateFocusTarget
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        internal Microsoft.UI.Xaml.UIElement FocusTargetDescendant
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_eFontSize")]
        public Windows.Foundation.Double FontSize
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_pFontFamily")]
        public Microsoft.UI.Xaml.Media.FontFamily FontFamily
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nFontWeight")]
        [CoreType(typeof(Windows.UI.Text.CoreFontWeight), NewCodeGenPropertyType = typeof(Windows.UI.Text.FontWeight))]
        public Windows.UI.Text.FontWeight FontWeight
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nFontStyle")]
        public Windows.UI.Text.FontStyle FontStyle
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nFontStretch")]
        public Windows.UI.Text.FontStretch FontStretch
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nCharacterSpacing")]
        public Windows.Foundation.Int32 CharacterSpacing
        {
            get;
            set;
        }

        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTextFormatting")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_pForeground")]
        public Microsoft.UI.Xaml.Media.Brush Foreground
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_isTextScaleFactorEnabled")]
        public Windows.Foundation.Boolean IsTextScaleFactorEnabled
        {
            get;
            set;
        }

        [NativeMethod("CControl", "Enabled")]
        [NativeStorageType(ValueType.valueBool)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Windows.Foundation.Boolean IsEnabled
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [NativeMethod("CUIElement", "TabFocusNavigation")]
        public Microsoft.UI.Xaml.Input.KeyboardNavigationMode TabNavigation
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTemplate")]
        public Microsoft.UI.Xaml.Controls.ControlTemplate Template
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueThickness)]
        [OffsetFieldName("m_padding")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Thickness Padding
        {
            get;
            set;
        }

        [PropertyFlags(AffectsArrange = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_horizontalContentAlignment")]
        public Microsoft.UI.Xaml.HorizontalAlignment HorizontalContentAlignment
        {
            get;
            set;
        }

        [PropertyFlags(AffectsArrange = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_verticalContentAlignment")]
        public Microsoft.UI.Xaml.VerticalAlignment VerticalContentAlignment
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pBackground")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush Background
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        public Microsoft.UI.Xaml.Controls.BackgroundSizing BackgroundSizing
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueThickness)]
        [OffsetFieldName("m_borderThickness")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Thickness BorderThickness
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pBorderBrush")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush BorderBrush
        {
            get;
            set;
        }

        [CoreType(typeof(Microsoft.UI.Xaml.DependencyObject))]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pDefaultStyleKey")]
        protected Windows.Foundation.Object DefaultStyleKey
        {
            get;
            set;
        }

        public Windows.Foundation.Uri DefaultStyleResourceUri
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        public Microsoft.UI.Xaml.ElementSoundMode ElementSoundMode
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.Idl)]
        [NativeName("IsEnabledChanged")]
        [EventFlags(IsControlEvent = true)]
        public event Microsoft.UI.Xaml.DependencyPropertyChangedEventHandler IsEnabledChanged;

        [EventFlags(IsControlEvent = true)]
        internal event Microsoft.UI.Xaml.DependencyPropertyChangedEventHandler InheritedPropertyChanged;

        protected Control() { }

        [PInvoke]
        internal Microsoft.UI.Xaml.UIElement GetImplementationRoot()
        {
            return default(Microsoft.UI.Xaml.UIElement);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [NativeClassName("CControl")]
        public Windows.Foundation.Boolean ApplyTemplate()
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        protected Microsoft.UI.Xaml.DependencyObject GetTemplateChild(Windows.Foundation.String childName)
        {
            return default(Microsoft.UI.Xaml.DependencyObject);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnPointerEntered(Microsoft.UI.Xaml.Input.PointerRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnPointerPressed(Microsoft.UI.Xaml.Input.PointerRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnPointerMoved(Microsoft.UI.Xaml.Input.PointerRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnPointerReleased(Microsoft.UI.Xaml.Input.PointerRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnPointerExited(Microsoft.UI.Xaml.Input.PointerRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnPointerCaptureLost(Microsoft.UI.Xaml.Input.PointerRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnPointerCanceled(Microsoft.UI.Xaml.Input.PointerRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnPointerWheelChanged(Microsoft.UI.Xaml.Input.PointerRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnTapped(Microsoft.UI.Xaml.Input.TappedRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnDoubleTapped(Microsoft.UI.Xaml.Input.DoubleTappedRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnHolding(Microsoft.UI.Xaml.Input.HoldingRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnRightTapped(Microsoft.UI.Xaml.Input.RightTappedRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromCore = true)]
        internal virtual void OnRightTappedUnhandled(Microsoft.UI.Xaml.Input.RightTappedRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnManipulationStarting(Microsoft.UI.Xaml.Input.ManipulationStartingRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnManipulationInertiaStarting(Microsoft.UI.Xaml.Input.ManipulationInertiaStartingRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnManipulationStarted(Microsoft.UI.Xaml.Input.ManipulationStartedRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnManipulationDelta(Microsoft.UI.Xaml.Input.ManipulationDeltaRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnManipulationCompleted(Microsoft.UI.Xaml.Input.ManipulationCompletedRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnKeyUp(Microsoft.UI.Xaml.Input.KeyRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnKeyDown(Microsoft.UI.Xaml.Input.KeyRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnPreviewKeyDown(Microsoft.UI.Xaml.Input.KeyRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnPreviewKeyUp(Microsoft.UI.Xaml.Input.KeyRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnGotFocus(Microsoft.UI.Xaml.RoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnLostFocus(Microsoft.UI.Xaml.RoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnCharacterReceived(Microsoft.UI.Xaml.Input.CharacterReceivedRoutedEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnDragEnter(Microsoft.UI.Xaml.DragEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnDragLeave(Microsoft.UI.Xaml.DragEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnDragOver(Microsoft.UI.Xaml.DragEventArgs e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnDrop(Microsoft.UI.Xaml.DragEventArgs e)
        {
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        public static Windows.Foundation.Boolean IsTemplateKeyTipTarget
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        internal Microsoft.UI.Xaml.UIElement TemplateKeyTipTarget
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueCornerRadius)]
        public Microsoft.UI.Xaml.CornerRadius CornerRadius
        {
            get;
            set;
        }
    }

    [CodeGen(partial: true)]
    [NativeName("CStackPanel")]
    [ClassFlags(AreMeasureAndArrangeSealed = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.Primitives.IScrollSnapPointsInfo))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IItemLookupPanel))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IInsertionPanel))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IPaginatedPanel))]
    [Guids(ClassGuid = "3f9d740b-365e-415d-bef4-abaf69d407dd")]
    public class StackPanel
     : Microsoft.UI.Xaml.Controls.Panel
    {
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bAreScrollSnapPointsRegular")]
        public Windows.Foundation.Boolean AreScrollSnapPointsRegular
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_orientation")]
        public Microsoft.UI.Xaml.Controls.Orientation Orientation
        {
            get;
            set;
        }

        public StackPanel() { }

        [NativeStorageType(ValueType.valueEnum)]
        [RenderDirtyFlagClassName("CPanel")]
        [RenderDirtyFlagMethodName("NWSetBackgroundDirty")]
        public Microsoft.UI.Xaml.Controls.BackgroundSizing BackgroundSizing
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CPanel")]
        [RenderDirtyFlagMethodName("NWSetBorderBrushDirty")]
        public Microsoft.UI.Xaml.Media.Brush BorderBrush
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueThickness)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Thickness BorderThickness
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueCornerRadius)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.CornerRadius CornerRadius
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueThickness)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Thickness Padding
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double Spacing
        {
            get;
            set;
        }
    }

    [FrameworkTypePattern]
    [Platform(typeof(PrivateApiContract), 1)]
    public interface ITextBoxPriv2
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        Windows.Foundation.Boolean IsDesktopPopupMenuEnabled
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void ForceEditFocusLoss();

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_preventEditFocusLoss")]
        Windows.Foundation.Boolean PreventEditFocusLoss
        {
            get;
            set;
        }
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface ITextBoxPrivate
    {
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_ensureRectVisibleEnabled")]
        Windows.Foundation.Boolean EnsureRectVisibleEnabled
        {
            get;
            set;
        }
    }

    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    public enum CandidateWindowAlignment
    {
        [NativeValueName("CandidateWindowAlignmentDefault")]
        Default = 0,
        [NativeValueName("CandidateWindowAlignmentBottomEdge")]
        BottomEdge = 1,
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface ITelemetryCollectionPriv
    {
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bIsTelemetryCollectionEnabled")]
        Windows.Foundation.Boolean IsTelemetryCollectionEnabled
        {
            get;
            set;
        }
    }

    [Guids(ClassGuid = "30a0b28c-f123-4b0f-a2e7-0bb976806f65")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CCandidateWindowBoundsChangedEventArgs")]
    public sealed class CandidateWindowBoundsChangedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [NativeStorageType(ValueType.valueRect)]
        [OffsetFieldName("m_bounds")]
        [DelegateToCore]
        public Windows.Foundation.Rect Bounds
        {
            get;
            internal set;
        }

        internal CandidateWindowBoundsChangedEventArgs() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CTextChangedEventArgs")]
    [Guids(ClassGuid = "f20ef486-d2d5-4e96-a0d4-55f968e9cbdd")]
    public sealed class TextChangedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        internal TextChangedEventArgs() { }
    }

    [Guids(ClassGuid = "b8f645fb-2ed2-4423-9a4f-0a518d0f05aa")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [NativeName("CFocusEngagedEventArgs")]
    [DXamlIdlGroup("Main")]
    public sealed class FocusEngagedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        internal FocusEngagedEventArgs() { }

        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

    }

    [Guids(ClassGuid = "f54f9db1-50fd-473c-8c09-be16340f43a7")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [NativeName("CFocusDisengagedEventArgs")]
    [DXamlIdlGroup("Main")]
    public sealed class FocusDisengagedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        internal FocusDisengagedEventArgs() { }
    }

    [Guids(ClassGuid = "c3c3155c-942a-4518-94c0-268a87057987")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CTextCompositionStartedEventArgs")]
    public sealed class TextCompositionStartedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_startIndex")]
        [DelegateToCore]
        public Windows.Foundation.Int32 StartIndex
        {
            get;
            internal set;
        }

        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_length")]
        [DelegateToCore]
        public Windows.Foundation.Int32 Length
        {
            get;
            internal set;
        }
        internal TextCompositionStartedEventArgs() { }
    }

    [Guids(ClassGuid = "575d5a8e-ab22-4219-9c8c-ce705657b298")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CTextCompositionChangedEventArgs")]
    public sealed class TextCompositionChangedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_startIndex")]
        [DelegateToCore]
        public Windows.Foundation.Int32 StartIndex
        {
            get;
            internal set;
        }

        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_length")]
        [DelegateToCore]
        public Windows.Foundation.Int32 Length
        {
            get;
            internal set;
        }
        internal TextCompositionChangedEventArgs() { }
    }

    [Guids(ClassGuid = "8c9253f5-28ad-46c3-8bb7-1b704c454e6a")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CTextCompositionEndedEventArgs")]
    public sealed class TextCompositionEndedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_startIndex")]
        [DelegateToCore]
        public Windows.Foundation.Int32 StartIndex
        {
            get;
            internal set;
        }

        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_length")]
        [DelegateToCore]
        public Windows.Foundation.Int32 Length
        {
            get;
            internal set;
        }
        internal TextCompositionEndedEventArgs() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CTextControlPasteEventArgs")]
    [Guids(ClassGuid = "90673785-8084-4490-bb6d-7379e6f2379a")]
    public sealed class TextControlPasteEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }
        internal TextControlPasteEventArgs() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CTextControlCopyingToClipboardEventArgs")]
    [Guids(ClassGuid = "9b2b32cd-09fb-4f55-92fa-dff26a685237")]
    public sealed class TextControlCopyingToClipboardEventArgs : Microsoft.UI.Xaml.EventArgs
    {
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }
        internal TextControlCopyingToClipboardEventArgs() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CTextControlCuttingToClipboardEventArgs")]
    [Guids(ClassGuid = "e263c58b-288a-40cb-ae7f-957351e49d12")]
    public sealed class TextControlCuttingToClipboardEventArgs : Microsoft.UI.Xaml.EventArgs
    {
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }
        internal TextControlCuttingToClipboardEventArgs() { }
    }

    [NativeName("CRowDefinition")]
    [Guids(ClassGuid = "e74b21a7-8081-4f19-bade-3fb9abda1ce5")]
    [ClassFlags(HasTypeConverter = true)]
    [ContentProperty(nameof(Height))]
    public sealed class RowDefinition
     : Microsoft.UI.Xaml.DependencyObject
    {
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueGridLength)]
        [OffsetFieldName("m_pUserSize")]
        public Microsoft.UI.Xaml.GridLength Height
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eUserMaxSize")]
        public Windows.Foundation.Double MaxHeight
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eUserMinSize")]
        public Windows.Foundation.Double MinHeight
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CDefinitionBase", "ActualSize")]
        [NativeStorageType(ValueType.valueFloat)]
        [ReadOnly]
        public Windows.Foundation.Double ActualHeight
        {
            get;
            private set;
        }

        public RowDefinition() { }
    }

    [NativeName("CColumnDefinition")]
    [Guids(ClassGuid = "21210372-dbf3-4f19-91ba-ee08043b5de8")]
    [ClassFlags(HasTypeConverter = true)]
    [ContentProperty(nameof(Width))]
    public sealed class ColumnDefinition
     : Microsoft.UI.Xaml.DependencyObject
    {
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueGridLength)]
        [OffsetFieldName("m_pUserSize")]
        public Microsoft.UI.Xaml.GridLength Width
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eUserMaxSize")]
        public Windows.Foundation.Double MaxWidth
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eUserMinSize")]
        public Windows.Foundation.Double MinWidth
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CDefinitionBase", "ActualSize")]
        [NativeStorageType(ValueType.valueFloat)]
        [ReadOnly]
        public Windows.Foundation.Double ActualWidth
        {
            get;
            private set;
        }

        public ColumnDefinition() { }
    }

    [NativeName("CGrid")]
    [ClassFlags(AreMeasureAndArrangeSealed = true)]
    [Guids(ClassGuid = "e23a1571-80e7-4fb5-a754-3e513559137a")]
    public class Grid
     : Microsoft.UI.Xaml.Controls.Panel
    {
        [PropertyFlags(AffectsMeasure = true, IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pRowDefinitions")]
        public Microsoft.UI.Xaml.Controls.RowDefinitionCollection RowDefinitions
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pColumnDefinitions")]
        public Microsoft.UI.Xaml.Controls.ColumnDefinitionCollection ColumnDefinitions
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.FrameworkElement))]
        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_pLayoutProperties")]
        [StorageGroupNames("EnsureGroupStorage", "FrameworkElementGroupStorage", "m_nGridRow")]
        public static Windows.Foundation.Int32 AttachedRow
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.FrameworkElement))]
        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_pLayoutProperties")]
        [StorageGroupNames("EnsureGroupStorage", "FrameworkElementGroupStorage", "m_nGridColumn")]
        public static Windows.Foundation.Int32 AttachedColumn
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.FrameworkElement))]
        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_pLayoutProperties")]
        [StorageGroupNames("EnsureGroupStorage", "FrameworkElementGroupStorage", "m_nGridRowSpan")]
        public static Windows.Foundation.Int32 AttachedRowSpan
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.FrameworkElement))]
        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_pLayoutProperties")]
        [StorageGroupNames("EnsureGroupStorage", "FrameworkElementGroupStorage", "m_nGridColumnSpan")]
        public static Windows.Foundation.Int32 AttachedColumnSpan
        {
            get;
            set;
        }

        public Grid() { }

        [NativeStorageType(ValueType.valueEnum)]
        [RenderDirtyFlagClassName("CPanel")]
        [RenderDirtyFlagMethodName("NWSetBackgroundDirty")]
        public Microsoft.UI.Xaml.Controls.BackgroundSizing BackgroundSizing
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CPanel")]
        [RenderDirtyFlagMethodName("NWSetBorderBrushDirty")]
        public Microsoft.UI.Xaml.Media.Brush BorderBrush
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueThickness)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Thickness BorderThickness
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueCornerRadius)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.CornerRadius CornerRadius
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueThickness)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Thickness Padding
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double RowSpacing
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double ColumnSpacing
        {
            get;
            set;
        }
    }

    [ClassFlags(HasBaseTypeInDXamlInterface = false, HasTypeConverter = true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CRowDefinitionCollection")]
    [Guids(ClassGuid = "1d4e2cf0-d66f-47d2-87f5-1516520a5ea5")]
    public sealed class RowDefinitionCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<RowDefinition>
    {
        [NativeStorageType(ValueType.valueObject)]
        public Microsoft.UI.Xaml.Controls.RowDefinition ContentProperty
        {
            get;
            set;
        }

        internal RowDefinitionCollection() { }
    }

    [ClassFlags(HasBaseTypeInDXamlInterface = false, HasTypeConverter = true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CColumnDefinitionCollection")]
    [Guids(ClassGuid = "4a83c97d-4e54-456d-a921-82b57d317445")]
    public sealed class ColumnDefinitionCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<ColumnDefinition>
    {
        [NativeStorageType(ValueType.valueObject)]
        public Microsoft.UI.Xaml.Controls.ColumnDefinition ContentProperty
        {
            get;
            set;
        }

        internal ColumnDefinitionCollection() { }
    }

    [CodeGen(partial: true)]
    [NativeName("CSwapChainBackgroundPanel")]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "9ae0ce29-4849-4b27-a378-66673305eaca")]
    public class SwapChainBackgroundPanel
     : Microsoft.UI.Xaml.Controls.Grid
    {
        public SwapChainBackgroundPanel() { }
    }

    [CodeGen(partial: true)]
    [NativeName("CSwapChainPanel")]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "bcb4fa81-c951-4de0-91f4-0fc0efa12d1d")]
    public class SwapChainPanel
     : Microsoft.UI.Xaml.Controls.Grid
    {
        [CoreType(typeof(Windows.Foundation.Double))]
        [NativeMethod("CSwapChainPanel", "GetCompositionScaleX")]
        [ReadOnly]
        public Windows.Foundation.Float CompositionScaleX
        {
            get;
            internal set;
        }

        [CoreType(typeof(Windows.Foundation.Double))]
        [NativeMethod("CSwapChainPanel", "GetCompositionScaleY")]
        [ReadOnly]
        public Windows.Foundation.Float CompositionScaleY
        {
            get;
            internal set;
        }

        public SwapChainPanel() { }

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.Controls.CompositionScaleChangedEventHandler CompositionScaleChanged;

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [AllowCrossThreadAccess]
        public Microsoft.UI.Input.InputPointerSource CreateCoreIndependentInputSource(Microsoft.UI.Input.InputPointerSourceDeviceKinds deviceKinds)
        {
            return default(Microsoft.UI.Input.InputPointerSource);
        }
    }

    [DXamlIdlGroup("Main")]
    [NativeName("CControlTemplate")]
    [Guids(ClassGuid = "aee587eb-de1e-4fa4-87fb-a130e8076673")]
    public sealed class ControlTemplate
     : Microsoft.UI.Xaml.FrameworkTemplate
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CControlTemplate", "TargetType")]
        public Windows.UI.Xaml.Interop.TypeName TargetType
        {
            get;
            set;
        }

        public ControlTemplate() { }
    }

    [CodeGen(partial: true)]
    [NativeName("CUserControl")]
    [ContentProperty("Content")]
    [Guids(ClassGuid = "fd08e601-4165-4aaf-8ed5-e91bddb43f6a")]
    public class UserControl
     : Microsoft.UI.Xaml.Controls.Control
    {
        [RequiresMultipleAssociationCheck]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeMethod("CUserControl", "Content")]
        public Microsoft.UI.Xaml.UIElement Content
        {
            get;
            set;
        }

        public UserControl() { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CItemCollection")]
    [ClassFlags(IsObservable = true, HasBaseTypeInDXamlInterface = false)]
    [OldCodeGenBaseType(typeof(Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<Windows.Foundation.Object>))]
    [Guids(ClassGuid = "39c26b2c-d5be-4ba8-a808-61f166b9a63f")]
    public sealed class ItemCollection
     : Microsoft.UI.Xaml.Collections.ObservablePresentationFrameworkCollection<Windows.Foundation.Object>
    {
        [CoreType(typeof(Microsoft.UI.Xaml.DependencyObject))]
        [NativeStorageType(ValueType.valueObject)]
        public Windows.Foundation.Object ContentProperty
        {
            get;
            set;
        }

        internal ItemCollection() { }
    }

    [CodeGen(partial: true)]
    [NativeName("CItemsControl")]
    [ContentProperty("Items")]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IItemContainerMapping))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IGroupHeaderMapping))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IContainerRecyclingContext))]
    [Guids(ClassGuid = "efe0d15e-beaa-4da1-9ddd-91a40dae27d5")]
    public class ItemsControl
     : Microsoft.UI.Xaml.Controls.Control
    {

        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Object ItemsSource
        {
            get;
            set;
        }

        [PropertyFlags(IsReadOnlyExceptForParser = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CItemsControl", "GetItems")]
        public Microsoft.UI.Xaml.Controls.ItemCollection Items
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pItemTemplate")]
        public Microsoft.UI.Xaml.DataTemplate ItemTemplate
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.Controls.DataTemplateSelector ItemTemplateSelector
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pItemsPanelTemplate")]
        public Microsoft.UI.Xaml.Controls.ItemsPanelTemplate ItemsPanel
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueString)]
        [OffsetFieldName("m_strDisplayMemberPath")]
        public Windows.Foundation.String DisplayMemberPath
        {
            get;
            set;
        }

        [PropertyFlags(DoNotEnterOrLeaveValue = true, IsGetterImplVirtual = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pItemsHost")]
        internal Microsoft.UI.Xaml.Controls.Panel ItemsHost
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Controls.Panel ItemsPanelRoot
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [PropertyFlags(IsGetterImplVirtual = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bItemsHostInvalid")]
        internal Windows.Foundation.Boolean IsItemsHostInvalid
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.Style ItemContainerStyle
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.Controls.StyleSelector ItemContainerStyleSelector
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Controls.ItemContainerGenerator ItemContainerGenerator
        {
            get;
            private set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pItemContainerTransitions")]
        [PropertyFlags(IsValueCreatedOnDemand = true)]
        public Microsoft.UI.Xaml.Media.Animation.TransitionCollection ItemContainerTransitions
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [DependencyPropertyModifier(Modifier.Internal)]
        [CollectionType(CollectionKind.Observable, NewCodeGenCollectionType = typeof(Windows.Foundation.Collections.IObservableVector<Microsoft.UI.Xaml.Controls.GroupStyle>))]
        [ReadOnly]
        public Microsoft.UI.Xaml.Controls.GroupStyleCollection GroupStyle
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.Controls.GroupStyleSelector GroupStyleSelector
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [PropertyFlags(IsSetterImplVirtual = true)]
        public Windows.Foundation.Boolean IsGrouping
        {
            get;
            internal set;
        }

        public ItemsControl() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual Windows.Foundation.Boolean IsItemItsOwnContainerOverride([Optional] Windows.Foundation.Object item)
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual Microsoft.UI.Xaml.DependencyObject GetContainerForItemOverride()
        {
            return default(Microsoft.UI.Xaml.DependencyObject);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void ClearContainerForItemOverride(Microsoft.UI.Xaml.DependencyObject element, [Optional] Windows.Foundation.Object item)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void PrepareContainerForItemOverride(Microsoft.UI.Xaml.DependencyObject element, [Optional] Windows.Foundation.Object item)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnItemsChanged(Windows.Foundation.Object e)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnItemContainerStyleChanged([Optional] Microsoft.UI.Xaml.Style oldItemContainerStyle, [Optional] Microsoft.UI.Xaml.Style newItemContainerStyle)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnItemContainerStyleSelectorChanged([Optional] Microsoft.UI.Xaml.Controls.StyleSelector oldItemContainerStyleSelector, [Optional] Microsoft.UI.Xaml.Controls.StyleSelector newItemContainerStyleSelector)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnItemTemplateChanged([Optional] Microsoft.UI.Xaml.DataTemplate oldItemTemplate, [Optional] Microsoft.UI.Xaml.DataTemplate newItemTemplate)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnItemTemplateSelectorChanged([Optional] Microsoft.UI.Xaml.Controls.DataTemplateSelector oldItemTemplateSelector, [Optional] Microsoft.UI.Xaml.Controls.DataTemplateSelector newItemTemplateSelector)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnGroupStyleSelectorChanged([Optional] Microsoft.UI.Xaml.Controls.GroupStyleSelector oldGroupStyleSelector, [Optional] Microsoft.UI.Xaml.Controls.GroupStyleSelector newGroupStyleSelector)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static Microsoft.UI.Xaml.Controls.ItemsControl GetItemsOwner(Microsoft.UI.Xaml.DependencyObject element)
        {
            return default(Microsoft.UI.Xaml.Controls.ItemsControl);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static Microsoft.UI.Xaml.Controls.ItemsControl ItemsControlFromItemContainer(Microsoft.UI.Xaml.DependencyObject container)
        {
            return default(Microsoft.UI.Xaml.Controls.ItemsControl);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [OptionalReturnValue]
        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.DependencyObject GroupHeaderContainerFromItemContainer(Microsoft.UI.Xaml.DependencyObject itemContainer)
        {
            return default(Microsoft.UI.Xaml.DependencyObject);
        }
    }

    [Comment("Interface through which an ItemContainerGenerator communicates with its host.")]
    internal interface IGeneratorHost
    {
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        [Comment("The view of the data.")]
        Windows.Foundation.Collections.IVector<Windows.Foundation.Object> View { get; }

        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        [Comment("The CollectionView (if exist).")]
        Microsoft.UI.Xaml.Data.ICollectionView CollectionView { get; }

        [Comment("Returns true if the item is (or should be) its own item container.")]
        bool IsItemItsOwnContainer(Windows.Foundation.Object item);

        [Comment("Returns the element used to display the given item.")]
        DependencyObject GetContainerForItem(Windows.Foundation.Object item, Microsoft.UI.Xaml.DependencyObject container);

        [Comment("Prepares the element to act as the ItemUI for the corresponding item.")]
        void PrepareItemContainer(Microsoft.UI.Xaml.DependencyObject container, Windows.Foundation.Object item);

        [Comment("Undo any initialization done on the element during GetContainerForItem and PrepareItemContainer.")]
        void ClearContainerForItem(Microsoft.UI.Xaml.DependencyObject container, Windows.Foundation.Object item);

        [Comment("Determines if the given element was generated for this host as an ItemUI.")]
        bool IsHostForItemContainer(Microsoft.UI.Xaml.DependencyObject container);

        [Comment("Returns the GroupStyle (if any) to use for the given group at the given level.")]
        Microsoft.UI.Xaml.Controls.GroupStyle GetGroupStyle(Microsoft.UI.Xaml.Data.ICollectionViewGroup group, uint level);

        [Comment("Communicates to the host that the generator is using grouping.")]
        void SetIsGrouping(bool isGrouping);

        [Comment("Returns the element used to display the given item.")]
        Microsoft.UI.Xaml.DependencyObject GetHeaderForGroup(Windows.Foundation.Object group);

        [Comment("Prepares a group container for binding.")]
        void PrepareGroupContainer(Microsoft.UI.Xaml.DependencyObject container, Microsoft.UI.Xaml.Data.ICollectionViewGroup group);

        [Comment("Undo any initialization done on the element during GetContainerForItem and PrepareItemContainer.")]
        void ClearGroupContainerForGroup(Microsoft.UI.Xaml.DependencyObject container, Microsoft.UI.Xaml.Data.ICollectionViewGroup group);

        [Comment("Called by the modern panels after prepare, this allows you to efficiently hook into the async tree building feature.")]
        void SetupContainerContentChangingAfterPrepare(Microsoft.UI.Xaml.DependencyObject container, Windows.Foundation.Object item, int itemIndex, Windows.Foundation.Size measureSize);

        [Comment("Based on the supplied args, determine if we need to register for async work.")]
        void RegisterWorkFromArgs(Microsoft.UI.Xaml.Controls.ContainerContentChangingEventArgs args);

        [Comment("For Container, determine if we need to register for async work.")]
        void RegisterWorkForContainer(Microsoft.UI.Xaml.UIElement container);

        [Comment(@"
        For certain situations (such as drag/drop), the host may hang onto a container for an extended period of time. That particular container shouldn't ever be recycled as long as it's being used.
        That particular container shouldn't ever be recycled as long as it's being used.
        This method asks whether or not the given container is eligible for recycling.")]
        bool CanRecycleContainer(Microsoft.UI.Xaml.DependencyObject container);

        [Comment("During lookups of duplicate or null values, there might be a container that the host can provide the ICG.")]
        Microsoft.UI.Xaml.DependencyObject SuggestContainerForContainerFromItemLookup();

        [Comment(@"
        Returns a value indicating whether or not ChoosingItemContainer should be raised
        based on whether or not it has any listeners and whether it's supported by the current items host.")]
        bool ShouldRaiseChoosingItemContainer();

        [Comment(@"
        Allows application authors to give us a container for a given item, which will cause us
        not to bother generating one on our own.")]
        void RaiseChoosingItemContainer(ChoosingItemContainerEventArgs args);

        [Comment(@"
        Raises ContainerContentChanging on recycle, enabling apps to reuse recycled containers.")]
        void RaiseContainerContentChangingOnRecycle(Microsoft.UI.Xaml.UIElement container, Windows.Foundation.Object item);


        [Comment(@"
        Returns a value indicating whether or not ChoosingItemContainer should be raised
        based on whether or not it has any listeners and whether it's supported by the current items host.")]
        bool ShouldRaiseChoosingGroupHeaderContainer();

        [Comment(@"
        Allows application authors to give us a header container for a given group")]
        void RaiseChoosingGroupHeaderContainer(ChoosingGroupHeaderContainerEventArgs args);

        [Comment(@"
        Called after the panel finishes virtualization setup")]
        void VirtualizationFinished();

        [Comment(@"
        Called to allow the host to override arrange bounds for a container at a given index.")]
        void OverrideContainerArrangeBounds(int index, Windows.Foundation.Rect suggestedBounds, out Windows.Foundation.Rect newBounds);
    }

    [Comment("Implemented by panels with a custom item container generator.")]
    internal interface ICustomGeneratorItemsHost
    {
        void RegisterItemsHost(IGeneratorHost host);
        void DisconnectItemsHost();

        Microsoft.UI.Xaml.Controls.IItemContainerMapping GetItemContainerMapping();
        Microsoft.UI.Xaml.Controls.IGroupHeaderMapping GetGroupHeaderMapping();

        void Refresh();

        void NotifyOfItemsChanging(Windows.Foundation.Collections.IVectorChangedEventArgs e);
        void NotifyOfItemsChanged(Windows.Foundation.Collections.IVectorChangedEventArgs e);

        void NotifyOfItemsReordered(uint count);

        Microsoft.UI.Xaml.Controls.IContainerContentChangingIterator GetContainersForIncrementalVisualization();
    }

    [ClassFlags(IsICollection = true)]
    [FrameworkTypePattern]
    [Modifier(Modifier.Internal)]
    [TypeTable(IsXbfType = true)]
    [Guids(ClassGuid = "0d3231cc-5027-4589-b690-fc8217e84618")]
    public sealed class GroupStyleCollection
    {
        internal GroupStyleCollection() { }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "b0201b45-cdf1-41dc-a575-f281eea3ad11")]
    public sealed class ItemContainerGenerator
    {
        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        [DependencyPropertyModifier(Modifier.Internal)]
        internal static Windows.Foundation.Object AttachedItemForItemContainer
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        [DependencyPropertyModifier(Modifier.Internal)]
        internal static Windows.Foundation.Object AttachedDeferredUnlinkingPayload
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [DependencyPropertyModifier(Modifier.Internal)]
        internal static Windows.Foundation.Boolean AttachedIsRecycledContainer
        {
            get;
            set;
        }

        public event Microsoft.UI.Xaml.Controls.Primitives.ItemsChangedEventHandler ItemsChanged;

        internal ItemContainerGenerator() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [Deprecated("ItemFromContainer may be altered or unavailable for releases after Windows 8.1. Instead, use ItemsControl.ItemFromContainer.")]
        public Windows.Foundation.Object ItemFromContainer(Microsoft.UI.Xaml.DependencyObject container)
        {
            return default(Windows.Foundation.Object);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [Deprecated("ContainerFromItem may be altered or unavailable for releases after Windows 8.1. Instead, use ItemsControl.ContainerFromItem.")]
        public Microsoft.UI.Xaml.DependencyObject ContainerFromItem([Optional] Windows.Foundation.Object item)
        {
            return default(Microsoft.UI.Xaml.DependencyObject);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [Deprecated("IndexFromContainer may be altered or unavailable for releases after Windows 8.1. Instead, use ItemsControl.IndexFromContainer.")]
        public Windows.Foundation.Int32 IndexFromContainer(Microsoft.UI.Xaml.DependencyObject container)
        {
            return default(Windows.Foundation.Int32);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [Deprecated("ContainerFromIndex may be altered or unavailable for releases after Windows 8.1. Instead, use ItemsControl.ContainerFromIndex.")]
        public Microsoft.UI.Xaml.DependencyObject ContainerFromIndex(Windows.Foundation.Int32 index)
        {
            return default(Microsoft.UI.Xaml.DependencyObject);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.Controls.ItemContainerGenerator GetItemContainerGeneratorForPanel(Microsoft.UI.Xaml.Controls.Panel panel)
        {
            return default(Microsoft.UI.Xaml.Controls.ItemContainerGenerator);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void StartAt(Microsoft.UI.Xaml.Controls.Primitives.GeneratorPosition position, Microsoft.UI.Xaml.Controls.Primitives.GeneratorDirection direction, Windows.Foundation.Boolean allowStartAtRealizedItem)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void Stop()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Microsoft.UI.Xaml.DependencyObject GenerateNext(out Windows.Foundation.Boolean isNewlyRealized)
        {
            isNewlyRealized = default(Windows.Foundation.Boolean);
            return default(Microsoft.UI.Xaml.DependencyObject);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void PrepareItemContainer(Microsoft.UI.Xaml.DependencyObject container)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void RemoveAll()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void Remove(Microsoft.UI.Xaml.Controls.Primitives.GeneratorPosition position, Windows.Foundation.Int32 count)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Microsoft.UI.Xaml.Controls.Primitives.GeneratorPosition GeneratorPositionFromIndex(Windows.Foundation.Int32 itemIndex)
        {
            return default(Microsoft.UI.Xaml.Controls.Primitives.GeneratorPosition);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Int32 IndexFromGeneratorPosition(Microsoft.UI.Xaml.Controls.Primitives.GeneratorPosition position)
        {
            return default(Windows.Foundation.Int32);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void Recycle(Microsoft.UI.Xaml.Controls.Primitives.GeneratorPosition position, Windows.Foundation.Int32 count)
        {
        }
    }

    [CodeGen(partial: true)]
    [ControlPattern]
    [Guids(ClassGuid = "56697bdf-b27c-4a4d-8af6-8fb7150d991c")]
    public abstract class VirtualizingPanel
     : Microsoft.UI.Xaml.Controls.Panel
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Controls.ItemContainerGenerator ItemContainerGenerator
        {
            get;
            private set;
        }

        internal VirtualizingPanel() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected void AddInternalChild(Microsoft.UI.Xaml.UIElement child)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected void InsertInternalChild(Windows.Foundation.Int32 index, Microsoft.UI.Xaml.UIElement child)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected void RemoveInternalChildRange(Windows.Foundation.Int32 index, Windows.Foundation.Int32 range)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnItemsChanged(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.Primitives.ItemsChangedEventArgs args)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnClearChildren()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void BringIndexIntoView(Windows.Foundation.Int32 index)
        {
        }
    }

    internal interface IOrientedPanel
    {
        [CodeGen(CodeGenLevel.Idl)]
        [Comment("The dimension in which the items are arranged.")]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Microsoft.UI.Xaml.Controls.Orientation LogicalOrientation
        {
            get;
        }

        [CodeGen(CodeGenLevel.Idl)]
        [Comment("The dimension in which the panel grows.")]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Microsoft.UI.Xaml.Controls.Orientation PhysicalOrientation
        {
            get;
        }
    }

    [CodeGen(partial: true)]
    [Guids(ClassGuid = "6546b3de-b086-4913-af0b-d70231a8a826")]
    public sealed class VirtualizingStackPanel
     : Microsoft.UI.Xaml.Controls.Primitives.OrientedVirtualizingPanel
    {
        public Windows.Foundation.Boolean AreScrollSnapPointsRegular
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Controls.Orientation Orientation
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        public static Microsoft.UI.Xaml.Controls.VirtualizationMode AttachedVirtualizationMode
        {
            get;
            set;
        }

        [Attached(TargetParameterName = "o", TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        public static Windows.Foundation.Boolean AttachedIsVirtualizing
        {
            get;
            internal set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [DependencyPropertyModifier(Modifier.Internal)]
        internal new static Windows.Foundation.Boolean AttachedIsContainerGeneratedForInsert
        {
            get;
            set;
        }

        public event Microsoft.UI.Xaml.Controls.CleanUpVirtualizedItemEventHandler CleanUpVirtualizedItemEvent;

        public VirtualizingStackPanel() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [ForceVirtual]
        protected void OnCleanUpVirtualizedItem(Microsoft.UI.Xaml.Controls.CleanUpVirtualizedItemEventArgs e)
        {
        }
    }

    public interface IItemContainerMapping
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Object ItemFromContainer(Microsoft.UI.Xaml.DependencyObject container);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.DependencyObject ContainerFromItem([Optional] Windows.Foundation.Object item);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Int32 IndexFromContainer(Microsoft.UI.Xaml.DependencyObject container);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.DependencyObject ContainerFromIndex(Windows.Foundation.Int32 index);
    }

    internal interface IGroupHeaderMapping
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Object GroupFromHeader(Microsoft.UI.Xaml.DependencyObject header);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.DependencyObject HeaderFromGroup(Windows.Foundation.Object group);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Int32 IndexFromHeader(Microsoft.UI.Xaml.DependencyObject header, Windows.Foundation.Boolean excludeHiddenEmptyGroups);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.DependencyObject HeaderFromIndex(Windows.Foundation.Int32 index);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [OptionalReturnValue]
        Microsoft.UI.Xaml.DependencyObject GroupHeaderContainerFromItemContainer(Microsoft.UI.Xaml.DependencyObject itemContainer);
    }

    internal interface IItemContainerGenerator2
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.UIElement GenerateContainerAtIndex(Windows.Foundation.Int32 index);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.UIElement GenerateHeaderAtGroupIndex(Windows.Foundation.Int32 index);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        bool GetContainerRecycleQueueEmpty();       // if we expose IICG2, we might not want to expose this one

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void RecycleContainer(Microsoft.UI.Xaml.UIElement container);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        bool TryRecycleContainer(Microsoft.UI.Xaml.UIElement container);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void RecycleAllContainers();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        bool GetHeaderRecycleQueueEmpty();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void RecycleHeader(Microsoft.UI.Xaml.UIElement header);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void RecycleAllHeaders();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void FindRecyclingCandidate(Windows.Foundation.Int32 index, out bool hasMatchingCandidate);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.UInt32 GetQueueLength();
    }

    [Platform(typeof(PrivateApiContract), 1)]
    [TypeTable(IsExcludedFromCore = true, IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    public struct LayoutReference
    {
        public Microsoft.UI.Xaml.Controls.ReferenceIdentity RelativeLocation { get; set; }
        public Windows.Foundation.Rect ReferenceBounds { get; set; }
        public Windows.Foundation.Rect HeaderBounds { get; set; }
        public Windows.Foundation.Boolean ReferenceIsHeader { get; set; }

    }

    [Platform(typeof(PrivateApiContract), 1)]
    [TypeTable(IsExcludedFromCore = true, IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    public struct EstimationReference
    {
        public Microsoft.UI.Xaml.Controls.ElementType ElementType { get; set; }
        public Windows.Foundation.Int32 ElementIndex { get; set; }
        public Windows.Foundation.Rect ElementBounds { get; set; }
    };

    [Platform(typeof(PrivateApiContract), 1)]
    [TypeTable(IsExcludedFromCore = true, IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    public enum ElementType
    {
        [NativeValueName("ItemContainer")]
        ItemContainer = 0,
        [NativeValueName("GroupHeader")]
        GroupHeader = 1
    }

    [Platform(typeof(PrivateApiContract), 1)]
    [TypeTable(IsExcludedFromCore = true, IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    public enum ReferenceIdentity
    {
        [NativeValueName("Myself")]
        Myself = 0,
        [NativeValueName("BeforeMe")]
        BeforeMe = 1,
        [NativeValueName("AfterMe")]
        AfterMe = 2
    }

    [Platform(typeof(PrivateApiContract), 1)]
    [TypeTable(IsExcludedFromCore = true, IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    public enum KeyNavigationAction
    {
        [NativeValueName("Next")]
        Next = 0,
        [NativeValueName("Previous")]
        Previous = 1,
        [NativeValueName("First")]
        First = 2,
        [NativeValueName("Last")]
        Last = 3,
        [NativeValueName("Left")]
        Left = 4,
        [NativeValueName("Right")]
        Right = 5,
        [NativeValueName("Up")]
        Up = 6,
        [NativeValueName("Down")]
        Down = 7,
    }

    [Platform(typeof(PrivateApiContract), 1)]
    [TypeTable(IsExcludedFromCore = true, IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    public enum IndexSearchHint
    {
        [NativeValueName("NoHint")]
        NoHint = 0,
        [NativeValueName("Exact")]
        Exact = 1,
        [NativeValueName("SearchBackwards")]
        SearchBackwards = 2,
        [NativeValueName("SearchForwards")]
        SearchForwards = 3,
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface ILayoutDataInfoProvider
    {
        int GetTotalItemCount();
        int GetTotalGroupCount();
        void GetGroupInformationFromItemIndex(
            int itemIndex,
            out int indexOfGroup,
            out int indexInsideGroup,
            out int itemCountInGroup);
        void GetGroupInformationFromGroupIndex(
            int groupIndex,
            out int startItemIndex,
            out int itemCountInGroup);
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface ILayoutStrategy
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void SetLayoutDataInfoProvider(ILayoutDataInfoProvider provider);

        #region Layout related

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void BeginMeasure();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void EndMeasure();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Size GetElementMeasureSize(
            ElementType elementType,
            Windows.Foundation.Int32 elementIndex,
            Windows.Foundation.Rect windowConstraint);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Rect GetElementBounds(
            ElementType elementType,
            int elementIndex,
            Windows.Foundation.Size containerDesiredSize,
            LayoutReference referenceInformation,
            Windows.Foundation.Rect windowConstraint);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Rect GetElementArrangeBounds(
            ElementType elementType,
            int elementIndex,
            Windows.Foundation.Rect containerBounds,
            Windows.Foundation.Rect windowConstraint,
            Windows.Foundation.Size finalSize);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Boolean ShouldContinueFillingUpSpace(
            ElementType elementType,
            int elementIndex,
            Microsoft.UI.Xaml.Controls.LayoutReference referenceInformation,
            Windows.Foundation.Rect windowToFill);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Point GetPositionOfFirstElement();

        #endregion

        #region Estimation and virtualization

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Controls.Orientation GetVirtualizationDirection();

        [Comment("Given a reference, estimates the index of the element at a certain window.")]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Int32 EstimateElementIndex(
            Microsoft.UI.Xaml.Controls.ElementType elementType,
            Microsoft.UI.Xaml.Controls.EstimationReference headerReference,
            Microsoft.UI.Xaml.Controls.EstimationReference containerReference,
            Windows.Foundation.Rect window,
            out Windows.Foundation.Rect targetRect);

        [Comment("Given a reference and an index delta, estimates the bounds of the element.")]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Rect EstimateElementBounds(
            Microsoft.UI.Xaml.Controls.ElementType elementType,
            int elementIndex,
            Microsoft.UI.Xaml.Controls.EstimationReference headerReference,
            Microsoft.UI.Xaml.Controls.EstimationReference containerReference,
            Windows.Foundation.Rect window);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Size EstimatePanelExtent(
            EstimationReference lastHeaderReference,
            EstimationReference lastContainerReference,
            Windows.Foundation.Rect windowConstraint);

        #endregion

        #region IItemLookupPanel related

        // Gets the closest index to a point or the insertion
        // index, given a reference.
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void EstimateIndexFromPoint(
            Windows.Foundation.Boolean requestingInsertionIndex,
            Windows.Foundation.Point point,
            Microsoft.UI.Xaml.Controls.EstimationReference reference,
            Windows.Foundation.Rect windowConstraint,
            out Microsoft.UI.Xaml.Controls.IndexSearchHint searchHint,
            out Microsoft.UI.Xaml.Controls.ElementType elementType,
            out Windows.Foundation.Int32 elementIndex);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void GetTargetIndexFromNavigationAction(
            Microsoft.UI.Xaml.Controls.ElementType elementType,
            Windows.Foundation.Int32 elementIndex,
            Microsoft.UI.Xaml.Controls.KeyNavigationAction action,
            Windows.Foundation.Rect windowConstraint,
            Windows.Foundation.Int32 itemIndexHintForHeaderNavigation,
            out Microsoft.UI.Xaml.Controls.ElementType targetElementType,
            out Windows.Foundation.Int32 targetElementIndex);

        // Determines whether or not the given item index
        // is a layout boundary.
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void IsIndexLayoutBoundary(
            Microsoft.UI.Xaml.Controls.ElementType elementType,
            Windows.Foundation.Int32 elementIndex,
            Windows.Foundation.Rect windowConstraint,
            out Windows.Foundation.Boolean isLeftBoundary,
            out Windows.Foundation.Boolean isTopBoundary,
            out Windows.Foundation.Boolean isRightBoundary,
            out Windows.Foundation.Boolean isBottomBoundary);

        #endregion

        #region Snap points related

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Boolean GetRegularSnapPoints(
            out Windows.Foundation.Float nearOffset,
            out Windows.Foundation.Float farOffset,
            out Windows.Foundation.Float spacing);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Boolean HasIrregularSnapPoints(
            Microsoft.UI.Xaml.Controls.ElementType elementType);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Boolean HasSnapPointOnElement(
            Microsoft.UI.Xaml.Controls.ElementType elementType,
            int elementIndex);

        #endregion

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Boolean GetIsWrappingStrategy();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Rect GetElementTransitionsBounds(
            ElementType elementType,
            int elementIndex,
            Windows.Foundation.Rect windowConstraint);
    }

    [DXamlIdlGroup("Controls")]
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Implements(typeof(Microsoft.UI.Xaml.Media.Animation.ITransitionContextProvider))]
    [Guids(ClassGuid = "1e1125ba-5e23-4ec1-be65-99c2a9935a61")]
    public abstract class ListViewBaseHeaderItem
     : Microsoft.UI.Xaml.Controls.ContentControl
    {
        internal ListViewBaseHeaderItem() { }
    }

    [DXamlIdlGroup("Controls2")]
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "d11c3d4d-bcd7-4b5b-83bb-4bfee78c7e8b")]
    public class GridViewHeaderItem
     : Microsoft.UI.Xaml.Controls.ListViewBaseHeaderItem
    {
    }

    [DXamlIdlGroup("Controls2")]
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "11f6effd-d421-4c6c-b5af-c2cfc7976851")]
    public class ListViewHeaderItem
     : Microsoft.UI.Xaml.Controls.ListViewBaseHeaderItem
    {
    }

    [FrameworkTypePattern]
    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromCore = true, IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    [Guids(ClassGuid = "39b08c7e-322a-4590-926e-9c7643ddea13")]
    internal class BudgetManager
     : Windows.Foundation.Object
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Int32 GetElapsedMilliSecondsSinceLastUITick()
        {
            return default(Windows.Foundation.Int32);
        }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ILayoutStrategy))]
    [Guids(ClassGuid = "34e2aa1d-0d8f-4a10-a3dd-ea7541c4cf94")]
    internal sealed class WrappingLayoutStrategy
    {

    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ILayoutStrategy))]
    [Guids(ClassGuid = "aa8ca2cc-5e88-4ad5-a4c0-f0cb3b38bd87")]
    internal sealed class StackingLayoutStrategy
    {

    }

    [FrameworkTypePattern]
    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true, IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    [Guids(ClassGuid = "a481f777-ad39-4511-9b1a-ae140530d030")]
    internal class BuildTreeService
     : Windows.Foundation.Object
    {

    }

    [DXamlIdlGroup("Controls2")]
    internal interface ITreeBuilder
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Boolean IsBuildTreeSuspended();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Boolean BuildTree();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void ShutDownDeferredWork();

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        Windows.Foundation.Boolean IsRegisteredForCallbacks
        {
            get;
            set;
        }
    }


    [CodeGen(partial: true)]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IItemContainerGenerator2))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IItemContainerMapping))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IGroupHeaderMapping))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IKeyboardNavigationPanel))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IKeyboardHeaderNavigationPanel))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IItemLookupPanel))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IInsertionPanel))]
    [Implements(typeof(Microsoft.UI.Xaml.Media.Animation.IChildTransitionContextProvider))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IPaginatedPanel))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.Primitives.IScrollSnapPointsInfo))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ITreeBuilder))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ICustomGeneratorItemsHost))]
    [ClassFlags(IsHiddenFromIdl = true)]
    [ControlPattern]
    [Guids(ClassGuid = "6d7d2d4d-4fb3-4f85-9aa1-a1c2950178fe")]
    public abstract class ModernCollectionBasePanel
        : Microsoft.UI.Xaml.Controls.Panel
    {
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Int32 FirstCacheIndexBase
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Int32 FirstVisibleIndexBase
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Int32 LastVisibleIndexBase
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Int32 LastCacheIndexBase
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        internal Windows.Foundation.Int32 FirstCacheGroupIndexBase
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        internal Windows.Foundation.Int32 FirstVisibleGroupIndexBase
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        internal Windows.Foundation.Int32 LastVisibleGroupIndexBase
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        internal Windows.Foundation.Int32 LastCacheGroupIndexBase
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Microsoft.UI.Xaml.Controls.PanelScrollingDirection PanningDirectionBase
        {
            get;
            private set;
        }

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        internal event Microsoft.UI.Xaml.EventHandler VisibleIndicesUpdated;

        [TypeTable(IsExcludedFromDXaml = true)]
        public bool AreStickyGroupHeadersEnabledBase
        {
            get;
            set;
        }
    }

    [DXamlIdlGroup("Controls2")]
    [CodeGen(partial: true)]
    [ControlPattern]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IOrientedPanel))]
    [Guids(ClassGuid = "81b3c2e9-dba8-4612-92f8-a3e96e437b0b")]
    public sealed class ItemsWrapGrid
        : Microsoft.UI.Xaml.Controls.ModernCollectionBasePanel
    {
        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.Thickness GroupPadding
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [Comment("The dimension in which the items are arranged.")]
        public Microsoft.UI.Xaml.Controls.Orientation Orientation
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [Comment("The maximum number of items per row or column (depending on the Orientation of ItemsWrapGrid).")]
        public Windows.Foundation.Int32 MaximumRowsOrColumns
        {
            get;
            set;
        }
        [Comment("The width of each item in the WrapGrid.  If unspecified, the width of the first item in the WrapGrid will be used for all other items.")]
        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Double ItemWidth
        {
            get;
            set;
        }

        [Comment("The height of each item in the WrapGrid.  If unspecified, the height of the first item in the WrapGrid will be used for all other items.")]
        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Double ItemHeight
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        public Windows.Foundation.Int32 FirstCacheIndex
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        public Windows.Foundation.Int32 FirstVisibleIndex
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        public Windows.Foundation.Int32 LastVisibleIndex
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        public Windows.Foundation.Int32 LastCacheIndex
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Controls.PanelScrollingDirection ScrollingDirection
        {
            get;
            private set;
        }

        [Comment("The location of a group's header with respect to the group's items.")]
        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.Controls.Primitives.GroupHeaderPlacement GroupHeaderPlacement
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Double CacheLength
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        public bool AreStickyGroupHeadersEnabled
        {
            get;
            set;
        }
    }

    [DXamlIdlGroup("Controls2")]
    [CodeGen(partial: true)]
    [ControlPattern]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IOrientedPanel))]
    [NativeName("CItemsStackPanel")]
    [Guids(ClassGuid = "9a58eb99-67ae-4361-b09f-6901cb9da053")]
    public sealed class ItemsStackPanel
        : Microsoft.UI.Xaml.Controls.ModernCollectionBasePanel
    {
        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.Thickness GroupPadding
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [Comment("The dimension in which the items are arranged.")]
        public Microsoft.UI.Xaml.Controls.Orientation Orientation
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        public Windows.Foundation.Int32 FirstCacheIndex
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        public Windows.Foundation.Int32 FirstVisibleIndex
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        public Windows.Foundation.Int32 LastVisibleIndex
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        public Windows.Foundation.Int32 LastCacheIndex
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Controls.PanelScrollingDirection ScrollingDirection
        {
            get;
            private set;
        }

        [Comment("The location of a group's header with respect to the group's items.")]
        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.Controls.Primitives.GroupHeaderPlacement GroupHeaderPlacement
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [DependencyPropertyModifier(Modifier.Internal)]
        [Comment("Specifies the scroll behavior when the content is updated.")]
        public ItemsUpdatingScrollMode ItemsUpdatingScrollMode
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Double CacheLength
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        public bool AreStickyGroupHeadersEnabled
        {
            get;
            set;
        }
    }

    internal interface IKeyboardNavigationPanel
    {
        [CodeGen(CodeGenLevel.Idl)]
        Windows.Foundation.Boolean SupportsKeyNavigationAction(Microsoft.UI.Xaml.Controls.KeyNavigationAction action);

        [CodeGen(CodeGenLevel.Idl)]
        void GetTargetIndexFromNavigationAction(
            Windows.Foundation.UInt32 sourceIndex,
            Microsoft.UI.Xaml.Controls.ElementType sourceType,
            Microsoft.UI.Xaml.Controls.KeyNavigationAction action,
            Windows.Foundation.Boolean allowWrap,
            Windows.Foundation.UInt32 itemIndexHintForHeaderNavigation,
            out Windows.Foundation.UInt32 computedTargetIndex,
            out Microsoft.UI.Xaml.Controls.ElementType computedTargetType,
            out Windows.Foundation.Boolean actionValidForSourceIndex);
    }

    internal interface IKeyboardHeaderNavigationPanel
    {
        [CodeGen(CodeGenLevel.Idl)]
        void GetTargetHeaderIndexFromNavigationAction(Windows.Foundation.UInt32 sourceIndex, Microsoft.UI.Xaml.Controls.KeyNavigationAction action, out Windows.Foundation.UInt32 computedTargetIndex, out Windows.Foundation.Boolean actionHandled);
    }

    internal interface IItemLookupPanel
    {
        [CodeGen(CodeGenLevel.Idl)]
        [Comment("Get the closest index to the point.")]
        Microsoft.UI.Xaml.Controls.Primitives.ElementInfo GetClosestElementInfo(Windows.Foundation.Point position);

        [CodeGen(CodeGenLevel.Idl)]
        [Comment("Get the index where an item should be inserted.")]
        Windows.Foundation.Int32 GetInsertionIndex(Windows.Foundation.Point position);

        [CodeGen(CodeGenLevel.Idl)]
        [Comment("Find whether the given Index is positioned on any layout boundry")]
        void IsLayoutBoundary(Windows.Foundation.Int32 index, out Windows.Foundation.Boolean isLeftBoundary, out Windows.Foundation.Boolean isTopBoundary, out Windows.Foundation.Boolean isRightBoundary, out Windows.Foundation.Boolean isBottomBoundary);

        [CodeGen(CodeGenLevel.Idl)]
        Windows.Foundation.Rect GetItemsBounds();
    }

    [TypeTable(IsExcludedFromCore = true)]
    [HideFromOldCodeGen]
    public interface IInsertionPanel
    {
        [Comment("Get the two indexes where an item should be inserted.")]
        void GetInsertionIndexes(Windows.Foundation.Point position, out Windows.Foundation.Int32 first, out Windows.Foundation.Int32 second);
    }

    [CodeGen(partial: true)]
    [Comment("VariableSizedWrapGrid provides the variable size tiles layout experience for the GridView control.")]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IKeyboardNavigationPanel))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IOrientedPanel))]
    [ControlPattern]
    [Guids(ClassGuid = "285d58ab-d64c-49f4-9740-060a0aabecb7")]
    public sealed class VariableSizedWrapGrid
     : Microsoft.UI.Xaml.Controls.Panel
    {
        public Windows.Foundation.Double ItemHeight
        {
            get;
            set;
        }

        public Windows.Foundation.Double ItemWidth
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        public static Windows.Foundation.Int32 AttachedRowSpan
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        public static Windows.Foundation.Int32 AttachedColumnSpan
        {
            get;
            set;
        }

        [Comment("The dimension in which the items are arranged.")]
        public Microsoft.UI.Xaml.Controls.Orientation Orientation
        {
            get;
            set;
        }

        [Comment("The horizontal alignment of the items.")]
        public Microsoft.UI.Xaml.HorizontalAlignment HorizontalChildrenAlignment
        {
            get;
            set;
        }

        [Comment("The vertical alignment of the items.")]
        public Microsoft.UI.Xaml.VerticalAlignment VerticalChildrenAlignment
        {
            get;
            set;
        }

        [Comment("The maximum number of items per row or column (depending on the Orientation of WrapGrid).")]
        public Windows.Foundation.Int32 MaximumRowsOrColumns
        {
            get;
            set;
        }

        public VariableSizedWrapGrid() { }
    }

    [NativeName("CViewbox")]
    [ContentProperty("Child")]
    [Guids(ClassGuid = "d3e3f989-57ae-43d0-a6e6-6e789d188657")]
    public sealed class Viewbox
     : Microsoft.UI.Xaml.FrameworkElement
    {
        [RequiresMultipleAssociationCheck]
        [PropertyFlags(AffectsMeasure = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CViewbox", "Child")]
        public Microsoft.UI.Xaml.UIElement Child
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_stretch")]
        public Microsoft.UI.Xaml.Media.Stretch Stretch
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_stretchDirection")]
        public Microsoft.UI.Xaml.Controls.StretchDirection StretchDirection
        {
            get;
            set;
        }

        public Viewbox() { }
    }

    [NativeName("CBorder")]
    [ContentProperty("Child")]
    [Guids(ClassGuid = "112ecdb8-acd6-4fb5-b8c8-f223d7cd94bf")]
    public sealed class Border
     : Microsoft.UI.Xaml.FrameworkElement
    {

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pBorderBrush")]
        [RenderDirtyFlagClassName("CBorder")]
        [RenderDirtyFlagMethodName("NWSetBorderBrushDirty")]
        public Microsoft.UI.Xaml.Media.Brush BorderBrush
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueThickness)]
        [OffsetFieldName("m_borderThickness")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Thickness BorderThickness
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pBackgroundBrush")]
        [RenderDirtyFlagClassName("CBorder")]
        [RenderDirtyFlagMethodName("NWSetBackgroundBrushDirty")]
        public Microsoft.UI.Xaml.Media.Brush Background
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [RenderDirtyFlagClassName("CBorder")]
        [RenderDirtyFlagMethodName("NWSetBackgroundBrushDirty")]
        public Microsoft.UI.Xaml.Controls.BackgroundSizing BackgroundSizing
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueCornerRadius)]
        [OffsetFieldName("m_cornerRadius")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.CornerRadius CornerRadius
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueThickness)]
        [OffsetFieldName("m_padding")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Thickness Padding
        {
            get;
            set;
        }

        [RequiresMultipleAssociationCheck]
        [PropertyFlags(AffectsMeasure = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CBorder", "Child")]
        public Microsoft.UI.Xaml.UIElement Child
        {
            get;
            set;
        }

        [PropertyFlags(HadFieldInBlue = true, IsValueCreatedOnDemand = true)]
        public Microsoft.UI.Xaml.Media.Animation.TransitionCollection ChildTransitions
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.BrushTransition BackgroundTransition
        {
            get;
            set;
        }

        public Border() { }
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface IContentControlPrivate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Rect GetGlobalBounds();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        float GetRasterizationScale();
    }

    [CodeGen(partial: true)]
    [NativeName("CContentControl")]
    [ContentProperty("Content")]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IContentControlPrivate))]
    [Guids(ClassGuid = "5a0b869f-3144-4ee6-bea5-9fa5f4ba5009")]
    public class ContentControl
     : Microsoft.UI.Xaml.Controls.Control
    {
        [RequiresMultipleAssociationCheck]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeMethod("CContentControl", "Content")]
        public Windows.Foundation.Object Content
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pContentTemplate")]
        public Microsoft.UI.Xaml.DataTemplate ContentTemplate
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.Controls.DataTemplateSelector ContentTemplateSelector
        {
            get;
            set;
        }

        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pSelectedContentTemplate")]
        internal Microsoft.UI.Xaml.DataTemplate SelectedContentTemplate
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pContentTransitions")]
        [PropertyFlags(IsValueCreatedOnDemand = true)]
        public Microsoft.UI.Xaml.Media.Animation.TransitionCollection ContentTransitions
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [TypeTable(IsExcludedFromCore = true)]
        [ReadOnly]
        public Microsoft.UI.Xaml.UIElement ContentTemplateRoot
        {
            get;
            set;
        }

        public ContentControl() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnContentChanged([Optional] Windows.Foundation.Object oldContent, [Optional] Windows.Foundation.Object newContent)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnContentTemplateChanged([Optional] Microsoft.UI.Xaml.DataTemplate oldContentTemplate, [Optional] Microsoft.UI.Xaml.DataTemplate newContentTemplate)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnContentTemplateSelectorChanged([Optional] Microsoft.UI.Xaml.Controls.DataTemplateSelector oldContentTemplateSelector, [Optional] Microsoft.UI.Xaml.Controls.DataTemplateSelector newContentTemplateSelector)
        {
        }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CContentPresenter")]
    [ContentProperty("Content")]
    [ClassFlags(IsPublicInSL4 = true)]
    [Guids(ClassGuid = "bf8bb5e1-5ff4-437b-a2a3-8bdc6a64544b")]
    public class ContentPresenter
     : Microsoft.UI.Xaml.FrameworkElement
    {
        [RequiresMultipleAssociationCheck]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeMethod("CContentPresenter", "Content")]
        public Windows.Foundation.Object Content
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pContentTemplate")]
        public Microsoft.UI.Xaml.DataTemplate ContentTemplate
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.Controls.DataTemplateSelector ContentTemplateSelector
        {
            get;
            set;
        }

        [PropertyKind(PropertyKind.DependencyPropertyOnly)]
        [NativeStorageType(ValueType.valueObject)]
        [PropertyFlags(HadFieldInBlue = true)]
        internal Microsoft.UI.Xaml.DataTemplate SelectedContentTemplate
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pContentTransitions")]
        [PropertyFlags(IsValueCreatedOnDemand = true)]
        public Microsoft.UI.Xaml.Media.Animation.TransitionCollection ContentTransitions
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_eFontSize")]
        public Windows.Foundation.Double FontSize
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_pFontFamily")]
        public Microsoft.UI.Xaml.Media.FontFamily FontFamily
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nFontWeight")]
        [CoreType(typeof(Windows.UI.Text.CoreFontWeight), NewCodeGenPropertyType = typeof(Windows.UI.Text.FontWeight))]
        public Windows.UI.Text.FontWeight FontWeight
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nFontStyle")]
        public Windows.UI.Text.FontStyle FontStyle
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nFontStretch")]
        public Windows.UI.Text.FontStretch FontStretch
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_nCharacterSpacing")]
        public Windows.Foundation.Int32 CharacterSpacing
        {
            get;
            set;
        }

        [PropertyFlags(IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTextFormatting")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_pForeground")]
        public Microsoft.UI.Xaml.Media.Brush Foreground
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.OpticalMarginAlignment OpticalMarginAlignment
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.TextLineBounds TextLineBounds
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true, IsInStorageGroup = true, IsValueInherited = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_pTextFormatting")]
        [StorageGroupNames("EnsureTextFormatting", "TextFormatting", "m_isTextScaleFactorEnabled")]
        public Windows.Foundation.Boolean IsTextScaleFactorEnabled
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.BrushTransition BackgroundTransition
        {
            get;
            set;
        }

        public ContentPresenter() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnContentTemplateChanged([Optional] Microsoft.UI.Xaml.DataTemplate oldContentTemplate, [Optional] Microsoft.UI.Xaml.DataTemplate newContentTemplate)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnContentTemplateSelectorChanged([Optional] Microsoft.UI.Xaml.Controls.DataTemplateSelector oldContentTemplateSelector, [Optional] Microsoft.UI.Xaml.Controls.DataTemplateSelector newContentTemplateSelector)
        {
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.TextWrapping TextWrapping
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueSigned)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Int32 MaxLines
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.LineStackingStrategy LineStackingStrategy
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueFloat)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double LineHeight
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CContentPresenter")]
        [RenderDirtyFlagMethodName("NWSetBorderBrushDirty")]
        public Microsoft.UI.Xaml.Media.Brush BorderBrush
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueThickness)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Thickness BorderThickness
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueCornerRadius)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.CornerRadius CornerRadius
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueThickness)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Thickness Padding
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CContentPresenter")]
        [RenderDirtyFlagMethodName("NWSetBackgroundDirty")]
        public Microsoft.UI.Xaml.Media.Brush Background
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [RenderDirtyFlagClassName("CContentPresenter")]
        [RenderDirtyFlagMethodName("NWSetBackgroundDirty")]
        public Microsoft.UI.Xaml.Controls.BackgroundSizing BackgroundSizing
        {
            get;
            set;
        }

        [PropertyFlags(AffectsArrange = true)]
        [NativeStorageType(ValueType.valueEnum)]
        public Microsoft.UI.Xaml.HorizontalAlignment HorizontalContentAlignment
        {
            get;
            set;
        }

        [PropertyFlags(AffectsArrange = true)]
        [NativeStorageType(ValueType.valueEnum)]
        public Microsoft.UI.Xaml.VerticalAlignment VerticalContentAlignment
        {
            get;
            set;
        }
    }

    [NativeName("CItemsPanelTemplate")]
    [Guids(ClassGuid = "421f4b9a-6327-4501-a8f2-1a53f8b0387d")]
    public sealed class ItemsPanelTemplate
     : Microsoft.UI.Xaml.FrameworkTemplate
    {
        public ItemsPanelTemplate() { }
    }

    [CodeGen(partial: true)]
    [NativeName("CItemsPresenter")]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.Primitives.IScrollInfo))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.Primitives.IScrollSnapPointsInfo))]
    [Guids(ClassGuid = "83545f9b-b339-4858-8756-f550c38f39af")]
    public sealed class ItemsPresenter
     : Microsoft.UI.Xaml.FrameworkElement
    {
        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Object Header
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.DataTemplate HeaderTemplate
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true, IsValueCreatedOnDemand = true)]
        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.Media.Animation.TransitionCollection HeaderTransitions
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Object Footer
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.DataTemplate FooterTemplate
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true, IsValueCreatedOnDemand = true)]
        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.Media.Animation.TransitionCollection FooterTransitions
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueThickness)]
        [OffsetFieldName("m_padding")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Thickness Padding
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pItemsPanelTemplate")]
        private Microsoft.UI.Xaml.Controls.ItemsPanelTemplate ItemsPanel
        {
            get;
            set;
        }

        public ItemsPresenter() { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "3b1f20e6-24b1-4cc0-9a93-b58354f86326")]
    public class SelectionChangedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CollectionType(CollectionKind.Vector)]
        public Microsoft.UI.Xaml.Controls.ItemCollection AddedItems
        {
            get;
            internal set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CollectionType(CollectionKind.Vector)]
        public Microsoft.UI.Xaml.Controls.ItemCollection RemovedItems
        {
            get;
            internal set;
        }

        internal SelectionChangedEventArgs() { }

        [FactoryMethodName("CreateInstanceWithRemovedItemsAndAddedItems")]
        public SelectionChangedEventArgs([CollectionType(CollectionKind.Vector)] Microsoft.UI.Xaml.Controls.ItemCollection removedItems, [CollectionType(CollectionKind.Vector)] Microsoft.UI.Xaml.Controls.ItemCollection addedItems) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CContextMenuEventArgs")]
    [Guids(ClassGuid = "d466a9ef-ec4f-4570-a8ad-918d31dd6965")]
    public sealed class ContextMenuEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_cursorLeft")]
        [DelegateToCore]
        public Windows.Foundation.Double CursorLeft
        {
            get;
            internal set;
        }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_cursorTop")]
        [DelegateToCore]
        public Windows.Foundation.Double CursorTop
        {
            get;
            internal set;
        }

        internal ContextMenuEventArgs() { }
    }

    [NativeName("CIsTextTrimmedChangedEventArgs")]
    [Guids(ClassGuid = "af32b2e1-9be5-4477-b529-55d4cfac5c01")]
    public sealed class IsTextTrimmedChangedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        internal IsTextTrimmedChangedEventArgs() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "f869cfa3-1e19-43a3-86f1-9de625522a6e")]
    public sealed class CleanUpVirtualizedItemEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Object Value
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.UIElement UIElement
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean Cancel
        {
            get;
            set;
        }

        internal CleanUpVirtualizedItemEventArgs() { }
    }

    [DXamlIdlGroup("Controls2")]
    [Platform(typeof(PrivateApiContract), 1)]
    public interface IListViewBasePrivate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void ScrollIntoViewWithOptionalAnimation(Windows.Foundation.Object item, Microsoft.UI.Xaml.Controls.ScrollIntoViewAlignment alignment, Windows.Foundation.Boolean disableAnimation);
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls")]
    [Comment("ListViewBase displays a rich, interactive collection of items.")]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ISemanticZoomInformation))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ITreeBuilder))]
    [Implements(typeof(Microsoft.Internal.FrameworkUdk.IBackButtonPressedListener))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IListViewBasePrivate))]
    [Guids(ClassGuid = "f65935c3-3e16-412f-8cc3-30d2456d9b4b")]
    public abstract class ListViewBase
     : Microsoft.UI.Xaml.Controls.Primitives.Selector
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DependencyPropertyModifier(Modifier.Private)]
        [CollectionType(CollectionKind.Vector)]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Controls.ItemCollection SelectedItems
        {
            get;
            private set;
        }

        public Microsoft.UI.Xaml.Controls.ListViewSelectionMode SelectionMode
        {
            get;
            set;
        }

        public Windows.Foundation.Boolean IsSwipeEnabled
        {
            get;
            set;
        }

        public Windows.Foundation.Boolean CanDragItems
        {
            get;
            set;
        }

        public Windows.Foundation.Boolean CanReorderItems
        {
            get;
            set;
        }

        public Windows.Foundation.Boolean IsItemClickEnabled
        {
            get;
            set;
        }

        public Windows.Foundation.Double DataFetchSize
        {
            get;
            set;
        }

        public Windows.Foundation.Double IncrementalLoadingThreshold
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Controls.IncrementalLoadingTrigger IncrementalLoadingTrigger
        {
            get;
            set;
        }

        [OrderHint(1)]
        public Windows.Foundation.Object Header
        {
            get;
            set;
        }

        [OrderHint(1)]
        public Microsoft.UI.Xaml.DataTemplate HeaderTemplate
        {
            get;
            set;
        }

        [OrderHint(1)]
        [PropertyFlags(IsExcludedFromVisualTree = true, IsValueCreatedOnDemand = true)]
        public Microsoft.UI.Xaml.Media.Animation.TransitionCollection HeaderTransitions
        {
            get;
            set;
        }

        [OrderHint(1)]
        public Windows.Foundation.Object Footer
        {
            get;
            set;
        }

        [OrderHint(1)]
        public Microsoft.UI.Xaml.DataTemplate FooterTemplate
        {
            get;
            set;
        }

        [OrderHint(1)]
        [PropertyFlags(IsExcludedFromVisualTree = true, IsValueCreatedOnDemand = true)]
        public Microsoft.UI.Xaml.Media.Animation.TransitionCollection FooterTransitions
        {
            get;
            set;
        }

        public event Microsoft.UI.Xaml.Controls.ItemClickEventHandler ItemClick;

        public event Microsoft.UI.Xaml.Controls.DragItemsStartingEventHandler DragItemsStarting;

        [TypeTable(IsExcludedFromCore = true)]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.DragItemsCompletedEventHandler DragItemsCompleted;

        [TypeTable(IsExcludedFromCore = true)]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.ContainerContentChangingEventHandler ContainerContentChanging;

        [TypeTable(IsExcludedFromCore = true)]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.ChoosingItemContainerEventHandler ChoosingItemContainer;

        [TypeTable(IsExcludedFromCore = true)]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.ChoosingGroupHeaderContainerEventHandler ChoosingGroupHeaderContainer;

        public ListViewBase() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlOverloadName("ScrollIntoView")]
        public void ScrollIntoView([Optional] Windows.Foundation.Object item)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SelectAll()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.IAsyncOperation<Microsoft.UI.Xaml.Data.LoadMoreItemsResult> LoadMoreItemsAsync()
        {
            return default(Windows.Foundation.IAsyncOperation<Microsoft.UI.Xaml.Data.LoadMoreItemsResult>);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlOverloadName("ScrollIntoView")]
        public void ScrollIntoViewWithAlignment(Windows.Foundation.Object item, Microsoft.UI.Xaml.Controls.ScrollIntoViewAlignment alignment)
        {
        }

        public bool ShowsScrollingPlaceholders
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetDesiredContainerUpdateDuration(Windows.Foundation.TimeSpan duration)
        {
        }

        public ListViewReorderMode ReorderMode { get; set; }

        [OptionalReturnValue]
        internal Windows.Foundation.String GetRelativeScrollPosition(ListViewItemToKeyHandler itemToKeyProvider)
        {
            return default(Windows.Foundation.String);
        }

        internal Windows.Foundation.IAsyncAction SetRelativeScrollPositionAsync(Windows.Foundation.String relativeScrollPosition, ListViewKeyToItemHandler keyToItemProvider)
        {
            return default(Windows.Foundation.IAsyncAction);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SelectRange(Microsoft.UI.Xaml.Data.ItemIndexRange itemIndexRange)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void DeselectRange(Microsoft.UI.Xaml.Data.ItemIndexRange itemIndexRange)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DependencyPropertyModifier(Modifier.Private)]
        [ReadOnly]
        public Windows.Foundation.Collections.IVectorView<Microsoft.UI.Xaml.Data.ItemIndexRange> SelectedRanges
        {
            get;
            private set;
        }

        public bool IsMultiSelectCheckBoxEnabled
        {
            get;
            set;
        }

        [FieldBacked]
        public Windows.Foundation.Boolean SingleSelectionFollowsFocus
        {
            get;
            set;
        }

        public bool IsDragSource()
        {
            return default(bool);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.IAsyncOperation<bool> TryStartConnectedAnimationAsync(Microsoft.UI.Xaml.Media.Animation.ConnectedAnimation animation, object item, string elementName)
        {
            return default(Windows.Foundation.IAsyncOperation<bool>);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Microsoft.UI.Xaml.Media.Animation.ConnectedAnimation PrepareConnectedAnimation(string key, object item, string elementName)
        {
            return default(Microsoft.UI.Xaml.Media.Animation.ConnectedAnimation);
        }
    }

    [FrameworkTypePattern]
    [DXamlIdlGroup("Controls")]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum ListViewReorderMode
    {
        Disabled = 0,
        Enabled = 1,
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [Comment("Provides data for the ItemClick event.")]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "1ada909e-7ad1-469c-83bd-85c13a6f172c")]
    public sealed class ItemClickEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Object ClickedItem
        {
            get;
            internal set;
        }

        public ItemClickEventArgs() { }
    }

    internal interface IContainerContentChangingIterator
    {
        [PropertyKind(PropertyKind.PropertyOnly)]
        Windows.Foundation.UInt32 Size { get; }

        [PropertyKind(PropertyKind.PropertyOnly)]
        Microsoft.UI.Xaml.DependencyObject Current { get; }

        bool MoveNext();

        void Reset();
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    [Guids(ClassGuid = "0d8167a7-2068-4f26-8c4e-0ba30b29f895")]
    public sealed class ContainerContentChangingEventArgs
    {
        public ContainerContentChangingEventArgs() { }

        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Controls.Primitives.SelectorItem ItemContainer
        {
            get;
            internal set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public bool InRecycleQueue
        {
            get;
            internal set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public int ItemIndex
        {
            get;
            internal set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Object Item
        {
            get;
            internal set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public uint Phase
        {
            get;
            internal set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        internal bool WantsCallBack
        {
            get;
            set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        internal bool ContentShouldBeSet
        {
            get;
            set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public bool Handled
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("RegisterUpdateCallback")]
        [DXamlOverloadName("RegisterUpdateCallback")]
        public void RegisterUpdateCallback(Windows.Foundation.TypedEventHandler<ListViewBase, ContainerContentChangingEventArgs> callback)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("RegisterUpdateCallbackWithPhase")]
        [DXamlOverloadName("RegisterUpdateCallback")]
        public void RegisterUpdateCallback(uint callbackPhase, Windows.Foundation.TypedEventHandler<ListViewBase, ContainerContentChangingEventArgs> callback)
        {
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        internal Windows.Foundation.TypedEventHandler<ListViewBase, ContainerContentChangingEventArgs> Callback
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        internal void ResetLifetime() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    [DXamlIdlGroup("Controls")]
    [Guids(ClassGuid = "4b1eb00a-4953-41cf-af7b-bbc75c13c475")]
    public sealed class ChoosingItemContainerEventArgs
    {
        public ChoosingItemContainerEventArgs() { }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public int ItemIndex
        {
            get;
            internal set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Object Item
        {
            get;
            internal set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Controls.Primitives.SelectorItem ItemContainer
        {
            get;
            set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public bool IsContainerPrepared
        {
            get;
            set;
        }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    [DXamlIdlGroup("Controls")]
    [Guids(ClassGuid = "79c8a29d-92f8-4513-bc54-2ce43a60463c")]
    public sealed class ChoosingGroupHeaderContainerEventArgs
    {
        public ChoosingGroupHeaderContainerEventArgs() { }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Controls.ListViewBaseHeaderItem GroupHeaderContainer
        {
            get;
            set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public int GroupIndex
        {
            get;
            internal set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Object Group
        {
            get;
            internal set;
        }
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [Comment("Represents a selectable item in a ListView or GridView.")]
    [ClassFlags(IsHiddenFromIdl = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Media.Animation.ITransitionContextProvider))]
    [NativeName("CListViewBaseItem")]
    [Guids(ClassGuid = "d91e6ee3-8fa0-430b-a59f-93a24e9f89f4")]
    public abstract class ListViewBaseItem
     : Microsoft.UI.Xaml.Controls.Primitives.SelectorItem
    {
        [DependencyPropertyModifier(Modifier.Internal)]
        internal bool IsDraggable
        {
            get;
            private set;
        }

        public ListViewBaseItem() { }
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [Comment("Represents a selectable item in a GridView.")]
    [Guids(ClassGuid = "4b4d2c72-14ce-40eb-a0f5-268e8786b0f0")]
    public class GridViewItem
     : Microsoft.UI.Xaml.Controls.ListViewBaseItem
    {
        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Controls.Primitives.GridViewItemTemplateSettings TemplateSettings
        {
            get;
            private set;
        }

        public GridViewItem() { }
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [Comment("Represents a selectable item in a ListView.")]
    [Guids(ClassGuid = "59c7c92f-e577-477a-945b-e029a065c409")]
    public class ListViewItem
     : Microsoft.UI.Xaml.Controls.ListViewBaseItem
    {
        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Controls.Primitives.ListViewItemTemplateSettings TemplateSettings
        {
            get;
            private set;
        }

        public ListViewItem() { }
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [InstanceCountTelemetry]
    [Comment("ListView displays a a rich, interactive collection of vertically stacked items.")]
    [Guids(ClassGuid = "3edbb9c1-9a2e-45bc-96df-5591b1e1d689")]
    public class ListView
     : Microsoft.UI.Xaml.Controls.ListViewBase
    {
        public ListView() { }
    }

    [CodeGen(partial: true)]
    [InstanceCountTelemetry]
    [DXamlIdlGroup("Controls2")]
    [Comment("GridView displays a a rich, interactive collection of items in a tabular format.")]
    [Guids(ClassGuid = "26257cfc-b0dc-4319-b015-c7c526dd1e9e")]
    public class GridView
     : Microsoft.UI.Xaml.Controls.ListViewBase
    {
        public GridView() { }
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [Comment("WrapGrid provides the default layout experience for the GridView control.")]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IKeyboardNavigationPanel))]
    [Guids(ClassGuid = "7892a18c-f5ca-4485-8b11-a1cd6462aa5b")]
    public sealed class WrapGrid
     : Microsoft.UI.Xaml.Controls.Primitives.OrientedVirtualizingPanel
    {
        [Comment("The width of each item in the WrapGrid.  If unspecified, the width of the first item in the WrapGrid will be used for all other items.")]
        public Windows.Foundation.Double ItemWidth
        {
            get;
            set;
        }

        [Comment("The height of each item in the WrapGrid.  If unspecified, the height of the first item in the WrapGrid will be used for all other items.")]
        public Windows.Foundation.Double ItemHeight
        {
            get;
            set;
        }

        [Comment("The dimension in which the items are arranged.")]
        public Microsoft.UI.Xaml.Controls.Orientation Orientation
        {
            get;
            set;
        }

        [Comment("The horizontal alignment of the items.")]
        public Microsoft.UI.Xaml.HorizontalAlignment HorizontalChildrenAlignment
        {
            get;
            set;
        }

        [Comment("The vertical alignment of the items.")]
        public Microsoft.UI.Xaml.VerticalAlignment VerticalChildrenAlignment
        {
            get;
            set;
        }

        [Comment("The maximum number of items per row or column (depending on the Orientation of WrapGrid).")]
        public Windows.Foundation.Int32 MaximumRowsOrColumns
        {
            get;
            set;
        }

        public WrapGrid() { }
    }

    [CodeGen(partial: true)]
    [InstanceCountTelemetry]
    [Guids(ClassGuid = "59251340-a834-4f86-83db-f15ce55c7ea4")]
    public class ToolTip
     : Microsoft.UI.Xaml.Controls.ContentControl
    {
        public Windows.Foundation.Double HorizontalOffset
        {
            get;
            set;
        }

        public Windows.Foundation.Boolean IsOpen
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Controls.Primitives.PlacementMode Placement
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.UIElement PlacementTarget
        {
            get;
            set;
        }

        public Windows.Foundation.Rect? PlacementRect
        {
            get;
            set;
        }

        public Windows.Foundation.Double VerticalOffset
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Controls.Primitives.ToolTipTemplateSettings TemplateSettings
        {
            get;
            private set;
        }

        public event Microsoft.UI.Xaml.RoutedEventHandler Closed;

        public event Microsoft.UI.Xaml.RoutedEventHandler Opened;

        public ToolTip() { }
    }

    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1, ForcePrimaryInterfaceGeneration = true)]
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "2b87b411-c80a-467d-972f-6ae0e4b20f4e")]
    public static class ToolTipService
    {
        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public static Microsoft.UI.Xaml.Controls.Primitives.PlacementMode AttachedPlacement
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public static Microsoft.UI.Xaml.UIElement AttachedPlacementTarget
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        public static Windows.Foundation.Object AttachedToolTip
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        internal static Windows.Foundation.Object AttachedKeyboardAcceleratorToolTip
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        internal static Microsoft.UI.Xaml.Controls.ToolTip AttachedKeyboardAcceleratorToolTipObject
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.DependencyObject))]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        internal static Microsoft.UI.Xaml.Controls.ToolTip AttachedToolTipObject
        {
            get;
            set;
        }

    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "2da913d5-3a7b-4fab-9d39-d9b6aeb8f15c")]
    public sealed class DragItemsStartingEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean Cancel
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CollectionType(CollectionKind.Vector)]
        public Microsoft.UI.Xaml.Controls.ItemCollection Items
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.ApplicationModel.DataTransfer.DataPackage Data
        {
            get;
            internal set;
        }

        public DragItemsStartingEventArgs() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "31f1244f-4b3d-4ec9-bbca-1a8062ba5fdd")]
    public sealed class DragItemsCompletedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CollectionType(CollectionKind.Indexable)]
        public Microsoft.UI.Xaml.Controls.ItemCollection Items
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.ApplicationModel.DataTransfer.DataPackageOperation DropResult
        {
            get;
            internal set;
        }

        internal DragItemsCompletedEventArgs() { }
    }

    [CodeGen(partial: true)]
    [InstanceCountTelemetry]
    [NativeName("CSemanticZoom")]
    [Comment("Represents a scrollable area that can contain either a normal view of content or a jump view used to navigate around the content via zoom gestures.")]
    [ContentProperty("ZoomedInView")]
    [Implements(typeof(Microsoft.Internal.FrameworkUdk.IBackButtonPressedListener))]
    [Guids(ClassGuid = "ce486330-0cc2-400f-8a75-945c018a8506")]
    public sealed class SemanticZoom
     : Microsoft.UI.Xaml.Controls.Control
    {
        [Comment("Gets or sets the ISemanticZoomInformation view for the SemanticZoom's content.")]
        public Microsoft.UI.Xaml.Controls.ISemanticZoomInformation ZoomedInView
        {
            get;
            set;
        }

        [Comment("Gets or sets the ISemanticZoomInformation view for the SemanticZoom's jump navigation.")]
        public Microsoft.UI.Xaml.Controls.ISemanticZoomInformation ZoomedOutView
        {
            get;
            set;
        }

        [Comment("Gets or sets a value indicating whether the ZoomedInView is currently active.")]
        public Windows.Foundation.Boolean IsZoomedInViewActive
        {
            get;
            set;
        }

        [Comment("Gets or sets a value indicating whether the SemanticZoom allows changing between the ZoomedInView and the ZoomedOutView.  This can be used to lock the current view.")]
        public Windows.Foundation.Boolean CanChangeViews
        {
            get;
            set;
        }

        [Comment("Gets or sets a value indicating whether the ZoomOutButton is shown.")]
        [OrderHint(1)]
        public Windows.Foundation.Boolean IsZoomOutButtonEnabled
        {
            get;
            set;
        }

        public event Microsoft.UI.Xaml.Controls.SemanticZoomViewChangedEventHandler ViewChangeStarted;

        public event Microsoft.UI.Xaml.Controls.SemanticZoomViewChangedEventHandler ViewChangeCompleted;

        public SemanticZoom() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void ToggleActiveView()
        {
        }
    }

    [Guids(ClassGuid = "63ad9e38-bc86-4f8a-85e3-f8b38c351953")]
    public interface ISemanticZoomInformation
    {
        [PropertyFlags(DoNotEnterOrLeaveValue = true, IsExcludedFromVisualTree = true)]
        [Comment("Gets or sets the SemanticZoom that controls navigation behavior.")]
        Microsoft.UI.Xaml.Controls.SemanticZoom SemanticZoomOwner
        {
            get;
            set;
        }

        [Comment("Gets or sets a value indicating whether this view is the currently active view in the SemanticZoom.  Note that this value only represents the current state - changing it will not trigger a zoom.")]
        Windows.Foundation.Boolean IsActiveView
        {
            get;
            set;
        }

        [Comment("Gets or sets a value indicating whether this view is the ZoomedInView of the SemanticZoom.")]
        Windows.Foundation.Boolean IsZoomedInView
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void InitializeViewChange();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void CompleteViewChange();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void MakeVisible(Microsoft.UI.Xaml.Controls.SemanticZoomLocation item);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void StartViewChangeFrom(Microsoft.UI.Xaml.Controls.SemanticZoomLocation source, Microsoft.UI.Xaml.Controls.SemanticZoomLocation destination);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void StartViewChangeTo(Microsoft.UI.Xaml.Controls.SemanticZoomLocation source, Microsoft.UI.Xaml.Controls.SemanticZoomLocation destination);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void CompleteViewChangeFrom(Microsoft.UI.Xaml.Controls.SemanticZoomLocation source, Microsoft.UI.Xaml.Controls.SemanticZoomLocation destination);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void CompleteViewChangeTo(Microsoft.UI.Xaml.Controls.SemanticZoomLocation source, Microsoft.UI.Xaml.Controls.SemanticZoomLocation destination);
    }

    [CodeGen(partial: true)]
    [Comment("SemanticZoomLocation represents either a value or region of an ISemanticZoomInformation view.")]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "0c15333e-3fb2-4a64-8511-2a225f0fcca7")]
    public sealed class SemanticZoomLocation
     : Windows.Foundation.Object
    {
        [Comment("Gets a point that specifies the gesture center.")]
        [PropertyKind(PropertyKind.PropertyOnly)]
        internal Windows.Foundation.Point ZoomPoint
        {
            get;
            set;
        }

        [Comment("Gets or sets an item in an ISemanticZoomInformation view.")]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Object Item
        {
            get;
            set;
        }

        [Comment("Gets or sets a region in the ISemanticZoomInformation view.")]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Rect Bounds
        {
            get;
            set;
        }

        [Comment("Gets or sets the remainder of the translate that the destination view was not able to perform. SemanticZoom might react by putting a manual correction on the view")]
        [PropertyKind(PropertyKind.PropertyOnly)]
        internal Windows.Foundation.Rect Remainder
        {
            get;
            set;
        }

        public SemanticZoomLocation() { }
    }

    [Comment("Provides data about a zoom transition between source and destination ISemanticZoomInformation views.")]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "a5185bdc-746c-45d8-9e75-8d4afe80977d")]
    public sealed class SemanticZoomViewChangedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [Comment("Gets or sets a value indicating whether the source of the transition is the ZoomedInView.")]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean IsSourceZoomedInView
        {
            get;
            set;
        }

        [Comment("Gets or sets the item in the source ISemanticZoomInformation view that is being transitioned from.")]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Controls.SemanticZoomLocation SourceItem
        {
            get;
            set;
        }

        [Comment("Gets or sets the item in the destination ISemanticZoomInformation view that is being transitioned to.")]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Controls.SemanticZoomLocation DestinationItem
        {
            get;
            set;
        }

        public SemanticZoomViewChangedEventArgs() { }
    }

    [Comment("Provides data about the changes occurring on the HorizontalOffset, VerticalOffset and ZoomFactor dependency properties.")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "30bf2d44-d000-4989-9ff2-58f68dbea743")]
    public sealed class ScrollViewerViewChangingEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [Comment("Gets the target value for the HorizontalOffset dependency property.")]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Controls.ScrollViewerView NextView
        {
            get;
            internal set;
        }

        [Comment("Gets the predicted final value for the HorizontalOffset dependency property at the end of inertia.")]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Controls.ScrollViewerView FinalView
        {
            get;
            internal set;
        }

        [Comment("Gets a value indicating whether the event is raised during an inertial phase.")]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean IsInertial
        {
            get;
            internal set;
        }

        internal ScrollViewerViewChangingEventArgs() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "14cf88fc-9593-4494-b37d-186240b68ae3")]
    public sealed class AnchorRequestedEventArgs
        : Microsoft.UI.Xaml.EventArgs
    {
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.UIElement Anchor
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CollectionType(CollectionKind.Vector)]
        public UIElementCollection AnchorCandidates
        {
            get;
            internal set;
        }

        internal AnchorRequestedEventArgs() { }
    }

    [Comment("Provides data about the changes occurring on the HorizontalOffset, VerticalOffset and ZoomFactor dependency properties.")]
    [FrameworkTypePattern]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "dca6d049-a5f5-40bf-b6d2-b02218ebb73e")]
    public sealed class ScrollViewerViewChangedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [Comment("Gets a value indicating whether another ViewChanged notification may occur for the current operation.")]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean IsIntermediate
        {
            get;
            internal set;
        }

        public ScrollViewerViewChangedEventArgs() { }
    }

    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromCore = true)]
    [Guids(ClassGuid = "4558ca3d-5b46-42e6-a285-cc070d1b0aca")]
    internal sealed class AppBarLightDismiss
     : Microsoft.UI.Xaml.Controls.Grid
    {
        protected override Microsoft.UI.Xaml.Automation.Peers.AutomationPeer OnCreateAutomationPeer()
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationPeer);
        }
    }

    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromCore = true)]
    [Guids(ClassGuid = "189fc0fe-79c2-478f-a59c-3880e26ed873")]
    internal sealed class ComboBoxLightDismiss
     : Microsoft.UI.Xaml.Controls.Canvas
    {
        protected override Microsoft.UI.Xaml.Automation.Peers.AutomationPeer OnCreateAutomationPeer()
        {
            return default(Microsoft.UI.Xaml.Automation.Peers.AutomationPeer);
        }
    }

    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum IncrementalLoadingTrigger
    {
        None = 0,
        Edge = 1,
    }

    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true)]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    internal enum GroupHeaderStrategy
    {
        None = 0,
        Parallel = 1,
        Inline = 2,
    }

    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true)]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    internal enum RelativePosition
    {
        Before = 0,
        Inside = 1,
        After = 2,
    }

    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum PanelScrollingDirection
    {
        None = 0,
        Forward = 1,
        Backward = 2,
    }

    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true)]
    public enum ItemsUpdatingScrollMode
    {
        KeepItemsInView = 0,
        KeepScrollOffset = 1,
        KeepLastItemInView = 2
    }

    [NativeName("Orientation")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    public enum Orientation
    {
        [NativeValueName("OrientationVertical")]
        Vertical = 0,
        [NativeValueName("OrientationHorizontal")]
        Horizontal = 1,
    }

    [NativeName("StretchDirection")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    [NativeComment("Determines stretch direction of items in Viewbox.")]
    public enum StretchDirection
    {
        [NativeValueName("StretchDirectionUpOnly")]
        UpOnly = 0,
        [NativeValueName("StretchDirectionDownOnly")]
        DownOnly = 1,
        [NativeValueName("StretchDirectionBoth")]
        Both = 2,
    }

    [Comment("Specifies the visibility of a ScrollBar for scrollable content.")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum ScrollBarVisibility
    {
        [NativeComment("A ScrollBar does not appear even when the viewport cannot display all of the content. The dimension of the content is set to the corresponding dimension of the ScrollViewer parent. For a horizontal ScrollBar, the width of the content is set to the ViewportWidth of the ScrollViewer. For a vertical ScrollBar, the height of the content is set to the ViewportHeight of the ScrollViewer.")]
        [NativeValueName("ScrollBarVisibilityDisabled")]
        Disabled = 0,
        [NativeComment("A ScrollBar appears and the dimension of the ScrollViewer is applied to the content when the viewport cannot display all of the content. For a horizontal ScrollBar, the width of the content is set to the ViewportWidth of the ScrollViewer. For a vertical ScrollBar, the height of the content is set to the ViewportHeight of the ScrollViewer.")]
        [NativeValueName("ScrollBarVisibilityAuto")]
        Auto = 1,
        [NativeComment("A ScrollBar does not appear even when the viewport cannot display all of the content. The dimension of the ScrollViewer is not applied to the content.")]
        [NativeValueName("ScrollBarVisibilityHidden")]
        Hidden = 2,
        [NativeComment("A ScrollBar always appears. The dimension of the ScrollViewer is applied to the content. For a horizontal ScrollBar, the width of the content is set to the ViewportWidth of the ScrollViewer. For a vertical ScrollBar, the height of the content is set to the ViewportHeight of the ScrollViewer.")]
        [NativeValueName("ScrollBarVisibilityVisible")]
        Visible = 3,
    }

    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum ClickMode
    {
        [NativeValueName("ClickModeRelease")]
        Release = 0,
        [NativeValueName("ClickModePress")]
        Press = 1,
        [NativeValueName("ClickModeHover")]
        Hover = 2,
    }

    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum SelectionMode
    {
        Single = 0,
        Multiple = 1,
        Extended = 2,
    }

    [Comment("Enumeration that specifies the virtualization mode of the VirtualizingStackPanel. Used by VirtualizationModeProperty.")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum VirtualizationMode
    {
        [NativeComment("Standard virtualization mode -- containers are thrown away when offscreen.")]
        [NativeValueName("VirtualizationModeStandard")]
        Standard = 0,
        [NativeComment("Recycling virtualization mode -- containers are re-used when offscreen.")]
        [NativeValueName("VirtualizationModeRecycling")]
        Recycling = 1,
    }

    [Comment("Specifies the selection behavior for a ListView.")]
    [DXamlIdlGroup("Controls")]
    [FrameworkTypePattern]
    public enum ListViewSelectionMode
    {
        None = 0,
        Single = 1,
        Multiple = 2,
        Extended = 3,
    }

    [Comment("Enumeration that specifies a scrolling mode for a ScrollViewer control.")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum ScrollMode
    {
        [NativeComment("Scrolling is turned off.")]
        [NativeValueName("ScrollModeDisabled")]
        Disabled = 0,
        [NativeComment("Scrolling is turned on.")]
        [NativeValueName("ScrollModeEnabled")]
        Enabled = 1,
        [NativeComment("Scrolling is turned on or off depending on the content size.")]
        [NativeValueName("ScrollModeAuto")]
        Auto = 2,
    }

    [Comment("Enumeration that specifies the zoom mode for a ScrollViewer control.")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true)]
    public enum ZoomMode
    {
        [NativeComment("Zoom is turned off.")]
        [NativeValueName("ZoomModeDisabled")]
        Disabled = 0,
        [NativeComment("Zoom is turned on.")]
        [NativeValueName("ZoomModeEnabled")]
        Enabled = 1,
    }

    [Comment("Determines the type of snap points.")]
    [NativeName("XcpSnapPointsType")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true, IsTypeConverter = true)]
    [NativeComment("Determines the type of snap points.")]
    public enum SnapPointsType
    {
        [NativeComment("Do not expose any snap points.")]
        [NativeValueName("XcpSnapPointsTypeNone")]
        None = 0,
        [NativeComment("Snap points are optional.")]
        [NativeValueName("XcpSnapPointsTypeOptional")]
        Optional = 1,
        [NativeComment("Snap points are mandatory.")]
        [NativeValueName("XcpSnapPointsTypeMandatory")]
        Mandatory = 2,
        [NativeComment("Snap points are optional and cannot be jumped over.")]
        [NativeValueName("XcpSnapPointsTypeOptionalSingle")]
        OptionalSingle = 3,
        [NativeComment("Snap points are mandatory and cannot be jumped over.")]
        [NativeValueName("XcpSnapPointsTypeMandatorySingle")]
        MandatorySingle = 4,
    }

    [Comment("Determines the alignment of ScrollIntoView method.")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true, IsTypeConverter = true)]
    public enum ScrollIntoViewAlignment
    {
        Default = 0,
        Leading = 1,
    }

    [StubDelegate]
    [DXamlIdlGroup("Controls2")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void ContentDialogOpenedEventHandler(Microsoft.UI.Xaml.Controls.ContentDialog sender, Microsoft.UI.Xaml.Controls.ContentDialogOpenedEventArgs e);

    [StubDelegate]
    [DXamlIdlGroup("Controls2")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void ContentDialogClosedEventHandler(Microsoft.UI.Xaml.Controls.ContentDialog sender, Microsoft.UI.Xaml.Controls.ContentDialogClosedEventArgs e);

    [StubDelegate]
    [DXamlIdlGroup("Controls2")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void ContentDialogClosingEventHandler(Microsoft.UI.Xaml.Controls.ContentDialog sender, Microsoft.UI.Xaml.Controls.ContentDialogClosingEventArgs e);

    [StubDelegate]
    [DXamlIdlGroup("Controls2")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void ContentDialogButtonClickEventHandler(Microsoft.UI.Xaml.Controls.ContentDialog sender, Microsoft.UI.Xaml.Controls.ContentDialogButtonClickEventArgs e);

    public delegate void TextChangedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.TextChangedEventArgs e);

    [StubDelegate]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void FocusEngagedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.FocusEngagedEventArgs e);

    [StubDelegate]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void FocusDisengagedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.FocusDisengagedEventArgs e);

    public delegate void SelectionChangedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs e);

    public delegate void ContextMenuOpeningEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.ContextMenuEventArgs e);

    public delegate void TextControlPasteEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.TextControlPasteEventArgs e);

    public delegate void CleanUpVirtualizedItemEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.CleanUpVirtualizedItemEventArgs e);

    public delegate void ItemClickEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.ItemClickEventArgs e);

    public delegate void DragItemsStartingEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.DragItemsStartingEventArgs e);

    [StubDelegate]
    [DXamlIdlGroup("Controls2")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void DragItemsCompletedEventHandler(Microsoft.UI.Xaml.Controls.ListViewBase sender, Microsoft.UI.Xaml.Controls.DragItemsCompletedEventArgs e);

    [StubDelegate]
    [DXamlIdlGroup("Controls2")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [Guids(ClassGuid = "c744dc00-747d-4a30-a8cd-b48b77494039")]
    public delegate void ContainerContentChangingEventHandler(Microsoft.UI.Xaml.Controls.ListViewBase sender, Microsoft.UI.Xaml.Controls.ContainerContentChangingEventArgs e);

    [StubDelegate]
    [DXamlIdlGroup("Controls2")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [Guids(ClassGuid = "22b8408b-7dc7-462f-bd1f-b4cd2fb08ff0")]
    public delegate void ChoosingItemContainerEventHandler(Microsoft.UI.Xaml.Controls.ListViewBase sender, Microsoft.UI.Xaml.Controls.ChoosingItemContainerEventArgs e);

    [StubDelegate]
    [DXamlIdlGroup("Controls2")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [Guids(ClassGuid = "19128f0b-b689-4992-963c-31299e914e50")]
    public delegate void ChoosingGroupHeaderContainerEventHandler(Microsoft.UI.Xaml.Controls.ListViewBase sender, Microsoft.UI.Xaml.Controls.ChoosingGroupHeaderContainerEventArgs e);

    [StubDelegate]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [DXamlIdlGroup("Controls2")]
    public delegate void TimeChangedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.TimePickerValueChangedEventArgs e);

    [StubDelegate]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [DXamlIdlGroup("Controls2")]
    public delegate void SelectedTimeChangedEventHandler(Microsoft.UI.Xaml.Controls.TimePicker sender, Microsoft.UI.Xaml.Controls.TimePickerSelectedValueChangedEventArgs e);

    public delegate void SemanticZoomViewChangedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.SemanticZoomViewChangedEventArgs e);

    [StubDelegate]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void ScrollViewerViewChangingEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.ScrollViewerViewChangingEventArgs e);

    [StubDelegate]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void ScrollViewerViewChangedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.ScrollViewerViewChangedEventArgs e);

    [StubDelegate]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [DXamlIdlGroup("Controls2")]
    public delegate void CompositionScaleChangedEventHandler(Windows.Foundation.Object sender, Windows.Foundation.Object e);

    [NativeName("CScrollContentControl")]
    [ContentProperty("Content")]
    [ClassFlags(IsHiddenFromIdl = true)]
    [Guids(ClassGuid = "db1648f9-5f53-464a-99da-9729521965f3")]
    public class ScrollContentControl
     : Microsoft.UI.Xaml.Controls.ContentControl
    {
    }

    [FrameworkTypePattern]
    [DXamlIdlGroup("Controls2")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1, ForcePrimaryInterfaceGeneration = true)]
    [Guids(ClassGuid = "2fcdcdae-6b96-4429-aebb-bbb67b502534")]
    public static class ListViewPersistenceHelper
    {
        public static Windows.Foundation.String GetRelativeScrollPosition(ListViewBase listViewBase, ListViewItemToKeyHandler itemToKeyHandler)
        {
            return default(Windows.Foundation.String);
        }

        public static Windows.Foundation.IAsyncAction SetRelativeScrollPositionAsync(ListViewBase listViewBase, Windows.Foundation.String relativeScrollPosition, ListViewKeyToItemHandler keyToItemHandler)
        {
            return default(Windows.Foundation.IAsyncAction);
        }
    }

    [DXamlIdlGroup("Controls2")]
    public delegate string ListViewItemToKeyHandler(Windows.Foundation.Object item);

    [DXamlIdlGroup("Controls2")]
    public delegate Windows.Foundation.IAsyncOperation<Windows.Foundation.Object> ListViewKeyToItemHandler(string key);

    [DXamlIdlGroup("Main")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    public enum LightDismissOverlayMode
    {
        Auto = 0,
        On = 1,
        Off = 2
    }

    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    public enum CharacterCasing
    {
        Normal = 0,
        Lower = 1,
        Upper = 2
    }

    [VelocityFeature("Feature_InputValidation")]
    public enum InputValidationKind
    {
        Auto,
        Compact,
        Inline,
    }

    [VelocityFeature("Feature_InputValidation")]
    public enum InputValidationMode
    {
        Auto,
        Default,
        Disabled
    }

    [Guids(ClassGuid = "95d4d3ce-d079-4901-830a-6994cf15e371")]
    [VelocityFeature("Feature_InputValidation")]
    public sealed class InputValidationErrorEventArgs : EventArgs
    {
        internal InputValidationErrorEventArgs(InputValidationErrorEventAction action, InputValidationError error) { }

        [DelegateToCore]
        public InputValidationErrorEventAction Action
        {
            get;
            internal set;
        }

        [DelegateToCore]
        public InputValidationError Error
        {
            get;
            internal set;
        }
    }

    [Guids(ClassGuid = "c12514d2-bca2-4286-bad3-50054edce5b0")]
    public sealed class HasValidationErrorsChangedEventArgs : EventArgs
    {
        internal HasValidationErrorsChangedEventArgs(bool newValue) { }

        [DelegateToCore]
        public bool NewValue
        {
            get;
            internal set;
        }
    }

    [Platform(typeof(PrivateApiContract), 1)]
    [Guids(ClassGuid = "4a8b17ef-c73f-428c-a646-81f02a300901")]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [NativeName("CInputValidationCommand")]
    [VelocityFeature("Feature_InputValidation")]
    public class InputValidationCommand : Microsoft.UI.Xaml.DependencyObject
    {
        [FieldBacked]
        [DependencyPropertyModifier(Modifier.Internal)]
        public InputValidationMode InputValidationMode
        {
            get;
            set;
        }

        [FieldBacked]
        [DependencyPropertyModifier(Modifier.Internal)]
        public InputValidationKind InputValidationKind
        {
            get;
            set;
        }

        public virtual bool CanValidate(IInputValidationControl validationControl) { return default(bool); }
        public virtual void Validate(IInputValidationControl validationControl) { }
    }

    [DXamlIdlGroup("Main")]
    [VelocityFeature("Feature_InputValidation")]
    public enum InputValidationErrorEventAction
    {
        Added,
        Removed
    }

    [Guids(ClassGuid = "2d40157d-ce2d-453a-ad35-afa5ea1997b8")]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [VelocityFeature("Feature_InputValidation")]
    public class InputValidationError
    {
        [PropertyKind(PropertyKind.PropertyOnly)]
        public string ErrorMessage
        {
            get;
            private set;
        }

        [FactoryMethodName("CreateInstance")]
        public InputValidationError(Windows.Foundation.String errorMessage) { }
    }

    [Guids(ClassGuid = "9ec15c00-97d1-4125-b0d8-7ac11ae0791e")]
    [VelocityFeature("Feature_InputValidation")]
    public class InputValidationContext
    {
        public InputValidationContext(string memberName, bool isRequired)
        {
        }

        public bool IsInputRequired
        {
            get;
            private set;
        }

        public string MemberName
        {
            get;
            private set;
        }
    }

    [VelocityFeature("Feature_InputValidation")]
    public interface IInputValidationControl
    {
        [CollectionType(CollectionKind.Observable)]
        [PropertyFlags(IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Internal)]
        Microsoft.UI.Xaml.Internal.ValidationErrorsCollection ValidationErrors
        {
            get;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        bool HasValidationErrors
        {
            get;
        }

        [NativeName("HasValidationErrorsChanged")]
        event Windows.Foundation.TypedEventHandler<IInputValidationControl, HasValidationErrorsChangedEventArgs> HasValidationErrorsChanged;

        [DependencyPropertyModifier(Modifier.Internal)]
        InputValidationContext ValidationContext
        {
            get;
            set;
        }

        Microsoft.UI.Xaml.DataTemplate ErrorTemplate
        {
            get;
            set;
        }

        InputValidationMode InputValidationMode
        {
            get;
            set;
        }

        InputValidationKind InputValidationKind
        {
            get;
            set;
        }

        [EventFlags(UseEventManager = true)]
        event Windows.Foundation.TypedEventHandler<IInputValidationControl, InputValidationErrorEventArgs> ValidationError;
    }

    [Platform(typeof(PrivateApiContract), 1)]
    [VelocityFeature("Feature_InputValidation")]
    public interface IInputValidationControl2
    {
        InputValidationCommand ValidationCommand
        {
            get;
            set;
        }
    }

    [global::System.AttributeUsage(global::System.AttributeTargets.Class, AllowMultiple = false, Inherited = true)]
    [IdlAttributeTarget(global::System.AttributeTargets.Class)]
    public class InputPropertyAttribute : global::System.Attribute
    {
        public InputPropertyAttribute(string name)
        {
            Name = name;
        }
        public string Name { get; set; }
    }

}


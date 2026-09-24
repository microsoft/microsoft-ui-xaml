using System;
using Microsoft.UI.Xaml;
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml.Controls;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace XAMLPerf.ControlsApp.Cs.MUX
{
    public class Item
    {
        public Item(string mountain, uint height_m, string range, uint prominence_m, string parent_mountain)
        {
            Mountain = mountain;
            Height_m = height_m;
            Range = range;
            Prominence_m = prominence_m;
            Parent_mountain = parent_mountain;
        }

        public string Mountain { get; }
        public uint Height_m { get; }
        public string Range { get; }
        public uint Prominence_m { get; }
        public string Parent_mountain { get; }
    }

    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MUXCPage : Page
    {
        private DispatcherTimer timer = new DispatcherTimer();
        private uint timerState = 0;

        private ObservableCollection<Item> Items = new ObservableCollection<Item>();
        private TeachingTip teachingTip;

        public MUXCPage()
        {
            InitializeComponent();

            PopulateData();
            teachingTip = (TeachingTip)navView.Resources["teachingTip"];

            timer.Interval = TimeSpan.FromSeconds(1.0);
            timer.Tick += Timer_Tick;
            timer.Start();
        }

        private void Timer_Tick(object sender, object e)
        {
            switch (timerState)
            {
                case 0:
                    teachingTip.IsOpen = true;
                    break;

                case 1:
                    teachingTip.IsOpen = false;
                    break;

                case 2:
                    navView.IsPaneOpen = false;
                    navView.IsSettingsVisible = false;
                    navView.IsBackButtonVisible = NavigationViewBackButtonVisible.Visible;
                    break;
            }

            progressBar.Value = timerState;

            ++timerState;
        }

        private void PopulateData()
        {
            Items.Add(new Item("Mount Everest", 8848, "Mahalangur Himalaya", 8848, "none"));
            Items.Add(new Item("K2/Qogir", 8611, "Baltoro Karakoram", 4017, "Mount Everest"));
            Items.Add(new Item("Kangchenjunga", 8586, "Kangchenjunga Himalaya", 3922, "Mount Everest"));
            Items.Add(new Item("Lhotse", 8516, "Mahalangur Himalaya", 610, "Mount Everest"));
            Items.Add(new Item("Makalu", 8485, "Mahalangur Himalaya", 2386, "Mount Everest"));
            Items.Add(new Item("Cho Oyu", 8188, "Mahalangur Himalaya", 2340, "Mount Everest"));
            Items.Add(new Item("Dhaulagiri I", 8167, "Dhaulagiri Himalaya", 3357, "K2"));
            Items.Add(new Item("Manaslu", 8163, "Manaslu Himalaya", 3092, "Cho Oyu"));
            Items.Add(new Item("Nanga Parbat", 8126, "Nanga Parbat Himalaya", 4608, "Dhaulagiri"));
            Items.Add(new Item("Annapurna I", 8091, "Annapurna Himalaya", 2984, "Cho Oyu"));
            Items.Add(new Item("Gasherbrum I", 8080, "Baltoro Karakoram", 2155, "K2"));
            Items.Add(new Item("Broad Peak/K3", 8051, "Baltoro Karakoram", 1701, "Gasherbrum I"));
            Items.Add(new Item("Gasherbrum II/K4", 8034, "Baltoro Karakoram", 1523, "Gasherbrum I"));
            Items.Add(new Item("Shishapangma", 8027, "Jugal Himalaya", 2897, "Cho Oyu"));
            Items.Add(new Item("Gyachung Kang", 7952, "Mahalangur Himalaya", 700, "Cho Oyu"));
            Items.Add(new Item("Gasherbrum III", 7946, "Baltoro Karakoram", 355, "Gasherbrum II"));
            Items.Add(new Item("Annapurna II", 7937, "Annapurna Himalaya", 2437, "Annapurna I"));
            Items.Add(new Item("Gasherbrum IV", 7932, "Baltoro Karakoram", 715, "Gasherbrum III"));
            Items.Add(new Item("Himalchuli", 7893, "Manaslu Himalaya", 1633, "Manaslu"));
            Items.Add(new Item("Distaghil Sar", 7884, "Hispar Karakoram", 2525, "K2"));
            Items.Add(new Item("Ngadi Chuli", 7871, "Manaslu Himalaya", 1020, "Manaslu"));
            Items.Add(new Item("Nuptse", 7864, "Mahalangur Himalaya", 319, "Lhotse"));
            Items.Add(new Item("Khunyang Chhish", 7823, "Hispar Karakoram", 1765, "Distaghil Sar"));
            Items.Add(new Item("Masherbrum/K1", 7821, "Masherbrum Karakoram", 2457, "Gasherbrum I"));
            Items.Add(new Item("Nanda Devi", 7816, "Garhwal Himalaya", 3139, "Dhaulagiri"));
            Items.Add(new Item("Chomo Lonzo", 7804, "Mahalangur Himalaya", 590, "Makalu"));
            Items.Add(new Item("Batura Sar", 7795, "Batura Karakoram", 3118, "Distaghil Sar"));
            Items.Add(new Item("Kanjut Sar", 7790, "Hispar Karakoram", 1690, "Khunyang Chhish"));
            Items.Add(new Item("Rakaposhi", 7788, "Rakaposhi-Haramosh Karakoram", 2818, "Khunyang Chhish"));
            Items.Add(new Item("Namcha Barwa", 7782, "Assam Himalaya", 4106, "Kangchenjunga"));
            Items.Add(new Item("Kamet", 7756, "Garhwal Himalaya", 2825, "Nanda Devi"));
            Items.Add(new Item("Dhaulagiri II", 7751, "Dhaulagiri Himalaya", 2396, "Dhaulagiri"));
            Items.Add(new Item("Saltoro Kangri/K10", 7742, "Saltoro Karakoram", 2160, "Gasherbrum I"));
            Items.Add(new Item("Jannu", 7711, "Kangchenjunga Himalaya", 1036, "Kangchenjunga"));
            Items.Add(new Item("Tirich Mir", 7708, "Hindu Kush", 3910, "Batura Sar"));
            Items.Add(new Item("Molamenqing", 7703, "Langtang Himalaya", 430, "Shishapangma"));
            Items.Add(new Item("Gurla Mandhata", 7694, "Nalakankar Himalaya", 2788, "Dhaulagiri"));
            Items.Add(new Item("Saser Kangri I/K22", 7672, "Saser Karakoram", 2304, "Gasherbrum I"));
            Items.Add(new Item("Chogolisa", 7665, "Masherbrum Karakoram", 1624, "Masherbrum"));
            Items.Add(new Item("Dhaulagiri IV", 7661, "Dhaulagiri Himalaya", 469, "Dhaulagiri II"));
            Items.Add(new Item("Kongur Tagh", 7649, "Kongur Shan Kunlun", 3585, "Distaghil Sar"));
            Items.Add(new Item("Dhaulagiri V", 7618, "Dhaulagiri Himalaya", 340, "Dhaulagiri IV"));
            Items.Add(new Item("Shispare", 7611, "Batura Karakoram", 1240, "Batura Sar"));
            Items.Add(new Item("Trivor", 7577, "Hispar Karakoram", 980, "Distaghil Sar"));
            Items.Add(new Item("Gangkhar Puensum", 7570, "Kula Kangri Himalaya", 2995, "Kangchenjunga"));
            Items.Add(new Item("Gongga Shan", 7556, "Daxue Shan", 3642, "Mount Everest"));
            Items.Add(new Item("Annapurna III", 7555, "Annapurna Himalaya", 703, "Annapurna I"));
            Items.Add(new Item("Muztagh Ata", 7546, "Muztagata Kunlun", 2735, "Kongur Tagh"));
            Items.Add(new Item("Skyang Kangri", 7545, "Baltoro Karakoram", 1085, "K2"));
            Items.Add(new Item("Changtse", 7543, "Mahalangur Himalaya", 520, "Mount Everest"));
            Items.Add(new Item("Kula Kangri", 7538, "Kula Kangri Himalaya", 1650, "Gangkhar Puensum"));
            Items.Add(new Item("Kongur Tiube", 7530, "Kongur Shan Kunlun", 840, "Kongur Tagh"));
            Items.Add(new Item("Mamostong Kangri", 7516, "Rimo Karakoram", 1803, "Gasherbrum I"));
            Items.Add(new Item("Saser Kangri II E", 7513, "Saser Karakoram", 1450, "Saser Kangri I"));
            Items.Add(new Item("Ismoil Somoni Peak", 7495, "Pamir (Akademiya Nauk Range)", 3402, "Muztagh Ata"));
            Items.Add(new Item("Saser Kangri III", 7495, "Saser Karakoram", 850, "Saser Kangri I"));
            Items.Add(new Item("Noshaq", 7492, "Hindu Kush", 2024, "Tirich Mir"));
            Items.Add(new Item("Pumari Chhish", 7492, "Hispar Karakoram", 890, "Khunyang Chhish"));
            Items.Add(new Item("Pasu Sar", 7476, "Batura Karakoram", 645, "Batura Sar"));
            Items.Add(new Item("Yukshin Gardan Sar", 7469, "Hispar Karakoram", 1313, "Pumari Chhish"));
            Items.Add(new Item("Teram Kangri I", 7462, "Siachen Karakoram", 1702, "Gasherbrum I"));
            Items.Add(new Item("Jongsong Peak", 7462, "Kangchenjunga Himalaya", 1298, "Kangchenjunga"));
            Items.Add(new Item("Malubiting", 7458, "Rakaposhi-Haramosh Karakoram", 2193, "Rakaposhi"));
            Items.Add(new Item("Gangapurna", 7455, "Annapurna Himalaya", 563, "Annapurna III"));
            Items.Add(new Item("Jengish Chokusu", 7439, "Tian Shan", 4148, "Ismail Samani Peak"));
            Items.Add(new Item("K12", 7428, "Saltoro Karakoram", 1978, "Saltoro Kangri"));
            Items.Add(new Item("Yangra", 7422, "Ganesh Himalaya", 2352, "Manaslu"));
            Items.Add(new Item("Sia Kangri", 7422, "Siachen Karakoram", 640, "Gasherbrum I"));
            Items.Add(new Item("Momhil Sar", 7414, "Hispar Karakoram", 980, "Trivor"));
            Items.Add(new Item("Kabru N", 7412, "Kangchenjunga Himalaya", 780, "Kangchenjunga"));
            Items.Add(new Item("Skil Brum", 7410, "Baltoro Karakoram", 1152, "K2"));
            Items.Add(new Item("Haramosh", 7409, "Rakaposhi Karakoram", 2277, "Malubiting"));
            Items.Add(new Item("Istor-o-Nal", 7403, "Hindu Kush", 1040, "Noshaq"));
            Items.Add(new Item("Ghent Kangri", 7401, "Saltoro Karakoram", 1493, "Saltoro Kangri"));
            Items.Add(new Item("Ultar Sar", 7388, "Batura Karakoram", 700, "Shispare"));
            Items.Add(new Item("Rimo I", 7385, "Rimo Karakoram", 1438, "Teram Kangri I"));
            Items.Add(new Item("Churen Himal", 7385, "Dhaulagiri Himalaya", 600, "Dhaulagiri IV"));
            Items.Add(new Item("Teram Kangri III", 7382, "Siachen Karakoram", 520, "Teram Kangri I"));
            Items.Add(new Item("Sherpi Kangri", 7380, "Saltoro Karakoram", 1000, "Ghent Kangri"));
            Items.Add(new Item("Labuche Kang", 7367, "Labuche Himalaya", 1957, "Cho Oyu"));
            Items.Add(new Item("Kirat Chuli", 7362, "Kangchenjunga Himalaya", 1168, "Kangchenjunga"));
            Items.Add(new Item("Abi Gamin", 7355, "Garhwal Himalaya", 217, "Kamet"));
            Items.Add(new Item("Nangpai Gosum", 7350, "Mahalangur Himalaya", 500, "Cho Oyu"));
            Items.Add(new Item("Gimmigela", 7350, "Kangchenjunga Himalaya", 432, "Kangchenjunga"));
            Items.Add(new Item("Saraghrar", 7349, "Hindu Kush", 1979, "Noshaq"));
            Items.Add(new Item("Jomolhari", 7326, "Jomolhari Himalaya", 2077, "Gangkhar Puensum"));
            Items.Add(new Item("Chamlang", 7321, "Mahalangur Himalaya", 1240, "Lhotse"));
            Items.Add(new Item("Chongtar", 7315, "Baltoro Karakoram", 1300, "Skil Brum"));
            Items.Add(new Item("Baltoro Kangri", 7312, "Masherbrum Karakoram", 1200, "Chogolisa"));
            Items.Add(new Item("Siguang Ri", 7309, "Mahalangur Himalaya", 650, "Cho Oyu"));
            Items.Add(new Item("The Crown", 7295, "Yengisogat Karakoram", 1919, "Skil Brum (K2)"));
            Items.Add(new Item("Gyala Peri", 7294, "Assam Himalaya", 2942, "Mount Everest"));
            Items.Add(new Item("Porong Ri", 7292, "Langtang Himalaya", 520, "Shisha Pangma"));
            Items.Add(new Item("Baintha Brakk", 7285, "Panmah Karakoram", 1891, "Kanjut Sar"));
            Items.Add(new Item("Yutmaru Sar", 7283, "Hispar Karakoram", 620, "Yukshin Gardan Sar"));
            Items.Add(new Item("Baltistan Peak/K6", 7282, "Masherbrum Karakoram", 1962, "Chogolisa"));
            Items.Add(new Item("Kangpenqing", 7281, "Baiku Himalaya", 1340, "Shisha Pangma"));
            Items.Add(new Item("Muztagh Tower", 7276, "Baltoro Karakoram", 1710, "Skil Brum"));
            Items.Add(new Item("Mana", 7272, "Garhwal Himalaya", 730, "Kamet"));
            Items.Add(new Item("Dhaulagiri VI", 7268, "Dhaulagiri Himalaya", 485, "Dhaulagiri IV"));
            Items.Add(new Item("Diran", 7266, "Rakaposhi-Haramosh Karakoram", 1325, "Malubiting"));
            Items.Add(new Item("Labuche Kang III/East[12]", 7250, "Labuche Himalaya", 570, "Labuche Kang"));
            Items.Add(new Item("Putha Hiunchuli", 7246, "Dhaulagiri Himalaya", 1151, "Churen Himal"));
            Items.Add(new Item("Apsarasas Kangri", 7245, "Siachen Karakoram", 635, "Teram Kangri I"));
            Items.Add(new Item("Mukut Parbat", 7242, "Garhwal Himalaya", 840, "Kamet"));
            Items.Add(new Item("Rimo III", 7233, "Rimo Karakoram", 615, "Rimo I"));
            Items.Add(new Item("Langtang Lirung", 7227, "Langtang Himalaya", 1525, "Shisha Pangma"));
            Items.Add(new Item("Karjiang", 7221, "Kula Kangri Himalaya", 880, "Kula Kangri"));
            Items.Add(new Item("Annapurna Dakshin", 7219, "Annapurna Himalaya", 775, "Annapurna"));
            Items.Add(new Item("Khartaphu", 7213, "Mahalangur Himalaya", 712, "Mount Everest"));
            Items.Add(new Item("Tongshanjiabu", 7207, "Lunana Himalaya", 1757, "Gangkar Puensum"));
            Items.Add(new Item("Malangutti Sar", 7207, "Hispar Karakoram", 515, "Distaghil Sar"));
            Items.Add(new Item("Noijin Kangsang", 7206, "Nagarze Himalaya", 2160, "Tongshanjiabu"));
            Items.Add(new Item("Langtang Ri", 7205, "Langtang Himalaya", 650, "Porong Ri"));
            Items.Add(new Item("Kangphu Kang", 7204, "Lunana Himalaya", 1200, "Tongshanjiabu"));
            Items.Add(new Item("Singhi Kangri", 7202, "Siachen Karakoram", 790, "Teram Kangri III"));
            Items.Add(new Item("Lupghar Sar", 7200, "Hispar Karakoram", 730, "Momhil Sar"));
        }
    }
}

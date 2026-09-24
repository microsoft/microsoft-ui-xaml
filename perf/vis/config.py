report_mode               = False

prehistory_data_path      = r'..\ci-prehistory'
wux_data_path             = r'..\wux-reference'
default_data_path         = r'D:\perf_analysis\experiments\periodic\ci'

show_historical_trendline = True

if report_mode:
    default_visualizations_path = r'report-21-11'
    plot_width                  = 800
    plot_height                 = 600
    show_prehistory             = True
    show_wux_reference          = True
    generate_reports            = False
    show_significant_changes    = True
else:
    default_visualizations_path = r'visualizations'
    plot_width                  = 1600
    plot_height                 = 1100
    show_prehistory             = False
    show_wux_reference          = False
    generate_reports            = True
    show_significant_changes    = False

# significant changes
cpu_changes = [
    ('210919.0', 'yellow', 'imp'),
    ('210921.0', 'lightblue', 'reg'),
    ('210928.1', 'orange', 'imp')]

mem_changes = [
    ('210826.1', 'hotpink', 'reg'),
    ('210919.0', 'yellow', 'reg'),
    ('210921.0', 'lightblue', 'reg'),
    ('210928.1', 'orange', 'imp'),
    ('210929.5', 'silver', 'reg')]
import pandas as pd
import bokeh.plotting as bplo
import bokeh.palettes as bpal
import re
import os
import glob
import plots
import data
import reports
import argparse
import util
import config

__parser = argparse.ArgumentParser(
    description = 'Generate reports from performance data')

__parser.add_argument('--data_path', required = True)
__parser.add_argument('--out_path', default = config.default_visualizations_path)

__args = __parser.parse_args()

if not os.path.isabs(__args.out_path):
    __args.out_path = os.path.join(__args.data_path, __args.out_path)

__scenario_colors = {}
__report_shift_deltas = ( 1, 5, 10, -1 )
__data_filename = r'cumulative.csv'

def __update_scenario_colors(scenarios):
    for scenario in scenarios:
        if scenario not in __scenario_colors:
            __scenario_colors[scenario] = bpal.d3['Category20'][20][len(__scenario_colors)]

def prepare_out_dir():
    if not os.path.isdir(__args.out_path):
        os.mkdir(__args.out_path)
    else:
        for f in glob.glob(os.path.join(__args.out_path, '*.html')):
            os.remove(f)

def load_data():
    data_full_path = os.path.join(__args.data_path, __data_filename)

    print ('Loading {}'.format(data_full_path))
    raw_data = pd.read_csv(data_full_path)

    if config.show_prehistory:
        prehistory_data_full_path = os.path.join(__args.data_path, config.prehistory_data_path, __data_filename)
        print ('Loading {}'.format(prehistory_data_full_path))
        raw_data = raw_data.append(pd.read_csv(prehistory_data_full_path))

        wux_data_full_path = os.path.join(__args.data_path, config.wux_data_path, __data_filename)
        print ('Loading {}'.format(wux_data_full_path))
        raw_data = raw_data.append(pd.read_csv(wux_data_full_path))

        raw_data['scenario'] = raw_data['scenario'].apply(lambda x: re.sub(r'\.(MUX|WUX)', '', x))

    meta_data = pd.DataFrame(raw_data.loc[raw_data.metric.str.startswith('Meta/'), ['shift', 'scenario', 'metric', 'value']])

    # Convert CI stamp to version
    meta_data.loc[meta_data.metric == 'Meta/Version', 'value'] = meta_data.loc[meta_data.metric == 'Meta/Version', 'value'].apply(lambda x: re.sub(r'3\.0\.0-[a-z0-9_]+\.([0-9]+\.[0-9]+).*', r'\1', x))

    value_data = pd.DataFrame(raw_data[~raw_data.metric.str.startswith('Meta/')])

    # Merge relevant metadata rows as columns
    for metadata_row in ['Meta/Version', 'Meta/OSVersion', 'Meta/Arch']:
        right = meta_data.loc[meta_data.metric == metadata_row, ['shift', 'scenario', 'value']]

        # We output metadata for each processing pipeline, so there will be duplicates, which will cause problems during merge.
        right.drop_duplicates(inplace = True)

        if len(right) == 0:
            raise 'Missing metadata row: {}'.format(metadata_row)

        value_data = value_data.merge(
            right,
            on = ['shift', 'scenario'],
            how = 'left',
            suffixes = ('', '_temp'))

        value_data.rename(columns = { 'value_temp' : metadata_row.replace(r'Meta/', '').lower() }, inplace = True)

    # Extract date time and build number from version to sort chronologically
    value_data = value_data.join(value_data.version.str.extract(r'^(?P<date>[0-9]{6})\.(?P<build>[0-9]+)'))
    value_data.build = pd.to_numeric(value_data.build)
    value_data.sort_values(by = ['date', 'build'], inplace = True)
    value_data = value_data.drop(columns = ['date', 'build'])

    # Get rid of NaNs
    value_data.interval = value_data.interval.fillna('Total')
    value_data.grouping = value_data.grouping.fillna('')

    return value_data

def output_plot(fig, title):
    filename = util.get_filename_from_title(__args.out_path, title)
    print ('Generating {}'.format(filename))
    bplo.output_file(filename, title = title)
    bplo.save(fig)

def get_delta(data, shift_delta):
    shifts = data['shift'].unique()

    if len(shifts) <= shift_delta or len(shifts) < 2:
        print ('No enough data to generate report with shift delta = {}'.format(shift_delta))
        return None

    if shift_delta < 0:
        start_index = 0
        filename_suffix = 'delta-all'
    else:
        start_index = -shift_delta - 1
        filename_suffix = 'delta-{}'.format(shift_delta)

    return ((shifts[start_index], shifts[-1]), filename_suffix)

def do_cpu_reports(cpu_data, shift_delta):
    delta = get_delta(cpu_data, shift_delta)

    if delta == None:
        return

    reports.comparison_report(
        output_filename = os.path.join(__args.out_path, '_CPU_report-all-changes-{}.html'.format(delta[1])),
        data_model = data.CPU,
        data_frame = cpu_data,
        shifts = delta[0])

    reports.largest_changes_report(
        output_filename = os.path.join(__args.out_path, '_CPU_report-top-regressions-improvements-{}.html'.format(delta[1])),
        data_model = data.CPU,
        data_frame = cpu_data,
        shifts = delta[0])

def do_cpu_plots(cpu_data):
    for (scenario, interval, metric, grouping, arch), values in cpu_data.groupby(['scenario', 'interval', 'metric', 'grouping', 'arch']):
        title = util.get_title(arch, scenario, metric, interval, grouping)
        metric_obj = data.CPU.find_metric_definition(metric)
        fig = plots.cpu_historical(
            values,
            'value_{}'.format(metric_obj.get_main_stat()),
            title,
            __scenario_colors[scenario],
            metric_obj.get_friendly_name(),
            metric_obj.get_unit())
        output_plot(fig, title)

def do_cpu():
    cpu_data = pd.DataFrame(raw_data[raw_data.metric.str.startswith('CPU/')])
    cpu_data.value = pd.to_numeric(cpu_data.value)
    cpu_data = data.CPU.transform(cpu_data);

    cpu_data = cpu_data[((cpu_data.metric == 'CPU/Cycles') | (cpu_data.metric == 'CPU/WallTime')) & ~((cpu_data.interval == 'Total') & (cpu_data.grouping == ''))]

    scenarios = cpu_data.scenario.unique()
    __update_scenario_colors(scenarios)

    if config.generate_reports:
        for delta in __report_shift_deltas:
            do_cpu_reports(cpu_data, delta)

    do_cpu_plots(cpu_data)

def do_mem_reports(mem_data, shift_delta):
    delta = get_delta(mem_data, shift_delta)

    if delta == None:
        return

    reports.comparison_report(
        output_filename = os.path.join(__args.out_path, '_Mem_report-all-changes-{}.html'.format(delta[1])),
        data_model = data.MEM,
        data_frame = mem_data,
        shifts = delta[0])

    reports.largest_changes_report(
        output_filename = os.path.join(__args.out_path, '_Mem_report-top-regressions-improvements-{}.html'.format(delta[1])),
        data_model = data.MEM,
        data_frame = mem_data,
        shifts = delta[0])

def do_mem_plots(data_frame):
    for (scenario, interval, metric, grouping, arch), values in data_frame.groupby(['scenario', 'interval', 'metric', 'grouping', 'arch']):
        title = util.get_title(arch, scenario, metric, interval, grouping)
        metric_obj = data.MEM.find_metric_definition(metric)
        fig = plots.mem_historical(
            values,
            title,
            __scenario_colors[scenario],
            metric_obj.get_friendly_name(),
            metric_obj.get_unit())
        output_plot(fig, title)

def do_mem():
    mem_data = pd.DataFrame(raw_data[raw_data.metric.str.startswith('Mem/RefSet/')])
    mem_data.value = pd.to_numeric(mem_data.value)
    mem_data = data.MEM.normalize(mem_data);

    scenarios = mem_data.scenario.unique()
    __update_scenario_colors(scenarios)
    is_mux = mem_data.grouping.str.contains(r'/Module:(microsoft\.ui\.xaml.*|winuiedit)\.dll')
    is_mrt = mem_data.grouping.str.contains(r'/Module:(mrm|microsoft.windows.applicationmodel.resources)\.dll')
    is_wasdk = mem_data.grouping.str.contains(r'/Module:(coremessagingxp|dcompi|dwmcorei|dwmscenei|marshal|microsoft.directmanipulation|microsoft.inputstatemanager|microsoft.internal.frameworkudk|microsoft.ui.composition.ossupport|microsoft.ui.input|microsoft.ui.windowing.core|wuceffectsi)\.dll')
    module_grouping = mem_data.grouping.str.contains('/Module')

    mem_data = mem_data[is_mux | is_mrt | is_wasdk | ~module_grouping]

    if config.generate_reports:
        mem_data_report = pd.DataFrame(mem_data.rename(columns = { 'value' : 'value_fP50' }))
        for delta in __report_shift_deltas:
            do_mem_reports(mem_data_report, delta)

    mem_data = data.MEM.transform(mem_data);

    do_mem_plots(mem_data)

prepare_out_dir()
raw_data = load_data()
do_cpu()
do_mem()
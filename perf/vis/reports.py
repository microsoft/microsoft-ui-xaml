import pandas as pd
import numpy as np
import jinja2
import data
import util

def sort_helper(array):
    array.sort()
    return array

def save_page(template_file, output_filename, args):
    env = jinja2.Environment(loader=jinja2.FileSystemLoader(searchpath=''))
    template = env.get_template(template_file)
    html = template.render(**args)
    with open(output_filename, 'w', encoding='utf-8') as f:
        f.write(html)

def __format_link(data_model, row, scenario_column):
    return '<a href="{}">{}</a>'.format(
        util.get_filename_from_title(
            '.',
            data_model.get_title(row.arch, row.scenario, row.metric, row.interval, row.grouping)), row[scenario_column])

def output(summary, data_model):
    if len(summary) == 0:
        return

    columns = ['arch', 'metric', 'version', 'interval', 'grouping', 'scenario', 'value', 'diff_abs', 'diff_rel']

    if not 'grouping' in summary:
        columns.remove('grouping')

    if not 'version' in summary:
        columns.remove('version')

    styled = pd.DataFrame(summary.loc[:, columns])

    if 'scenario_trial' in summary:
        styled['scenario'] = summary.apply(
            lambda x: '{}-{}'.format(__format_link(data_model, x, 'scenario_trial'), __format_link(data_model, x, 'scenario')),
            axis = 1)
    else:
        styled['scenario'] = styled.apply(
            lambda x: __format_link(data_model, x, 'scenario'),
            axis = 1)

    styled['value']    = styled[['metric', 'value']].apply(lambda x: data_model.find_metric_definition(x[0]).format(value = x[1]), axis = 1)
    styled['diff_abs'] = styled[['metric', 'diff_abs']].apply(lambda x: data_model.find_metric_definition(x[0]).format(value = x[1], is_delta = True), axis = 1)
    styled['diff_rel'] = styled[['diff_rel']].apply(lambda x: '{0:+,.1f}%'.format(x[0]), axis = 1)

    styled.rename(
        columns = {
            'value'    : 'baseline',
            'diff_abs' : 'trial_diff_abs',
            'diff_rel' : 'trial_diff_rel',
           }, inplace = True)

    return styled.style.hide().to_html()

def __merge_helper(data_frame, shifts, mismatched_scenarios, value_cols):
    merge_on = ['scenario', 'interval', 'metric', 'grouping', 'arch']

    if mismatched_scenarios:
        merge_on.remove('scenario')

    columns = ['scenario', 'interval', 'metric', 'grouping', 'arch'] + value_cols

    return pd.DataFrame(data_frame[data_frame['shift'] == shifts[0]]).merge(
        data_frame.loc[data_frame['shift'] == shifts[1], columns],
        how = 'outer',
        on = merge_on,
        suffixes = ('', '_trial'))

def __call_wrapper(data, calc_func):
    if isinstance(data, list):
        for data_frame in data:
            calc_func(data_frame)
    else:
        calc_func(data)

def __calculate_tops(data, data_model):
    result = {
        'top_abs_reg'        : [],
        'top_rel_reg'        : [],
        'top_abs_imp'        : [],
        'top_rel_imp'        : []
    }

    def __helper(data_frame):
        reg = data_frame[data_frame['diff_abs'] > 0.0]
        result['top_abs_reg'].append(output(reg.nlargest(20, ['diff_abs']), data_model))
        result['top_rel_reg'].append(output(reg.nlargest(20, ['diff_rel']), data_model))

        imp = data_frame[data_frame['diff_abs'] < 0.0]
        result['top_abs_imp'].append(output(imp.nsmallest(20, ['diff_abs']), data_model))
        result['top_rel_imp'].append(output(imp.nsmallest(20, ['diff_rel']), data_model))

    __call_wrapper(data, __helper)

    return result

def __compare(data, data_model):
    result = {
        'comparison_data'   : []
    }

    def __helper(data_frame):
        result['comparison_data'].append(output(data_frame, data_model))

    __call_wrapper(data, __helper)

    return result

def __largest_changes_report(title, output_filename, data_model, data_frame, shifts, value_columns, calc_func, template_filename, mismatched_scenarios, mix_metrics, only_significant):
    print ('Generating {}'.format(output_filename))

    if len(np.unique(np.array(shifts))) != 2:
        print ('Data does not contain at least two shifts, report will not be generated.')
        return

    merged = __merge_helper(data_frame, shifts, mismatched_scenarios, value_columns)
    merged = merged.drop(columns = ['shift', 'version']).dropna()

    versions = data_frame.loc[data_frame['shift'].isin(shifts), ['shift', 'version']].dropna().drop_duplicates()

    version_baseline = versions[versions['shift'] == shifts[0]].iloc[0]['version']
    version_trial = versions[versions['shift'] == shifts[1]].iloc[0]['version']

    tables = {
        'show_versions'      : True,
        'title'              : title,
        'baseline'           : version_baseline,
        'baseline_scenarios' : sort_helper(data_frame[data_frame['version'] == version_baseline]['scenario'].unique()),
        'trial'              : version_trial,
        'trial_scenarios'    : sort_helper(data_frame[data_frame['version'] == version_trial]['scenario'].unique())
    }

    results = {}

    for (interval, metric, grouping, scenario), values in merged.groupby(['interval', 'metric', 'grouping', 'scenario']):
        if not metric in results.keys():
            results[metric] = pd.DataFrame()
            main_stat_str = data_model.find_metric_definition(metric).get_main_stat()

        diffed = pd.DataFrame(values)
        diffed['value'] = diffed['value_{}'.format(main_stat_str)]
        diffed['value_trial'] = diffed['value_{}_trial'.format(main_stat_str)]
        diffed['diff_abs'] = diffed['value_trial'] - diffed['value']
        diffed['diff_rel'] = 100.0 * (diffed['value_trial'] - diffed['value']) / diffed['value']

        if only_significant:
            append = diffed[(diffed['diff_rel'].abs() >= 0.5) & (diffed['diff_abs'].abs() >= 1.0)]
        else:
            append = diffed

        results[metric] = results[metric]._append(append)

    if mix_metrics:
        tables.update(calc_func(pd.concat(results.values()), data_model))
    else:
        tables.update(calc_func(list(results.values()), data_model))

    save_page(template_filename, output_filename, tables)

def __largest_historical_changes_report(title, output_filename, data_model, data_frame, shifts, mix_metrics):
    print ('Generating {}'.format(output_filename))

    if len(np.unique(np.array(shifts))) != 2:
        print ('Data does not contain at least two shifts, report will not be generated.')
        return

    data_frame = data_frame[(data_frame['shift'] >= shifts[0]) & (data_frame['shift'] <= shifts[1])]

    tables = {
        'title'              : title
    }

    results = {}

    for (metric, interval, grouping, scenario), values in data_frame.groupby(['metric', 'interval', 'grouping', 'scenario']):
        if not metric in results.keys():
            results[metric] = pd.DataFrame()
            main_stat_str = data_model.find_metric_definition(metric).get_main_stat()

        diffed = pd.DataFrame(values)
        diffed['value'] = diffed['value_{}'.format(main_stat_str)]
        diffed['diff_abs'] = diffed['value'].diff()
        diffed['diff_rel'] = 100.0 * diffed['diff_abs'] / diffed['value']
        significant = diffed[(diffed['diff_rel'].abs() >= 0.5) & (diffed['diff_abs'].abs() >= 1.0)]
        results[metric] = results[metric].append(significant)

    if mix_metrics:
        tables.update(__calculate_tops(pd.concat(results.values()), data_model))
    else:
        tables.update(__calculate_tops(list(results.values()), data_model))

    save_page('template-tops.html', output_filename, tables)

def __call_largest_changes_report(title, output_filename, data_model, data_frame, shifts, calc_func, template_filename, mismatched_scenarios, mix_metrics, only_significant):
    if data_model == data.CPU:
        __largest_changes_report(
            title,
            output_filename = output_filename,
            data_model = data_model,
            data_frame = data_frame,
            shifts = shifts,
            value_columns = ['value_fAvg', 'value_fP50', 'value_Min', 'value_Max'],
            calc_func = calc_func,
            template_filename = template_filename,
            mismatched_scenarios = mismatched_scenarios,
            mix_metrics = mix_metrics,
            only_significant = only_significant)
    else:
        __largest_changes_report(
            title,
            output_filename = output_filename,
            data_model = data_model,
            data_frame = data_frame,
            shifts = shifts,
            value_columns = ['value_fP50'],
            calc_func = calc_func,
            template_filename = template_filename,
            mismatched_scenarios = mismatched_scenarios,
            mix_metrics = mix_metrics,
            only_significant = only_significant)

def largest_changes_report(output_filename, data_model, data_frame, shifts, mismatched_scenarios = False, mix_metrics = False):
    __call_largest_changes_report(
        title = 'Top {} regressions/improvements between versions'.format(data_model.get_type()),
        output_filename = output_filename,
        data_model = data_model,
        data_frame = data_frame,
        shifts = shifts,
        calc_func = __calculate_tops,
        template_filename = 'template-tops.html',
        mismatched_scenarios = mismatched_scenarios,
        mix_metrics = mix_metrics,
        only_significant = True)

def comparison_report(output_filename, data_model, data_frame, shifts, mismatched_scenarios = False, mix_metrics = False):
    __call_largest_changes_report(
        title = '{} changes between versions'.format(data_model.get_type()),
        output_filename = output_filename,
        data_model = data_model,
        data_frame = data_frame,
        shifts = shifts,
        calc_func = __compare,
        template_filename = 'template-complete.html',
        mismatched_scenarios = mismatched_scenarios,
        mix_metrics = mix_metrics,
        only_significant = False)

def largest_historical_changes_report(output_filename, data_model, data_frame, shifts, mix_metrics = False):
    __largest_historical_changes_report(
        title = 'Top historical {} regressions/improvements'.format(data_model.get_type()),
        output_filename = output_filename,
        data_model = data_model,
        data_frame = data_frame,
        shifts = shifts,
        mix_metrics = mix_metrics)

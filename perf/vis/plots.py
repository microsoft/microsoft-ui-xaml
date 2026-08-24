import bokeh.plotting as bplo
import bokeh.models as bmod
import pandas as pd
import numpy as np
import bokeh.colors as bcol
import math
import config

__x_axis_parameter = 'version_index'

__area_alpha = 0.07
__limit_alpha = 0.1
__limit_width = 1
__main_alpha = 0.3
__main_width = 1
__main_dot_size = 2
__ra_alpha = 0.8
__ra_width = 1.5
__ra_window = 9
__flag_length = 20

def __setup_axes(fig, versions, y_axis_label, unit):
    fig.x_range.range_padding = 0
    fig.xaxis.major_label_orientation = math.pi/2
    fig.xaxis.major_label_overrides = { x: versions[x] if x < len(versions) else '' for x in range(int(len(versions) * 1.5)) }
    fig.xaxis.ticker.num_minor_ticks = 0
    fig.xaxis.ticker.min_interval = 1
    fig.xaxis.ticker.max_interval = 5
    fig.xaxis.bounds = (0, len(versions))
    fig.yaxis.axis_label_text_font_size = "12pt"
    fig.yaxis.axis_label = r'{} ({})'.format(y_axis_label, unit)

    if config.show_prehistory:
        # mark prehistory
        found = np.where(versions >= '210331.0')[0]

        if len(found) > 0:
            fig.add_layout(bmod.Span(
                location = found[0],
                dimension = 'height',
                line_alpha = 0.5,
                line_width = 2,
                line_color = 'green'))

        # mark last report
        found = np.where(versions >= '210826.0')[0]

        if len(found) > 0:
            fig.add_layout(bmod.Span(
                location = found[0],
                dimension = 'height',
                line_alpha = 0.5,
                line_width = 2,
                line_color = 'orange'))
                 
def __split_data(data):
    return (data[~data['version'].str.startswith('WUX')], data[data['version'].str.startswith('WUX')])

def __add_wux_reference(fig, data, reference, main_statistic):
    versions = data['version'].unique()
    reference_versions = reference['version'].unique()
    found = np.where(versions >= '210331.0')[0]

    if len(found) == 0:
        found = 0
    else:
        found = found[0]

    y = reference.loc[reference['version'] == reference_versions[-1], main_statistic].iloc[0]

    fig.line(
        x = [found, len(versions)],
        y = [y, y],
        line_width = 2,
        alpha = 0.5,
        color = 'red')

    fig.text(
        x = [len(versions)],
        y = [y],
        x_offset = -40,
        y_offset = 15,
        text_color = ['red'],
        text_font_size = '12px',
        text = ['WUX'])

def __add_change_markers(fig, main_statistic, changes, data):
    versions = data['version'].unique()

    id = 0

    for change in changes:
        id += 1
        found = np.where(versions == change[0])[0]

        if len(found) > 0:
            y = data[main_statistic].iloc[found[0]]

            if not math.isnan(y):
                fig.ray(
                    x = found[0],
                    y = y,
                    angle = -90 if change[2] == 'imp' else 90,
                    angle_units = 'deg',
                    length = __flag_length,
                    color = 'dimgray',
                    length_units = 'screen')

                fig.circle(
                    x = found[0],
                    y = y,
                    color = 'lime' if change[2] == 'imp' else 'red',
                    alpha = 0.8,
                    size = 4.0)

                fig.add_layout(bmod.Label(
                    x = found[0],
                    y = y,
                    y_offset = (-__flag_length - 14) if change[2] == 'imp' else __flag_length,
                    text_font_size = '12px',
                    background_fill_color = change[1],
                    border_line_color = 'dimgray',
                    text = f' {id} '))

                #fig.text(
                #    x = [found[0]],
                #    y = [y],
                #    y_offset = -30 if change[2] == 'imp' else 30,
                #    text_color = 'black',
                #    text_font_size = '12px',
                #    text = [str(id)])


def cpu_historical(data, main_statistic, title, color, y_axis_label, unit):
    if config.show_wux_reference:
        data, reference = __split_data(data)
        reference[main_statistic] = reference[main_statistic].apply(lambda x: x / unit[1])

    versions = data['version'].unique()

    fig = bplo.figure(
#        x_range = data[__x_axis_parameter].unique(),
        title = title,
        width = config.plot_width,
        height = config.plot_height)

    __setup_axes(fig, versions, y_axis_label, unit[0])

    hover = bmod.HoverTool(
        tooltips=[
            ( 'version',  '@version' ),
            ( 'value',    '@value_fmt {}'.format(unit[0]) ),
            ( 'diff',     '@value_diff_fmt {}'.format(unit[0]) ),
            ( 'min',      '@value_Min_fmt {}'.format(unit[0]) ),
            ( 'max',      '@value_Max_fmt {}'.format(unit[0]) ),
            ( 'value_ra', '@value_ra_fmt {}'.format(unit[0]) ),
            ( 'fN',       '@value_fN' ),
            ( 'fCoV',     '@value_fCoV' ),
            ( 'shift',    '@shift' )
        ])

    fig.tools.append(hover)

    dt = pd.DataFrame(data)

    dt['version_index'] = dt['version'].apply(lambda x: np.where(versions == x)[0][0])

    dt.value_fAvg = dt.value_fAvg.apply(lambda x: x / unit[1])

    dt.value_fP50 = dt.value_fP50.apply(lambda x: x / unit[1])

    dt.value_Min = dt.value_Min.apply(lambda x: x / unit[1])
    dt['value_Min_fmt'] = dt.apply(lambda x: unit[2].format(x.value_Min), axis = 1)

    dt.value_Max = dt.value_Max.apply(lambda x: x / unit[1])
    dt['value_Max_fmt'] = dt.apply(lambda x: unit[2].format(x.value_Max), axis = 1)

    dt['value_fmt'] = dt.apply(lambda x: unit[2].format(x[main_statistic]), axis = 1)
    dt['value_diff'] = dt[main_statistic].diff().fillna(0)
    dt['value_diff_fmt'] = dt.apply(lambda x: unit[3].format(x.value_diff), axis = 1)

    dt['value_ra'] = dt[main_statistic].rolling(__ra_window, center = True).mean()
    dt['value_ra_fmt'] = dt.apply(lambda x: unit[2].format(x['value_ra']), axis = 1)

    source = bmod.ColumnDataSource(dt)

    fig.varea(
        x = __x_axis_parameter,
        y1 = 'value_Min',
        y2 = 'value_Max',
        color = color,
        alpha = __area_alpha,
        source = source)

    fig.line(
        name = 'value_Max',
        x = __x_axis_parameter,
        y = 'value_Max',
        color = color,
        alpha = __limit_alpha,
        line_width = __limit_width,
        source = source)

    fig.line(
        name = 'value_Min',
        x = __x_axis_parameter,
        y = 'value_Min',
        color = color,
        alpha = __limit_alpha,
        line_width = __limit_width,
        source = source)

    fig.line(
        x = __x_axis_parameter,
        y = main_statistic,
        color = color,
        alpha = __main_alpha,
        line_width = __main_width,
        source = source)

    fig.circle(
        name = 'value',
        x = __x_axis_parameter,
        y = main_statistic,
        color = color,
        alpha = __main_alpha,
        size = __main_dot_size,
        source = source)

    if config.show_historical_trendline:
        fig.line(
            name = 'value_ra',
            x = __x_axis_parameter,
            y = 'value_ra',
            color = color,
            alpha = __ra_alpha,
            line_width = __ra_width,
            source = source)

    if config.show_wux_reference and len(reference) > 0:
        __add_wux_reference(fig, data, reference, main_statistic)

    if config.show_significant_changes:
        __add_change_markers(fig, main_statistic, config.cpu_changes, dt)

    return fig

def mem_historical(data, title, color, y_axis_label, unit):
    main_statistic = 'value_Steady'

    if config.show_wux_reference:
        data, reference = __split_data(data)
        reference[main_statistic] = reference[main_statistic].apply(lambda x: x / unit[1])

    versions = data['version'].unique()

    fig = bplo.figure(
#        x_range = data.version.unique(),
        title = title,
        width = config.plot_width,
        height = config.plot_height)

    __setup_axes(fig, versions, y_axis_label, unit[0])

    hover = bmod.HoverTool(
        tooltips=[
            ( 'version',     '@version' ),
            ( 'steady',      '@value_Steady_fmt {}'.format(unit[0]) ),
            ( 'steady_diff', '@value_Steady_diff_fmt {}'.format(unit[0]) ),
            ( 'peak',        '@value_Peak_fmt {}'.format(unit[0]) ),
            ( 'peak_diff',   '@value_Peak_diff_fmt {}'.format(unit[0]) ),
            ( 'shift',       '@shift' )
        ])

    fig.tools.append(hover)

    dt = pd.DataFrame(data)

    dt['version_index'] = dt['version'].apply(lambda x: np.where(versions == x)[0][0])

    dt.value_Steady = dt.value_Steady.apply(lambda x: x / unit[1])
    dt['value_Steady_fmt'] = dt.apply(lambda x: unit[2].format(x.value_Steady), axis = 1)

    dt['value_Steady_diff'] = dt.value_Steady.diff().fillna(0)
    dt['value_Steady_diff_fmt'] = dt.apply(lambda x: unit[3].format(x.value_Steady_diff), axis = 1)

    dt.value_Peak = dt.value_Peak.apply(lambda x: x / unit[1])
    dt['value_Peak_fmt'] = dt.apply(lambda x: unit[2].format(x.value_Peak), axis = 1)

    dt['value_Peak_diff'] = dt.value_Peak.diff().fillna(0)
    dt['value_Peak_diff_fmt'] = dt.apply(lambda x: unit[3].format(x.value_Peak_diff), axis = 1)

    dt['value_ra'] = dt['value_Steady'].rolling(__ra_window, center = True).mean()
    dt['value_ra_fmt'] = dt.apply(lambda x: unit[2].format(x['value_ra']), axis = 1)

    source = bmod.ColumnDataSource(dt)

    fig.varea(
        x = __x_axis_parameter,
        y1 = 'value_Steady',
        y2 = 'value_Peak',
        color = color,
        alpha = __area_alpha,
        source = source)

    fig.line(
        x = __x_axis_parameter,
        y = 'value_Steady',
        color = color,
        alpha = __main_alpha,
        line_width = __main_width,
        source = source)

    fig.line(
        x = __x_axis_parameter,
        y = 'value_Peak',
        color = color,
        alpha = __main_alpha,
        line_width = __main_width,
        source = source)

    fig.circle(
        x = __x_axis_parameter,
        y = 'value_Peak',
        color = color,
        size = __main_dot_size,
        alpha = __main_alpha,
        source = source)

    fig.circle(
        x = __x_axis_parameter,
        y = 'value_Steady',
        color = color,
        size = __main_dot_size,
        alpha = __main_alpha,
        source = source)

    if config.show_historical_trendline:
        fig.line(
            x = __x_axis_parameter,
            y = 'value_ra',
            color = color,
            alpha = __ra_alpha,
            line_width = __ra_width,
            source = source)

    if config.show_wux_reference and len(reference) > 0:
        __add_wux_reference(fig, data, reference, main_statistic)

    if config.show_significant_changes:
        __add_change_markers(fig, main_statistic, config.mem_changes, dt)

    return fig
import pandas as pd
import util
import re

units_ms = ( 'ms', 1.0, '{0:,.1f}', '{0:+,.1f}' )
units_M  = ( 'x 10⁶', 1000000.0, '{0:,.3f}', '{0:+,.3f}' )
units_k  = ( 'x 10³', 1000.0, '{0:,.3f}', '{0:+,.3f}' )
units_1  = ( '', 1.0, '{0:,.1f}', '{0:+,.1f}' )
units_1i = ( '', 1.0, '{0:,.0f}', '{0:+,.0f}' )
units_kB = ( 'kB', 1024, '{0:,.0f}', '{0:+,.0f}' )
units_MB = ( 'MB', 1024 * 1024, '{0:,.3f}', '{0:+,.3f}' )

class MetricDefinition:
    def __init__(self, name, main_stat, friendly_name, unit):
        self.__name = name
        self.__main_stat = main_stat
        self.__friendly_name = friendly_name
        self.__unit = unit

    def get_name(self):
        return self.__name

    def get_main_stat(self):
        return self.__main_stat

    def get_friendly_name(self):
        return self.__friendly_name

    def get_unit(self):
        return self.__unit

    def format(self, value, use_scale = True, use_unit = True, is_delta = False):
        fmt_str = self.__unit[2] if not is_delta else self.__unit[3]

        if use_unit and self.__unit[0] != '':
            fmt_str = fmt_str + '{}'.format(self.__unit[0])

        if use_scale:
            value /= self.__unit[1]

        return fmt_str.format(value)

def _get_metric_definitions(obj):
    return [x for x in vars(obj).values() if isinstance(x, MetricDefinition)]

class base:
    def get_metric_definitions(self):
        return _get_metric_definitions(self)

    def find_metric_definition(self, s):
        for q in self.get_metric_definitions():
            if q.get_name() == s:
                return q
        return None

def _merge_helper(data_frame, merged, merge_column, merge_value):
    merge_on = list(data_frame.columns)

    merge_on.remove('value')

    if merge_column in merge_on:
        merge_on.remove(merge_column)

    right_columns = list(data_frame.columns)
    right_columns.remove(merge_column)

    right = data_frame.loc[data_frame[merge_column] == merge_value, right_columns]

    return merged.merge(
        right,
        how = 'outer',
        on = merge_on,
        suffixes = ('', '_{}'.format(merge_value)))

class cpu(base):
    def __init__(self):
        self.DURATION = MetricDefinition(
            'CPU/WallTime',
            'fAvg',
            'Avg duration',
            units_ms)

        self.INSTRUCTIONS = MetricDefinition(
            'CPU/Instructions',
            'fAvg',
            'Avg instructions',
            units_M)

        self.CYCLES = MetricDefinition(
            'CPU/Cycles',
            'fAvg',
            'Avg cycles',
            units_M)

        self.LLCMISSES = MetricDefinition(
            'CPU/LLCMisses',
            'fAvg',
            'Avg LLC misses',
            units_k)

        self.COUNT = MetricDefinition(
            'CPU/ROICount',
            'fP50',
            'Median event count',
            units_1i)

        self.BRANCH_MISPREDICTIONS = MetricDefinition(
            'CPU/BranchMispredictions',
            'fAvg',
            'Avg branch mispredictions',
            units_1i)

    def transform(self, data_frame):
        # Merge all statistics rows as columns
        merged = pd.DataFrame(data_frame[data_frame.statistic == 'fAvg'])
        merged = merged.drop(columns = ['statistic'])

        merged = _merge_helper(data_frame, merged, 'statistic', 'fN')
        merged = _merge_helper(data_frame, merged, 'statistic', 'fCoV')
        merged = _merge_helper(data_frame, merged, 'statistic', 'fP50')
        merged = _merge_helper(data_frame, merged, 'statistic', 'Min')
        merged = _merge_helper(data_frame, merged, 'statistic', 'Max')

        merged.rename(columns = { 'value' : 'value_fAvg' }, inplace = True)

        return merged

    def get_type(self):
        return 'CPU'

    def get_title(self, arch, scenario, metric, interval, grouping):
        return util.get_title(arch, scenario, metric, interval, grouping)

class mem(base):
    def __init__(self):
        self.REFSET_IMPACTING = MetricDefinition(
            'Mem/RefSet/Impacting',
            'fP50',
            'RefSet Impacting',
            units_MB)

        self.REFSET_IMPACTING_STEADY = MetricDefinition(
            'Mem/RefSet/Impacting/Steady',
            'fP50',
            'RefSet Impacting Steady',
            units_MB)

        self.REFSET_IMPACTING_PEAK = MetricDefinition(
            'Mem/RefSet/Impacting/Peak',
            'fP50',
            'RefSet Impacting Peak',
            units_MB)

    def normalize(self, data_frame):
        normalized = pd.DataFrame(data_frame)
        normalized['grouping'] = normalized['grouping'].apply(lambda x: re.sub(r'Run:\d+/?', '', x))
        normalized['statistic'] = 'fP50'
        return normalized

    def transform(self, data_frame):
        # Merge peak metric as column
        merged = pd.DataFrame(data_frame[data_frame.metric == 'Mem/RefSet/Impacting/Steady'])
        merged.metric = 'Mem/RefSet/Impacting'
        merged = _merge_helper(data_frame, merged, 'metric', 'Mem/RefSet/Impacting/Peak')
        merged.rename(columns = { 'value' : 'value_Steady', 'value_Mem/RefSet/Impacting/Peak' : 'value_Peak' }, inplace = True)
        return merged

    def get_type(self):
        return 'memory'

    def get_title(self, arch, scenario, metric, interval, grouping):
        metric_definition = self.find_metric_definition(metric)

        if (metric_definition == self.REFSET_IMPACTING_STEADY or metric_definition == self.REFSET_IMPACTING_PEAK):
            metric_definition = self.REFSET_IMPACTING

        return util.get_title(arch, scenario, metric_definition.get_name(), interval, grouping)

CPU = cpu()
MEM = mem()
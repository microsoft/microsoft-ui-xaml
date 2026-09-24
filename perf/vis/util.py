import os
import re

def get_title(arch, scenario, metric, interval, grouping):
    return '{}-{}-{}-{}'.format(metric, interval, grouping, scenario)

def get_filename_from_title(out_path, title):
    return os.path.join(out_path,
                        re.sub(r'(/|\$|\[|\]|:)', r'_', title) + r'.html')
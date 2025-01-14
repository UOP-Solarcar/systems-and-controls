import re
from typing import List
from ..models import Metric
import sys


def parse_line(line: str) -> List[Metric]:
    """Parse a single line of input and return list of metrics"""
    metrics = []

    # Define patterns for each metric we care about
    patterns = {
        "RPM": r"RPM: (\d+)",
        "current": r"current: (-?\d+)",
        "duty_cycle": r"duty cycle: (\d+)",
        "Wh Used": r"Wh Used: (\d+)",
        "Wh Charged": r"Wh Charged: (\d+)",
        "Voltage In": r"Voltage In: (\d+)",
        "High Temperature": r"High Temperature: (\d+)",
        "Low Temperature": r"Low Temperature: (\d+)",
        "Adaptive Total Capacity": r"Adaptive Total Capacity: (\d+)",
        "Pack SOC": r"Pack SOC: (\d+)",
        "Pack Current": r"Pack Current: (\d+)",
        "Pack Inst. Voltage": r"Pack Inst\. Voltage: (\d+)",
    }

    # Extract each metric using its pattern
    for key, pattern in patterns.items():
        match = re.search(pattern, line)
        if match:
            try:
                metric = Metric(value=match.group(1))
                metrics.append((key, metric))
            except ValueError as e:
                print(f"Error parsing metric {key}: {e}", file=sys.stderr)

    return metrics

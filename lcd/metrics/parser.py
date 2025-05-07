import re
from typing import Dict, List
import sys


def split_metrics(line: str) -> List[Dict[str, str]]:
    """Split a line into individual metrics using regex pattern matching"""
    metrics = []

    # Define patterns for each metric we care about
    patterns = {
        "RPM": r"RPM: (\d+)",
        "current": r"current: (-?\d+)",
        "duty cycle": r"duty cycle: (\d+)",
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
            metrics.append({key: match.group(1)})

    return metrics


def parse_line(line: str) -> List[Dict[str, str]]:
    """Parse a single line of input and return list of key-value pairs"""
    try:
        metrics = split_metrics(line)
        
        # Verify all metrics are dictionaries and skip those that aren't
        valid_metrics = []
        for metric in metrics:
            if isinstance(metric, dict):
                valid_metrics.append(metric)
            else:
                print(f"Warning: Invalid metric format, expected dict but got {type(metric)}: {metric}", file=sys.stderr)
        
        # Replace metrics with valid_metrics
        metrics = valid_metrics
        
        # Calculate average temperature if we have both high and low
        high_temp = None
        low_temp = None

        for metric in metrics:
            if "High Temperature" in metric:
                try:
                    high_temp = int(metric["High Temperature"])
                except (ValueError, KeyError):
                    pass
            elif "Low Temperature" in metric:
                try:
                    low_temp = int(metric["Low Temperature"])
                except (ValueError, KeyError):
                    pass

        if high_temp is not None and low_temp is not None:
            avg_temp = (high_temp + low_temp) / 2
            metrics.append({"Average Temperature": str(avg_temp)})

        return metrics
    except Exception as e:
        print(f"Error in parse_line: {e}", file=sys.stderr)
        # Return an empty list of metrics if there's an error
        return []

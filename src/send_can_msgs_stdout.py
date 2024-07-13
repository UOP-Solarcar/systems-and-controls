#! /usr/bin/env python3

import sys
import time
import itertools

# Read the files
with open('/Users/pranavjay/Downloads/penis', 'r') as file1:
    lines1 = file1.readlines()

with open('/Users/pranavjay/Downloads/sample_bms_data (1).txt', 'r') as file2:
    lines2 = file2.readlines()

# Function to extract all metrics
def extract_metrics(lines, metrics):
    for line in lines:
        for key in metrics.keys():
            if key in line:
                metrics[key] = line.split(f"{key}:")[1].split()[0]
    return metrics

# Initialize metric dictionaries
metrics1 = {
    "Wh Used": "",
    "Wh Charged": "",
    "RPM": "",
    "current": "",
    "duty cycle": "",
    "Ah Used": "",
    "Ah Charged": "",
    "Tachometer": "",
    "Voltage In": "",
    "Reserved": "",
    "ADC1": "",
    "ADC2": "",
    "ADC3": "",
    "PPM": ""
}

metrics2 = {
    "Pack Health": "",
    "Adaptive Total Capacity": "",
    "Pack Current": "",
    "Pack Inst. Voltage": "",
    "Pack SOC": "",
    "Relay State": "",
    "Input Supply Voltage": "",
    "Pack DCL": "",
    "Pack CCL": "",
    "High Temperature": "",
    "Low Temperature": "",
    "Internal Temperature": "",
    "High Cell Voltage": "",
    "Low Cell Voltage": ""
}

# Extract metrics
metrics_list1 = [extract_metrics(lines1[i:i+15], metrics1.copy()) for i in range(0, len(lines1), 15)]
metrics_list2 = [extract_metrics(lines2[i:i+15], metrics2.copy()) for i in range(0, len(lines2), 15)]

# Merge metrics lists
metrics_list = [dict(list(m1.items()) + list(m2.items())) for m1, m2 in zip(metrics_list1, metrics_list2)]

# Function to print metrics continuously
def print_metrics(metrics_list):
    try:
        for metrics in itertools.cycle(metrics_list):
            output = f"RPM: {metrics['RPM']}\nAdaptive Total Capacity: {metrics['Adaptive Total Capacity']}\nVoltage In: {metrics['Voltage In']}\ncurrent: {metrics['current']}"
            print(output)
            sys.stdout.flush()
            time.sleep(0.1)  # Adjust the sleep time as needed
    except BrokenPipeError:
        pass

# Start printing metrics
print_metrics(metrics_list)

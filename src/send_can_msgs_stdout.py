#! /usr/bin/env python3

import sys
import time
import itertools

# Ensure at least two args are passed
if len(sys.argv) < 2:
    print("Usage: ./send_can_msgs_stdout.py <file1>")


combined_file_path = sys.argv[1]

with open(combined_file_path, 'r') as combined_file:
    combined_lines = combined_file.readlines()

# Function to extract all metrics
def extract_metrics(lines, metrics):
    for line in lines:
        for key in metrics.keys():
            if key in line:
                metrics[key] = line.split(f"{key}:")[1].split()[0]
    return metrics

# Initialize a single metric dictionary
metrics = {
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
    "PPM": "",
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

metrics_list = [extract_metrics(combined_lines[i:i+15], metrics.copy()) for i in range(0, len(combined_lines), 15)]

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

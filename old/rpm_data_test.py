#!/usr/bin/env python3
import time

file_path = "/Users/pranavjay/Downloads/stripped"


def read_metrics_from_file():
    metrics = []
    try:
        with open(file_path, "r") as f:
            lines = f.readlines()
            for line in lines:
                if line.strip():  # Add non-empty lines
                    metrics.append(line.strip())
    except FileNotFoundError:
        print(f"File {file_path} not found.")
    except Exception as e:
        print(f"Error reading from file: {e}")
    return metrics


try:
    while True:
        metrics = read_metrics_from_file()
        for metric in metrics:
            print(metric)
        time.sleep(1)

except KeyboardInterrupt:
    print("Metrics reader stopped.")

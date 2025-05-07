import sys
import queue
import select
import time
from .parser import parse_line


def read_metrics_from_stdin(output_queue: queue.Queue) -> None:
    while True:
        try:
            rlist, _, _ = select.select([sys.stdin], [], [], 0.1)
            if rlist:
                line = sys.stdin.readline().strip()
                if line:
                    # Parse the line into separate metrics
                    metrics = parse_line(line)
                    # Put each parsed metric in the queue
                    for metric_entry in metrics:
                        # Handle tuples (key, value) returned from parse_line
                        if isinstance(metric_entry, tuple) and len(metric_entry) == 2:
                            key, metric_obj = metric_entry
                            # Extract the value from the Metric object
                            if hasattr(metric_obj, "value"):
                                output_queue.put(f"{key}: {metric_obj.value}")
                        # Also handle dictionary format for compatibility
                        elif isinstance(metric_entry, dict):
                            for key, value in metric_entry.items():
                                output_queue.put(f"{key}: {value}")
                        else:
                            print(f"Warning: Unexpected metric format: {metric_entry}", file=sys.stderr)
        except Exception as e:
            print(f"Error in reader thread: {e}", file=sys.stderr)
        time.sleep(0.1)


def calculate_speed(rpm: int, wheel_diameter: float = 0.6) -> float:
    return (rpm * wheel_diameter * 3.141593 * 60) / 1000


def kwh_per_100_km(wh_used: int, wh_charged: int, speed: float) -> float:
    power = (wh_used - wh_charged) / 1000
    time_hours_per_100_km = 100 / speed
    energy_consumption = power * time_hours_per_100_km
    return energy_consumption

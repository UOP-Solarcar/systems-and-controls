from .calculations import read_metrics_from_stdin, calculate_speed, kwh_per_100_km
import sys
import queue

def read_metrics_from_stdin(output_queue: queue.Queue) -> None:
    """
    Reads metrics from stdin and puts them into the output queue.
    Each line should be in the format "key: value"
    """
    while True:
        try:
            line = sys.stdin.readline()
            if not line:  # EOF
                break
                
            line = line.strip()
            if line:  # Only process non-empty lines
                output_queue.put(line)
                
        except KeyboardInterrupt:
            break
        except Exception as e:
            print(f"Error reading from stdin: {e}", file=sys.stderr)

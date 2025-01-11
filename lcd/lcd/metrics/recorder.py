import pyarrow as pa
from pyarrow import csv
import signal
import atexit
from datetime import datetime
from typing import Dict, List
import os


class MetricsRecorder:
    def __init__(self, output_dir: str = "data/recordings"):
        self.metrics_data: Dict[str, List] = {
            "timestamp": [],
            "RPM": [],
            "current": [],
            "duty_cycle": [],
            "wh_used": [],
            "wh_charged": [],
            "voltage_in": [],
            "high_temperature": [],
            "low_temperature": [],
            "average_temperature": [],
            "adaptive_total_capacity": [],
            "pack_soc": [],
            "pack_current": [],
            "pack_voltage": [],
        }

        self.output_dir = output_dir
        os.makedirs(output_dir, exist_ok=True)

        # Register save handlers
        atexit.register(self.save_to_csv)
        signal.signal(signal.SIGTERM, self._signal_handler)
        signal.signal(signal.SIGINT, self._signal_handler)

    def record_metric(self, key: str, value: str) -> None:
        """Record a metric with current timestamp"""
        normalized_key = self._normalize_key(key)

        # Debug prints
        print(f"Processing metric: {key} -> {normalized_key} = {value}")

        # If this is a metric we care about
        if normalized_key in self.metrics_data:
            # Start a new row if this is RPM (our sentinel metric) or first metric ever
            if normalized_key == "RPM" or len(self.metrics_data["timestamp"]) == 0:
                # Create new row with timestamp
                timestamp = datetime.now().isoformat()
                print(f"Creating new row at {timestamp}")  # Debug print
                for metric_key in self.metrics_data.keys():
                    if metric_key == "timestamp":
                        self.metrics_data[metric_key].append(timestamp)
                    else:
                        self.metrics_data[metric_key].append(None)

            # Update the specific metric in the current row
            self.metrics_data[normalized_key][-1] = value
            print(f"Updated {normalized_key} to {value}")  # Debug print

    def _normalize_key(self, key: str) -> str:
        """Convert metric keys to column names"""
        # First normalize the input key
        key = key.lower().strip()

        # Direct mappings for exact matches
        key_mapping = {
            "rpm": "RPM",
            "current": "current",
            "duty cycle": "duty_cycle",
            "wh used": "wh_used",
            "wh charged": "wh_charged",
            "voltage in": "voltage_in",
            "high temperature": "high_temperature",
            "low temperature": "low_temperature",
            "average temperature": "average_temperature",
            "adaptive total capacity": "adaptive_total_capacity",
            "pack soc": "pack_soc",
            "pack current": "pack_current",
            "pack inst. voltage": "pack_voltage",
        }

        normalized = key_mapping.get(key)
        if normalized:
            print(f"Normalized key '{key}' to '{normalized}'")  # Debug print
        else:
            print(f"Failed to normalize key '{key}'")  # Debug print

        return normalized if normalized else key

    def _create_arrow_table(self) -> pa.Table:
        """Create an Arrow table from recorded metrics"""
        arrays = []
        fields = []

        for column_name, values in self.metrics_data.items():
            # Determine appropriate type for the column
            if column_name == "timestamp":
                arrow_type = pa.string()
            else:
                arrow_type = pa.float64()

            # Convert string values to appropriate type
            converted_values = []
            for v in values:
                try:
                    converted_values.append(float(v) if v is not None else None)
                except (ValueError, TypeError):
                    converted_values.append(None)

            arrays.append(pa.array(converted_values, type=arrow_type))
            fields.append(pa.field(column_name, arrow_type))

        return pa.Table.from_arrays(arrays, schema=pa.schema(fields))

    def save_to_csv(self) -> None:
        """Save recorded metrics to CSV file"""
        if not any(len(v) > 0 for v in self.metrics_data.values()):
            return  # No data to save

        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = os.path.join(self.output_dir, f"metrics_{timestamp}.csv")

        table = self._create_arrow_table()
        csv.write_csv(table, filename)
        print(f"Metrics saved to {filename}")

    def _signal_handler(self, signum, frame):
        """Handle termination signals"""
        self.save_to_csv()
        # Re-raise the signal for proper termination
        signal.default_int_handler(signum, frame)

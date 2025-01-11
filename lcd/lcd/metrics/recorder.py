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
        timestamp = datetime.now().isoformat()
        self.metrics_data["timestamp"].append(timestamp)
        
        # Fill all columns with None for this row
        for metric_key in self.metrics_data.keys():
            if metric_key != "timestamp":
                self.metrics_data[metric_key].append(None)
        
        # Update the specific metric
        normalized_key = self._normalize_key(key)
        if normalized_key in self.metrics_data:
            self.metrics_data[normalized_key][-1] = value
    
    def _normalize_key(self, key: str) -> str:
        """Convert metric keys to column names"""
        key = key.lower().replace(" ", "_")
        key_mapping = {
            "duty_cycle": "duty_cycle",
            "wh_used": "wh_used",
            "wh_charged": "wh_charged",
            "voltage_in": "voltage_in",
            "high_temperature": "high_temperature",
            "low_temperature": "low_temperature",
            "average_temperature": "average_temperature",
            "adaptive_total_capacity": "adaptive_total_capacity",
            "pack_soc": "pack_soc",
            "pack_current": "pack_current",
            "pack_inst._voltage": "pack_voltage",
        }
        return key_mapping.get(key, key)
    
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
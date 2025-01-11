import re
from typing import Dict, List
import sys

def split_metrics(line: str) -> List[Dict[str, str]]:
    """Split a line into individual metrics"""
    metrics = []
    
    # Remove ID prefixes and other unnecessary info
    line = re.sub(r'Motor Controller ID: 0x\w+ Status #\d+\s*', '', line)
    line = re.sub(r'Pack Health: \d+\s*', '', line)
    line = re.sub(r'Cell ID: \d+\s*', '', line)
    line = re.sub(r'Checksum: \d+\s*', '', line)
    line = re.sub(r'Reserved: \d+\s*', '', line)
    line = re.sub(r'ADC\d+: \d+\s*', '', line)
    line = re.sub(r'PPM: \d+\s*', '', line)
    line = re.sub(r'Thermistor ID: \d+\s*', '', line)
    
    # Split on common delimiters
    parts = re.split(r'\s+(?=[A-Za-z])', line)
    
    for part in parts:
        # Skip empty parts or parts without colon
        if not part or ':' not in part:
            continue
            
        try:
            key, value = part.split(':', 1)
            key = key.strip()
            value = value.strip()
            
            # Skip if key or value is empty
            if not key or not value:
                continue
                
            # Skip certain metrics we don't need
            if any(skip in key for skip in ['Cell', 'Checksum', 'ADC', 'PPM', 'Reserved']):
                continue
                
            # Try to convert value to number if possible
            try:
                value = str(int(value))  # Convert to int and back to string to clean it
            except ValueError:
                pass
                
            metrics.append({key: value})
            
        except Exception as e:
            print(f"Error parsing metric '{part}': {e}", file=sys.stderr)
            continue
            
    return metrics

def parse_line(line: str) -> List[Dict[str, str]]:
    """Parse a single line of input and return list of key-value pairs"""
    metrics = split_metrics(line)
    
    # Calculate average temperature if we have both high and low
    high_temp = None
    low_temp = None
    
    for metric in metrics:
        if 'High Temperature' in metric:
            try:
                high_temp = int(metric['High Temperature'])
            except ValueError:
                pass
        elif 'Low Temperature' in metric:
            try:
                low_temp = int(metric['Low Temperature'])
            except ValueError:
                pass
    
    if high_temp is not None and low_temp is not None:
        avg_temp = (high_temp + low_temp) / 2
        metrics.append({'Average Temperature': str(avg_temp)})
    
    return metrics 
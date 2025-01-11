import re
from typing import Optional, Dict, List

def parse_motor_controller(line: str) -> List[Dict[str, str]]:
    """Parse motor controller messages into separate metrics"""
    metrics = []
    
    # Status #1: RPM, current, duty cycle
    rpm_match = re.search(r'RPM: (\d+)', line)
    current_match = re.search(r'current: (-?\d+)', line)
    duty_match = re.search(r'duty cycle: (\d+)', line)
    
    # Status #2: Ah Used/Charged
    ah_used_match = re.search(r'Ah Used: (\d+)', line)
    ah_charged_match = re.search(r'Ah Charged: (\d+)', line)
    
    # Status #3: Wh Used/Charged
    wh_used_match = re.search(r'Wh Used: (\d+)', line)
    wh_charged_match = re.search(r'Wh Charged: (\d+)', line)
    
    # Status #5: Tachometer, Voltage
    tach_match = re.search(r'Tachometer: (\d+)', line)
    voltage_match = re.search(r'Voltage In: (\d+)', line)
    
    # Add each match as a separate metric
    if rpm_match: metrics.append({'RPM': rpm_match.group(1)})
    if current_match: metrics.append({'current': current_match.group(1)})
    if duty_match: metrics.append({'duty cycle': duty_match.group(1)})
    if ah_used_match: metrics.append({'Ah Used': ah_used_match.group(1)})
    if ah_charged_match: metrics.append({'Ah Charged': ah_charged_match.group(1)})
    if wh_used_match: metrics.append({'Wh Used': wh_used_match.group(1)})
    if wh_charged_match: metrics.append({'Wh Charged': wh_charged_match.group(1)})
    if tach_match: metrics.append({'Tachometer': tach_match.group(1)})
    if voltage_match: metrics.append({'Voltage In': voltage_match.group(1)})
    
    return metrics

def parse_bms(line: str) -> List[Dict[str, str]]:
    """Parse BMS messages into separate metrics"""
    metrics = []
    
    # Pack metrics
    pack_current_match = re.search(r'Pack Current: (\d+)', line)
    pack_voltage_match = re.search(r'Pack Inst\. Voltage: (\d+)', line)
    pack_soc_match = re.search(r'Pack SOC: (\d+)', line)
    
    # Temperature metrics
    high_temp_match = re.search(r'High Temperature: (\d+)', line)
    low_temp_match = re.search(r'Low Temperature: (\d+)', line)
    
    # Battery capacity
    capacity_match = re.search(r'Adaptive Total Capacity: (\d+)', line)
    
    # Add each match as a separate metric
    if pack_current_match: metrics.append({'Pack Current': pack_current_match.group(1)})
    if pack_voltage_match: metrics.append({'Pack Inst. Voltage': pack_voltage_match.group(1)})
    if pack_soc_match: metrics.append({'Pack SOC': pack_soc_match.group(1)})
    if high_temp_match: metrics.append({'High Temperature': high_temp_match.group(1)})
    if low_temp_match: metrics.append({'Low Temperature': low_temp_match.group(1)})
    if capacity_match: metrics.append({'Adaptive Total Capacity': capacity_match.group(1)})
    
    # Calculate average temperature if both high and low are present
    if high_temp_match and low_temp_match:
        avg_temp = (int(high_temp_match.group(1)) + int(low_temp_match.group(1))) / 2
        metrics.append({'Average Temperature': str(avg_temp)})
    
    return metrics

def parse_line(line: str) -> List[Dict[str, str]]:
    """Parse a single line of input and return list of key-value pairs"""
    # Skip lines that don't contain useful metrics
    if "Cell ID:" in line or "Checksum:" in line or "ADC" in line:
        return []
        
    if "Motor Controller ID:" in line:
        return parse_motor_controller(line)
    else:
        return parse_bms(line) 
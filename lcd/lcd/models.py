from pydantic import BaseModel, Field, validator
from typing import Optional
from datetime import datetime


class Metric(BaseModel):
    """Base model for all metrics"""

    timestamp: datetime = Field(default_factory=datetime.now)
    value: float

    @validator("value", pre=True)
    def parse_value(cls, v):
        if isinstance(v, str):
            try:
                return float(v)
            except ValueError:
                raise ValueError(f"Cannot convert {v} to float")
        return v


class MotorMetrics(BaseModel):
    """Motor-related metrics"""

    rpm: Optional[float] = None
    current: Optional[float] = None
    duty_cycle: Optional[float] = None
    wh_used: Optional[float] = None
    wh_charged: Optional[float] = None
    voltage_in: Optional[float] = None
    temperature: Optional[float] = None


class BatteryMetrics(BaseModel):
    """Battery-related metrics"""

    high_temperature: Optional[float] = None
    low_temperature: Optional[float] = None
    average_temperature: Optional[float] = None
    adaptive_total_capacity: Optional[float] = None
    pack_soc: Optional[float] = None
    pack_current: Optional[float] = None
    pack_voltage: Optional[float] = None


class VehicleMetrics(BaseModel):
    """Combined vehicle metrics"""

    motor: MotorMetrics = Field(default_factory=MotorMetrics)
    battery: BatteryMetrics = Field(default_factory=BatteryMetrics)

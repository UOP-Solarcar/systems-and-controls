use pyo3::prelude::*;
use std::collections::HashMap;
use chrono::{DateTime, Utc};
use std::path::PathBuf;
use arrow::datatypes::{Schema, Field, DataType};
use arrow::array::{StringArray, Float64Array, Array, ArrayRef};
use arrow::record_batch::RecordBatch;
use arrow::error::ArrowError;
use std::sync::Arc;

#[pyclass]
struct MetricsRecorder {
    metrics_data: HashMap<String, Vec<Option<f64>>>,
    timestamps: Vec<DateTime<Utc>>,
    output_dir: PathBuf,
}

#[pymethods]
impl MetricsRecorder {
    #[new]
    fn new(output_dir: &str) -> PyResult<Self> {
        std::fs::create_dir_all(output_dir)?;
        Ok(MetricsRecorder {
            metrics_data: HashMap::from([
                ("RPM".to_string(), Vec::new()),
                ("current".to_string(), Vec::new()),
                ("duty_cycle".to_string(), Vec::new()),
                ("wh_used".to_string(), Vec::new()),
                ("wh_charged".to_string(), Vec::new()),
                ("voltage_in".to_string(), Vec::new()),
                ("high_temperature".to_string(), Vec::new()),
                ("low_temperature".to_string(), Vec::new()),
                ("average_temperature".to_string(), Vec::new()),
                ("adaptive_total_capacity".to_string(), Vec::new()),
                ("pack_soc".to_string(), Vec::new()),
                ("pack_current".to_string(), Vec::new()),
                ("pack_voltage".to_string(), Vec::new()),
            ]),
            timestamps: Vec::new(),
            output_dir: PathBuf::from(output_dir),
        })
    }

    fn record_metric(&mut self, key: &str, value: &str) -> PyResult<()> {
        let normalized_key = self.normalize_key(key);
        
        // Create new row if needed
        if normalized_key == "RPM" || self.timestamps.is_empty() {
            self.timestamps.push(Utc::now());
            // Initialize new row with None values
            for v in self.metrics_data.values_mut() {
                v.push(None);
            }
        }
        
        // Update the specific value
        if let Some(values) = self.metrics_data.get_mut(&normalized_key) {
            if let Some(last_idx) = values.len().checked_sub(1) {
                values[last_idx] = value.parse().ok();
            }
        }
        Ok(())
    }

    fn save_to_csv(&self) -> PyResult<()> {
        if self.timestamps.is_empty() {
            return Ok(());
        }

        // Create schema
        let mut fields = vec![Field::new("timestamp", DataType::Utf8, false)];
        for key in self.metrics_data.keys() {
            fields.push(Field::new(key, DataType::Float64, true));
        }
        let schema = Arc::new(Schema::new(fields));

        // Create arrays
        let timestamp_array = Arc::new(StringArray::from(
            self.timestamps
                .iter()
                .map(|ts| ts.to_rfc3339())
                .collect::<Vec<_>>()
        )) as ArrayRef;

        let mut arrays: Vec<ArrayRef> = vec![timestamp_array];
        for values in self.metrics_data.values() {
            arrays.push(Arc::new(Float64Array::from(values.clone())) as ArrayRef);
        }

        // Create record batch
        let batch = RecordBatch::try_new(schema, arrays)
            .map_err(|e| PyErr::new::<pyo3::exceptions::PyValueError, _>(e.to_string()))?;

        // Save to CSV
        let filename = format!(
            "metrics_{}.csv",
            Utc::now().format("%Y%m%d_%H%M%S")
        );
        let path = self.output_dir.join(filename);
        
        // Write batch to CSV
        let file = std::fs::File::create(path)?;
        let mut writer = arrow::csv::Writer::new(file);
        writer.write(&batch)
            .map_err(|e| PyErr::new::<pyo3::exceptions::PyValueError, _>(e.to_string()))?;

        Ok(())
    }
}

impl MetricsRecorder {
    fn normalize_key(&self, key: &str) -> String {
        let key = key.trim().to_lowercase();
        match key.as_str() {
            "rpm" => "RPM",
            "current" => "current",
            "duty cycle" => "duty_cycle",
            "wh used" => "wh_used",
            "wh charged" => "wh_charged",
            "voltage in" => "voltage_in",
            "high temperature" => "high_temperature",
            "low temperature" => "low_temperature",
            "average temperature" => "average_temperature",
            "adaptive total capacity" => "adaptive_total_capacity",
            "pack soc" => "pack_soc",
            "pack current" => "pack_current",
            "pack inst. voltage" => "pack_voltage",
            _ => &key,
        }.to_string()
    }
}

#[pymodule]
fn lcd_metrics(_py: Python, m: &PyModule) -> PyResult<()> {
    m.add_class::<MetricsRecorder>()?;
    Ok(())
} 
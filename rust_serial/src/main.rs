use std::time::Duration;

fn main() {
    let ports = serialport::available_ports().expect("No ports found!");
    for p in ports {
        println!("{}", p.port_name);
    }
    let mut port = serialport::new("/dev/ttyS0", 9600)
        .timeout(Duration::from_millis(10))
        .open()
        .expect("Failed to open port");

    loop {
        let mut serial_buf = vec![0; 32];
        let _ = unsafe { port.read(serial_buf.as_mut_slice()).unwrap_unchecked() };
        println!("{}", unsafe {
            String::from_utf8(serial_buf).unwrap_unchecked()
        });
    }
}

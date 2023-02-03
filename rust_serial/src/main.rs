use std::time::Duration;

fn main() {
    let ports = serialport::available_ports().expect("No ports found!");
    for p in ports {
        println!("{}", p.port_name);
    }
    let mut port = serialport::new("/dev/ttyACM0", 9600)
        .timeout(Duration::from_millis(10))
        .open()
        .expect("Failed to open port");

    loop {
        let mut serial_buf = vec![0; 1024];
        if let Ok(thingy) = port.read(serial_buf.as_mut_slice()) {
            println!(
                "{}",
                String::from_utf8_lossy(&serial_buf[..thingy]).replace('\0', "")
            );
        }
    }
}

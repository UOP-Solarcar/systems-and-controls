use std::io;
use std::io::Write;
use std::time::Duration;

fn main() {
    let ports = serialport::available_ports().expect("No ports found!");
    for p in ports {
        println!("{}", p.port_name);
    }
    let port_name = "/dev/ttyACM0";
    let baud_rate = 9600;
    let mut port = serialport::new(port_name, baud_rate)
        .timeout(Duration::from_millis(10))
        .open()
        .unwrap();
    println!("Receiving data on {} at {} baud:", &port_name, &baud_rate);
    let mut serial_buf = vec![0; 1024];
    loop {
        match port.read(serial_buf.as_mut_slice()) {
            Ok(t) => io::stdout().write_all(&serial_buf[..t]).unwrap(),
            Err(ref e) if e.kind() == io::ErrorKind::TimedOut => (),
            Err(e) => eprintln!("{e:?}"),
        }
    }
}

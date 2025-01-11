{ lib
, python
, rustPlatform
, maturin
}:

rustPlatform.buildRustPackage {
  pname = "lcd-metrics";
  version = "0.1.0";
  src = ./.;

  cargoLock = {
    lockFile = ./Cargo.lock;
  };

  nativeBuildInputs = [
    maturin
    python
  ];

  buildPhase = ''
    maturin build --release
  '';

  installPhase = ''
    mkdir -p $out/${python.sitePackages}
    cp target/release/liblcd_metrics.so $out/${python.sitePackages}/lcd_metrics.so
  '';
} 
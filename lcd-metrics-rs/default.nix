{ lib
, python
, rustPlatform
, openssl
, pkgs
}:

python.pkgs.buildPythonPackage rec {
  pname = "lcd-metrics";
  version = "0.1.0";
  format = "pyproject";

  src = ./.;

  cargoDeps = rustPlatform.fetchCargoTarball {
    inherit src;
    name = "${pname}-${version}";
    hash = "sha256-as6+LFHHBpMvntCnIctqPcPwOGM4CXrMICRfMQSZjpM=";
  };

  nativeBuildInputs = with pkgs; [
    rustPlatform.cargoSetupHook
    rustPlatform.maturinBuildHook
    pkg-config
  ] ++ lib.optionals pkgs.stdenv.isDarwin [
    pkgs.darwin.apple_sdk.frameworks.CoreFoundation
  ];

  buildInputs = [
    openssl
  ];

  pythonImportsCheck = [ "lcd_metrics" ];

  meta = with lib; {
    description = "Metrics recorder for LCD display";
    homepage = "https://github.com/your/repo";
    license = licenses.mit;
  };
} 
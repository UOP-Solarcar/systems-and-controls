{
  description = "Solar Car LCD Display";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    darwin.url = "github:LnL7/nix-darwin";
    darwin.inputs.nixpkgs.follows = "nixpkgs";
    #cargo-utils.url = "github:rust-lang/cargo";
    #cargo-utils.flake = false;
  };

  outputs = { self, nixpkgs, flake-utils, darwin }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        python = pkgs.python3;
      in
      {
        packages.default = python.pkgs.buildPythonApplication {
          pname = "lcd";
          version = "0.1.0";
          format = "setuptools";

          src = self;

          propagatedBuildInputs = with python.pkgs; [
            pyqt6
            pyarrow
            pydantic
            (pkgs.callPackage ../lcd-metrics-rs {
              inherit python pkgs;
            })
          ];

          nativeBuildInputs = with pkgs; [
            python.pkgs.setuptools
            qt6Packages.wrapQtAppsHook
          ];

          buildInputs = with pkgs; [
            qt6.qtbase
            qt6.qtsvg
          ];
        };

        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            python3
            python.pkgs.pyqt6
            python.pkgs.pyarrow
            python.pkgs.virtualenv
            qt6.qtbase
            qt6.qtsvg
            black
            rustc
            cargo
            maturin
          ];

        };
      }
    );
}

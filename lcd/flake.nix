{
  description = "Solar Car LCD Display";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
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

          src = builtins.path {
            path = ./.;
            name = "lcd-source";
          };

          propagatedBuildInputs = with python.pkgs; [
            pyqt6
            pyarrow
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
            qt6.qtbase
            qt6.qtsvg
            black
            pyarrow
          ];
        };
      }
    );
}

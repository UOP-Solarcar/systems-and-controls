{
  description = "Solar Car LCD Display";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    darwin.url = "github:LnL7/nix-darwin";
    darwin.inputs.nixpkgs.follows = "nixpkgs";
    cargo-utils.url = "github:rust-lang/cargo";
    cargo-utils.flake = false;
  };

  outputs = { self, nixpkgs, flake-utils, darwin, cargo-utils }:
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

          src = ./.;

          propagatedBuildInputs = with python.pkgs; [
            pyqt6
            pyarrow
            (pkgs.callPackage ./lcd-metrics-rs {
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

          shellHook = ''
            echo "Setting up pre-commit hook for black..."
            if [ ! -f .git/hooks/pre-commit ]; then
              mkdir -p .git/hooks
              cat > .git/hooks/pre-commit << 'EOF'
#!/usr/bin/env bash
set -e
FILES=$(git diff --cached --name-only --diff-filter=d | grep -E '\.py$' || true)
if [ -n "$FILES" ]; then
  echo "Running black on Python files..."
  black $FILES
  git add $FILES
fi
EOF
              chmod +x .git/hooks/pre-commit
              echo "Pre-commit hook installed!"
            fi
          '';
        };
      }
    );
}

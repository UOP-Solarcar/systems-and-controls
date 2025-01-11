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
            python.pkgs.pyarrow
            qt6.qtbase
            qt6.qtsvg
            black
          ];

          shellHook = ''
            echo "Setting up pre-commit hook for black..."
            if [ ! -f .git/hooks/pre-commit ]; then
              mkdir -p .git/hooks
              cat > .git/hooks/pre-commit << 'EOF'
#!/usr/bin/env bash
set -e
echo "Running black on Python files..."
black $(git diff --cached --name-only --diff-filter=d | grep -E '\.py$')
git add $(git diff --cached --name-only --diff-filter=d | grep -E '\.py$')
EOF
              chmod +x .git/hooks/pre-commit
              echo "Pre-commit hook installed!"
            fi
          '';
        };
      }
    );
}

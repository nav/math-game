{ pkgs ? import <nixpkgs> {} }:

# Minimal dev shell for the SDL2 math-game spike.
# Provides just enough to compile/run a plain-SDL2 C program on macOS
# (Apple Silicon / arm64 Darwin) so the same source can later be
# rebuilt unmodified on the Raspberry Pi with apt-installed libsdl2-dev.
pkgs.mkShell {
  buildInputs = [
    pkgs.SDL2
    pkgs.freetype
  ];

  nativeBuildInputs = [
    pkgs.pkg-config
    pkgs.gcc
  ];
}

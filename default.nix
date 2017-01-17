with import <nixpkgs> {};

stdenv.mkDerivation rec {
  name = "snackbar-${version}";
  version = "0.0.1";

  src = "./";

  buildInputs = [
    cmake
    pkgconfig

    glib

    cairo
    pango
    pcre

    xorg.libX11
    xorg.libxcb
    xorg.xcbutil
    xorg.xcbutilwm
    xorg.libXdmcp
    xorg.libpthreadstubs

    nlohmann_json

    zeromq
    cppzmq
  ];
}

{
    pkgs ? import <nixpkgs> { },
}:

pkgs.mkShell {
    name = "c-dev";

    buildInputs = with pkgs; [
        gcc
        gnumake
        gdb
        pkg-config
        # add libraries here, e.g. pkgs.openssl
    ];

    # Optional: set env vars
    shellHook = ''
        echo "C dev shell loaded"
    '';
}

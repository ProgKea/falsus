{
  outputs = {self, nixpkgs} : let
    system = "x86_64-linux";
    pkgs = import nixpkgs { inherit system; };
  in
  {
    devShells.${system} = {
      default = (with pkgs; mkShell {
        buildInputs = [
          gnumake
          gf
          clang
          pkg-config
          texlive.combined.scheme-full
        ];
      });
    };
  };
}

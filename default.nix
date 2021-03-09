let pkgs = import <nixpkgs> { };
in with pkgs; rec {

  free_fdb = (callPackage ./recipe.nix) { };

}

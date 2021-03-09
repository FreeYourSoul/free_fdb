# Free FoundationDB

A C++ binding library for [FoundationDB](https://www.foundationdb.org/).

## Dependencies

* fmt : C++ formatting library (standardized in c++20)
* FoundationDB C binding : official C binding library shipped by foundationdb

## Feature & doc

* Put key/value
* Remove key/value
* Get key/value
* Get a range of key/value
* Delete a range of key/value
* Iterator implementation for range access

[Quick Start doc available here](doc/quick_start.md)

A complete documentation is available [here](doc/README.md). 

## Installation



### Using Nix

A nix utility is also present in order to install/build locally free_fdb for local development. It is used by the CI.

```shell
nix-build . -A ffdb
```

If you want to integrate free_fdb from your local nix project, you can add an attribute by fetching the recipe.nix file to your nix file directly with the url. Or you can just copy the recipe.nix file into your repository.

Example taken from the [FyS project](https://github.com/FreeYourSoul/FyS), {commit-hash} has to be replaced in order to fix the recipe to use in your project:

```shell
let pkgs = import <nixpkgs> { };
in with pkgs; rec {

  # External Dependencies (personal)
  ffdb = (callPackage (builtins.fetchurl
    "https://raw.githubusercontent.com/FreeYourSoul/free_fdb/{commit-hash}/recipee.nix") {
      rev = "{commit-hash}";
    });
    
    #.... your nix
```


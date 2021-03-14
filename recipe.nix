{ stdenv, callPackage, lib, cmake, catch2, fmt, nix-gitignore, foundationdb, use_revision ? null }:

stdenv.mkDerivation rec {
  version = "1.0.0";
  pname = "free_fdb";

  src = if (builtins.isNull use_revision || use_revision == "") then
    nix-gitignore.gitignoreSource [ ".git" ] ./.
  else
    builtins.fetchGit {
      url = "https://github.com/FreeYourSoul/free_fdb.git";
      rev = use_revision;
    };

  buildInputs = [ cmake catch2 fmt foundationdb ];
  cmakeFlags = [ "-DBUILD_TESTING=ON" ];

  doCheck = true;

  checkPhase = ''
    fdbserver -p 127.0.0.1:4500 -C ${src}/test/fdb.cluster -d /tmp

    ctest -VV    
  '';

  meta = with lib; {
    maintainers = "quentin_balland";
    homepage = "http://freeyoursoul.online";
    description = description;
    licences = licenses.mit;
    platforms = platforms.linux;
  };

}

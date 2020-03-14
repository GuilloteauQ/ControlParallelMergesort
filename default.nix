with import <nixpkgs>{};

let
  version = "1.0";
in {
  feedback_parsort = stdenv.mkDerivation rec {
    name = "feedback_parsort-${version}";
    src = ./.;
    buildInputs = [
      gcc
      coreutils
      # openmp
    ];
    installPhase = ''
      mkdir -p $out
      make feedback_parsort
      cp feedback_parsort $out
    '';
  };
}

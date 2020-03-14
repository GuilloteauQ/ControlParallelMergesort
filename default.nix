with import <nixpkgs>{};

let
  version = "1.0";
in {
  feedback_parsort = stdenv.mkDerivation rec {
    name = "feedback_parsort-${version}";
    src = ./.;
    buildInputs = [
      gcc
      clang
      coreutils
      # openmp
      texlive.combined.scheme-full
      R
      pandoc
      rPackages.ggplot2
      rPackages.knitr
      rPackages.rmarkdown
    ];
    installPhase = ''
      mkdir -p $out
      make feedback_parsort
      cp feedback_parsort $out
    '';
  };
}

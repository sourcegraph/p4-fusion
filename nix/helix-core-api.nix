{ fetchzip }:
{
  "1.1" = {
    aarch64-darwin = fetchzip {
      name = "helix-core-api";
      url = "https://filehost.perforce.com/perforce/r25.2/bin.macosx12arm64/p4api-openssl1.1.1.tgz";
      hash = "sha256-Itj1Tv1iAQysLcSNi3FyGYzYKpPoqGoowowitHHLpUk=";
    };
    x86_64-darwin = fetchzip {
      name = "helix-core-api";
      url = "https://filehost.perforce.com/perforce/r25.2/bin.macosx12x86_64/p4api-openssl1.1.1.tgz";
      hash = "sha256-A/oBv5vcxKp77Bfv/L8AmWPvwOn7AKvcbwp8Jq5I+JE=";
    };
    x86_64-linux = fetchzip {
      name = "helix-core-api";
      url = "https://filehost.perforce.com/perforce/r25.2/bin.linux26x86_64/p4api-glibc2.3-openssl1.1.1.tgz";
      hash = "sha256-UgC8ENS6jCRDBjznuwr4DbkirFHr6SS1Ay4NEWkmykU=";
    };
  };
  "3.0" = {
    aarch64-darwin = fetchzip {
      name = "helix-core-api";
      url = "https://filehost.perforce.com/perforce/r25.2/bin.macosx12arm64/p4api-openssl3.tgz";
      hash = "sha256-ab6wYii7X2qlStQrS4dKXGkHbURkEA4q1o5nwFxtkbg=";
    };
    x86_64-darwin = fetchzip {
      name = "helix-core-api";
      url = "https://filehost.perforce.com/perforce/r25.2/bin.macosx12x86_64/p4api-openssl3.tgz";
      hash = "sha256-yi1OrOXK5zpjWbXAwb/TNmB46y+sx3d3HIyJ10P5wfA=";
    };
    x86_64-linux = fetchzip {
      name = "helix-core-api";
      url = "https://filehost.perforce.com/perforce/r25.2/bin.linux26x86_64/p4api-glibc2.3-openssl3.tgz";
      hash = "sha256-Edo4gVmn0JuR/twVEYA9oekC4GXQpV1PlH24j7jyLh4=";
    };
  };
}

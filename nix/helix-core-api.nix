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
      hash = "sha256-fkUww2UsmFZKIL7SRzBhSdc5tE5D41ZlB/TYgpFcgF0=";
    };
    x86_64-darwin = fetchzip {
      name = "helix-core-api";
      url = "https://filehost.perforce.com/perforce/r25.2/bin.macosx12x86_64/p4api-openssl3.tgz";
      hash = "sha256-TbZTFvUBWLdHZwgn26bI2vTUO/UTs32qILPnujkccn4=";
    };
    x86_64-linux = fetchzip {
      name = "helix-core-api";
      url = "https://filehost.perforce.com/perforce/r25.2/bin.linux26x86_64/p4api-glibc2.3-openssl3.tgz";
      hash = "sha256-r3S9juAgCcTHKzac5MOsqhVhdCGK1TUwjUc1pPN01ec=";
    };
  };
}

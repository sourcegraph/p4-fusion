{ fetchzip }:
{
  "1.1" = {
    aarch64-darwin = fetchzip {
      name = "helix-core-api";
      url = "https://filehost.perforce.com/perforce/r23.1/bin.macosx12arm64/p4api-openssl1.1.1.tgz";
      hash = "sha256-3j3WnBdfTa9job+w8mzcAltO8wgbMlxA/aJxWlLn/Dk=";
    };
    x86_64-darwin = fetchzip {
      name = "helix-core-api";
      url = "https://filehost.perforce.com/perforce/r23.1/bin.macosx12x86_64/p4api-openssl1.1.1.tgz";
      hash = "sha256-go7uhe6eAX+rnnn5zzmKPya93ZK65q6bL0nsDXLx0ZA=";
    };
    x86_64-linux = fetchzip {
      name = "helix-core-api";
      url = "https://filehost.perforce.com/perforce/r23.1/bin.linux26x86_64/p4api-glibc2.3-openssl1.1.1.tgz";
      hash = "sha256-8Nv2xbXEYrheS930zyozSqMuC7X2V81IWW2woD/jqB4=";
    };
  };
  "3.0" = {
    aarch64-darwin = fetchzip {
      name = "helix-core-api";
      url = "https://filehost.perforce.com/perforce/r23.1/bin.macosx12arm64/p4api-openssl3.tgz";
      hash = "sha256-dCeOGcJ3+wHwFFL03J+KwpDHB+Kx0IdQxJ82IBScJXA=";
    };
    x86_64-darwin = fetchzip {
      name = "helix-core-api";
      url = "https://filehost.perforce.com/perforce/r23.1/bin.macosx12x86_64/p4api-openssl3.tgz";
      hash = "sha256-esmStuzKcpLcw18pTzsR1ZQUfoEqLhOw0yIZxFFlgu0=";
    };
    x86_64-linux = fetchzip {
      name = "helix-core-api";
      url = "https://filehost.perforce.com/perforce/r23.1/bin.linux26x86_64/p4api-glibc2.3-openssl3.tgz";
      hash = "sha256-OAR11FKkulpoReMGnDQwFOeJtF+WA/4lxWMqrSNiLPI=";
    };
  };
}

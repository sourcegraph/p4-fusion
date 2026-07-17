{ fetchzip }:
{
  "1.1" = {
    aarch64-darwin = fetchzip {
      name = "helix-core-api";
      url = "https://filehost.perforce.com/perforce/r23.1/bin.macosx12arm64/p4api-openssl1.1.1.tgz";
      hash = "sha256-oUzpbJhcDEgJ5gxQ2dHZ/M2hhWTj0p4Pv9iL6GOvCXY=";
    };
    x86_64-darwin = fetchzip {
      name = "helix-core-api";
      url = "https://filehost.perforce.com/perforce/r23.1/bin.macosx12x86_64/p4api-openssl1.1.1.tgz";
      hash = "sha256-1HQXcF/3lRv5H2L2IKvWNyYYY6myTOSTttIl46UBTfY=";
    };
    x86_64-linux = fetchzip {
      name = "helix-core-api";
      url = "https://filehost.perforce.com/perforce/r23.1/bin.linux26x86_64/p4api-glibc2.3-openssl1.1.1.tgz";
      hash = "sha256-891vpiR0Ni3ntdHvsQ4hviS/et2YKDBgIg3bGQ0xFI0=";
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

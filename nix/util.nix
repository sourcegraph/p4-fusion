# yoinked from https://sourcegraph.com/github.com/sourcegraph/sourcegraph/-/blob/dev/nix/util.nix
{ lib, pkgs }:
{
  # utility function to add some best-effort flags for emitting static objects instead of dynamic.
  # needed until https://github.com/NixOS/nixpkgs/pull/256590 makes pkgsStatic != pkgs in darwin.
  mkStatic = drv:
    assert lib.assertMsg (lib.isDerivation drv) "mkStatic expects a derivation, got ${builtins.typeOf drv}";
    assert lib.assertMsg (drv ? "overrideAttrs") "mkStatic expects an overridable derivation";

    let
      auto = builtins.intersectAttrs drv.override.__functionArgs { withStatic = true; static = true; enableStatic = true; enableShared = false; };
      overridden = drv.overrideAttrs (oldAttrs: {
        dontDisableStatic = true;
      } // lib.optionalAttrs (!(oldAttrs.dontAddStaticConfigureFlags or false)) {
        configureFlags = (oldAttrs.configureFlags or [ ]) ++ [ "--disable-shared" "--enable-static" "--enable-shared=false" ];
      });
    in
    if drv.pname == "openssl" then drv.override { static = true; } else overridden.override auto;

  # doesn't actually change anything in practice, just makes otool -L not display nix store paths for dynamic libraries.
  # these are expected to exist in macos dydl cache anyways, so where they point to is irrelevant. worst case, this will let 
  # you catch earlier when a library that should be statically linked, or that isnt in dydl cache, is dynamically linked.
  unNixifyDylibs = drv:
    assert lib.assertMsg (lib.isDerivation drv) "unNixifyDylibs expects a derivation, got ${builtins.typeOf drv}";
    assert lib.assertMsg (drv ? "overrideAttrs") "unNixifyDylibs expects an overridable derivation";

    drv.overrideAttrs (oldAttrs: lib.optionalAttrs pkgs.hostPlatform.isMacOS {
      preFixup = (oldAttrs.postFixup or "") + ''
        for bin in $(${pkgs.findutils}/bin/find $out/bin -type f); do
          for lib in $(${pkgs.darwin.cctools}/bin/otool -L $bin | ${pkgs.coreutils}/bin/tail -n +2 | ${pkgs.coreutils}/bin/cut -d' ' -f1 | ${pkgs.gnugrep}/bin/grep nix); do
            echo "patching dylib from "$lib" to "@rpath/$(basename $lib)""
            ${pkgs.darwin.cctools}/bin/install_name_tool -change "$lib" "@rpath/$(basename $lib)" $bin
          done
        done
      '';
    });
}

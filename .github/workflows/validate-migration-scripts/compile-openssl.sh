#!/usr/bin/env bash

SCRIPT_ROOT="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
cd "${SCRIPT_ROOT}/../../.."

set -euxo pipefail

pushd "${OPENSSL_SOURCE_DIR}"

CONFIG_ARGS=(
  --prefix="${OPENSSL_INSTALL_DIR}"
)

read -r -a array <<<"${OPENSSL_FLAGS:-}"
for flag in "${array[@]}"; do
  if [[ -n "${flag}" ]]; then
    CONFIG_ARGS+=("$flag")
  fi
done

./config "${CONFIG_ARGS[@]}"
make "-j$(nproc)"

sudo make install

popd
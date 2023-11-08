#!/usr/bin/env bash

SCRIPT_ROOT="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
cd "${SCRIPT_ROOT}/../../.."

set -euxo pipefail

pushd "${VALGRIND_SOURCE_DIR}"

./configure --prefix="${VALGRIND_INSTALLATION_DIR}"
make "-j$(nproc)"
sudo make install

popd

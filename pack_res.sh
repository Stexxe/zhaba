#!/usr/bin/env bash

if ! which xxd > /dev/null; then
    echo "xxd program not found"
    exit 1
fi

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
mkdir -p "${SCRIPT_DIR}/lib/res"
xxd -include -name res_style_css "${SCRIPT_DIR}/data/css/style.css" > "${SCRIPT_DIR}/lib/res/style.inc"
xxd -include -name res_reset_css "${SCRIPT_DIR}/data/css/reset.css" > "${SCRIPT_DIR}/lib/res/reset.inc"

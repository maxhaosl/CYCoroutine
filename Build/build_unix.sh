#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OS=$(uname -s)

if [[ "$OS" == "Darwin" ]]; then
    exec "$SCRIPT_DIR/build_mac.sh" "$@"
fi

if [[ "$OS" != "Linux" ]]; then
    echo "Unsupported UNIX-like platform: $OS"
    exit 1
fi

transform_shared_kinds() {
    local input="$1"
    local -a result=()
    local token
    for token in $input; do
        token=$(printf '%s' "$token" | tr '[:upper:]' '[:lower:]')
        case "$token" in
            on|shared|dyn|dynamic) result+=("Shared") ;;
            off|static) result+=("Static") ;;
        esac
    done
    if [ "${#result[@]}" -eq 0 ]; then
        result=("Static" "Shared")
    fi
    local IFS=','
    echo "${result[*]}"
}

linux_build_types=${CYCOROUTINE_BUILD_TYPES:-"Release"}
linux_arches=${CYCOROUTINE_LINUX_ARCH:-$(uname -m)}

if [ -n "${CYCOROUTINE_SHARED_KINDS:-}" ]; then
    linux_lib_types=$(transform_shared_kinds "$CYCOROUTINE_SHARED_KINDS")
else
    linux_lib_types=${CYCOROUTINE_LIB_TYPES:-"Static,Shared"}
fi

exec "$SCRIPT_DIR/build_linux_all.sh" "$linux_build_types" "$linux_lib_types" "$linux_arches"

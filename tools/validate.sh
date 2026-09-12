#!/usr/bin/env bash
set -euo pipefail

mode="${1:---all}"
project="${2:-}"
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

usage() {
    echo "Usage: $0 [--all|--static|--firmware] [project]" >&2
}

run_static_checks() {
    local actionlint_bin
    local test_dir

    python3 tools/check_repo.py

    actionlint_bin="${ACTIONLINT_BIN:-}"
    if [[ -z "${actionlint_bin}" ]]; then
        actionlint_bin="$(command -v actionlint || true)"
    fi
    if [[ -z "${actionlint_bin}" || ! -x "${actionlint_bin}" ]]; then
        actionlint_bin="$(./tools/install-actionlint.sh)"
    fi
    "${actionlint_bin}" -color .github/workflows/*.yml

    test_dir="$(mktemp -d /tmp/ai-passport-host-tests.XXXXXX)"
    host_test() {
        local name="$1" app="$2"
        shift 2
        local sources=() source
        for source in "$@"; do sources+=("projects/${app}/main/${source}.c"); done
        "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -I"projects/${app}/main" \
            "projects/${app}/tests/test_${name}.c" "${sources[@]}" -o "${test_dir}/${name}"
        "${test_dir}/${name}"
    }
    host_test ui_pixel_math bsp-demo ui_pixel_math
    host_test hanzi_story animal-hanzi-story hanzi_story hanzi_save
    host_test hanzi_adpcm animal-hanzi-story hanzi_adpcm hanzi_player
    host_test cards_model hanzi-cards cards_model
    host_test cards_adpcm hanzi-cards cards_adpcm cards_player cards_model
    host_test passport_protocol codex-passport passport_protocol passport_storage
    host_test passport_idle codex-passport passport_idle
    host_test passport_alert codex-passport passport_alert
    host_test passport_adpcm codex-passport passport_adpcm
    host_test passport_voice codex-passport passport_adpcm
    host_test xiaozhi_state xiaozhi-deepseek xiaozhi_state
    host_test deepseek_parser xiaozhi-deepseek deepseek_sse_parser
    for suite in projects/*/tests/test_*.py; do
        python3 "${suite}"
    done
    rm -rf "${test_dir}"
    echo "Host tests: PASS"
}

run_firmware_checks() (
    local validation_build_dir

    if ! command -v idf.py >/dev/null 2>&1; then
        echo "ERROR: idf.py is not available; activate ESP-IDF 5.5.3 first." >&2
        return 1
    fi

    local app app_dir
    local apps=()
    if [[ -n "${project}" ]]; then
        [[ "${project}" != */* && -f "projects/${project}/CMakeLists.txt" ]] || {
            echo "Unknown project: ${project}" >&2; return 1;
        }
        apps+=("projects/${project}")
    else
        for app_dir in projects/*; do
            [[ -f "${app_dir}/CMakeLists.txt" ]] && apps+=("${app_dir}")
        done
    fi
    mkdir -p "${repo_root}/build"
    for app_dir in "${apps[@]}"; do
        app="$(basename "${app_dir}")"
        validation_build_dir="${repo_root}/build/validate-${app}"
        SDKCONFIG_DEFAULTS="${repo_root}/${app_dir}/sdkconfig.defaults" \
            idf.py -C "${app_dir}" -B "${validation_build_dir}" \
            -D "SDKCONFIG=${validation_build_dir}/sdkconfig" build
        idf.py -C "${app_dir}" -B "${validation_build_dir}" merge-bin \
            -o "${validation_build_dir}/${app}-full.bin"
        python3 tools/verify_firmware.py "${validation_build_dir}" "${app}"
        cp "${validation_build_dir}/${app}-full.bin" "${repo_root}/build/${app}-full.bin"
        if [[ "${app}" == "animal-hanzi-story" ]]; then
            cp "${repo_root}/build/${app}-full.bin" "${repo_root}/build/FoloToy-AI-Passport-full.bin"
        fi
    done
    echo "Firmware build: PASS"
)

cd "${repo_root}"
case "${mode}" in
    --all)
        run_static_checks
        run_firmware_checks
        ;;
    --static)
        run_static_checks
        ;;
    --firmware)
        run_firmware_checks
        ;;
    *)
        usage
        exit 2
        ;;
esac

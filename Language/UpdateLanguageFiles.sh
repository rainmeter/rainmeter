#!/bin/sh

# Synchronize language files with en.ini while preserving existing translations.

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
template="$script_dir/en.ini"

for language_file in "$script_dir"/*.ini; do
  [ "$language_file" = "$template" ] && continue

  temporary_file=$(mktemp "${language_file}.XXXXXX")
  trap 'rm -f "$temporary_file"' EXIT HUP INT TERM

  awk '
    function trim(value) {
      sub(/^[[:space:]]+/, "", value)
      sub(/[[:space:]]+$/, "", value)
      return value
    }

    NR == FNR {
      if ($0 ~ /^\[/) {
        section = $0
        next
      }

      equals = index($0, "=")
      if (equals && $0 !~ /^[[:space:];#]/) {
        key = trim(substr($0, 1, equals - 1))
        value = substr($0, equals + 1)
        overrides[section SUBSEP key] = value
      }
      next
    }

    /^\[/ {
      section = $0
      print
      next
    }

    {
      equals = index($0, "=")
      if (equals && $0 !~ /^[[:space:];#]/) {
        key = trim(substr($0, 1, equals - 1))
        id = section SUBSEP key
        print key "=" (id in overrides ? overrides[id] : "")
      } else {
        print
      }
    }
  ' "$language_file" "$template" > "$temporary_file"

  # Language files in the repository are world-readable after synchronization.
  chmod 644 "$temporary_file"
  mv "$temporary_file" "$language_file"
  trap - EXIT HUP INT TERM
done

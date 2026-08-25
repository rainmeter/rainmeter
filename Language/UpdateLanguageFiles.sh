#!/bin/sh

# Synchronize language files with en.ini while preserving existing translations.

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
template="$script_dir/en.ini"
temp_file="$script_dir/temp.txt"
trap 'rm -f "$temp_file"' EXIT HUP INT TERM

for language_file in "$script_dir"/*.ini; do
	[ "$language_file" = "$template" ] && continue

	awk -v language_name="$(basename "$language_file")" '
    function trim(value) {
      sub(/^[[:space:]]+/, "", value)
      sub(/[[:space:]]+$/, "", value)
      return value
    }

    function count(text, needle,   total, position) {
      total = 0
      while ((position = index(text, needle)) > 0) {
        total++
        text = substr(text, position + length(needle))
      }
      return total
    }

    # Verify that the translation uses the same amount of line breaks as the
    # English string. "$\n" is used by the installer strings and "\n" elsewhere.
    function check_line_breaks(key, translation, original,   installer, plain) {
      if (translation == "") return

      installer = count(original, "$\\n")
      plain = count(original, "\\n") - installer
      if (count(translation, "$\\n") != installer ||
          count(translation, "\\n") - count(translation, "$\\n") != plain) {
        printf "%s: %s has %d \"$\\n\" and %d \"\\n\" instead of %d and %d\n",
          language_name, key, count(translation, "$\\n"),
          count(translation, "\\n") - count(translation, "$\\n"), installer, plain > "/dev/stderr"
      }
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
        override_value[key] = value
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
        # Prefer a translation in the current section. If a key has moved,
        # retain its translation from its previous section.
        if (id in overrides) {
          value = overrides[id]
        } else if (key in override_value) {
          value = override_value[key]
        } else {
          value = ""
        }
        check_line_breaks(key, value, substr($0, equals + 1))
        print key "=" value
      } else {
        print
      }
    }
  ' "$language_file" "$template" > "$temp_file"

	mv "$temp_file" "$language_file"
done

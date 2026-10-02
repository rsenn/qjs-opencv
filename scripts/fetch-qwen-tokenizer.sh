#!/bin/sh
# Download the Qwen2.5 tokenizer fixture used by tests/test_tokenizer.js into
# tests/qwen2.5-tokenizer/ (tokenizer.json + a config.json declaring the
# model_type OpenCV's Tokenizer::load() needs).
set -eu

repo=Qwen/Qwen2.5-0.5B-Instruct
dir=${1:-$(dirname "$0")/../tests/qwen2.5-tokenizer}
url=https://huggingface.co/$repo/resolve/main/tokenizer.json

mkdir -p "$dir"

if command -v curl >/dev/null 2>&1; then
  curl -fL --retry 3 -o "$dir/tokenizer.json.tmp" "$url"
elif command -v wget >/dev/null 2>&1; then
  wget -O "$dir/tokenizer.json.tmp" "$url"
else
  echo "need curl or wget" >&2
  exit 1
fi

mv "$dir/tokenizer.json.tmp" "$dir/tokenizer.json"
printf '{"model_type": "qwen2.5"}\n' >"$dir/config.json"
echo "wrote $dir/tokenizer.json, $dir/config.json"

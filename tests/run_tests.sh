!/bin/sh
set -eu

PASS_COUNT=0
FAIL_COUNT=0
SKIPPED_COUNT=0

cleanup() {
  rm -f out.txt in.txt grep.txt out.tmp minishell 2>/dev/null || true
}
trap cleanup EXIT

# Prefer GNU coreutils `timeout`; fall back to `gtimeout` (macOS via brew).
TIMEOUT_CMD=""
if command -v timeout >/dev/null 2>&1; then
  TIMEOUT_CMD="timeout"
elif command -v gtimeout >/dev/null 2>&1; then
  TIMEOUT_CMD="gtimeout"
fi

assert_contains() {
  desc=$1
  data=$2
  expected=$3
  if echo "$data" | grep -Fq "$expected"; then
    echo "PASS: $desc"
    PASS_COUNT=$((PASS_COUNT+1))
  else
    echo "FAIL: $desc"
    echo "  expected to find: $expected"
    echo "  ---- output ----"
    echo "$data"
    echo "  ----------------"
    FAIL_COUNT=$((FAIL_COUNT+1))
  fi
}

assert_file_contains() {
  desc=$1
  file=$2
  expected=$3
  if [ ! -f "$file" ]; then
    echo "FAIL: $desc (missing file: $file)"
    FAIL_COUNT=$((FAIL_COUNT+1))
    return
  fi
  if grep -Fq "$expected" "$file"; then
    echo "PASS: $desc"
    PASS_COUNT=$((PASS_COUNT+1))
  else
    echo "FAIL: $desc"
    echo "  expected to find: $expected in $file"
    echo "  ---- $file ----"
    cat "$file" || true
    echo "  ---------------"
    FAIL_COUNT=$((FAIL_COUNT+1))
  fi
}

echo "Building minishell..."
gcc -Wall -std=c11 -o minishell ../bourne-shell/b-shell_main.c ../bourne-shell/b-shell_function.c

# 1) simple echo
out=$(printf 'echo hello\nexit\n' | ./minishell)
assert_contains "echo prints 'hello'" "$out" "hello"

# 2) pipeline
out=$(printf 'echo hello | tr a-z A-Z\nexit\n' | ./minishell)
assert_contains "pipeline uppercases to HELLO" "$out" "HELLO"

# 3) output redirection >
printf 'echo hi > out.txt\nexit\n' | ./minishell >/dev/null
assert_file_contains "redirection > writes to out.txt" "out.txt" "hi"
rm -f out.txt

# 4) input redirection <
echo "world" > in.txt
out=$(printf 'cat < in.txt\nexit\n' | ./minishell)
assert_contains "redirection < reads from in.txt" "$out" "world"
rm -f in.txt

# 5) cd builtin to parent
parent=$(cd .. && pwd -P)
out=$(printf 'cd ..\npwd\nexit\n' | ./minishell)
assert_contains "cd builtin changes directory" "$out" "$parent"

# 6) cd error (no args)
out=$(printf 'cd\nexit\n' | ./minishell 2>&1 || true)
assert_contains "cd without arg errors" "$out" "cd: missing argument"

# 7) command not found
out=$(printf 'nosuchcmd\nexit\n' | ./minishell 2>&1 || true)
assert_contains "unknown command reports error" "$out" "nosuchcmd: command not found"

# 8) background job (&)
if [ -n "$TIMEOUT_CMD" ]; then
  out=$($TIMEOUT_CMD 2s sh -c "printf 'sleep 3 &\necho done\nexit\n' | ./minishell" || true)
  assert_contains "background job does not block foreground" "$out" "done"
else
  echo "SKIP: background job (requires 'timeout' or 'gtimeout')"
  SKIPPED_COUNT=$((SKIPPED_COUNT+1))
fi

# 9) grep pipeline
printf 'echo foo > grep.txt\necho bar >> grep.txt\ncat grep.txt | grep foo\nexit\n' | ./minishell > out.tmp
assert_file_contains "pipeline with grep filters lines" "out.tmp" "foo"
rm -f grep.txt out.tmp

# Summary
TOTAL=$((PASS_COUNT+FAIL_COUNT))
if [ "$FAIL_COUNT" -gt 0 ]; then
  echo "RESULT: $PASS_COUNT/$TOTAL tests passed, $FAIL_COUNT failed, $SKIPPED_COUNT skipped."
  exit 1
fi
echo "RESULT: $PASS_COUNT/$TOTAL tests passed, $SKIPPED_COUNT skipped. All tests passed."



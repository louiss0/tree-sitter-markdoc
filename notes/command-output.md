# Command output

## `npx tree-sitter test --overview-only`

```
npm warn Unknown env config "http-proxy". This will stop working in the next major version of npm.
```

- The command exited successfully with no segmentation faults observed.

## `npx tree-sitter parse --config-path /tmp/tree-sitter-config.json --scope source.markdoc <file>`

Each file in `test/corpus/*.txt` was parsed in a loop. Output consisted of the npm proxy warning only; no segmentation faults were observed.

Example output for each invocation:

```
npm warn Unknown env config "http-proxy". This will stop working in the next major version of npm.
```

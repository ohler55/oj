# Oj Install Options

### Enable trace log

```
$ gem install oj -- --enable-trace-log
```

To enable Oj trace feature, it uses `--enable-trace-log` option when installing the gem.
Then, the trace logs will be displayed when `:trace` option is set to `true`.


### SIMD instructions

SIMD optimizations are enabled automatically, so no install option is
required. At runtime Oj detects the CPU features that are available and
selects the best implementation: SSE4.2 or SSE2 on x86 / x86_64, NEON on ARM,
and a scalar fallback when none is available.

Earlier versions required a `--with-sse42` option at install time to turn on
SSE4.2. That option has been removed (#982); SSE4.2 is now selected
automatically when the CPU supports it.

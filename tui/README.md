# TUI library

## Input reader

Exists to abstract os-specific reader such as editline on linux/mac and generic fgets in order to have
the behavior for editing line and history on both windows and posix.

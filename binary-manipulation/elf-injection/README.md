# ELF PT_LOAD Injection

A simple implementation of ELF binary infection using PT_LOAD segment injection technique.

## Overview
This code demonstrates how to inject custom code into ELF binaries by adding a new PT_LOAD segment. The injected code executes before the original program and then returns control to the original entry point.

## Usage
```bash
gcc -o injection injection.c
./injection <target_binary> <output_binary>
```

## Features
- Adds new PT_LOAD segment
- Preserves original program functionality
- Properly handles memory alignment
- Maintains ELF structure integrity

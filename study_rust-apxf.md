# Background

Recent compiler releases - [GCC][1] and [Clang][2] have introduced 
support for [Intel APX][3], which primarily adds access to the extended 
general-purpose registers (EGPRs, `R16-R31`).

The Linux kernel recently gains `CONFIG_X86_NATIVE_CPU`, which allows 
building the kernel with `-march=native` so that the compiler can 
optimize for the feature available on the build machine. Once APX-capable 
systems become available, enabling this option could cause the compiler 
to emit APX instructions opportunistically.

The kernel, however, is not yet prepared to use EGPRs internally. In 
particular, there is currently no support for saving and restoring EGPR 
state for general in-kernel uses. Until such support exists, kernel 
builds must avoid generating APX instructions.

# Rust/APX Build Investigation

While the GCC and Clang behaviors are relatively straightforward, the 
Rust toolchain is newer and interacts closely with LLVM (Clang) for code 
generation. Rust has been gradually introducing APX-related support, but 
much of it remain unstable and its interaction with CPU selection option 
(such as `native`) is not immediately obvious.

The goal of this investigation is to determine which Rust verisons and 
compiler options can result APX instructions (actually EGPR accesses) 
being generated. This was [asked][8] by the Rust maintainer:

> But please double-check the interaction between those and test that 
> LLVM is actually getting the right set of features you want.


## Approach

The evaluation compares the generated object code while varying:

 * Rust version
 * CPU target: `native` vs `x86-64`
 * APX target feature: `+/- apxf` or unspecified

Rather than relying on compiler documentation or source code alone, the 
generated object files are disassembled and inspected for accesses EGPRs. 

## Setup

Rust support may require additional setup beyond the default kernel 
build environment. The kernel documentation already describes those 
prerequisites in detail.

The test setup used:

 * Kernel tree: Linux v6.12-rc2
 * LLVM/Clang : version 20.1.2

```
$ clang --version
Ubuntu clang version 20.1.2 (0ubuntu1)
...
```

## Rust Versions

Rust has evolved rapidly with respect to APX support. Two milestones are 
particularly relevant according to release and pull requests: 

 * v1.88: [initial APX target feature support][5] 
 * v1.91: [runtime APX detection support][6]

Currently, 

 * v1.97: Stable
 * v1.98: Beta
 * v1.99: Nightly

So version 1.87 ... 1.99 were tested.

Each version was built under six configurations:

 1. `native`
 2. `native + apxf`
 3. `native - apxf`
 4. `x86-64`
 5. `x86-64 + apxf`
 6. `x86-64 - apxf` 

## Reference Object

The generated `rust/core.o` object was selected as the insepction target 
because it appears to represent the core Rust support built into the 
kernel.

## Test Script

```
#!/bin/sh

rustup override set $1
rustup component add rust-src

rm -f rust/core.o
make LLVM=1 rust/ V=1

objdump -d --no-show-raw-insn rust/core.o | \
  sed -E 's/^[[:space:]]+[0-9a-fA-F]+:[[:space:]]+//' | \
  grep -E '%r16|%r17|%r18|%r19|%r20|%r21|%r22|%r23|%r24|%r25|%r26|%r27|%r28|%r29|%r30|%r31' | \ 
  wc -l
```

The reported number is the count of instructions referencing EGPRs

# Result

| Rust version   | `native` | `native +apxf` | `native -apxf` | `x86-64` | `x86-64 +apxf` |  `x86-64 -apxf` |
| ---            | ---      | ---            | ---            | ---      | ---            | ---             |
| 1.87           | 736      | 736            | 736            | 0        | 0              | 0               |
| 1.88           | 673      | 673            | 0              | 0        | 658            | 0               |
| 1.89           | 529      | 529            | 0              | 0        | 531            | 0               |
| 1.90           | 525      | 525            | 0              | 0        | 531            | 0               |
| 1.91           | 448      | 448            | 0              | 0        | 465            | 0               |
| 1.92           | 442      | 442            | 0              | 0        | 459            | 0               |
| 1.93           | 553      | 553            | 0              | 0        | 584            | 0               |
| 1.94           | 494      | 494            | 0              | 0        | 536            | 0               |
| 1.95           | 0        | 501            | 0              | 0        | 526            | 0               |
| 1.96           | 0        | 494            | 0              | 0        | 535            | 0               |
| 1.97 (stable)  | 0        | 492            | 0              | 0        | 533            | 0               |
| 1.98 (beta)    | 0        | 437            | 0              | 0        | 476            | 0               |
| 1.99 (nightly) | 0        | 437            | 0              | 0        | 476            | 0               |

## Observations

 * The effective APX option (`apxf`) first appears in Rust 1.88
   Explicit `+apxf` looks always generating APX instructions
 * Beginning with Rust 1.95, `target-cpu=native` gates APX code 
   generation by itself.
 * Over time, the compiler has reducted the amount of generated APX code,
   perhaps indicating improvements in register allocation and instruction 
   selections while using the same LLVM backend.

# Possible Solution

 1. Beginning with Rust 1.95, native builds appear to avoid generating 
    APX instructions automatically. One possible approach would therefore 
    be to require Rust 1.95 as the minimum version for native kernel 
    builds.

 2. Another option is explicitly disable APX by passing the negative APX 
    target feature (`-apxf`). However, current Rust releases still warn 
    about this option because the feature is not stablized yet. 

    Miguel Ojeda suggested [a workaround][8] to suppress the warning. If 
    that proves reliable, a more attractive option would be to lower the 
    minimum version to 1.88 while always passing the negative APX target 
    feature, thereby preventing APX code generation regardless of future 
    changes in compiler defaults.

## Depressing `apxf` Warning

Using [a custom target JSON file][9] provides a workaround for the 
warning against the currently unstable target feature.

```
$ rustc ... --target ./scripts/target.json ...
$ cat ./scripts/target.json

{
    "arch": "x86_64",
    "rustc-abi": "x86-softfloat",
    "data-layout": "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128",
    "features": "-mmx,+soft-float,+retpoline-external-thunk,+retpoline-indirect-branches,+retpoline-indirect-calls",
    "llvm-target": "x86_64-linux-gnu",
    "supported-sanitizers": ["kcfi","kernel-address"],
    "target-pointer-width": 64,
    "emit-debug-gdb-scripts": false,
    "frame-pointer": "may-omit",
    "stack-probes": {"kind": "none"}
}
```

This approach suppresses the warning that would otherwise be emitted when 
specifying the target feature directly:

```
warning: unstable feature specified for `-Ctarget-feature`: `apxf`
  |
  = note: this feature is not stably supported; its behavior can change in the future

warning: 1 warning emitted
```

Older Rust releases prior to 1.93 however also emit an additional 
warning:

```
'-apxf' is not a recognized feature for this target (ignoring feature)
```

Note that Rust 1.93 still emits the "unstable feature" warning when 
`-Ctarget-feature=-apxf` is passed directly on the command line. However, 
using the generated JSON file avoids this warning while still disabling 
APX.

So, Rust 1.93 with the `--target JSON` mechanism appears to provide the 
balance. It allows APX to be disabled without warning, so making Rust 
1.93 the minimum version for native builds.

[1]: https://gcc.gnu.org/pipermail/gcc-patches/2023-August/628905.html
[2]: https://github.com/llvm/llvm-project/pull/74199
[3]: https://cdrdv2.intel.com/v1/dl/getContent/784266
[4]: https://docs.kernel.org/rust/quick-start.html
[5]: https://github.com/rust-lang/rust/pull/139534
[6]: https://github.com/rust-lang/rust/pull/145531
[7]: https://releases.rs/
[8]: https://lore.kernel.org/lkml/20260712202538.GA1697833@ax162/
[9]: https://rust-lang.github.io/rfcs/0131-target-specification.html

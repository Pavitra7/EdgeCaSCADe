# Comet

[![pipeline status](https://gitlab.inria.fr/srokicki/Comet/badges/master/pipeline.svg)](https://gitlab.inria.fr/srokicki/Comet/commits/master)

A RISC-V 32-bit processor written in C++ for High Level Synthesis (HLS).
Support for the RV32I base ISA, the **M** (Multiplication), the **A** (Atomic), and the **Zicsr** extension. The Comet simulator is able to simulate the **Core-Local Interrupt controller** (CLINT). There is [branch dedicated to the support of the F extension](https://gitlab.inria.fr/srokicki/Comet/tree/rv32imf) but it is not stable yet and may not be in a functional state.

![Comet Architecture Pipeline](./docs/global_macro_2.PNG)

The full documentation of the core is inside `<repo_root>/docs/comet_doc.pdf`

## Dependencies
The only dependency to satisfy in order to build the simulator is `cmake`.

To compile the tests, a RISC-V toolchain and libraries are needed:
  - [RISC-V GNU toolchain](https://github.com/riscv/riscv-gnu-toolchain)

## Building the simulator

Make sure that the dependencies are met by installing `cmake` using your package manager or directly from the sources.

For exemple on a debian based system, run `sudo apt install cmake`.

Once the repository cloned, `cd` into it.
```
mkdir build && cd build
cmake ..
make
```

The simulator binary is located in `<repo_root>/build/bin` under the name `comet.sim`

#### Building the GNU RISC-V toolchain

To produce binaries that can be executed by Comet, a working 32bit RISC-V toolchain and libraries are needed.
To build a such a set of tools, these steps can be followed :

```
git clone --recursive https://github.com/riscv/riscv-gnu-toolchain
export RISCV=/path/to/install/riscv/toolchain
cd riscv-gnu-toolchain
```

The toolchain is dependant on a few packages that can be installed using :

```
sudo apt-get install autoconf automake autotools-dev curl libmpc-dev libmpfr-dev libgmp-dev gawk\
 build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev
```
on a Debian based system.
Or using :
```
sudo yum install autoconf automake libmpc-devel mpfr-devel gmp-devel gawk  bison flex texinfo patchutils\
 gcc gcc-c++ zlib-devel expat-devel
```
on Fedora/CentOS/RHEL OS systems

The build process needs to be configured function of which extension is supported by the core. In our case, we enable support for the base ISA only:

```
./configure --prefix=$RISCV --with-arch=rv32im --with-abi=ilp32
make
```

Once the toolchain compiled it is a good idea to add it's installation directory to the your PATH :
```
export PATH = $PATH:$RISCV/bin
```

> You may want to add that command to your shell startup file

#### Building the tests

This repository includes a basic set of benchmarks (`dijkstra`, `matmul`, `qsort` and `dct`) working on different datatypes.

```
cd <repo_root>/tests/basic_tests/
make
```
The `make` command will compile the tests for a RISC-V target and for your system and will collect their expected output.
This will allow the `runTests.sh` script to check the behavior of Comet against a verified CPU (your system's).

The repository also includes [Riscv compliance tests](https://github.com/riscv/riscv-arch-test) (see `<repo_root>/tests/riscv-compliance/`)  and the [Embench](https://www.embench.org/) benchmark (see `<repo_root>/tests/embench/`) configuration files.

##### Adding a test

To add a test to the test pool, add a folder in the `tests` directory at the root of the repository. This folder needs to be named like the binary it will contain.

> Exemple : the `qsort` folder contains the `qsort.riscv32` binary

The folder must contain the sources of the program and a makefile.
The binary produced by the makefile must be named like the folder it's contained in and bear the `.riscv32` extension.

For the test to be valid, a file named `expectedOutput` needs to be created. It must contain the standard output that the test is supposed to produce.

> You can reuse the makefile of the existing tests as a starting point.
> This makefile compiles the sources for the RISC-V target as well as for the host system, runs the native binary and creates the `expectedOutput` file automatically

All the tests **must** be reproducible output-wise, no random or system dependent data should be used.

The `runTests.sh` script executes all the tests present in the `tests` subfolders with a timeout (set in the script).
Please make sure that the tests you add all execute within the bounds of this timeout with a reasonable margin.

## Simulator
To run the simulator execute the `comet.sim` located in `<repo_root>/build/bin`.
You can provide a specific binary to be run by the simulator using the `-f <path_to_the_binary>` switch.

The `-i` and `-o` switches are used to specify the benchmark's standard input and output respectively.

The `-a` switch allows to pass arguments to the benchmark that is being run by the simulator.

The `-s` switch specifies the output file for testing purposes.

The `-p` switch specifies if we need to output benchmark performances (0:No/1:Yes).

The `-b` switch specifies if you want to instantiate a breakpoint.

For further information about the arguments of the simulator, run `comet.sim -h`.

## OS Specific Configuration

If you are willing to use an operating system (managing system calls), you will need to modify the main configuration file `<repo_root>/include/soc_config.h`.

> Disable bare-metal simulator syscalls handler: `HAS_KERNEL_SYSCALL_HANDLER 1`

And if you want to enable the console (memory mapped):

> Enable console: `PRINT_CONSOLE 1`

You should see something like this in case of Zephyr OS with the `<repo_zephyr>/samples/synchronization` example:

![Zephyr OS Boot](./docs/synchro_comet_zephyr.png)

### Comet System on Chip (SoC)


#### Memory Map

| Base          | Length         | Attributes       | Description                    |
| ------------- | -------------- | ---------------- | ------------------------------ |
| 0x0000_0010   | 0x0000_0300    |                  | CLINT                          |
| 0x0000_0310   | 0x0000_0480    |                  | (Early Console) Secure Enclave |
| 0x0000_0790   | 0x0000_07A8    |                  | UART (LiteUART from LiteX)     |
| 0x0003_0000   | ND             | EX, NI, C        | ROM + DRAM                     |

*(EX: Executable, NI: Non-idempotent, C: Cached, ND: Not defined)*

### Comet Multi-core System

You can choose either to use Comet as a single core or with multiple cores. In order to have a multi-core, you will need to modify the main configuration file `<repo_root>/include/soc_config.h`.

> Number of cores: `NB_CORES 2`

Then, you will need to re-compile the core with cmake thanks to the following command (indicating SMP support) :

```
cmake -DSMP=ON .. && make
```

## Logic Synthesis

Using HLS tools, the Comet core can be synthesized and implemented on FPGA targets or mapped to standard cells using a design kit.
RTL synthesis is tested using [Mentor Catapult HLS](https://www.mentor.com/hls-lp/catapult-high-level-synthesis/). If you wish to have the RTL output, please send an email to simon.rokicki@irisa.fr.

### RTL ZephyrOS Boot - Test Example

- Enter the `<repo_root>/build/bin/` folder that gathers all binaries of the project once correctly built.
```
./generateBin /path/to/your/elf/file/zephyr.elf > /path/to/your/rtl/simulation//zephyr.bin
```

## Publication

- "<a href="https://hal.archives-ouvertes.fr/hal-02303453v1">What You Simulate Is What You Synthesize: Designing a Processor Core from C++ Specifications</a>", in 38th IEEE/ACM International Conference on Computer-Aided Design


## Contributors

The following persons contributed to the Comet project:

- Simon Rokicki
- Davide Pala
- Joseph Paturel
- Valentin Egloff
- Edwin Mascarenhas
- Gurav Datta
- Lauric Desauw
- Olivier Sentieys
- Logan Fortune

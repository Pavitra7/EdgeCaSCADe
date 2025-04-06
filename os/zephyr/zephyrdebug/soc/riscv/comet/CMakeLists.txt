#
# Copyright (c) 2018 - 2019 Antmicro <www.antmicro.com>
#
# SPDX-License-Identifier: Apache-2.0
#../riscv-privilege/common/soc_irq.S
#../riscv-privilege/common/vector.S
#
zephyr_compile_options(-march=rv32ima)

zephyr_sources(
    ./soc.h
    ./soc_common_irq.c
    ./soc_irq.S
    ./vector.S
)

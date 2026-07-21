# Contributing

Contributions should preserve the separation between the Zephyr integration and
the upstream STSELib dependency.

Before submitting a change:

1. format C sources with the repository `.clang-format`;
2. run the host tests with `west twister -T stsephyr/tests`;
3. compile the hardware samples with
   `west twister -T stsephyr/samples --build-only`;
4. when the Nucleo and shield are available, run the fixture-gated sample
   harnesses using the hardware-map command documented in `README.md`;
5. update documentation and tests when changing public behavior;
6. include SPDX headers on source files.

STSELib revision updates must be isolated changes. They require review of its
platform callback signatures, explicit CMake source list, release notes, and a
hardware smoke test.

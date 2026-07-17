# Contributing

Contributions should preserve the separation between the Zephyr integration and
the upstream STSELib dependency.

Before submitting a change:

1. format C sources with the repository `.clang-format`;
2. run `west twister -T stsephyr/tests -T stsephyr/samples`;
3. build the basic sample for `nucleo_l452re` with the
   `x_nucleo_ese01a1` shield;
4. update documentation and tests when changing public behavior;
5. include SPDX headers on source files.

STSELib revision updates must be isolated changes. They require review of its
platform callback signatures, explicit CMake source list, release notes, and a
hardware smoke test.

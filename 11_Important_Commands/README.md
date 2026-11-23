# 11 — Important Commands

A single place for the commands you use most often. Run them from **your
project folder** (with `ZEPHYR_BASE` set) or from the Zephyr workspace.

> **Main reference:** [Zephyr documentation](https://docs.zephyrproject.org/)

## Environment (every new terminal)

```bash
source ~/zephyrproject/.venv/bin/activate
export ZEPHYR_BASE=~/zephyrproject/zephyr
```

## Build

```bash
west build -b <board> -d build .            # build into ./build
west build -p always -b <board> -d build .  # clean build
west build -t menuconfig -d build           # edit Kconfig (terminal)
west build -t guiconfig -d build            # edit Kconfig (graphical)
west build -t run -d build                  # run on native_sim / QEMU
```

## Flash

```bash
west flash -d build                         # flash the board
west flash -d build --runner <runner>       # choose a specific runner
```

## Inspect the result

```bash
cat build/zephyr/.config                    # final Kconfig
cat build/zephyr/zephyr.dts                 # final merged Devicetree
ls build/zephyr/zephyr.elf                  # the firmware
```

## Discovery

```bash
west boards | grep -i <name>                # find a board target
west --version                              # west version
cmake --version                             # CMake version
ninja --version                             # Ninja version
```

> **Order tip:** activate the venv first, then set `ZEPHYR_BASE`, then build.
> If you open a new terminal, do the first two steps again.

---

## Related documents

- [Create a New Zephyr Project](../03_Create_New_Project/README.md)
- [CMake](../04_CMake/README.md)
- [Kconfig vs Devicetree](<../05_Kconfig_vs_Devicetree_DTS/README.md>)
- [Kconfig deep dive](../06_KConfig/README.md)
- [Custom Module](<../07_Create_Custom_Module_Digital_To_Volt/README.md>)
- [Devicetree deep dive](../08_Devicetree/README.md)
- [Create a Custom Devicetree](../09_Create_Custom_Devicetree/README.md)
- [How to Create a Zephyr Project](../10_How_To_Create_A_Zephyr_Project/README.md)
- [Zephyr documentation](https://docs.zephyrproject.org/) — the main reference.

*Zephyr v4.4.0-rc1 — Ubuntu host.*

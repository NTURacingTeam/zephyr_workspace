zephyr_library()

zephyr_include_directories(
    include
)

zephyr_linker_sources(SECTIONS ld/sections.ld)
zephyr_linker_sources(DATA_SECTIONS ld/data_sections.ld)

zephyr_library_sources(
    src/err.c
    src/msg.c
    src/sensors.c
    src/util.c
)

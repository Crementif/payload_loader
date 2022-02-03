#include "elf_abi.h"
#include "fs_defs.h"
#include "common.h"
#include "utils.h"
#include "structs.h"
#include "elf_loading.h"
#include "memory_setup.h"

#include "../../../config.h"

int _start(int argc, char **argv) {
    setup_memory(1);

    uint32_t newEntry = DownloadPayloadIntoMemory(CONFIG_PAYLOAD_URL);

    setup_memory(0);
    return ((int (*)(int, char **))newEntry)(argc, argv);
}

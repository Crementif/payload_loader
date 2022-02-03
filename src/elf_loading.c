#include "elf_abi.h"
#include "fs_defs.h"
#include "common.h"
#include "utils.h"
#include "structs.h"
#include "elf_loading.h"
#include "memory_setup.h"

static uint32_t load_elf_image_to_mem (private_data_t *private_data, uint8_t *elfstart) {
    Elf32_Ehdr *ehdr;
    Elf32_Phdr *phdrs;
    uint8_t *image;
    int32_t i;

    ehdr = (Elf32_Ehdr *) elfstart;

    if(ehdr->e_phoff == 0 || ehdr->e_phnum == 0) {
        return 0;
    }

    if(ehdr->e_phentsize != sizeof(Elf32_Phdr)) {
        return 0;
    }

    phdrs = (Elf32_Phdr*)(elfstart + ehdr->e_phoff);

    for(i = 0; i < ehdr->e_phnum; i++) {
        if(phdrs[i].p_type != PT_LOAD) {
            continue;
        }

        if(phdrs[i].p_filesz > phdrs[i].p_memsz) {
            continue;
        }

        if(!phdrs[i].p_filesz) {
            continue;
        }

        uint32_t p_paddr = phdrs[i].p_paddr;
        image = (uint8_t *) (elfstart + phdrs[i].p_offset);

        private_data->memcpy ((void *) p_paddr, image, phdrs[i].p_filesz);
        private_data->DCFlushRange((void*)p_paddr, phdrs[i].p_filesz);

        if(phdrs[i].p_flags & PF_X) {
            private_data->ICInvalidateRange ((void *) p_paddr, phdrs[i].p_memsz);
        }
    }

    //! clear BSS
    Elf32_Shdr *shdr = (Elf32_Shdr *) (elfstart + ehdr->e_shoff);
    for(i = 0; i < ehdr->e_shnum; i++) {
        const char *section_name = ((const char*)elfstart) + shdr[ehdr->e_shstrndx].sh_offset + shdr[i].sh_name;
        if(section_name[0] == '.' && section_name[1] == 'b' && section_name[2] == 's' && section_name[3] == 's') {
            private_data->memset((void*)shdr[i].sh_addr, 0, shdr[i].sh_size);
            private_data->DCFlushRange((void*)shdr[i].sh_addr, shdr[i].sh_size);
        } else if(section_name[0] == '.' && section_name[1] == 's' && section_name[2] == 'b' && section_name[3] == 's' && section_name[4] == 's') {
            private_data->memset((void*)shdr[i].sh_addr, 0, shdr[i].sh_size);
            private_data->DCFlushRange((void*)shdr[i].sh_addr, shdr[i].sh_size);
        }
    }

    return ehdr->e_entry;
}


#define CURLOPT_URL (10002)
#define CURLOPT_FOLLOWLOCATION (52)
#define CURLOPT_WRITEFUNCTION (20011)
#define CURLOPT_FILE (10001)

#define CURL_GLOBAL_SSL (1<<0)
#define CURL_GLOBAL_WIN32 (1<<1)
#define CURL_GLOBAL_ALL (CURL_GLOBAL_SSL|CURL_GLOBAL_WIN32)
#define CURL_GLOBAL_NOTHING 0
#define CURL_GLOBAL_DEFAULT CURL_GLOBAL_ALL

unsigned char* realloc_downloadBuffer(DownloadStruct_t* downloadBuffer, size_t new_size) {
    void *new_ptr = downloadBuffer->private_data->MEMAllocFromDefaultHeapEx(new_size, 4);
    if (!new_ptr)
        return 0;

    size_t old_size = downloadBuffer->size+1;
    downloadBuffer->private_data->memcpy(new_ptr, downloadBuffer->buffer, old_size <= new_size ? old_size : new_size);
    downloadBuffer->private_data->MEMFreeToDefaultHeap(downloadBuffer->buffer);
    return new_ptr;
}

size_t downloadingCallback(char *data, size_t size, size_t nmemb, void* userp) {
    DownloadStruct_t *mem = (DownloadStruct_t*)userp;
    size_t real_size = size * nmemb;
    mem->private_data->OSReport("Writing %u bytes to %u sized buffer at 0x%p...\n", real_size, mem->size + real_size, mem->buffer);

    unsigned char* reallocated_buffer = realloc_downloadBuffer(mem, mem->size + real_size + 1);
    if (!reallocated_buffer) {
        OSFatal("Failed to reallocate memory");
        return 0;
    }

    mem->buffer = reallocated_buffer;
    mem->private_data->memcpy(&(mem->buffer[mem->size]), data, real_size);
    mem->size += real_size;
    mem->buffer[mem->size] = 0;
    
    return real_size;
}

uint32_t DownloadFile(private_data_t *private_data, const char* payloadUrl, unsigned char **buffer, unsigned int *size) {    
    int32_t success = 0;

    DownloadStruct_t downloadBuffer = {
        .buffer = private_data->MEMAllocFromDefaultHeapEx(1, 4), // initial size /*1MB = 1000000*/
        .size = 0,
        .private_data = private_data
    };

    if (!downloadBuffer.buffer)
        OSFatal("Failed to allocate initial download buffer.");

    private_data->socket_lib_init();

    int globalInitRet = private_data->curl_global_init(CURL_GLOBAL_NOTHING);
    if (globalInitRet) {
        private_data->OSReport("Curl global init error is %s\n", private_data->curl_easy_strerror(globalInitRet));
    }

    void* curl_handle = private_data->curl_easy_init();
    if (!curl_handle)
        OSFatal("Failed to initialize curl_easy_init()");

    private_data->curl_easy_setopt(curl_handle, CURLOPT_URL, payloadUrl);
    private_data->curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, (const void*)1L);
    private_data->curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, downloadingCallback);
    private_data->curl_easy_setopt(curl_handle, CURLOPT_FILE, (const void*)&downloadBuffer);

    int32_t curlRet = private_data->curl_easy_perform(curl_handle);
    if (curlRet) {
        private_data->OSReport("Curl error is %d\n", curlRet);
    }
    else {
        private_data->OSReport("Successfully downloaded CustomRPXLoader payload!\n");
        success = 1;
    }

    private_data->curl_easy_cleanup(curl_handle);
    private_data->curl_global_cleanup();
    private_data->socket_lib_finish();

    *buffer = downloadBuffer.buffer;
    *size = downloadBuffer.size;
    return success;
}

uint32_t DownloadPayloadIntoMemory(const char* payloadUrl) {
    private_data_t private_data;
    loadFunctionPointers(&private_data);

    unsigned char *pElfBuffer = NULL;
    unsigned int uiElfSize = 0;

    DownloadFile(&private_data, payloadUrl, &pElfBuffer, &uiElfSize);

    if (!pElfBuffer) {
        OSFatal("Failed to download CustomRPXLoader payload!");
    }
    unsigned int newEntry = load_elf_image_to_mem(&private_data, pElfBuffer);
    if (newEntry == 0) {
        OSFatal("failed to load .elf from CustomRPXLoader");
    }

    private_data.MEMFreeToDefaultHeap(pElfBuffer);
    return newEntry;
}

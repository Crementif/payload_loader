#ifndef STRUCTS_H
#define	STRUCTS_H

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _private_data_t {
    EXPORT_DECL(void *, MEMAllocFromDefaultHeapEx,int size, int align);
    EXPORT_DECL(void, MEMFreeToDefaultHeap,void *ptr);

    EXPORT_DECL(void*, memcpy, void *p1, const void *p2, unsigned int s);
    EXPORT_DECL(void*, memset, void *p1, int val, unsigned int s);

    EXPORT_DECL(unsigned int, OSEffectiveToPhysical, const void*);
    EXPORT_DECL(void, exit, int);
    EXPORT_DECL(void, DCInvalidateRange, const void *addr, unsigned int length);
    EXPORT_DECL(void, DCFlushRange, const void *addr, unsigned int length);
    EXPORT_DECL(void, ICInvalidateRange, const void *addr, unsigned int length);

    EXPORT_DECL(void, socket_lib_init, void);
    EXPORT_DECL(void, socket_lib_finish, void);

    EXPORT_DECL(int, curl_global_init, long);
    EXPORT_DECL(void*, curl_easy_init, void);
    EXPORT_DECL(int, curl_easy_setopt, void *handle, uint32_t param, const void *op);
    EXPORT_DECL(int, curl_easy_perform, void *handle);
    EXPORT_DECL(void, curl_easy_getinfo, uint32_t param, void *op);
    EXPORT_DECL(const char*, curl_easy_strerror, int error);
    EXPORT_DECL(void, curl_easy_cleanup, void *handle);
    EXPORT_DECL(void, curl_global_cleanup, void);

    EXPORT_DECL(void, OSReport, const char* fmt, ...);

    EXPORT_DECL(int, SYSRelaunchTitle, int argc, char** argv);
} private_data_t;

// Try moving this struct
typedef struct DownloadStruct {
    unsigned char* buffer;
    unsigned int size;
    private_data_t* private_data;
} DownloadStruct_t;


#ifdef __cplusplus
}
#endif

#endif	/* STRUCTS_H */


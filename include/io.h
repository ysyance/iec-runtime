#ifndef __IO_H__
#define __IO_H__

#include <native/heap.h>
#include <stdlib.h>
#include <string.h>
#include "ivalue.h"
#include "helper.h"
#include "logger.h"

#define IO_TASK_NAME "io_task"
#define IO_TASK_PRIORITY 99

/*-----------------------------------------------------------------------------
 * I/O Configuration
 *---------------------------------------------------------------------------*/
#define IO_TYPE_COUNT 8
typedef struct {
	uint32_t update_interval; /* I/O data update interval */
    uint8_t iou_count[IO_TYPE_COUNT];
	/*uint8_t ldi_count;        [> number of local digital input module <]*/
	/*uint8_t ldo_count;        [> number of local digital output module <]*/
	/*uint8_t lai_count;        [> number of local analog input module <]*/
	/*uint8_t lao_count;        [> number of local analog output module <]*/
	/*uint8_t rdi_count;        [> number of remote digital input module <]*/
	/*uint8_t rdo_count;        [> number of remote digital output module <]*/
	/*uint8_t rai_count;        [> number of remote analog input module <]*/
	/*uint8_t rao_count;        [> number of remote analog output module <]*/
} IOConfig;

#define IO_CONF_SIZE (sizeof(IOConfig))
#define IO_CONF_NAME "io_conf"
/*-----------------------------------------------------------------------------
 * I/O Shared Memory Structure
 *---------------------------------------------------------------------------*/
/**
 * Block0: Local Digital Input Unit(LDI)
 * Block1: Local Digital Output Unit(LDO)
 * Block2: Remote Digital Input Unit(RDI)
 * Block3: Remote Digital Output Unit(RDO)
 * Block4: Local Analog Input Unit(LAI)
 * Block5: Local Analog Output Unit(LAO)
 * Block6: Remote Analog Input Unit(RAI)
 * Block7: Remote Analog Output Unit(RAO)
 */
#define BLOCK_COUNT 8
typedef struct {
    char *base[BLOCK_COUNT];
    RT_HEAP heap[BLOCK_COUNT];
} IOMem;
static const char *heap_name[] = {
    "ldi_shm", "ldo_shm", "rdi_shm", "rdo_shm", "lai_shm", "lao_shm", "rai_shm", "rao_shm"
};

#define AU_CHANNELS 8 /* channels per analog unit */
#define AU_CH_SIZE 4  /* bytes per channel (uint: Byte) */
#define AU_SIZE (AU_CHANNELS*AU_CH_SIZE) /* size of analog unit (uint: Byte) */
#define DU_SIZE 1 /* size of digital unit (uint: Byte) */

#define LDI_COUNT(config) ((config)->iou_count[0])
#define LDO_COUNT(config) ((config)->iou_count[1])
#define RDI_COUNT(config) ((config)->iou_count[2])
#define RDO_COUNT(config) ((config)->iou_count[3])
#define LAI_COUNT(config) ((config)->iou_count[4])
#define LAO_COUNT(config) ((config)->iou_count[5])
#define RAI_COUNT(config) ((config)->iou_count[6])
#define RAO_COUNT(config) ((config)->iou_count[7])

#define LDI_SIZE(config) (LDI_COUNT(config) * DU_SIZE)
#define LDO_SIZE(config) (LDO_COUNT(config) * DU_SIZE)
#define RDI_SIZE(config) (RDI_COUNT(config) * DU_SIZE)
#define RDO_SIZE(config) (RDO_COUNT(config) * DU_SIZE)
#define LAI_SIZE(config) (LAI_COUNT(config) * AU_SIZE)
#define LAO_SIZE(config) (LAO_COUNT(config) * AU_SIZE)
#define RAI_SIZE(config) (RAI_COUNT(config) * AU_SIZE)
#define RAO_SIZE(config) (RAO_COUNT(config) * AU_SIZE)

#define IO_SIZE(config) { \
    LDI_SIZE(config), LDO_SIZE(config), RDI_SIZE(config), RDO_SIZE(config), \
    LAI_SIZE(config), LAO_SIZE(config), RAI_SIZE(config), RAO_SIZE(config)}

#define DIU_COUNT(config) (LDI_COUNT(config) + RDI_COUNT(config))
#define DOU_COUNT(config) (LDO_COUNT(config) + RDO_COUNT(config))
#define AIU_COUNT(config) (LAI_COUNT(config) + RAI_COUNT(config))
#define AOU_COUNT(config) (LAO_COUNT(config) + RAO_COUNT(config))

#define DIU_SIZE(config) (DIU_COUNT(config) * DU_SIZE)
#define DOU_SIZE(config) (DOU_COUNT(config) * DU_SIZE)
#define AIU_SIZE(config) (AIU_COUNT(config) * AU_SIZE)
#define AOU_SIZE(config) (AOU_COUNT(config) * AU_SIZE)

#define IO_SHM_SIZE(config) \
    (DIU_SIZE(config) + DOU_SIZE(config) + AIU_SIZE(config) + AOU_SIZE(config))

#define LDI(iomem) (iomem.base[0])
#define LDO(iomem) (iomem.base[1])
#define RDI(iomem) (iomem.base[2])
#define RDO(iomem) (iomem.base[3])
#define LAI(iomem) (iomem.base[4])
#define LAO(iomem) (iomem.base[5])
#define RAI(iomem) (iomem.base[6])
#define RAO(iomem) (iomem.base[7])
/*-----------------------------------------------------------------------------
 * Digital I/O Unit Operation Macros
 * I/O data is stored in Ivalue.v.value_u (type IUInt)
 *---------------------------------------------------------------------------*/
#undef MASK1
#define MASK1(p,n)	((~((~(IUInt)0)<<(n)))<<(p))
#undef MASK0
#define MASK0(p,n)	(~MASK1(p,n))
#define RES(num, shift) ((num) & MASK1(0,shift)) /* get residue */

#define SHIFT 3 /* 8 = 2^3; 8 is the bit width of IOMem.diu */
#define BASE(base, pos) (*(IUInt*)&(base)[(pos)>>SHIFT]) /* MUST dereference first, then cast type!! */
#define getdch(diu, pos, size) ((BASE(diu,pos) >> RES(pos,SHIFT)) & MASK1(0,size))
#define setdch(dou, pos, size, value) {                                                       \
    BASE(dou,pos) = BASE(dou,pos) & MASK0(RES(pos,SHIFT), size) | ((value)<<RES(pos, SHIFT)); \
}
/*-----------------------------------------------------------------------------
 * Analog I/O Unit Operation Macros
 *---------------------------------------------------------------------------*/
#define getach(aiu, iuint, ich) (*(uint32_t*)&(aiu)[iuint*AU_SIZE+ich*AU_CH_SIZE])
//TODO!!!
#define setach(aou, iuint, ich, value) //{aou[iuint*AU_CHANNELS+ich] = value;}
/*-----------------------------------------------------------------------------
 * I/O Config Shared Memory Operation Funcions
 *---------------------------------------------------------------------------*/
inline void io_conf_create(RT_HEAP *heap, IOConfig **conf) {
    if (rt_heap_create(heap, IO_CONF_NAME, IO_CONF_SIZE, H_SHARED) < 0) {
        LOGGER_ERR(E_HEAP_CREATE, "(name=%s, size=%d)", IO_CONF_NAME, IO_CONF_SIZE);
    }
    if (rt_heap_alloc(heap, IO_CONF_SIZE, TM_INFINITE, (void **)conf) < 0) {
        LOGGER_ERR(E_HEAP_ALLOC, "(name=%s, size=%d)", IO_CONF_NAME, IO_CONF_SIZE);
    }
}
inline void io_conf_bind(RT_HEAP *heap, IOConfig **conf) {
    if (rt_heap_bind(heap, IO_CONF_NAME, TM_INFINITE) < 0) {
        LOGGER_ERR(E_HEAP_BIND, "(name=%s, size=%d)", IO_CONF_NAME, IO_CONF_SIZE);
    }
    if (rt_heap_alloc(heap, 0, TM_NONBLOCK, (void **)conf) < 0) {
        LOGGER_ERR(E_HEAP_ALLOC, "(name=%s, size=%d)", IO_CONF_NAME, IO_CONF_SIZE);
    }
}
inline void io_conf_unbind(RT_HEAP *heap) {
    if (rt_heap_unbind(heap) < 0) {
        LOGGER_ERR(E_HEAP_UNBIND, "(name=%s, size=%d)", IO_CONF_NAME, IO_CONF_SIZE);
    }
}
inline void io_conf_delete(RT_HEAP *heap) {
    if (rt_heap_delete(heap) < 0) {
        LOGGER_ERR(E_HEAP_DELETE, "(name=%s, size=%d)", IO_CONF_NAME, IO_CONF_SIZE);
    }
}
/*-----------------------------------------------------------------------------
 * I/O Shared Memory Operation Funcions
 *---------------------------------------------------------------------------*/
#define M_LOCAL  1
#define M_SHARED 2
inline void io_mem_create(IOMem *iomem, IOConfig *config, int mode) {
    int io_size[BLOCK_COUNT] = IO_SIZE(config);
    if (mode == M_LOCAL) {
        for (int i = 0; i < BLOCK_COUNT; i++) {
            if (io_size[i] == 0) continue;
            if ((iomem->base[i] = new char[io_size[i]]) == NULL) {
                LOGGER_ERR(E_OOM, "initializing iomem(Mode=M_LOCAL)");
            }
        }
    } else if (mode == M_SHARED) {
        int ret;
        for (int i = 0; i < BLOCK_COUNT; i++) {
            if (io_size[i] == 0) continue;
            if ((ret = rt_heap_create(&iomem->heap[i], heap_name[i], io_size[i], H_SHARED)) < 0) {
                LOGGER_ERR(E_HEAP_CREATE, "(name=%s, size=%d, ret=%d)", heap_name[i], io_size[i], ret);
            }
            /* MUST called from realtime context (REF: Xenomai API) */
            if ((ret = rt_heap_alloc(&iomem->heap[i], io_size[i], TM_INFINITE, (void **)&iomem->base[i])) < 0) {
                LOGGER_ERR(E_HEAP_ALLOC, "(name=%s, size=%d, ret=%d)", heap_name[i], io_size[i], ret);
            }
        }
    }
}
inline void io_mem_zero(IOMem *iomem, IOConfig *config) {
    int io_size[BLOCK_COUNT] = IO_SIZE(config);
    for (int i = 0; i < BLOCK_COUNT; i++) {
        if (io_size[i] == 0) continue;
        memset(iomem->base[i], 0, io_size[i]);
    }
}
inline void io_mem_bind(IOMem *iomem, IOConfig *config) {
    int io_size[BLOCK_COUNT] = IO_SIZE(config);
    for (int i = 0; i < BLOCK_COUNT; i++) {
        if (io_size[i] == 0) continue;
        if (rt_heap_bind(&iomem->heap[i], heap_name[i], TM_INFINITE) < 0) {
            LOGGER_ERR(E_HEAP_BIND, "(name=%s, size=%d)", heap_name[i], io_size[i]);
        }
        if (rt_heap_alloc(&iomem->heap[i], 0, TM_NONBLOCK, (void **)&iomem->base[i]) < 0) {
            LOGGER_ERR(E_HEAP_ALLOC, "(name=%s, size=%d)", heap_name[i], io_size[i]);
        }
    }
}
inline void io_mem_unbind(IOMem *iomem, IOConfig *config) {
    int io_size[BLOCK_COUNT] = IO_SIZE(config);
    for (int i = 0; i < BLOCK_COUNT; i++) {
        if (io_size[i] == 0) continue;
        if (rt_heap_unbind(&iomem->heap[i]) < 0) {
            LOGGER_ERR(E_HEAP_UNBIND, "(name=%s, size=%d)", heap_name[i], io_size[i]);
        }
    }
}
//TODO delete local
inline void io_mem_delete(IOMem *iomem, IOConfig *config) {
    int io_size[BLOCK_COUNT] = IO_SIZE(config);
    for (int i = 0; i < BLOCK_COUNT; i++) {
        if (io_size[i] == 0) continue;
        if (rt_heap_delete(&iomem->heap[i]) < 0) {
            LOGGER_ERR(E_HEAP_DELETE, "(name=%s, size=%d)", heap_name[i], io_size[i]);
        }
    }
}
inline void io_mem_cpy(IOMem *mem1, IOMem *mem2, IOConfig *config) {
    int io_size[BLOCK_COUNT] = IO_SIZE(config);
    for (int i = 0; i < BLOCK_COUNT; i++) {
        if (io_size[i] == 0) continue;
        memcpy(mem1->base[i], mem2->base[i], io_size[i]);
    }
}
/*-----------------------------------------------------------------------------
 * I/O Unit Debug Macros
 *---------------------------------------------------------------------------*/
#if LEVEL_DBG <= LOGGER_LEVEL
#define dump_io_conf(config) {\
	LOGGER_DBG(DFLAG_LONG, \
		"IOConfig:\n .update_interval = %d\n .ldi_count = %d\n .ldo_count = %d\n .rdi_count = %d\n" \
        " .rdo_count = %d\n .lai_count = %d\n .lao_count = %d\n .rai_count = %d\n .rao_count = %d", \
	 	config->update_interval, LDI_COUNT(config), LDO_COUNT(config), RDI_COUNT(config), \
        RDO_COUNT(config), LAI_COUNT(config), LAO_COUNT(config), RAI_COUNT(config), RAO_COUNT(config)); \
}
#define dump_mem(name, base, size) {                     \
    fprintf(stderr, name "(low -> high):");              \
    for (int i = 0; i < (size); i++) {                   \
        if (i % 16 == 0)                                 \
            fprintf(stderr, "\n");                       \
        fprintf(stderr, "%02x ", ((uint8_t*)(base))[i]); \
    }                                                    \
    fprintf(stderr, "\n");                               \
}
#else
#define dump_io_conf(config)
#define dump_mem(name, base, size)
#endif

#endif

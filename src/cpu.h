#ifndef _NES_CPU_H_
#define _NES_CPU_H_

#include "bus.h"

#define NMI_VECTOR  0xfffa
#define RST_VECTOR  0xfffc
#define IRQ_VECTOR  0xfffe

typedef struct {
    uint16_t pc;
    uint8_t  sp;
    uint8_t  ax;
    uint8_t  xi;
    uint8_t  yi;
    uint8_t  ps;
    BUS   *cbus; // cpu bus
    uint8_t cram[2048]; // cpu ram, 2KB

// private:
    int   nmi_last;
    int   nmi_cur;
    int   irq_flag;
    int   cclk_counter;
    int   cclk_instr;
    int   cclk_dma;
    void (*ndb_callback)(void *ndb);
    void  *ndb_context;
} CPU;

void cpu_init (CPU *cpu, BUS *cbus);
void cpu_free (CPU *cpu);
void cpu_reset(CPU *cpu);
void cpu_nmi  (CPU *cpu, int nmi);
void cpu_irq  (CPU *cpu, int irq);
void cpu_run  (CPU *cpu);

#endif


#ifndef PLIC_H
#define PLIC_H

#ifdef __cplusplus
extern "C" {
#endif

#define PLIC            0x0c000000L 
#define PLIC_PRIORITY   (PLIC + 0x0) 
#define PLIC_PENDING    (PLIC + 0x1000) 
#define PLIC_ENABLE     (PLIC + 0x2080) 
#define PLIC_THRESHOLD  (PLIC + 0x201000)  
#define PLIC_CLAIM      (PLIC + 0x201004) 

void plic_init(void);
int plic_claim(void);
void plic_complete(int);

#ifdef __cplusplus
}
#endif
#endif
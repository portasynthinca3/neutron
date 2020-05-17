//Neutron Project
//CPUID processor feature detection

#include "./cpuid.h"
#include "./stdlib.h"

/*
 * Gets a leaf of CPUID (leaf=EAX_in, subleaf=ECX_in)
 */
void cpuid_get_leaf(uint32_t leaf, uint32_t subleaf, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx){
    //Load EAX and ECX with leaf and subleaf values
    __asm__ volatile("mov %0, %%eax" : : "m"(leaf) : "eax");
    __asm__ volatile("mov %0, %%ecx" : : "m"(subleaf) : "ecx");
    //Execute CPUID instruction
    __asm__ volatile("cpuid" : : : "eax", "ebx", "ecx", "edx");
    //Get resulting EAX, EBX, ECX and EDX
    if(eax != NULL)
        __asm__ volatile("mov %%eax, %0" : "=m"(*eax));
    if(ebx != NULL)
        __asm__ volatile("mov %%ebx, %0" : "=m"(*ebx));
    if(ecx != NULL)
        __asm__ volatile("mov %%ecx, %0" : "=m"(*ecx));
    if(edx != NULL)
        __asm__ volatile("mov %%edx, %0" : "=m"(*edx));
}

/*
 * Reads CPU vendor string and maximum CPUID leaf
 */
void cpuid_get_vendor(char str[13], uint32_t* max){
    //Read CPUID leaf
    uint32_t eax, ebx, ecx, edx;
    cpuid_get_leaf(0, 0, &eax, &ebx, &ecx, &edx);
    //Construct a string
    if(str != NULL){
        str[ 0] = (uint8_t)(ebx >>  0);
        str[ 1] = (uint8_t)(ebx >>  8);
        str[ 2] = (uint8_t)(ebx >> 16);
        str[ 3] = (uint8_t)(ebx >> 24);
        str[ 4] = (uint8_t)(edx >>  0);
        str[ 5] = (uint8_t)(edx >>  8);
        str[ 6] = (uint8_t)(edx >> 16);
        str[ 7] = (uint8_t)(edx >> 24);
        str[ 8] = (uint8_t)(ecx >>  0);
        str[ 9] = (uint8_t)(ecx >>  8);
        str[10] = (uint8_t)(ecx >> 16);
        str[11] = (uint8_t)(ecx >> 24);
        str[12] =                    0;
    }
    //Maximum CPUID value is stored in EAX
    if(max != NULL)
        *max = eax;
}

/*
 * Reads CPU features
 */
void cpuid_get_feat(uint32_t* edx, uint32_t* ecx){
    cpuid_get_leaf(1, 0, NULL, NULL, ecx, edx);
}

/*
 * Reads CPU brand string
 */
void cpuid_get_brand(char* str){
    //Read brand string
    cpuid_get_leaf(0x80000002, 0, (uint32_t*)(str + 0x00), (uint32_t*)(str + 0x04),
                                  (uint32_t*)(str + 0x08), (uint32_t*)(str + 0x0C));
    cpuid_get_leaf(0x80000003, 0, (uint32_t*)(str + 0x10), (uint32_t*)(str + 0x14),
                                  (uint32_t*)(str + 0x18), (uint32_t*)(str + 0x1C));
    cpuid_get_leaf(0x80000004, 0, (uint32_t*)(str + 0x20), (uint32_t*)(str + 0x24),
                                  (uint32_t*)(str + 0x28), (uint32_t*)(str + 0x2C));
    //Add a null terminator
    str[48] = 0;
}
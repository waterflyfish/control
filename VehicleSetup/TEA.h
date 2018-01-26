
#ifndef __TEA_H__
#define __TEA_H__

#if 0
uint32_t v[2];
uint32_t k[4]={0x1,0x2,0x3,0x4};
#endif
#include <stdio.h>
#include <string.h>

bool encrypt_file(const char * fileName);
void decrypt_qgc(uint32_t* v);
#endif 

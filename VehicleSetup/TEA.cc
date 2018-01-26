#include <stdio.h>
#include <string.h>
#include <QDebug>

#include "TEA.h"

static const uint32_t k[4]={0x9,0x1,0x3,0x5};
static void encrypt(uint32_t* v, const uint32_t* k);
static void decrypt(uint32_t* v, const uint32_t* k);

bool encrypt_file(const char *fileName){
    qDebug("%s\n", fileName);
    if(fileName != NULL && fileName[0] == '\0'){
        return false;
    }
    const char * FileName = fileName;
    FILE *File = NULL;
    File = fopen(FileName, "rb+");
    if (!File){
        return false;
    }
    uint32_t vbuf[1024];
    long unsigned int size = sizeof(vbuf);
    memset(vbuf, 0x00, size);
    long unsigned int read_size = 0;
    while ((read_size = fread(vbuf, 1, size , File)) == size) {
        for(long unsigned int i = 0; i < (read_size / 4); i += 2){
            encrypt(&vbuf[i], k);
        }
        fseek(File, 0 - read_size, SEEK_CUR);
        if(fwrite(vbuf, 1, read_size, File) != read_size){
            fclose(File);
            return false;
        }
        fseek(File, 0, SEEK_CUR);
        memset(vbuf, 0x00, size);
    }
    unsigned int write_size = read_size;
    if(read_size%8 != 0){
        read_size = read_size - read_size%8;
    }
    for(long unsigned int i = 0; i < (read_size / 4); i += 2){
        encrypt(&vbuf[i], k);
    }
    fseek(File, 0 - write_size, SEEK_CUR);
    if(fwrite(vbuf, 1, write_size, File) != write_size){
        fclose(File);
        return false;
    }
    if(feof(File)){
        fclose(File);
        return true;
    }
    if(!ferror(File))
        fclose(File);
    return false;
}

void decrypt_qgc(uint32_t* v){
    decrypt(v, k);
}

static inline void encrypt(uint32_t* v, const uint32_t* k) {
    uint32_t v0 = v[0], v1 = v[1], sum = 0, i;           /* set up */
    uint32_t delta = 0x9e3779b9;                     /* a key schedule constant */
    uint32_t k0 = k[0], k1 = k[1], k2 = k[2], k3 = k[3];   /* cache key */
    for (i = 0; i < 32; i++) {                       /* basic cycle start */
        sum += delta;
        v0 += ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
        v1 += ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3);
    }                                              /* end cycle */
    v[0] = v0; v[1] = v1;
}

static inline void decrypt(uint32_t* v, const uint32_t* k) {
    uint32_t v0 = v[0], v1 = v[1], sum = 0xC6EF3720, i;  /* set up */
    uint32_t delta = 0x9e3779b9;                     /* a key schedule constant */
    uint32_t k0 = k[0], k1 = k[1], k2 = k[2], k3 = k[3];   /* cache key */
    for (i = 0; i<32; i++) {                         /* basic cycle start */
        v1 -= ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3);
        v0 -= ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
        sum -= delta;
    }                                              /* end cycle */
    v[0] = v0; v[1] = v1;
}

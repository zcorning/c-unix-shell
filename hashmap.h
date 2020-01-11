#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdbool.h>

typedef struct hashmap_pair {
    char* key; // null terminated strings
    char* val;
    bool used;
    bool tomb;
} hashmap_pair;

typedef struct hashmap {
    hashmap_pair* data;
    int size;
    int capacity;
    
} hashmap;

hashmap* make_hashmap();
void free_hashmap(hashmap* h);
int hashmap_has(hashmap* h, char* k);
char* hashmap_get(hashmap* h, char* k);
void hashmap_put(hashmap* h, char* k, char* v);
void hashmap_del(hashmap* h, char* k);
hashmap_pair hashmap_get_pair(hashmap* h, int i);
void hashmap_dump(hashmap* h);

#endif

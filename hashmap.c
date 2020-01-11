
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

// This provides strlcpy
// See "man strlcpy"
#include <bsd/string.h>
#include <string.h>

#include "hashmap.h"


int
hash(char* key)
{
	// String hash function #2 from http://cseweb.ucsd.edu/~kube/cls/100/Lectures/lec16/lec16-15.html
	int i = 0;
	int h = 0;
	while (key[i] != '\0') {
		h = 31*h + key[i];
		++i;
	}
	return h;
}

hashmap*
make_hashmap_presize(int n)
{
	hashmap* h = calloc(1, sizeof(hashmap));
	h->data = calloc(n, sizeof(hashmap_pair));
	h->size = 0;
	h->capacity = n;
	return h;
}

hashmap*
make_hashmap()
{
	return make_hashmap_presize(4);
}

void
free_hashmap(hashmap* h)
{
	for (int i = 0; i < h->capacity; ++i) {
		if (h->data[i].used) {
			free(h->data[i].key);
			free(h->data[i].val);
		}
	}
	free(h->data);
	free(h);
}

// helper that returns the index of the given key in the map
int
index_of(hashmap* h, char* k) {
	int i = hash(k) % h->capacity;
	while (hashmap_get_pair(h, i).used) {
		hashmap_pair cur = hashmap_get_pair(h, i);
		if (!cur.tomb && strcmp(cur.key, k) == 0) {
			return i;
		}
		i = (i + 1) % h->capacity;
	}
	return -1;
}

int
hashmap_has(hashmap* h, char* k)
{
	return index_of(h, k) != -1;
}

char*
hashmap_get(hashmap* h, char* k)
{
	int i = index_of(h, k);
	if (i == -1) {
		return 0;
	} else {
		return hashmap_get_pair(h, i).val;	
	}
}

void
hashmap_put(hashmap* h, char* k, char* v)
{
	// double capacity if necessary
	if ((h->capacity) / (h->size + 1) < 2) { // load cap : 1/2
		hashmap_pair* old_data = h->data;
		h->data = calloc(2 * h->capacity, sizeof(hashmap_pair));
		h->size = 0;
		h->capacity *= 2;

		for (int i = 0; i < h->capacity / 2; ++i) {
			hashmap_pair cur = old_data[i];
			if (cur.used && !cur.tomb) {
				hashmap_put(h, cur.key, cur.val);
			}
		}
		free(old_data);
	}
	
	// place the new pair
	int idx = index_of(h, k);
	if (idx == -1) { // key is not already in the map
		int i = hash(k) % h->capacity;
		bool placed = 0;
		while (!placed) {
			hashmap_pair cur = hashmap_get_pair(h, i);
			if (!cur.used || cur.tomb) {
				h->data[i].key = strdup(k);
				h->data[i].val = strdup(v);
				h->data[i].used = 1;
				h->data[i].tomb = 0;
				placed = 1;
			}
			i = (i + 1) % h->capacity;
		}
		++h->size;
	} else { // key is already in the map
		h->data[idx].val = strdup(v);
	}
}

void
hashmap_del(hashmap* h, char* k)
{
	int idx = index_of(h, k);
	if (!idx == -1) {
		h->data[idx].tomb = 1;
		free(h->data[idx].key);
		free(h->data[idx].val);
	}
}

hashmap_pair
hashmap_get_pair(hashmap* h, int i)
{
	assert(i < h->capacity && i >= 0);
	return h->data[i];
}

void
hashmap_dump(hashmap* h)
{
	printf("== hashmap dump ==\n");
	for (int i = 0; i < h->capacity; ++i) {
		hashmap_pair cur = hashmap_get_pair(h, i);
		if (cur.used && !cur.tomb) {
			printf("{%s, %s, %d, %d}\n", cur.key, cur.val, cur.used, cur.tomb);
		}
	}
}

/* This file is lecture notes from CS 3650, Fall 2018 */
/* Author: Nat Tuck */

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "svec.h"

svec*
make_svec()
{
	svec* sv = malloc(sizeof(svec));
	sv->data = malloc(2 * sizeof(char*));
	sv->size = 0;
	sv->capacity = 2;
	return sv;
}

void
free_svec(svec* sv)
{
	for (int i = 0; i < sv->size; ++i) {
		free(sv->data[i]);
	}
	free(sv->data);
	free(sv);
}

char*
svec_get(svec* sv, int ii)
{
	assert(ii >= 0 && ii < sv->size);
	return sv->data[ii];
}

void
svec_put(svec* sv, int ii, char* item)
{
	assert(ii >= 0 && ii < sv->size);
	char* prev;
	char* cur = strdup(item);

	for (int i = ii; i < sv->size; ++i) {
		prev = cur;
		cur = sv->data[i];
		sv->data[i] = prev;
	}
	// Note: push_back duplicates the string, so it needs to be freed
	// also push_back adds 1 to the size, so don't need to do that here
	svec_push_back(sv, cur);
	free(cur);
}

void
svec_push_back(svec* sv, char* item)
{
	if (sv->size == sv->capacity) {
		char** new_data = malloc(sizeof(char*) * sv->capacity * 2);
		for (int i = 0; i < sv->size; ++i) {
			// don't need to dup/free the strings because they're already owned
			new_data[i] = sv->data[i];
		}
		free(sv->data);
		sv->data = new_data;
		sv->capacity *= 2;
	}
	sv->data[sv->size] = strdup(item);
	++sv->size;	
}

void
svec_swap(svec* sv, int ii, int jj)
{
	char* tmp = sv->data[ii];
	sv->data[ii] = sv->data[jj];
	sv->data[jj] = tmp;
}

#ifndef AST_H
#define AST_H

#include "svec.h"

typedef struct node {
	char flag;
} node;

typedef struct leaf {
	char flag;
	char* tok;
} leaf;

typedef struct cmd {
	char flag;
	int argc;
	char** argv;
} cmd;

typedef struct parens {
	char flag;
	node* inside;
} parens;

typedef struct two_child {
	char flag;
	node* left;
	node* right;
} two_child;

typedef struct redir {
	char flag;
	node* node;
	char* filename;
} redir;

typedef struct assign {
	char flag;
	char* key;
	char* val;
} assign;

node* make_ast(svec* tokens);
node* make_ast_help(int num_trees, node** trees);
void free_ast(node* tree);
void dump_ast(node* ast);

#endif

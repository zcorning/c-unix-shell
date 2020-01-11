#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "svec.h"

node** // turns all the given tokens into a list of leaves
make_leaves(svec* tokens) {
	node** out = malloc(sizeof(node*) * (tokens->size + 1));
	for (int i = 0; i < tokens->size; ++i) {
		leaf* l = malloc(sizeof(leaf));
		l->flag = 'l';
		l->tok = strdup(svec_get(tokens, i));
		out[i] = (node*) l;
	}
	out[tokens->size] = 0;
	return out;
}

int
isop(char* tok) {
	char c = tok[0];
	return c == '(' || c == ')' || c == ';' || c == '|' || c == '&' || c == '>' || c == '<';
}

// returns the new number of trees in the array
int // removes the nodes in the given interval and shifts everything to the left (incuding idx)
remove_chunk(int num_trees, node** trees, int idx, int chunk_size) {
	int i = idx;
	// free stuff up to end of chunk
	for(; i < chunk_size + idx && i < num_trees; ++i) {
		// only free leaves, not nodes since nodes are aliased
		if (trees[i]->flag == 'l') {
			free_ast(trees[i]);
		}
	}
	// move everything down
	// NOTE: need to move the whole tree until null regardless of num_trees because
	// this function is also used in the recursive call, but the outer tree needs to remain
	// consistent (trees now ends in null)
	int j = idx;
	while (trees[i]) {
		trees[j] = trees[i];
		++i;
		++j;
	}

	if (idx + chunk_size >= num_trees) {
		return idx;
	} else {
		return num_trees - chunk_size;
	}
}

int // finds the index of the closing paren to go with the open paren at the given index
find_close(int open_idx, int size, node** trees) {
	int open_count = 0; // number of still open parens before token
	for (int i = open_idx + 1; i < size; ++i) { // +1 because given index of first open
		if (trees[i]->flag == 'l') {
			leaf* leaf = (struct leaf*)trees[i];
			if (strcmp(")", leaf->tok) == 0) {
				if (open_count == 0) {
					return i;
				} else {
					--open_count;
				}
			} else if (strcmp("(", leaf->tok) == 0) {
				++open_count;
			}
		}
	}
	return -1;
}

// returns the new number of trees in the array
int // replaces all tokens used inside parens (including parens) in the tree with a single node
make_parens(int num_trees, node** trees, int open_idx) {
	int close_idx = find_close(open_idx, num_trees, trees);
	assert(close_idx != -1);

	int inside_size = close_idx - open_idx - 1;
	node** inner_trees = &(trees[open_idx + 1]);
	node* inside = make_ast_help(inside_size, inner_trees);
	num_trees = num_trees - (inside_size - 1);

	parens* p = malloc(sizeof(parens));
	p->flag = 'p';
	p->inside = inside;
	
	free_ast(trees[open_idx]); // frees the open paren leaf and the paren token dup
	trees[open_idx] = (node*)p;
	return remove_chunk(num_trees, trees, open_idx + 1, 2); // rm node alias + close paren leaf
}

int // finds the index before the next operation (the last index of the right side of an operation)
find_cmd_end(int start, int size, node** trees) {
	if (trees[start]->flag != 'l') { 
		return start; 
	} else if (strcmp("(", ((leaf*)trees[start])->tok) == 0) {
		return find_close(start, size, trees);
	}
	for (int i = start; i < size; ++i) {
		if (trees[i]->flag == 'l') {
			leaf* leaf = (struct leaf*)trees[i];
			// ( ; !! && & | < 
			if (isop(leaf->tok)) {
				return i - 1;
			} else if (strcmp("=", leaf->tok) == 0) {
				if (i == start + 1) { 
					return i + 1; 
				} else { 
					return i - 2; 
				}
			}
		} else {
			return i - 1;
		}
	}
	return size - 1;
}

// returns the new number of trees in the array0
int // replaces all tokens used in operation with a single node
make_two_child(int num_trees, node** trees, int op_idx) {
	// left should already be a single node (how this tree reduction works)
	node* left = trees[op_idx - 1]; 
	node* right;
	char* op = ((leaf*)trees[op_idx])->tok;

	if (op_idx == num_trees - 1) {
		if (strcmp("&", op) == 0) {
			right = 0;
		} else {
			printf("Operation %s meant to have two commands, but only has one\n", op);
			exit(1);
		}
	} else {
		int last_idx = find_cmd_end(op_idx + 1, num_trees, trees);
		right = make_ast_help(last_idx - op_idx, &(trees[op_idx + 1]));
		num_trees = num_trees - (last_idx - op_idx - 1);
	}

	two_child* t = malloc(sizeof(two_child));
	
	if (strcmp("||", op) == 0) {
		t->flag = 'v';
	} else if (strcmp("&&", op) == 0) {
		t->flag = '^';
	} else {
		t->flag = op[0];
	}
	t->left = left;
	t->right = right;
	
	// should not free node since it's aliased
	trees[op_idx - 1] = (node*)t;
	return remove_chunk(num_trees, trees, op_idx, 2);
}

// returns the new number of trees in the array
int 
make_redir(int num_trees, node** trees, int op_idx) {
	redir* r = malloc(sizeof(redir));
	r->flag = ((leaf*)trees[op_idx])->tok[0];
	r->node = trees[op_idx - 1];
	r->filename = strdup(((leaf*)trees[op_idx + 1])->tok);
	
	// should not free node since it's aliased
	trees[op_idx - 1] = (node*)r;
	return remove_chunk(num_trees, trees, op_idx, 2);
}

// returns the new number of trees in the array
int // takes index of '=' leaf
make_assign(int num_trees, node** trees, int a_idx) {
	assign* a = malloc(sizeof(assign));
	a->flag = '=';
	a->key = strdup(((leaf*)trees[a_idx - 1])->tok);
	a->val = strdup(((leaf*)trees[a_idx + 1])->tok);
	
	free_ast(trees[a_idx - 1]); // should free left since it's a leaf
	trees[a_idx - 1] = (node*)a;
	remove_chunk(num_trees, trees, a_idx, 2);
}

// returns the new number of trees in the array
int
make_cmd(int num_trees, node** trees, int arg_idx) {
	if (arg_idx < num_trees - 2 && trees[arg_idx + 1]->flag == 'l') {
		char* tmp = ((struct leaf*)trees[arg_idx + 1])->tok;
		if (strcmp("=", tmp) == 0) {
			return make_assign(num_trees, trees, arg_idx + 1);
		}
	}
	int i = arg_idx + 1;
	for (; i < num_trees; ++i) {
		if (trees[i]->flag != 'l') {
			--i;
			break;
		} else {
			leaf* leaf = (struct leaf*)trees[i];
			if (isop(leaf->tok)) {
				break;
			} else if (strcmp("=", leaf->tok) == 0) {
				--i;
				break;
			}
		}
	}// i = num_trees after loop (i = index after last arg)
	char** args = malloc((i - arg_idx) * sizeof(char*));
	for (int j = arg_idx; j < i; ++j) {
		args[j - arg_idx] = strdup(((struct leaf*)trees[j])->tok);
	}
	cmd* c = malloc(sizeof(cmd));
	c->flag = 'c';
	c->argc = i - arg_idx;
	c->argv = args;

	free_ast(trees[arg_idx]);
	trees[arg_idx] = (node*)c;
	return remove_chunk(num_trees, trees, arg_idx + 1, i - arg_idx - 1); 
}

void
free_ast(node* tree) {
	char f = tree->flag;
	if (f == 'l') {
		leaf* l = (leaf*)tree;
		free(l->tok);
		free(l);
	} else if (f == 'c') {
		cmd* c = (cmd*)tree;
		for (int i = 0; i < c->argc; ++i) {
			free(c->argv[i]);
		}
		free(c->argv);
		free(c);
	} else if (f == 'p') {
		parens* p = (parens*)tree;
		free_ast(p->inside);
		free(p);
	} else if (f == ';' || f == 'v' || f == '^' || f == '&' || f == '|') {
		two_child* t = (two_child*)tree;
		free_ast(t->left);
		if (!(f == '&' && t->right == 0)) {
			free_ast(t->right);
		}
		free(t);
	} else if (f == '>' || f == '<') {
		redir* r = (redir*)tree;
		free_ast(r->node);
		free(r->filename);
		free(r);
	} else if (f == '=') {
		assign* a = (assign*)tree;
		free(a->key);
		free(a->val);
		free(a);
	} else {
		puts("Invalid AST flag");
		exit(1);
	}
}

node* // helper used to do recusive calls easier
make_ast_help(int num_trees, node** trees) {
	int i = 0;
	do {
		if (trees[i]->flag == 'l') {
			leaf* leaf = (struct leaf*)trees[i];
			char c = leaf->tok[0];
			if (strcmp("(", leaf->tok) == 0) {
				num_trees = make_parens(num_trees, trees, i);
			} else if (c == ';' || c == '|' || c == '&') {
				// if there's a semicolon at the end of a command, ignore it
				if (c == ';' && num_trees == 2) {
					return trees[0];
				}
				num_trees = make_two_child(num_trees, trees, i);
				--i;
			} else if (c == '<' || c == '>') {
				num_trees = make_redir(num_trees, trees, i);
				--i;
			} else {
				num_trees = make_cmd(num_trees, trees, i);
			}
		}
		++i;
	} while (num_trees > 1 && i < num_trees);
	
	if (num_trees != 1) {
		return 0;
	} else {
		return trees[0];
	}
}

node*
make_ast(svec* tokens) {
	// have to free array in here
	node** trees = make_leaves(tokens);
	node* out = make_ast_help(tokens->size, trees);
	free(trees);
	return out;
}

void
dump_ast(node* ast) {
	char f = ast->flag;
	if (f == 'l') {
		printf("Leaf %s\n", ((leaf*)ast)->tok);
	} else if (f == ';' || f == 'v' || f == '^' || f == '|') {
		two_child* t = (two_child*)ast;
		printf("Two Child %c\n", f);
		puts("Left [");
		dump_ast(t->left);
		puts("]");
		puts("Right [");
		dump_ast(t->right);
		puts("]");
	} else if (f == '&') {
		two_child* t = (two_child*)ast;
		printf("Two Child %c\n", f);
		puts("Left [");
		dump_ast(t->left);
		puts("]");
		puts("Right [");
		if (t->right != 0) {
			dump_ast(t->right);
		} else {
			puts("Null");
		}
		puts("]");
		
	} else if (f == '<' || f == '>') {
		printf("Redirect %c\n %s\n", f, ((redir*)ast)->filename);
		puts("Node [");
		dump_ast(((redir*)ast)->node);
		puts("]");
	} else if (f == '=') {
		printf("Assignment: %s = %s\n", ((assign*)ast)->key, ((assign*)ast)->val);
	} else if (f == 'p') {
		puts("Parentheses (");
		dump_ast(((parens*)ast)->inside);
		puts(")");
	} else if (f = 'c') {
		cmd* c = (cmd*)ast;
		printf("Command %d | ", c->argc);
		for (int i = 0; i < c->argc; ++i) {
			printf("%s ", c->argv[i]);
		}
		puts("");
	} else {
		printf("UNKNOWN NODE. Flag = %c\n", f);
	}
}	

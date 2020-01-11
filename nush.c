#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "svec.h"
#include "tokens.h"
#include "ast.h"
#include "hashmap.h"
#include "nush.h"

hashmap* vars;

long
read_line(char* buf, int size, FILE* file) {
	// if second to last character (i.e. the character before newline is a '\'
	// then should keep reading since the newline is escaped
	if ((long)fgets(buf, size, file)) {
		int len = strlen(buf);
		if (buf[len - 2] == '\\') {
			read_line(&buf[len], size - len, file);
		}
	} else {
		return 0;
	}
}

int
eval_cmd(cmd* c) {
	int cpid;
	char* name = c->argv[0];
	if (strcmp(name, "exit") == 0) {
		exit(0);
	} else if (strcmp(name, "cd") == 0) {
		return chdir(c->argv[1]);
	}
	if ((cpid = fork())) {
		// parent
		int status;
		waitpid(cpid, &status, 0);
		return WEXITSTATUS(status);
	} else {
		// child
		char* args[c->argc + 1];
		for (int i = 0; i < c->argc; ++i) {
			if (c->argv[i][0] == '$') {
				char* val = hashmap_get(vars, &(c->argv[i][1]));
				if (val != 0) {
					args[i] = val;
				}
			} else {
				args[i] = c->argv[i];
			}
		}
		args[c->argc] = 0;
		execvp(name, &(args[0]));
		exit(1);
	}
}

int
eval_parens(parens* p) {
	int cpid;
	if ((cpid = fork())) {
		// parent
		int status;
		waitpid(cpid, &status, 0);
		return WEXITSTATUS(status);
	} else {
		// child
		int ec = evaluate(p->inside);
		exit(ec);
	}
}

int 
eval_semi_colon(two_child* s) { 
	evaluate(s->left);
	return evaluate(s->right);
}

int 
eval_or(two_child* o) { 
	int out = evaluate(o->left);
	if (out) {
		return evaluate(o->right);
	} else {
		return 0;
	}

}

int
eval_and(two_child* a) {
	int out = evaluate(a->left);
	if (out) {
		return out;
	} else {
		return evaluate(a->right);
	}
}

int
eval_background(two_child* b) { 
	int cpid;
	if ((cpid = fork())) {
		// parent
		if (b->right) {
			return evaluate(b->right);
		} else {
			return 0;
		}
	} else {
		// child
		int out = evaluate(b->left);
		exit(out);
	}
}

int
eval_pipe(two_child* p) { 
	int cpid;
	if((cpid = fork())) {
		// parent
		int status;
		waitpid(cpid, &status, 0);
		return WEXITSTATUS(status);
	} else {
		// child
		int pipes[2];
		int rv = pipe(pipes);
		assert(rv != -1);

		if ((cpid = fork())) {
			// parent
			close(pipes[1]);
			close(0);
			dup(pipes[0]);
			close(pipes[0]);
			
			int out = evaluate(p->right);
			
			int status;
			waitpid(cpid, &status, 0);
			exit(out);
		} else {
			// child
			close(pipes[0]);
			close(1);
			dup(pipes[1]);
			close(pipes[1]);

			int out = evaluate(p->left);
			exit(out);
		}
	}
}

int
eval_redir_in(redir* r) { 
	int cpid;
	if ((cpid = fork())) {
		// parent
		int status;
		waitpid(cpid, &status, 0);
		return WEXITSTATUS(status);
	} else {
		// child
		int fd = open(r->filename, O_RDONLY);
		close(0);
		dup(fd);
		close(fd);
		
		int out = evaluate(r->node);
		exit(out);
	}
}

int
eval_redir_out(redir* r) { 
	int cpid;
	if ((cpid = fork())) {
		// parent
		int status;
		waitpid(cpid, &status, 0);
		return WEXITSTATUS(status);
	} else {
		// child
		int fd = open(r->filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
		close(1);
		dup(fd);
		close(fd);

		int out = evaluate(r->node);
		exit(out);
	}
}

int 
eval_assign(assign* a) {
	hashmap_put(vars, a->key, a->val);
}

int
evaluate(node* ast) {
	char f = ast->flag;
	if (f == 'c') {
		return eval_cmd((cmd*)ast);
	} else if (f == 'p') {
		return eval_parens((parens*)ast);
	} else if (f == ';') {
		return eval_semi_colon((two_child*)ast);
	} else if (f == 'v') {
		return eval_or((two_child*)ast);
	} else if (f == '^') {
		return eval_and((two_child*)ast);
	} else if (f == '&') {
		return eval_background((two_child*)ast);
	} else if (f == '|') {
		return eval_pipe((two_child*)ast);
	} else if (f == '<') {
		return eval_redir_in((redir*)ast);
	} else if (f == '>') {
		return eval_redir_out((redir*)ast);
	} else if (f == '=') {
		return eval_assign((assign*)ast);
	} else {
		puts("Invalid node to evaluate");
		printf("Flag = %c", f);
		exit(1);
	}
}

int
main(int argc, char* argv[])
{			   
	vars = make_hashmap();
	FILE* file = stdin;
	if (argc == 2) {
		file = fopen(argv[1], "r");
		assert(file != 0);
	}

	int prev_loc = 0;

	while (1) {
    		char* cmd = malloc(256 * sizeof(char));

    		if (argc == 1) {
        		printf("nush$ ");
        		fflush(stdout);
		} 
		
		if (read_line(cmd, 256, file) == 0) {
			fclose(file);
			exit(0);
		} else if (prev_loc >= ftell(file) && ftell(file) != -1) {
			free(cmd);
			continue;
		} else {
			prev_loc = ftell(file);
		}

		svec* tokens = tokenize(cmd);
		free(cmd);

		node* ast = make_ast(tokens);
		free_svec(tokens);
		
		evaluate(ast);
		free_ast(ast);
	}
	free_hashmap(vars);
	fclose(file);
	return 0;
}

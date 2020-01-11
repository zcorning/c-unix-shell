#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "svec.h"
#include "tokens.h"

char*
handle_escaped(int size, char* str) {
	int s = 0; // index of str
	int n = 0; // index of new
	char* new = malloc(size * sizeof(char));
	while (str[s]) {
		if (str[s] == '\\') {
			++s;
		}
		new[n] = str[s];
		++s;
		++n;
	}
	new[n] = '\0';
	return new;
}


int // returns the new index position after the end of the newly added arg token
add_arg(svec* tokens, char* line, int idx) {
	int off = 0;
	while (line[idx + off]) {
		char cur = line[idx + off];
		if (isspace(cur) || cur == '>' || cur == '<' ||cur == ';' || cur == '&' 
				|| cur == '|' ||cur == '(' || cur == ')' || cur == '=' 
				|| cur == '\"') {
			break;
		} else {
			++off;
		}
	}

	char arg[off + 1];
	memcpy(arg, line + idx, off);
	arg[off] = '\0';

	char* noesc = handle_escaped(off + 1, arg);
	svec_push_back(tokens, noesc);
	free(noesc);
	
	return idx + off;
}

int // returns the new index position after the end of the newly added quote token
add_quote(svec* tokens, char* line, int idx) {
	int off = 0;
	++idx; // takes the index of the starting quote, the token should not include the quotations
	while (line[idx + off]) {
		char cur = line[idx + off];
		if (cur == '\"') {
			break;
		} else {
			++off;
		}
	}

	char arg[off + 1];
	memcpy(arg, line + idx, off);
	arg[off] = '\0';

	char* noesc = handle_escaped(off + 1, arg);
	svec_push_back(tokens, noesc);
	free(noesc);

	return idx + off + 1; // new index should be after the quotes
}

svec*
tokenize(char* line) {
	svec* tokens = make_svec();
	int i = 0;
	while(line[i]) {
		char cur = line[i];

		if (isspace(cur)) {
			++i;
			continue;

		} else if (cur == '<' || cur == '>' || cur == ';' || cur == '(' || cur == ')' 
				|| cur == '=') {
			char tmp[] = {cur, '\0'};
			svec_push_back(tokens, tmp);
			++i;
		
		} else if (cur == '&') {
			if (line[i + 1] == '&') {
				svec_push_back(tokens, "&&");
				i += 2;
			} else {
				svec_push_back(tokens, "&");
				++i;
			}
		
		} else if (cur == '|') {
			if (line[i + 1] == '|') {
                                svec_push_back(tokens, "||");
                                i += 2;
                        } else {
                                svec_push_back(tokens, "|");
                                ++i;
                        }
		
		} else if (cur == '\"') {
			// note, add_quote takes the index of the open quote
			i = add_quote(tokens, line, i);
	        } else {
			i = add_arg(tokens, line, i);
		}
	}
	return tokens;	
}

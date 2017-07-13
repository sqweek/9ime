#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

/* Minimum number of characters in a compose sequence. The plan 9 keyboard
 * format relies on this to infer the <space><space> -> U+2423 sequence. */
#define MINSEQ 2

#define PANIC_IF(cond, args...) if (cond) { fprintf(stderr, args); fprintf(stderr, "\n"); exit(1); }

/* A Trie has three "types":
 * 1. The "nil" or empty type (.rune == 0 && .branch == NULL)
 * 2. A "leaf" node (.rune != 0)
 * 3. A "branch" node (.branch != NULL)
 * The only valid transitions are "nil" -> "leaf" or "nil" -> "branch". */
typedef struct Trie_ {
	long rune; /* the rune generated by the sequence leading to this node */
	struct Trie_ *branch; /* lazily allocated Trie[128] */
	int count; /* number of branches actually used */
} Trie;

Trie root;

void*
ealloc(size_t nbytes)
{
	void *mem = calloc(nbytes, 1);
	PANIC_IF(!mem, "alloc failed on %lu bytes", nbytes);
	return mem;
}

#define trie_isleaf(node) (!!(node)->rune)
#define trie_isbranch(node) (!!(node)->branch)
#define trie_isnil(node) (!trie_isleaf(node) && !trie_isbranch(node))

/* Takes one step down a Trie, initialising it as a branch if necessary. */
Trie*
trie_walk(Trie *node, char chr)
{
	Trie *next;
	PANIC_IF(chr & 0x80, "illegal non-ASCII character (%d)", chr);
	PANIC_IF(node->rune, "can't walk a leaf node");
	if (!node->branch) {
		node->branch = ealloc(128 * sizeof(Trie));
	}
	next = &node->branch[chr];
	if (trie_isnil(next)) {
		node->count++; // is new branch
	}
	return next;
}

/* Takes an arbitrary step down an already initialised branch Trie. */
Trie*
trie_walk_any(Trie *node, char *which)
{
	PANIC_IF(node->rune, "can't walk a leaf node");
	PANIC_IF(!node->count, "can't walk an empty node");
	int i;
	for (i = 0; i < 128; i++) {
		Trie *t = node->branch + i;
		if (!trie_isnil(t)) {
			*which = i;
			return t;
		}
	}
	abort(); // unreachable
}

/* Turns a nil Trie into a leaf. */
void
trie_terminate(Trie *node, long rune)
{
	PANIC_IF(!trie_isnil(node), "can only terminate a nil node");
	node->rune = rune;
}

/* name used in diagnostics */
char*
runename(long rune)
{
	static char buf[32];
	if (rune > 0) {
		sprintf(buf, "%04lx", rune);
	} else switch (rune) {
	case RESERVED_HEX_ESCAPE:
		sprintf(buf, "%s", "<hexadecimal entry>");
		break;
	default:
		sprintf(buf, "%s", "???");
	}
	return buf;
}

/* used for generating C header file */
char*
cchr(char c)
{
	static char buf[8];
	if (c < 27) {
		sprintf(buf, "\\%03o");
	} else if (c == '\'') {
		sprintf(buf, "\\'");
	} else {
		sprintf(buf, "%c", c);
	}
	return buf;
}

/* Adds a new character replacement sequence. Warns on stderr for invalid configurations. */
void
insert(long rune, char *seq, int seqlen)
{
	Trie *node = &root;
	int i;

	for (i = 0; i < seqlen; i++) {
		node = trie_walk(node, seq[i]);
		if (node->rune) {
			fprintf(stderr, "duplicate sequence: %.*s for %s", i + 1, seq, runename(node->rune));
			fprintf(stderr, " (skipping %.*s for %s)\n", seqlen, seq, runename(rune));
			return;
		}
	}
	if (trie_isbranch(node)) {
		char c;
		fprintf(stderr, "duplicate sequence: %*.s", seqlen, seq);
		while (!trie_isleaf(node)) {
			node = trie_walk_any(node, &c);
			fprintf(stderr, "%c", c);
		}
		fprintf(stderr, " for %s", runename(node->rune));
		fprintf(stderr, " (skipping %.*s for %s)\n", seqlen, seq, runename(rune));
		return;
	}
	trie_terminate(node, rune);
}

void
nl(int indent)
{
	int i;
	printf("\n");
	for (i = 0; i < indent; i++) {
		printf("\t");
	}
}

/* Recursively prints a Trie node in C initialiser format. */
void
emit(int indent, Trie *node)
{
	if (trie_isleaf(node)) {
		printf("LEAF(%#4lx)", node->rune);
		return;
	}
	int i, first = 1;
	printf("BRANCH(%d,{", node->count);
	for (i = 0; i < 128; i++) {
		if (!trie_isnil(&node->branch[i])) {
			if (!first) {
				printf(",");
			}
			first = 0;
			nl(indent);
			printf("STEM('%s', ", cchr(i));
			emit(indent + 1, &node->branch[i]);
			printf(")");
		}
	}
	nl(indent - 1);
	printf("})");
}

int
main(int argc, char **argv)
{
	int lineno = 0;
	char line[128], *cp, *seq;

	insert(RESERVED_HEX_ESCAPE, "U+", 2);

	/* read stdin line by line. lines longer than 128 chars will cause incorrect
	 * line numbers to be reported on parsing errors; deal with it. */
	while (fgets(line, sizeof(line), stdin)) {
		int inseq;
		long r = strtol(line, &cp, 16);
		++lineno;
		if (r == 0 || cp != line+4 || *cp != ' ' || cp[1] != ' ') {
			fprintf(stderr, "%d: cannot parse line\n", lineno);
			continue;
		}
		cp += 2;
		for (inseq=1, seq=cp; !(*cp & 0x80) ; cp++) {
			if (*cp == '\0' || *cp == ' ' || *cp == '\r' || *cp == '\n') {
				if (inseq && cp - seq >= MINSEQ) {
					inseq = 0;
					insert(r, seq, cp - seq);
				}
			} else if (!inseq) {
				seq = cp;
				inseq = 1;
			}
		}
	}

	emit(1, &root);
	return 0;
}
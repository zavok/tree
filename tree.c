/* see LICENSE.tree */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <fnmatch.h>
#include "arg.h"

enum {
	IGNORE_CASE = 1,
	PRUNE = 2,
	MATCHDIR = 4,
};	

typedef struct counter {
	size_t dirs;
	size_t files;
} counter_t;

typedef struct entry {
	char *name;
	int is_dir;
	struct entry *next;
} entry_t;

char *argv0;

int
walk(const char* directory, const char* prefix, const char *pattern, int pflags, counter_t *counter)
{
	entry_t *head = NULL, *current, *iter;
	size_t size = 0, index;

	struct dirent *file_dirent;
	DIR *dir_handle;

	char *full_path, *segment, *pointer, *next_prefix;

	dir_handle = opendir(directory);
	if (!dir_handle) {
		fprintf(stderr, "Cannot open directory \"%s\"\n", directory);
		return -1;
	}

	counter->dirs++;

	while ((file_dirent = readdir(dir_handle)) != NULL) {
		if (file_dirent->d_name[0] == '.') {
			continue;
		}

		current = malloc(sizeof(entry_t));
		current->name = strcpy(malloc(strlen(file_dirent->d_name)), file_dirent->d_name);
		current->is_dir = file_dirent->d_type == DT_DIR;
		current->next = NULL;

		if (head == NULL) {
			head = current;
		} else if (strcmp(current->name, head->name) < 0) {
			current->next = head;
			head = current;
		} else {
			for (iter = head; iter->next && strcmp(current->name, iter->next->name) > 0; iter = iter->next);

			current->next = iter->next;
			iter->next = current;
		}

		size++;
	}

	closedir(dir_handle);
	if (!head) {
		return 0;
	}

	for (index = 0; index < size; index++) {
		int match;
		int flags;
		match = 0;
		flags = (pflags&IGNORE_CASE)*FNM_CASEFOLD;
		
		if (fnmatch(pattern, head->name, flags) == 0) {
			match = 1;
		}
		
		if (index == size - 1) {
			pointer = "└── ";
			segment = "    ";
		} else {
			pointer = "├── ";
			segment = "│   ";
		}

		if (match != 0) printf("%s%s%s\n", prefix, pointer, head->name);

		if (head->is_dir) {
			full_path = malloc(strlen(directory) + strlen(head->name) + 2);
			sprintf(full_path, "%s/%s", directory, head->name);

			next_prefix = malloc(strlen(prefix) + strlen(segment) + 1);
			sprintf(next_prefix, "%s%s", prefix, segment);

			walk(full_path, next_prefix, pattern, pflags, counter);
			free(full_path);
			free(next_prefix);
		} else {
			if (match != 0) counter->files++;
		}

		current = head;
		head = head->next;

		free(current->name);
		free(current);
	}

	return 0;
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-lC] [-P pattern] [--noreport] [PATH]\n", argv0);
	exit(0);
}

int
main(int argc, char *argv[])
{
	char *longarg, *pattern;
	int pflags, noreport;
	noreport = 0;
	pflags = 0;
	pattern = "*";
	ARGBEGIN {
	case 'l':
		/* in original this enables symlink following */
		break;
	case 'P':
		/* in original this enables pattern matching */
		pattern = EARGF(usage());
		break;
	case 'C':
		/* in original this enables colors */
		break;
	case '-':
		/* I guess I can add --arg parsing here */
		/* pass uses --noreport --prune --matchdirs --ignore-case */
		longarg = EARGF(usage());	
		if (longarg == NULL) break;
		if (strcmp(longarg, "noreport") == 0) noreport = 1;
		if (strcmp(longarg, "ignore-case") == 0) pflags |= IGNORE_CASE;
		if (strcmp(longarg, "prune") == 0) pflags |= PRUNE;
		if (strcmp(longarg, "matchdirs") == 0) pflags |= MATCHDIR;
		break;
	default:
	} ARGEND
	
	char* directory = argc > 0 ? argv[0] : ".";
	printf("%s\n", directory);

	counter_t counter = {0, 0};
	walk(directory, "", pattern, pflags, &counter);

	if (noreport == 0)
		printf("\n%zu directories, %zu files\n", counter.dirs - 1, counter.files);
	return 0;
}

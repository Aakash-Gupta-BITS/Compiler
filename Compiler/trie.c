/***************************************
				GROUP-08
  Yash Bansal			-   2019A7PS0484P
  Sourabh S Yelluru		-   2018B3A70815P
  Nihir Agarwal			-   2018B4A70701P
  Aakash				-   2018B4A70887P
*****************************************/

#include "trie.h"
#include <assert.h>
#include <stdlib.h>

int getIndex(char c)
{
	if (c >= 'A' && c <= 'Z')
		return c - 'A';

	if (c >= 'a' && c <= 'z')
		return 26 + (c - 'a');

	if (c >= '0' && c <= '9')
		return 52 + (c - '0');

	if (c == '_')
		return 62;

	if (c == '#')
		return 63;

	assert(0);
}

// returns 0 on success, -1 on error
TrieNode* trie_getRef(Trie* t, char* key)
{
	int keyIndex = 0;
	int TrieIndex;

	if (t->root == NULL)
		t->root = calloc(1, sizeof(TrieNode));

	TrieNode* traverse = t->root;

	while (key[keyIndex] != '\0')
	{
		TrieIndex = getIndex(key[keyIndex++]);

		if (!traverse->children[TrieIndex])
			traverse->children[TrieIndex] = calloc(1, sizeof(TrieNode));

		traverse = traverse->children[TrieIndex];
	}

	return traverse;
}

// returns 1 on success, 0 on not found
int trie_exists(Trie* t, char* key)
{
	return trie_getVal(t, key).value != 0;
}

TrieEntry trie_getVal(Trie* t, char* key)
{
	TrieEntry entry;
	entry.value = 0;

	int keyIndex = 0;
	int TrieIndex;
	
	if (t->root == NULL)
		return entry;

	TrieNode* traverse = t->root;

	while (traverse && key[keyIndex] != '\0')
	{
		TrieIndex = getIndex(key[keyIndex++]);
		traverse = traverse->children[TrieIndex];
	}

	if (!traverse)
		return entry;

	return traverse->entry;
}

void dfs(TrieNode* node, void (*fptr)(TrieEntry*))
{
	if (node == NULL)
		return;

	if (node->entry.value != 0)
		fptr(&node->entry);
	
	for (int i = 0; i < TRIE_CHILD_COUNT; ++i)
		dfs(node->children[i], fptr);
}

void iterateTrie(Trie* t, void (*fptr)(TrieEntry*))
{
	dfs(t->root, fptr);
}
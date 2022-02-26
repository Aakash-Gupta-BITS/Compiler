#define TRIE_CHILD_COUNT 64

typedef struct TrieNode
{
	void* value;
	struct TrieNode* children[TRIE_CHILD_COUNT];
} TrieNode;

typedef struct Trie{
	TrieNode* root;
} Trie;


// returns 0 on success, -1 on error
TrieNode* getRef(Trie* t, char* key);

// returns 1 on success, 0 on not found
int exists(Trie* t, char* key);

void* getVal(Trie* t, char* key);
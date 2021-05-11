#include <stdlib.h>
#include "../client/hashmap.h"

struct HashmapNode {
        unsigned int key;
        void* value;
        struct HashmapNode* next;
};

static unsigned int GetBucket(unsigned int key) {
        return key % HASHMAP_SIZE;
}

static struct HashmapNode* HashmapNodeNew() {
        return malloc(sizeof(struct HashmapNode));
}

static void HashmapNodeFree(struct HashmapNode* node) {
        free(node);
}

struct Hashmap* HashmapNew() {
        struct Hashmap* hm;
        hm = calloc(sizeof(struct Hashmap), 1);
        return hm;
}

void HashmapPut(struct Hashmap* hashmap, unsigned int key, void *value) {
        unsigned int bucket;
        struct HashmapNode** curnode;
        struct HashmapNode* node;

        bucket = GetBucket(key);
        curnode = &hashmap->buckets[bucket];
        while (*curnode != NULL) {
                if((*curnode)->key == key) {
                        (*curnode)->value = value;
                        return;
                }
                curnode = &(*curnode)->next;
        }

        /* If key wasn't found insert a new node */
        node = HashmapNodeNew();
        node->key = key;
        node->value = value;
        *curnode = node;
}

void HashmapRemove(struct Hashmap* hashmap, unsigned int key) {
        struct HashmapNode* next = NULL;
        struct HashmapNode** node;
        unsigned int bucket;

        bucket = GetBucket(key);
        node = &hashmap->buckets[bucket];
        while (*node != NULL) {
                if ((*node)->key == key) {
                        next = (*node)->next;
                        HashmapNodeFree(*node);
                        *node = next;
                        return;
                }
                node = &(*node)->next;
        }
}

void* HashmapGet(struct Hashmap* hashmap, unsigned int key) {
        struct HashmapNode* node;
        unsigned int bucket;

        bucket = GetBucket(key);
        if(hashmap->buckets[bucket]) {
                node = hashmap->buckets[bucket];
                while(node != NULL) {
                        if(node->key == key) {
                                return node->value;
                        }
                        node = node->next;
                }
        }

        return NULL;
}

void HashmapFree(struct Hashmap* hashmap) {
        unsigned int i;
        struct HashmapNode* node;
        struct HashmapNode* next;

        while (i < HASHMAP_SIZE) {
                if(hashmap->buckets[i] != NULL) {
                        node = hashmap->buckets[i];
                        do {
                                next = node->next;
                                HashmapNodeFree(node);
                                node = next;
                        } while(node != NULL);
                }
                i++;
        }

        free(hashmap);
}

struct HashmapIterator* HashmapIterator(struct Hashmap* hashmap) {
        struct HashmapIterator* iter;

        iter = malloc(sizeof(struct HashmapIterator));
        iter->hashmap = hashmap;
        iter->curbucket = 0;
        iter->curnode = NULL;

        return iter;
}

void* HashmapNext(struct HashmapIterator* iter) {
        if(iter->curnode != NULL) {
                iter->curnode = iter->curnode->next;
                if(iter->curnode != NULL) {
                        return iter->curnode->value;
                }
                iter->curbucket++;
        }

        while (iter->curbucket < HASHMAP_SIZE) {
                if (iter->hashmap->buckets[iter->curbucket] != NULL) {
                        iter->curnode = iter->hashmap->buckets[iter->curbucket];
                        return iter->curnode->value;
                }
                iter->curbucket++;
        }

        free(iter);
        return NULL;
}
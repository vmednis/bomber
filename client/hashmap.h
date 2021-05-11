#ifndef HASHMAP_H
#define HASHMAP_H

#define HASHMAP_SIZE 4096

struct Hashmap {
        struct HashmapNode* buckets[HASHMAP_SIZE];
};

struct HashmapIterator {
        struct Hashmap* hashmap;
        unsigned int curbucket;
        struct HashmapNode* curnode;
};

struct Hashmap* HashmapNew();
void HashmapPut(struct Hashmap* hashmap, unsigned int key, void *value);
void HashmapRemove(struct Hashmap* hashmap, unsigned int key);
void* HashmapGet(struct Hashmap* hashmap, unsigned int key);
void HashmapFree(struct Hashmap* hashmap);
struct HashmapIterator* HashmapIterator(struct Hashmap* hashmap);
void* HashmapNext(struct HashmapIterator* iter);

#endif
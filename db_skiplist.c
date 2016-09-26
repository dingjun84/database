//	skip list implementation

#include "db.h"
#include "db_map.h"

//	search SkipList node for key value
//	return highest entry <= key

SkipEntry *skipSearch(SkipList *skipList, int high, uint64_t key) {
int low = 0, diff;

	//	key < high
	//	key >= low

	while ((diff = (high - low) / 2))
		if (key < *skipList->array[low + diff].key)
			high = low + diff;
		else
			low += diff;

	return skipList->array + low;
}

//	find key value in skiplist, return entry address

void *skipFind(DbMap *map, DbAddr *skip, uint64_t key) {
DbAddr *next = skip;
SkipList *skipList;
SkipEntry *entry;

  while (next->addr) {
	skipList = getObj(map, *next);

	if (*skipList->array->key <= key) {
	  entry = skipSearch(skipList, next->nslot, key);

	  if (*entry->key == key)
		return entry->val;

	  return NULL;
	}

	next = skipList->next;
  }

  return NULL;
}

//	remove key from skip list

void skipDel(DbMap *map, DbAddr *skip, uint64_t key) {
SkipList *skipList = NULL, *prevList;
DbAddr *next = skip;
SkipEntry *entry;

  while (next->addr) {
	prevList = skipList;
	skipList = getObj(map, *next);

	if (*skipList->array->key <= key) {
	  entry = skipSearch(skipList, next->nslot, key);

	  if (*entry->key != key)
		return;

	  //  remove the entry slot

	  if (--next->nslot) {
		while (entry - skipList->array < next->nslot) {
		  entry[0] = entry[1];
		  entry++;
		}

		return;
	  }

	  //  skip list node is empty, remove it

	  if (prevList)
		prevList->next->bits = skipList->next->bits;
	  else
		skip->bits = skipList->next->bits;

	  freeBlk(map, next);
	  return;
	}

	next = skipList->next;
  }
}

//	Push new maximal key onto head of skip list
//	return the value slot address

void *skipPush(DbMap *map, DbAddr *skip, uint64_t key) {
SkipList *skipList;
SkipEntry *entry;
uint64_t next;

	if (!skip->addr || skip->nslot == SKIP_node) {
		next = skip->bits;
		skip->bits = allocBlk(map, sizeof(SkipList), true);
		skipList = getObj(map, *skip);
		skipList->next->bits = next;
	}

	entry = skipList->array + skip->nslot++;
	*entry->key = key;
	return entry->val;
}

//	Add new key to skip list
//	return val address

void *skipAdd(DbMap *map, DbAddr *skip, uint64_t key) {
SkipList *skipList = NULL, *nextList;
DbAddr *next = skip;
uint64_t prevBits;
SkipEntry *entry;
int min, max;

  while (next->addr) {
	skipList = getObj(map, *next);

	//  find skipList node that covers key

	if (skipList->next->bits && *skipList->array->key > key) {
	  next = skipList->next;
	  continue;
	}

	if (*skipList->array->key <= key) {
	  entry = skipSearch(skipList, next->nslot, key);
	
	  //  does key already exist?

	  if (*entry->key == key)
		return entry->val;

	  min = ++entry - skipList->array;
	} else
	  min = 0;

	//  split node if already full

	if (next->nslot == SKIP_node) {
	  prevBits = skipList->next->bits;
	  skipList->next->bits = allocBlk(map, sizeof(SkipList), true);

	  nextList = getObj(map, *skipList->next);
	  nextList->next->bits = prevBits;
	  memcpy(nextList->array, skipList->array + SKIP_node / 2, sizeof(SkipList) * (SKIP_node - SKIP_node / 2));

	  skipList->next->nslot = SKIP_node - SKIP_node / 2;
	  next->nslot = SKIP_node / 2;
	  continue;
	}

	//  insert new entry slot

	max = next->nslot++;

	while (max > min)
	  skipList->array[max] = skipList->array[max - 1], max--;

	return skipList->array[max].val;
  }

  // initialize empty list

  skip->bits = allocBlk(map, sizeof(SkipList), true);
  skipList = getObj(map, *skip);

  *skipList->array->key = key;
  skip->nslot = 1;

  return skipList->array->val;
}

// regular list entry

void *listAdd(DbMap *map, DbAddr *list, uint64_t key) {
DbAddr *next = list, addr;
DbList *entry;

  while (next->addr) {
	entry = getObj(map, *next);

	if (*entry->node->key == key)
		return entry->node->val;

	next = entry->next;
  }

  // insert new node onto beginning of list

  addr.bits = allocBlk(map, sizeof(DbList), true) | list->bits & ADDR_MUTEX_SET;

  entry = getObj(map, addr);
  entry->next->bits = list->bits;
  *entry->node->key = key;
  list->bits = addr.bits;
  return entry->node->val;
}

// search list for entry

void *listFind(DbMap *map, DbAddr *list, uint64_t key) {
DbAddr *next = list;
DbList *entry;

  while (next->addr) {
	entry = getObj(map, *next);

	if (*entry->node->key == key)
		return entry->node->val;

	next = entry->next;
  }

  return NULL;
}

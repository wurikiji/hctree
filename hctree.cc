#define __GNU_LINUX
#include <stdio.h>
#include <algorithm>
#include <inttypes.h>
#include <utility>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "hctree.h"
#include <unordered_map>
#include <map>
#include <fcntl.h>
#include <vector>
#include <set> 

using namespace std;

#define KEY_TYPE uint64_t

#define USE_MEMMOVE 1
/* 	*
	* Open new db files using hctree 
	* This function will allocate a space for hctree **tree
	*/
int hc_open(hctree *tree, char *dbname)
{
	int ret = 0;
	node* lru;
	void* data;
	int fd;
	if (tree == NULL) {
		ret = RETURN_NOMEM;
		goto open_fail;
	}

	lru = (node*) malloc(sizeof(node));
	if (NULL == lru) {
		ret = RETURN_NOMEM;
		goto open_fail;
	}
	memset(lru, 0, sizeof(node));

	ret = __memalign(&data, 512, BLOCK_SIZE);
	if (ret) {
		ret = RETURN_NOMEM;
		goto open_fail;
	}
	memset(data, 0xff, BLOCK_SIZE);

	tree->fd = 0;

	if (dbname) {
		fd = open(dbname, O_RDWR | O_DIRECT);
		if (fd) {
			ret = RETURN_SYSTEM;
			goto open_fail;
		}
	}

	lru->data = data;

	(tree)->lru = lru;
	(tree)->hmap[0] = lru; // add lru head
	(tree)->isHctree = 0;
	(tree)->iTouched = 0;
	(tree)->pgCount = 1;
	(tree)->bSize = BLOCK_SIZE;
	(tree)->cmp_func = NULL;
	tree->iAccessed = 0;
	tree->iSplit = 0;

	return RETURN_SUCCESS;

open_fail:
	if (lru) {
		if (data) {
			__free(data);
		}
		__free(lru);
	}
	return ret;
}

/*	*
	* Close db 
	*/
int hc_close(hctree *tree)
{
	if (tree) {
		if (tree->lru) {
			if (tree->lru->data) {
				__free(tree->lru->data);
			}
			__free(tree->lru->data);
		}
		__free(tree);
	}
	if (tree->fd > 0) {
		close(tree->fd);
	}
	return RETURN_SUCCESS;
}

#define CUR_EXIST		0x00
#define CUR_NOT			0x01
#define RIGHT_NODE		0x02
#define LEFT_NODE		0x04

int compare(KEY_TYPE a, KEY_TYPE b) {
	if (a == b) return 0;
	if (a < b)  return -1;
	return 1;
}
// using sorted array to represent data in node 
int get_array_index(node* next_node, KEY_TYPE key, int *exist) 
{
	int idx = 0;
	KEY_TYPE *array = (KEY_TYPE*)next_node->data;
	int cResult = 0;

	
	*exist = CUR_NOT;

	for (idx = 0; idx < next_node->iCount; idx++)
	{
		cResult = compare(key, array[idx]);
		if (cResult == 0) {
			*exist = CUR_EXIST;
			break;
		} else if (cResult < 0) {
			*exist = LEFT_NODE; 
			break;
		} else {
			*exist = RIGHT_NODE;
		}
	}

	return idx;
}


int search_key_pos(hctree *tree, KEY_TYPE key, int *exist, 
					set<node*> *vct, int flag) 
{
	bool finish = false;
	int idx; 
	int exact;
	KEY_TYPE *data;
	uint64_t *pointer;
	uint64_t pgno = 0;
	node *next_node;
	map<uint64_t, node*>::iterator it;

	*exist = CUR_NOT;
	it = tree->hmap.find(0);
	next_node = it->second;

	if (next_node->iCount == 0)
		return 0;

	while (next_node) {
		//statistic
		tree->iAccessed++;

		idx = get_array_index(next_node, key, &exact);

		data = (KEY_TYPE*)next_node->data;
		pointer = (uint64_t*)(data + MAX_RECORDS);
		*exist = exact;

		tree->lru = next_node;
		 
		vct->insert(next_node); // push internal node list 

		if (exact == LEFT_NODE) {
			pgno = pointer[idx]; //left node
		} else if (exact == CUR_EXIST) {
			pgno = pointer[idx + 1]; //right node
		} else {
			pgno = pointer[idx]; //right node
		}
		//printf(" go to pgno %llu\n", pgno);

		it = tree->hmap.find(pgno);
		if (it != tree->hmap.end()) {
			next_node = it->second;
		} else {
			next_node = NULL;
		}
	}
	return idx;
}

void increase_touchcount(hctree *tree, set<node*> *vct, bool hot) 
{
	if (hot == HOT_DATA) 
		tree->iTouched += 2;
	else if (hot != COLD_DATA) 
		tree->iTouched += 1;

	for (set<node*>::iterator it = vct->begin(); it != vct->end(); ++it) {
		if (hot == HOT_DATA) 
			(*it)->iTouched += 2;
		else if (hot != COLD_DATA) 
			(*it)->iTouched += 1;
	}
}
/*	*
	* Get data from hctree, return whether it exists or not
	* If you already know hotness, give a hint to tree
	* If data are hot then touch count will increase by 2
	* If data are warm then touch count will increase by 1
	* If data are cold then touch count will not increase
	*/
int hc_get(hctree *tree, KEY_TYPE key, bool hot)
{
	int exist;
	set<node*> vct;
	int idx = search_key_pos(tree, key, &exist, &vct, 1);
	
	if (exist == CUR_EXIST) {
		if (hot == 0x00) {
			hot = WARM_DATA;
		}
		increase_touchcount(tree, &vct, hot);
		return RETURN_EXIST;
	}

	printf("Not exist %" PRIu64 ", page %" PRIu64 " idx: %d\n",
				key, tree->lru->pgno, idx);
	printf("Page information--\n");
	printf("Total records: %" PRIu64 " \n", tree->lru->iCount);
	printf("Record in %d is %" PRIu64 "\n",
				idx, ((KEY_TYPE*)tree->lru->data)[idx]);

	return RETURN_NOTEXIST;
}

static void __set_parent(hctree* tree, node* target) {
	KEY_TYPE *data = (KEY_TYPE*)target->data;
	uint64_t *pointer = (uint64_t*)(data + MAX_RECORDS);
	map<uint64_t, node*>::iterator it;

	for (int i = 0 ;i < target->iCount;i++)
	{
		it = tree->hmap.find(pointer[i]);
		if (it != tree->hmap.end()) {
			it->second->iParent = target->pgno;
		}
	}
	it = tree->hmap.find(pointer[target->iCount]);
	if (it != tree->hmap.end()) {
		it->second->iParent = target->pgno;
	}
}
static int __split_root(hctree* tree, node* target, int isLeaf) {
	int ret; 
	node *leaf1, *leaf2;
	KEY_TYPE *data1, *data2;
	uint64_t pgno = tree->pgCount;

	leaf1 = (node*) malloc(sizeof(node));
	leaf2 = (node*) malloc(sizeof(node));

	memset(leaf1, 0, sizeof(node));
	memset(leaf2, 0, sizeof(node));

	ret = __memalign((void**)&data1, 512, BLOCK_SIZE);
	ret = __memalign((void**)&data2, 512, BLOCK_SIZE);

	memset(data1, 0xff, BLOCK_SIZE);
	memset(data2, 0xff, BLOCK_SIZE);

	leaf1->data = (void*)data1;
	leaf1->pgno = pgno;
	leaf1->iParent = 0;

	leaf2->data = (void*)data2;
	leaf2->pgno = pgno + 1;
	leaf2->iParent = 0;

	tree->hmap[pgno] = leaf1;
	tree->hmap[pgno + 1] =  leaf2;

	tree->pgCount += 2;

	KEY_TYPE *data = (KEY_TYPE*)(target->data);
	uint64_t *pointer = (uint64_t*)(data + MAX_RECORDS);
	uint64_t *pointer1, *pointer2; 

	pointer1 = (uint64_t*)(data1 + MAX_RECORDS);
	pointer2 = (uint64_t*)(data2 + MAX_RECORDS);

	// move half to leaf1
	memcpy(data1, data, 
			sizeof(KEY_TYPE) * (target->iCount/2));
	memcpy(pointer1, pointer, 
			sizeof(uint64_t) * (target->iCount/2 + 1));

	leaf1->iCount = target->iCount/2;
	for (int i = 0 ;i < target->iCount/2;i++)
	{
		leaf1->iTouched += target->aCount[i];
		leaf1->aCount[i] = target->aCount[i];
	}
	

	// move remains to leaf2 
	memcpy(data2, data + (target->iCount/2),
						sizeof(KEY_TYPE) * ((target->iCount+1)/2));
	memcpy(pointer2, pointer + (target->iCount/2), 
						sizeof(uint64_t) * ((target->iCount+1)/2 + 1));

	leaf2->iCount = (target->iCount + 1)/2;
	for (int i = target->iCount/2 ;i < target->iCount;i++)
	{
		leaf2->iTouched += target->aCount[i];
		leaf2->aCount[i - target->iCount/2] = target->aCount[i];
	}
	
	memset(data, 0xff, BLOCK_SIZE);
	data[0] = data2[0]; //insert middle key to root 

	if (!isLeaf) {
		leaf2->iCount -= 1;
		leaf2->iTouched -= leaf1->aCount[0];
		memmove(leaf2->aCount, &leaf2->aCount[1], 
						sizeof(leaf2->aCount[0]) * leaf2->iCount);
		memmove(data2, &data2[1] , sizeof(KEY_TYPE) * leaf2->iCount);
		memmove(pointer2, &pointer2[1], 
					sizeof(uint64_t) * (leaf2->iCount + 1));
	}

//printf("Middle key %" PRIu64 "\n", data[0]);
	/* Set left and right pointer */
	pointer[0] = leaf1->pgno;
	pointer[1] = leaf2->pgno;
	memset(target->aCount, 0, sizeof(MAX_RECORDS) * sizeof(uint64_t));
	target->iCount = 1;
	target->aCount[0] = leaf1->aCount[leaf1->iCount - 1];

	__set_parent(tree, leaf1);
	__set_parent(tree, leaf2);
//printf("Split pgcount changes to %llu\n", tree->pgCount);
//printf("Create new pgno %llu, %llud\n", leaf1->pgno, leaf2->pgno);
//printf("Split root\n");
	return RETURN_SUCCESS;

split_fail: 
	return ret;
}

static int __get_pos_in_this_node(node* target, KEY_TYPE key, int *exist) {
	int idx = 0;
	KEY_TYPE *array = (KEY_TYPE*)target->data;
	int cResult = 0;

	*exist = CUR_NOT;

	for (idx = 0; idx < target->iCount; idx++)
	{
		cResult = compare(key, array[idx]);
		if (cResult == 0) {
			*exist = CUR_EXIST;
			break;
		} else if (cResult < 0) {
			*exist = LEFT_NODE; 
			break;
		} else {
			*exist = RIGHT_NODE;
		}
	}
	return idx;
}

static int hc_split(hctree* tree, node* target, int isLeaf);
static int __split_other(hctree* tree, node* target, int isLeaf) {
	int ret, idx, exist;
	node *parent;
	node *new_node;
	KEY_TYPE *new_data, *data;
	uint64_t *new_pointer, *pointer;
	KEY_TYPE middle_key;

	new_node = (node*) malloc(sizeof(node));
	memset(new_node, 0x00 , sizeof(node));

	ret = __memalign((void**)&new_data, 512, BLOCK_SIZE);
	memset(new_data, 0xff , BLOCK_SIZE);

	new_node->data = (void*) new_data;
	new_node->pgno = tree->pgCount;
	new_node->iParent = target->iParent;
	tree->hmap[new_node->pgno] = new_node;

	(tree->pgCount) += 1;

	data = (KEY_TYPE*)target->data;
	new_pointer = (uint64_t*)(new_data + MAX_RECORDS);
	pointer = (uint64_t*)(data + MAX_RECORDS);

	// move right half to new_node
	memcpy(new_data, data + (target->iCount/2),
						sizeof(KEY_TYPE) * ((target->iCount+1)/2));
	memcpy(new_pointer, pointer + (target->iCount/2), 
						sizeof(uint64_t) * ((target->iCount+1)/2 + 1));

	new_node->iCount = (target->iCount + 1)/2;
	for (int i = target->iCount/2 ;i < target->iCount;i++)
	{
		new_node->iTouched += target->aCount[i];
		new_node->aCount[i - target->iCount/2] = target->aCount[i];
		target->iTouched -= target->aCount[i];
		target->aCount[i] = 0;
		memset(&data[i], 0xff, sizeof(KEY_TYPE));
		memset(&pointer[i+1], 0xff, sizeof(uint64_t));
	}

	target->iCount -= new_node->iCount;


	middle_key = new_data[0];
	uint64_t new_count = new_node->aCount[0];
	parent = tree->hmap.find(target->iParent)->second; // get parent node

	if (!isLeaf) { // remove from internal node
		new_node->iCount -= 1;
		new_node->iTouched -= target->aCount[0];
		memmove(new_node->aCount, &new_node->aCount[1],
						sizeof(new_node->aCount[0]) * new_node->iCount);
		memmove(new_data, &new_data[1], sizeof(KEY_TYPE) * new_node->iCount);
		memmove(new_pointer, &new_pointer[1], 
					sizeof(uint64_t) * (new_node->iCount + 1));
	}
//printf("Middle key %" PRIu64 "\n", middle_key);
	idx = __get_pos_in_this_node(parent, middle_key, &exist);

	int start;

	data = (KEY_TYPE*) parent->data;
	pointer = (uint64_t*)(data + MAX_RECORDS);

	if (exist == CUR_EXIST) {
		printf("Error on split: middle key %" PRIu64 " exists on internal node %" PRIu64 "\n",
					middle_key, parent->pgno);
		//tree->lru = parent;
		//hc_dump(tree);
		exit(0);
	} else if (exist == LEFT_NODE) {
		start = idx;
		memmove(&data[start + 1], &data[start],
				sizeof(KEY_TYPE) * (parent->iCount - start));
		memmove(&pointer[(start + 1)],
				&pointer[start],
				sizeof(uint64_t) * (parent->iCount - start + 1));
		memmove(&parent->aCount[start + 1], &parent->aCount[start],
				sizeof(parent->aCount[0]) * (parent->iCount - idx));
	} else if (exist == RIGHT_NODE) { 
	} else {
		printf("Error on split: node has no data\n");
		exit(0);
	}

	// save key 
	memcpy(&data[idx], &middle_key, sizeof(KEY_TYPE));
	pointer[idx] = target->pgno;
	pointer[idx + 1] = new_node->pgno;

#if 0 // older version 
	if (idx != parent->iCount) // if it is not the last key
		pointer[(idx + 1) * 2] = new_node->pgno; // set left child of next key
	if (idx != 0)  // if it is not the first key 
		pointer[(idx - 1) * 2 + 1] = target->pgno; //set right child of prev key
#endif

	parent->iCount++;
	parent->aCount[idx] = new_count;
	parent->iTouched += parent->aCount[idx];
	
//printf("Split pgcount changes to %llu\n", tree->pgCount);
//printf("Create new pgno %llu\n", new_node->pgno);
	__set_parent(tree, new_node);
	__set_parent(tree, target);

	if (parent->iCount == MAX_RECORDS)
		return hc_split(tree, parent, 0);

	return RETURN_SUCCESS;
}

static int hc_split(hctree* tree, node* target, int isLeaf) {
	//statistic
	tree->iSplit++;
	if (target->pgno == 0) {
		return __split_root(tree, target, isLeaf);
	} else {
		return __split_other(tree, target, isLeaf);
	}
}
/* 	*
	* Insert data into hctree
	* If you already know hotness, give a touch count hint
	*/
int hc_put(hctree *tree, KEY_TYPE key, bool hot) 
{
	int idx;
	int exist;
	set<node*> vct;
	node *target; 
	KEY_TYPE *data;
	uint64_t *pointer;

	idx = search_key_pos(tree, key, &exist, &vct, 0);

	increase_touchcount(tree, &vct, hot);

	vct.clear();

	target = tree->lru;
	data = (KEY_TYPE*) target->data;
	pointer = (uint64_t*) (data + MAX_RECORDS);

	if (hot == 0x00) {
		hot = WARM_DATA;
	}

	if (exist == CUR_NOT) { // first insertion
		memcpy(&data[0], &key, sizeof(KEY_TYPE));
		target->aCount[0]++;
		target->iCount++;
	} else if (exist == CUR_EXIST) {
		target->aCount[idx]++;
		// do nothing because it already exists
	} else {
		int start = idx;
		// move data 
		if (exist == LEFT_NODE) { 
			memmove(&data[start + 1], &data[start],
					sizeof(KEY_TYPE) * (target->iCount - start));
			memmove(&target->aCount[start+1], &target->aCount[start],
						sizeof(target->aCount[0]) * (target->iCount - start));
		}
		// save key 
		memcpy(&data[idx], &key, sizeof(KEY_TYPE));
		target->aCount[idx]++;
		target->iCount++;

		if (target->iCount == MAX_RECORDS) { //need split
			hc_split(tree, target, 1);
		}
	}
	return RETURN_SUCCESS;
}

/*	*
	* Remove data from hctree
	*/
int hc_remove(hctree *tree, KEY_TYPE key) 
{
	return RETURN_SUCCESS;
}

/* 	*
	* Bulk load the data from files 
	*/
int hc_load(hctree *tree, char *filename)
{
	return RETURN_SUCCESS;
}

/*	*
	* Set tree to be b+tree or hctree
	*/
int hc_set_hctree(hctree *tree, bool flag)
{
	tree->isHctree = flag;
	return RETURN_SUCCESS;
}

void trip(hctree *tree, node *next, int level, uint64_t *count) {
	if (next == NULL || next->iCount == 0) {
		return;
	}
	node* temp;
	KEY_TYPE *data;
	uint64_t *pointer;
	map<uint64_t, node*>::iterator it;
	int i;
	data = (KEY_TYPE*)next->data;
	pointer = (uint64_t*)(data + MAX_RECORDS);

	it = tree->hmap.find(pointer[0]);
	if (it != tree->hmap.end())  {
		trip(tree, it->second, level + 1, count);
	}
	for(i = 0 ;i < next->iCount;i++)
	{
		it = tree->hmap.find(pointer[i + 1]);
		if (it != tree->hmap.end())  {
			trip(tree, it->second, level + 1, count);
		}
	}
#if 1
	printf("PGNO %u, PARENT %u, LEVEL %d\n", 
			next->pgno, next->iParent, level);
	
	for (int i =0 ;i < next->iCount; i++) {
		printf("%" PRIu64 " ", data[i]);
	}
	printf("\n");
#endif
	if (count) *count += 1;
}
void hc_dump(hctree *tree) {
	node *next = tree->hmap.find(0)->second;
	uint64_t count;

	trip(tree, next, 0, &count);
	printf("Total %" PRIu64 " pages\n", count);
}

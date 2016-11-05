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
#include <fcntl.h>
#include <vector>
#include <set> 

using namespace std;

#define KEY_TYPE uint64_t

/* 	*
	* Open new db files using hctree 
	* This function will allocate a space for hctree **tree
	*/
int open(hctree **tree, char *dbname)
{
	int ret = 0;
	node* lru;
	void* data;
	int fd;
	*tree = (hctree*) malloc(sizeof(hctree));
	if (tree == NULL) {
		ret = RETURN_NOMEM;
		goto fail;
	}

	lru = (node*) malloc(sizeof(node));
	if (NULL == lru) {
		ret = RETURN_NOMEM;
		goto fail;
	}
	memset(lru, 0, sizeof(node));

	ret = __memalign(&data, 512, BLOCK_SIZE);
	if (ret) {
		ret = RETURN_NOMEM;
		goto fail;
	}
	memset(data, 0xff, BLOCK_SIZE);

	fd = open(dbname, O_RDWR | O_DIRECT);
	if (fd) {
		ret = RETURN_SYSTEM;
		goto fail;
	}

	lru->data = data;

	(*tree)->lru = lru;
	(*tree)->hmap.insert(pair<uint64_t, node*>(0, lru)); // add lru head
	(*tree)->isHctree = 0;
	(*tree)->fd = 0;
	(*tree)->iTouched = 0;
	(*tree)->pgCount = 0;

	return RETURN_SUCCESS;

fail:
	if (*tree) {
		if (lru) {
			if (data) {
				__free(data);
			}
			__free(lru);
		}
		__free(*tree);
	}
	return ret;
}

/*	*
	* Close db 
	*/
int close(hctree *tree)
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

// using sorted array to represent data in node 
int get_array_index(void* data, KEY_TYPE key, int *exist) 
{
	int idx = 0;
	KEY_TYPE *array = (KEY_TYPE*)data;
	KEY_TYPE *temp;

	temp = (KEY_TYPE*)malloc(sizeof(KEY_TYPE));
	memset(temp, 0xff, sizeof(KEY_TYPE));
	
	*exist = CUR_NOT;

	for (idx = 0; idx < MAX_RECORDS; idx++)
	{
		if (array[idx] == key) {
			*exist = CUR_EXIST;
			break;
		} else if (0 == memcmp(&array[idx], temp, sizeof(KEY_TYPE))) { // empty Record
			*exist = RIGHT_NODE;
			idx -= 1;
			break;
		} else if (array[idx] > key) { // smallest record larger than key 
			*exist = LEFT_NODE; 
			break;
		}
	}
	__free(temp);
	return idx;
}


/* 	*
	* find key from current node 
	* This code is for further optimizations.
	*/
int get_binary_index(void* data, KEY_TYPE key, int *exist) 
{
	int idx = 0;
	int prev_idx = 0;
	int cmp = 0;
	int height = 0;
	int len = sizeof(KEY_TYPE);
	KEY_TYPE *temp;
	KEY_TYPE *binary = (KEY_TYPE*)data;


	temp = (KEY_TYPE*)malloc(sizeof(KEY_TYPE));
	memset(temp, 0xff, sizeof(KEY_TYPE));
	
	*exist = CUR_NOT; // no data on hctree
	while (idx <= MAX_RECORDS) { 
		if (memcmp(&binary[idx], temp, sizeof(KEY_TYPE) == 0)) { // end of binary tree
			break;
		}
		cmp = memcmp(&binary[idx], &key, sizeof(KEY_TYPE));
		if (cmp == 0) {
			*exist = CUR_EXIST;
			free(temp);
			return idx;
		} else if (cmp < 0) {
			prev_idx = idx;
			idx = idx * 2 + 1;
			*exist = LEFT_NODE;
		} else {
			prev_idx = idx;
			idx = idx * 2 + 2;
			*exist = RIGHT_NODE;
		}
	}

	free(temp);
	return prev_idx;
}

int search_key_pos(hctree *tree, KEY_TYPE key, const bool hot, int *exist, 
					set<node*> *vct) 
{
	bool finish = false;
	int idx; 
	int exact;
	KEY_TYPE *data;
	char* next_page;
	uint64_t pgno = 0;
	node *next_node;
	unordered_map<uint64_t, node*>::iterator it;

	*exist = CUR_NOT;
	it = tree->hmap.find(0);
	next_node = it->second;

	while (next_node) {
		idx = get_array_index(next_node->data, key, &exact);

		data = (KEY_TYPE*)next_node->data;
		next_page = ((char*)next_node->data) + BLOCK_SIZE;
		*exist = exact;

		tree->lru = next_node;
		 
		vct->insert(next_node); // push internal node list 

		if (exact == CUR_NOT) {
			return idx;
		} else if (exact == CUR_EXIST) {
			memcpy(&pgno, next_page - idx * 2, sizeof(uint64_t)); //left node
		} else if (exact == LEFT_NODE) {
			memcpy(&pgno, next_page - idx * 2, sizeof(uint64_t)); //left node
		} else {
			memcpy(&pgno, next_page - idx * 2 + 1, sizeof(uint64_t)); //right node
		}

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
int get(hctree *tree, KEY_TYPE key, const bool hot)
{
	int exist;
	set<node*> vct;
	search_key_pos(tree, key, hot, &exist, &vct);

	if (exist == CUR_EXIST) {
		increase_touchcount(tree, &vct, hot);
		return RETURN_EXIST;
	}

	return RETURN_NOTEXIST;
}

/* 	*
	* Insert data into hctree
	* If you already know hotness, give a touch count hint
	*/
int put(hctree *tree, KEY_TYPE key, bool hot) 
{
	int idx;
	int exist;
	set<node*> vct;
	node *target; 
	KEY_TYPE *data;
	char* page;

	idx = search_key_pos(tree, key, hot, &exist, &vct);

	target = tree->lru;

	increase_touchcount(tree, &vct, hot);

	vct.clear();
	data = (KEY_TYPE*) target->data;
	page = ((char*) target->data) + BLOCK_SIZE;

	if (exist == CUR_NOT) { // first insertion
		target->iCount++;
		memcpy(&data[0], &key, sizeof(KEY_TYPE));
	} else if (exist == CUR_EXIST) {
		// do nothing because it already exists
	} else {
		if (target->iCount == MAX_RECORDS) { //need split
		}
		if (exist == LEFT_NODE) {
			//memcpy(
		} else {
		}
		target->iCount++;
	}
	return RETURN_SUCCESS;
}

/*	*
	* Remove data from hctree
	*/
int remove(hctree *tree, KEY_TYPE key) 
{
	return RETURN_SUCCESS;
}

/* 	*
	* Bulk load the data from files 
	*/
int load(hctree *tree, char *filename)
{
	return RETURN_SUCCESS;
}

/*	*
	* Set tree to be b+tree or hctree
	*/
int set_hctree(hctree *tree, bool flag)
{
	tree->isHctree = flag;
	return RETURN_SUCCESS;
}

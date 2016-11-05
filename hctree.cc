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
	memset(*tree, 0, sizeof(hctree));

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
	memset(data, 0, BLOCK_SIZE);

	fd = open(dbname, O_RDWR | O_DIRECT);
	if (fd) {
		ret = RETURN_SYSTEM;
		goto fail;
	}

	lru->data = data;
	lru->iTouched = 0;
	(*tree)->lru = lru;
	(*tree)->hmap.insert(pair<uint64_t, node*>(0, lru)); // add lru head
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

/*	*
	* Get data from hctree
	* If you already know hotness, give a hint to tree
	* If data are hot then touch count will increase by 2
	* If data are warm then touch count will increase by 1
	* If data are cold then touch count will not increase
	*/
int get(hctree *tree, KEY_TYPE key, bool hot) 
{
	return RETURN_SUCCESS;
}

/* 	*
	* Insert data into hctree
	* If you already know hotness, give a touch count hint
	*/
int put(hctree *tree, KEY_TYPE key, uint64_t hint) 
{
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

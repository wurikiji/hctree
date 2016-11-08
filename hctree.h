#include <inttypes.h>
#include <unordered_map>
#include <map>


// user can define KEY_TYPE before including hctree.h
#ifndef KEY_TYPE
#define KEY_TYPE uint64_t
#endif // end of KEY_TYPE


#define		BTREE 		0x00
#define		HCTREE		0x01


#define		HOT_DATA	0x01
#define		COLD_DATA	0x02
#define		WARM_DATA	0x04

#define 	RETURN_SUCCESS 	0x00
#define 	RETURN_NOMEM 	0x01
#define		RETURN_FAIL		0x02
#define		RETURN_SYSTEM	0x04

#define		RETURN_EXIST	0x00
#define		RETURN_NOTEXIST	0x01

#define BLOCK_SIZE		(4096)  //block size in bytes
//#define MAX_RECORDS		(BLOCK_SIZE / (sizeof(KEY_TYPE) + sizeof(uint64_t) * 2))
#define MAX_RECORDS		(BLOCK_SIZE / (sizeof(KEY_TYPE) + sizeof(uint64_t)) - 1)
#if 0 //for test 
#define MAX_RECORDS		(3)  // for test 
#define BLOCK_SIZE		(128)  //block size in bytes
#endif


typedef struct __hctree hctree;
typedef struct __node node;

using namespace std;

struct __hctree{
	bool isHCtree;		// true for hctree, false for b+tree
	int fd;				// file descriptor for this tree
	node*	lru;		// head page of LRU list
	int (*cmp_func)(KEY_TYPE a, KEY_TYPE b);
	map<uint64_t, node*> hmap; // hash map for searching page
	uint64_t iTouched; 	// total touchcount for this tree
	uint64_t pgCount;	// # of pages
	uint64_t bSize; 	// block size
	uint64_t iAccessed; // total accessed pages
	uint64_t iSplit;	// total split count
};

// This will be managed as LRU list
struct __node{
	void*		data; 		// Memmory addresses for data
	uint64_t 	iTouched; 	// total touchcount for this node
	uint64_t	iParent; 	// parent node page number
	uint64_t	iCount; 	// # of total records in this node
	uint64_t	pgno;
	uint64_t	next; 		// next page pgno
	uint64_t	prev;		// prev page pgno
	uint64_t	aCount[MAX_RECORDS]; 	// touch count array for records in this node
};


struct __file_header{
};

struct __page_header{
	uint64_t pgno;
};

struct __record{

};
/* 	*
	* Open new db files using hctree 
	* This function will allocate a space for hctree **tree
	*/
int hc_open(hctree *tree, char *dbname);
int hc_open(hctree *tree) {
	return hc_open(tree, NULL);
}

/*	*
	* Close db 
	*/
int hc_close(hctree **tree);

/*	*
	* Get data from hctree
	* If you already know hotness, give a hint to tree
	* If data are hot then touch count will increase by 2
	* If data are warm then touch count will increase by 1
	* If data are cold then touch count will not increase
	*/
int hc_get(hctree *tree, KEY_TYPE key, bool hot) ;

/* 	*
	* Insert data into hctree
	* If you already know hotness, give a touch count hint
	*/
int hc_put(hctree *tree, KEY_TYPE key, uint64_t hint) ;
int hc_put(hctree *tree, KEY_TYPE key, bool hot) {
	if (hot == WARM_DATA)
		hc_put(tree, key, (uint64_t)1);
	else if (hot == HOT_DATA)
		hc_put(tree, key, (uint64_t)2);
	else
		hc_put(tree, key, (uint64_t)0);
}

/*	*
	* Remove data from hctree
	*/
int hc_remove(hctree *tree, KEY_TYPE key) ;

/* 	*
	* Bulk load the data from files 
	*/
int hc_load(hctree *tree, char *filename);

/*	*
	* Set tree to be b+tree or hctree
	*/
int hc_set_hctree(hctree *tree, bool flag) ;

void hc_dump(hctree *tree);

#include <inttypes.h>
#include <hash_map>


// user can define KEY_TYPE before including hctree.h
#ifndef KEY_TYPE
#define KEY_TYPE uint64_t
#endif // end of KEY_TYPE


#define		BTREE 		0x01
#define		HCTREE		0x02


#define		HOT_DATA	0x01
#define		COLD_DATA	0x02
#define		WARM_DATA	0x04

#define 	RETURN_SUCCESS 	0x00
#define 	RETURN_NOMEM 	0x01
#define		RETURN_FAIL		0x02
#define		RETURN_SYSTEM	0x04


#define BLOCK_SIZE		(4096)  //block size in bytes


typedef struct __hctree hctree;
typedef struct __node node;

using namespace stdext;
struct __hctree{
	bool isHctree;		// true for hctree, false for b+tree
	int fd;				// file descriptor for this tree
	node*	lru;		// head page of LRU list
	hash_map<KEY_TYPE, void*> hmap; // hash map for searching page
	uint64_t iTouched; 	// total touchcount for this tree
};

// This will be managed as LRU list
struct __node{
	uint64_t 	iTouched; 	// total touchcount for this node
	node*		next;
	node*		prev;
	void*		data; 		// Memmory addresses for data
};


struct __file_header{
};

struct __page_header{
	uint64_t pgno;
};

/* 	*
	* Open new db files using hctree 
	* This function will allocate a space for hctree **tree
	*/
int open(hctree **tree, char *dbname);

/*	*
	* Close db 
	*/
int close(hctree **tree);

/*	*
	* Get data from hctree
	* If you already know hotness, give a hint to tree
	* If data are hot then touch count will increase by 2
	* If data are warm then touch count will increase by 1
	* If data are cold then touch count will not increase
	*/
int get(hctree *tree, KEY_TYPE key, bool hot) ;

/* 	*
	* Insert data into hctree
	* If you already know hotness, give a touch count hint
	*/
int put(hctree *tree, KEY_TYPE key, uint64_t hint) ;

/*	*
	* Remove data from hctree
	*/
int remove(hctree *tree, KEY_TYPE key) ;

/* 	*
	* Bulk load the data from files 
	*/
int load(hctree *tree, char *filename);

/*	*
	* Set tree to be b+tree or hctree
	*/
int set_hctree(hctree *tree, bool flag) ;

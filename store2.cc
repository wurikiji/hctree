#include "hctree.h"
#include <iostream>
#include <algorithm>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <inttypes.h>
#include <time.h>
#include <map>

using namespace std;

struct kv{
	uint64_t key;
	uint64_t count;
};
const bool cmp (struct kv a, struct kv b) {
	return a.count > b.count;
}
int main(int argc, char *argv[]) 
{
	hctree tree;
	vector<uint64_t> v;
	int num = MAX_RECORDS ;
	FILE *fp;
	int hc = 0;

	if (argc > 1) {
		fp = fopen(argv[1], "r+");
	} else {
		fp = fopen("linkbench.trace", "r+");
	}

	if (argc > 2) {
		hc = atoi(argv[2]);
	}

	printf("Hello\n");
	hc_open(&tree);

	srand(time(NULL));
	if (0 != hc)
		hc_set_hctree(&tree, HCTREE);
	long long unsigned key;

	printf("Load linkbench\n");

	map<uint64_t, uint64_t> counter;
	map<uint64_t, uint64_t>::iterator it;
	while (EOF != fscanf(fp, "%llu", &key)) {
		hc_put(&tree, key, true);
		it = counter.find(key);
		if (it != counter.end()) {
			counter[key] ++ ;
		} else {
			counter[key] = 1;
		}
		v.push_back(key);
	}

	printf("End insert\n");
	printf("Accessed pages %" PRIu64 "\n", tree.iAccessed);
	printf("Split %" PRIu64 "\n", tree.iSplit);

	tree.iAccessed = 0;
	for (int i = 0;i < v.size();i++)
	{
		//printf("Get %d\n", i);
		if (RETURN_EXIST != hc_get(&tree, v[i],true))
		{
			hc_dump(&tree);
			break;
		}
	}

	printf("End read\n");
	printf("Accessed pages %" PRIu64 "\n", tree.iAccessed);

	vector<struct kv> vv;

	hctree tree3;
	hc_open(&tree3);
	if (hc) 
		hc_set_hctree(&tree3, HCTREE);

	for (map<uint64_t, uint64_t>::iterator i = counter.begin();
		i != counter.end();++i) {
		struct kv temp;
		temp.key = i->first;
		temp.count = i->second;
		hc_put(&tree3, temp.key, temp.count);
		vv.push_back(temp);
	}

	printf("End random load\n");
	printf("Accessed pages %" PRIu64 "\n", tree3.iAccessed);
	printf("Split %" PRIu64 "\n", tree3.iSplit);
	sort(vv.begin(), vv.end(), cmp);
	cout << vv[0].count  <<endl;

	tree3.iAccessed = 0;
	for (int i = 0;i < v.size();i++)
	{
		//printf("Get %d\n", i);
		if (RETURN_EXIST != hc_get(&tree3, v[i],true))
		{
			hc_dump(&tree3);
			break;
		}
	}

	printf("End read tree3\n");
	printf("Accessed pages %" PRIu64 "\n", tree3.iAccessed);

	hctree tree2;
	hc_open(&tree2);
	if (0 != hc)
		hc_set_hctree(&tree2, HCTREE);
	
	for (int i = 0;i < vv.size();i++) {
		hc_put(&tree2, vv[i].key, vv[i].count);
	}

	printf("End seq load\n");
	printf("Accessed pages %" PRIu64 "\n", tree2.iAccessed);
	printf("Split %" PRIu64 "\n", tree2.iSplit);


	tree2.iAccessed = 0;
	for (int i = 0;i < v.size();i++)
	{
		//printf("Get %d\n", i);
		if (RETURN_EXIST != hc_get(&tree2, v[i],true))
		{
			hc_dump(&tree2);
			break;
		}
	}

	printf("End read tree2\n");
	printf("Accessed pages %" PRIu64 "\n", tree2.iAccessed);

	fclose(fp);

	return 0;
}

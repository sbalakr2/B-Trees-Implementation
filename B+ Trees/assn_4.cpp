// CSC 541 Assignment 4 -  B-trees

#include <iostream>
#include <string>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <iomanip>
using namespace std;

struct btree_node {
	int n;
	int *key;
	long *child;
};

struct queue_node {
	long *queue;
	int front;
	int rear;
};

void getUserCommand(fstream &fp, int order);
void addKey(fstream &file, int order, int key);
void findKey(fstream &file, int order, int key);
void printTree(fstream &file, int order);
void closeandExit(fstream &file);
btree_node createNode(int order);
btree_node insertKey(btree_node node, int key, long leftOffset, long rightOffset, int order);
long writeNodeGetOffset(fstream &file, btree_node node, int order);
void updateRootOffset(fstream &file, long offset);
btree_node searchBtree(fstream &file, int order, int key);
btree_node btree_search(fstream &file, long nodeOffset, int order, int targetKey);
btree_node getNodeAtOffset(fstream &file, long nodeOffset, int order);
bool isKeyInNode(int key, btree_node node);
void writeNodeAtOffset(fstream &file, btree_node node, long offset, int order);
int *getSortedAuxArray(int auxSiz, btree_node curNode, int key);
void insertKeyInTarget(fstream &file, int key, btree_node node, int order, long leftOffset, long rightOffset);
long createParentNodeAndWrite(fstream &file, int parentKey, long leftOffset, long rightOffset, int order);
long createRightNodeAndWrite(fstream &file, int *rightArr, int rightCnt, int order, btree_node curNode, int *ptrArr);
long copyLeftNodeAndWrite(fstream &file, btree_node curNode, int *leftArr, int leftArrCnt, int order, int *ptrArr);
int *getLeftArray(int leftCnt, int *auxArrOfKeys);
int *getRightArray(int median, int *auxArrOfKeys, int order);
void splitPromoteNode(fstream &file, int key, btree_node curNode, int order, long leftOff, long rightOff);
int getIndexOfKey(int key, int *array, int length);
long getRootOffset(fstream &file);
int *getAuxArrOfPtrs(int order, btree_node node, long leftOff, long rightOff, int keyIdx);
btree_node printKeysInNode(long offset, int lvl, fstream &file, int order, bool isNewLvl, int nodeCnt);
void deQueueAndPrint(queue_node que, fstream &file, int order, int level, int nodeCnt, int nextLvlCnt, bool isNewLvl);
queue_node enQueue(queue_node que, int offset);
queue_node createQueue(int len);

long curNodeOffset = -1;
long parentOffset = -1;
int qsortComp(const void * x, const void * y) {
	return (*(int*)x - *(int*)y);
}

int main(int argc, char *argv[]) {
	//int a;
	if (argc != 3) {
		cout << "Enter the right number of parameters of the format: index-file order" << endl;
	}
	else {
		string indexFileName = argv[1]; 
		int order = atoi(argv[2]);
		if (order >= 3) {
			long root = -1;
			ifstream file(indexFileName.c_str());
			if (!file) {
				// create an index file
				fstream fp(indexFileName.c_str(), ios::out);
				fp.write(reinterpret_cast<const char*>(&root), sizeof(long));
				fp.close();
			}
			else {
				// read from existing index file
				fstream fp(indexFileName.c_str(), ios::out | ios::in | ios::binary);
				fp.read(reinterpret_cast<char*>(&root), sizeof(long));
				fp.close();
			}
			fstream filePtr(indexFileName.c_str(), ios::out | ios::in | ios::binary);
			getUserCommand(filePtr, order);
		}
		else {
			cout << "Enter an order greater than or equal to 3" << endl;
		}
	}
	//cin >> a;
	return 0;
}

void getUserCommand(fstream &file, int order) {
	string str;
	getline(cin, str);
	int pos = str.find(" ");
	string cmd = str.substr(0, pos);
	string keystr = str.substr(pos + 1, str.size());	
	int key = atoi(keystr.c_str());

	if (cmd.compare("add") == 0) {
		addKey(file, order, key);
	}
	else if (cmd.compare("find") == 0) {
		findKey(file, order, key);
	}
	else if (cmd.compare("print") == 0) {
		printTree(file, order);
	}
	else if (cmd.compare("end") == 0) {
		closeandExit(file);
	}
	else {
		cout << "Enter a valid command: add, find, print or end" << endl;
		getUserCommand(file, order);
	}
}

void addKey(fstream &file, int order, int key) {
	long root = getRootOffset(file);
	if (root == -1) {
		btree_node node = createNode(order);
		node = insertKey(node, key, -1, -1, order);
		root = writeNodeGetOffset(file, node, order);
		updateRootOffset(file, root);
	}
	else {
		// search tree to find target node
		btree_node targetNode;
		bool isFound = false;
		long rootOffset = -1;
		file.seekg(0, ios::beg);
		file.read(reinterpret_cast<char*> (&rootOffset), sizeof(long));
		targetNode = btree_search(file, rootOffset, order, key);
		isFound = isKeyInNode(key, targetNode);
		if (isFound) {
			cout << "Entry with key=" << key << " already exists" << endl;
		}
		else {
			insertKeyInTarget(file, key, targetNode, order, -1, -1);
		}
	}
	getUserCommand(file, order);
}

void insertKeyInTarget(fstream &file, int key, btree_node node, int order, long leftOffset, long rightOffset) {
	int maxKey = order - 1;
	btree_node newNode;
	if (node.n < maxKey) {
		newNode = insertKey(node, key, leftOffset, rightOffset, order);
		writeNodeAtOffset(file, newNode, curNodeOffset, order);
	}
	else if(node.n == maxKey){
		splitPromoteNode(file, key, node, order, leftOffset, rightOffset);
	}
}

void splitPromoteNode(fstream &file, int key, btree_node curNode, int order, long leftOff, long rightOff) {
	int *auxArrOfKeys = getSortedAuxArray(order, curNode, key);
	int keyIdx = getIndexOfKey(key, auxArrOfKeys, order);
	int median = (int)ceil(((float)order - 1) / 2);
	int *leftArr = getLeftArray(median, auxArrOfKeys);
	int *rightArr = getRightArray(median, auxArrOfKeys, order);
	int parentKey = auxArrOfKeys[median];

	int *auxArrOfPtrs = 0;
	if (curNode.child[0] != 0) {
		auxArrOfPtrs = getAuxArrOfPtrs(order, curNode, leftOff, rightOff, keyIdx);
	}
	
	int rightCnt = order - (median+1);
	long rightNodeOffset = createRightNodeAndWrite(file, rightArr, rightCnt, order, curNode, auxArrOfPtrs);
	long leftNodeOffset = copyLeftNodeAndWrite(file, curNode, leftArr, median, order, auxArrOfPtrs);
	delete[] auxArrOfKeys, auxArrOfPtrs, leftArr, rightArr;

	if (parentOffset == -1) {
		long newRootOffset = createParentNodeAndWrite(file, parentKey, leftNodeOffset, rightNodeOffset, order);
		updateRootOffset(file, newRootOffset);
	}
	else {
		long targetNodeOffset = curNodeOffset;
		btree_node parentNode = getNodeAtOffset(file, parentOffset, order);
		parentOffset = -1;
		long rootOffset = getRootOffset(file);
		btree_node targetNode;
		if (rootOffset != -1) {
			targetNode = btree_search(file, rootOffset, order, parentNode.key[0]);
		}
		insertKeyInTarget(file, parentKey, parentNode, order, leftNodeOffset, rightNodeOffset);
	}
}

int *getAuxArrOfPtrs(int order, btree_node node, long leftOff, long rightOff, int keyIdx) {
	int ptrCnt = order + 1;
	int *auxArrOfPtrs = new int[ptrCnt];
	int lastChildIdx = order-1;
	for (int c = (lastChildIdx + 1); c > (keyIdx + 1); c--) {
		auxArrOfPtrs[c] = node.child[c - 1];
	}
	auxArrOfPtrs[keyIdx + 1] = rightOff;
	auxArrOfPtrs[keyIdx] = leftOff;
	for (int d = 0; d < keyIdx; d++) {
		auxArrOfPtrs[d] = node.child[d];
	}
	return auxArrOfPtrs;
}

long createParentNodeAndWrite(fstream &file, int parentKey, long leftOffset, long rightOffset, int order) {
	btree_node parentNode = createNode(order);
	parentNode = insertKey(parentNode, parentKey, leftOffset, rightOffset, order);
	long offset = writeNodeGetOffset(file, parentNode, order);
	return offset;
}

long createRightNodeAndWrite(fstream &file, int *rightArr, int rightCnt, int order, btree_node curNode, int *ptrArr) {
	btree_node rightNode = createNode(order);
	rightNode.n = rightCnt;
	for (int i = 0; i < rightCnt; i++) {
		rightNode.key[i] = rightArr[i];
	}
	// child ptrs
	if (curNode.child[0] != 0 && ptrArr != 0) {
		int rightPtrStart = order - rightCnt;
		for (int j = 0; j <= rightCnt; j++) {
			rightNode.child[j] = ptrArr[rightPtrStart];
			rightPtrStart++;
		}
	}
	long offset = writeNodeGetOffset(file, rightNode, order);
	return offset;
}

long copyLeftNodeAndWrite(fstream &file, btree_node curNode, int *leftArr, int leftArrCnt, int order, int *ptrArr) {
	curNode.n = leftArrCnt;
	for (int i = 0; i < leftArrCnt; i++) {
		curNode.key[i] = leftArr[i];
	}
	for (int j = leftArrCnt; j < (order - 1); j++) {
		curNode.key[j] = 0;
	}
	// update child ptrs
	if (curNode.child[0] != 0 && ptrArr != 0) {
		for (int l = 0; l <= leftArrCnt; l++) {
			curNode.child[l] = ptrArr[l];
		}
		for (int k = (order - 1); k > leftArrCnt; k--) {
			curNode.child[k] = 0;
		}
	}
	writeNodeAtOffset(file, curNode, curNodeOffset, order);
	return curNodeOffset;
}

int *getLeftArray(int leftCnt, int *auxArrOfKeys) {
	int *leftArr = new int[leftCnt];
	for (int a = 0; a < leftCnt; a++) {
		leftArr[a] = auxArrOfKeys[a];
	}
	return leftArr;
}

int *getRightArray(int median, int *auxArrOfKeys, int order) {
	int rightStart = median + 1;
	int rightCnt = order - rightStart;
	int *rightArr = new int[rightCnt];
	int rightIdx = 0;
	for (int b = rightStart; b < order; b++) {
		rightArr[rightIdx] = auxArrOfKeys[b];
		rightIdx++;
	}
	return rightArr;
}

int getIndexOfKey(int key, int *array, int length) {
	int a = 0;
	for (a = 0; a < length; a++) {
		if (array[a] == key) {
			break;
		}
	}
	return a;
}

void writeNodeAtOffset(fstream &file, btree_node node, long offset, int order) {
	file.seekg(offset, ios::beg);
	file.write(reinterpret_cast<char*> (&node.n), sizeof(int));
	file.write(reinterpret_cast<char*> (&*node.key), (order - 1)*sizeof(int));
	file.write(reinterpret_cast<char*> (&*node.child), (order)*sizeof(long));
}

long writeNodeGetOffset(fstream &file, btree_node node, int order) {
	file.seekg(0, ios::end);
	long offset = (long)file.tellg();
	file.write(reinterpret_cast<char*> (&node.n), sizeof(int));
	file.write(reinterpret_cast<char*> (&*node.key), (order - 1)*sizeof(int));
	file.write(reinterpret_cast<char*> (&*node.child), (order)*sizeof(long));
	return offset;
}

bool isKeyInNode(int key, btree_node node) {
	bool flag = false;
	int cnt = node.n;
	for (int i = 0; i < cnt; i++) {
		if (key == node.key[i]) {
			flag = true;
			break;
		}
	}
	return flag;
}


btree_node btree_search(fstream &file, long nodeOffset, int order, int targetKey) {
	btree_node node = getNodeAtOffset(file, nodeOffset, order);
	curNodeOffset = nodeOffset;
	int s = 0;
	for (s = 0; s < node.n; s++) {
		if (targetKey == node.key[s]) {
			return node;
		}
		else if (targetKey < node.key[s]) {
			break;
		}
	}
	if (node.child[s] != 0) {
		parentOffset = nodeOffset;
		return btree_search(file, node.child[s], order, targetKey);
	}
	else {
		return node;
	}
}

btree_node getNodeAtOffset(fstream &file, long nodeOffset, int order) {
	file.seekg(nodeOffset, ios::beg);
	btree_node node;
	file.read(reinterpret_cast<char*>(&node.n), sizeof(int));
	node.key = new int[order - 1];
	node.child = new long[order];
	for (int i = 0; i < (order - 1); i++) {
		file.read(reinterpret_cast<char*>(&node.key[i]), sizeof(int));
	}
	for (int j = 0; j < order; j++) {
		file.read(reinterpret_cast<char*>(&node.child[j]), sizeof(long));
	}
	return node;
}

void updateRootOffset(fstream &file, long offset) {
	file.seekg(0, ios::beg);
	file.write(reinterpret_cast<char*> (&offset), sizeof(long));
}

btree_node createNode(int order) {
	btree_node node;
	node.n = 0;
	node.key = new int[order - 1];
	for (int i = 0; i < (order - 1); i++) {
		node.key[i] = 0;
	}
	node.child = new long[order];
	for (int j = 0; j < order; j++) {
		node.child[j] = 0;
	}
	return node;
}

int *getSortedAuxArray(int auxSiz, btree_node curNode, int key) {
	int *auxArr = new int[auxSiz];
	for (int i = 0; i < (auxSiz - 1); i++) {
		auxArr[i] = curNode.key[i];
	}
	auxArr[auxSiz - 1] = key;
	qsort(auxArr, auxSiz, sizeof(int), qsortComp);
	return auxArr;
}

btree_node insertKey(btree_node node, int key, long leftOffset, long rightOffset, int order) {
	int cnt = node.n;
	if (cnt > 0) {
		cnt += 1;
		int *auxArr = getSortedAuxArray(cnt, node, key);
		for (int b = 0; b < cnt; b++) {
			node.key[b] = auxArr[b];
		}
		node.n = cnt;
		if (node.child[0] != 0 && leftOffset != -1) {
			int keyIdx = getIndexOfKey(key, auxArr, order);
			int lastChildIdx = cnt-1;
			for (int c = (lastChildIdx+1); c > (keyIdx+1); c--) {
				node.child[c] = node.child[c - 1];
			}
			node.child[keyIdx + 1] = rightOffset;
			node.child[keyIdx] = leftOffset;
		}
		delete[] auxArr;
	}
	else {
		node.key[cnt] = key;
		if (leftOffset != -1) node.child[cnt] = leftOffset;
		cnt += 1;
		node.n = cnt;
		if (rightOffset != -1) node.child[cnt] = rightOffset;
	}
	return node;
}

long getRootOffset(fstream &file) {
	long rootOffset = -1;
	file.seekg(0, ios::beg);
	file.read(reinterpret_cast<char*> (&rootOffset), sizeof(long));
	return rootOffset;
}

void findKey(fstream &file, int order,int key) {
	btree_node targetNode;
	bool isFound = false;
	long rootOffset = getRootOffset(file);
	if (rootOffset != -1) {
		targetNode = btree_search(file, rootOffset, order, key);
		isFound = isKeyInNode(key, targetNode);
	}
	if (isFound) {
		cout << "Entry with key=" << key << " exists" << endl;
	}
	else {
		cout << "Entry with key=" << key << " does not exist" << endl;
	}
	getUserCommand(file, order);
}

void printTree(fstream &file, int order) {
	int queueSiz = 1000;
	queue_node que = createQueue(queueSiz);
	long rootOffset = getRootOffset(file);
	que = enQueue(que, rootOffset);
	deQueueAndPrint(que, file, order, 1, 1, 0, true);
	delete[] que.queue;
	getUserCommand(file, order);
}

void closeandExit(fstream &file) {
	file.close();
	exit(0);
}

queue_node createQueue(int len) {
	queue_node que;
	que.queue = new long[len];
	que.front = 0;
	que.rear = -1;
	return que;
}

queue_node enQueue(queue_node que, int offset) {
	que.rear += 1;
	int rear = que.rear;
	que.queue[rear] = offset;
	return que;
}

void deQueueAndPrint(queue_node que, fstream &file, int order, int level, int nodeCnt, int nextLvlCnt, bool isNewLvl) {
	int front = que.front;
	int rear = que.rear;
	if (front <= rear) {
		int curOffset = que.queue[front];
		que.front += 1; 
		btree_node curNode = printKeysInNode(curOffset, level, file, order, isNewLvl, nodeCnt);
		for (int k = 0; k < order; k++) {
			long curOff = curNode.child[k];
			if (curOff != 0) {
				que = enQueue(que, curOff);
				nextLvlCnt++;
			}
		}

		nodeCnt -=1;
		if (nodeCnt == 0) { 
			level++; 
			nodeCnt = nextLvlCnt;
			nextLvlCnt = 0;
			isNewLvl = true;
		}
		else {
			isNewLvl = false;
		}
		deQueueAndPrint(que, file, order, level, nodeCnt, nextLvlCnt, isNewLvl);
	}
}

btree_node printKeysInNode(long offset, int lvl, fstream &file, int order, bool isNewLvl, int nodeCnt) {
	btree_node curNode = getNodeAtOffset(file, offset, order);
	int keyCnt = curNode.n;
	for (int a = 0; a < keyCnt; a++) {
		int curKey = curNode.key[a];
		if (isNewLvl) {
			if (a == 0) {
				cout << setfill(' ') << setw(2) << lvl << ": " << curKey;
			}
			else {
				cout << "," << curKey;
			}
		}
		else {
			if (a == 0) {
				cout << curKey;
			}
			else {
				cout << "," << curKey;
			}
		}
	}
	cout << " ";
	if(nodeCnt == 1)cout << endl;
	return curNode;
}

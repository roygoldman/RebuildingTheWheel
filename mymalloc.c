#include "mymalloc.h"


#define MEMBLOCK_SIZE (5000)

char memory_block[MEMBLOCK_SIZE];

void intiMemoryBlock(){
	static int first = 1;
	if(first){
		atexit(printStats);
		MemBlock* head = (MemBlock*) memory_block;
		head->prev = head;
		head->next = head;
		head->free = 1;
		head->size = MEMBLOCK_SIZE - sizeof(MemBlock);
		first--;
	}
}

void* memMalloc(unsigned int s, char * fileName, unsigned int lineNum){
	static int initial = 1;
	static MemBlock *head;
	MemBlock *curr, *new;

	if(initial){
		initial = 0;
		intiMemoryBlock();
		head = (MemBlock*)memory_block;
	}

	curr = head->prev;
	unsigned int newSize = s + sizeof(MemBlock);
	do{
		if(curr->free && curr->size == s){
			curr->free = 0;
			printf("malloced new block of size %d at %p\n",s,(((char*)curr) + sizeof(MemBlock)));
			return (void *)(((char*)curr) + sizeof(MemBlock));
		}else if(curr->free && curr->size >= newSize){
			curr->size -= newSize;
			new = (MemBlock*) (((char*)curr) + sizeof(MemBlock) + curr->size);
			new->next = curr->next;
			new->next->prev = new;
			curr->next = new;
			new->prev = curr;

			new->size = s;
			new->free = 0;
			printf("malloced new block of size %d at %p\n",s,(void *)((char*)new + sizeof(MemBlock)));
			return (void *)((char*)new + sizeof(MemBlock));
		}else
			curr = curr->prev;
	}while (curr != head->prev);
	printf("Saturation\n");
	return NULL;
	
}
void* memCalloc( unsigned int s,char * fileName, unsigned int lineNum){
	printf("Callocing block size %d...\n",s);
	void* ptr = malloc(s);
	if(ptr != NULL)
		memset(ptr, 0, s);
	else
		printf("NULL!!!\n");
	printf("%p\n",ptr);
	return ptr;
}

void* memRealloc(void* p, unsigned int s, char * fileName, unsigned int lineNum){
	
	static int initial = 1;
	if(initial){
		initial = 0;
		intiMemoryBlock();
	}
	
	printf("reallocing block %p...\n",p);
	if(p < (void*)&memory_block || p > (void*) &(memory_block[MEMBLOCK_SIZE])){
		printf("pointer %p was not in range\n",p);
		return NULL;
	}
	MemBlock* m = (MemBlock*)((char*)p - sizeof(MemBlock));
	unsigned int diff = s - m->size;
	if(diff <= 0)
		return p;
	else if(m->next != m && m->next != (MemBlock*)memory_block && m->next->free == 1 && m->next->size >= diff){

		MemBlock* next = m->next;
		MemBlock* new = (MemBlock*)(((char*) next) + diff);
		printf("extending block %p by %d...\n",p,diff);
		printf("next block at %p (%d)to be moved to %p\n",next,(int)next->size,new);
		MemBlock* nprev = next->prev;
		MemBlock* nnext = next->next;
		unsigned int nsize = next->size;
		int nfree = next->free;
		new->prev = nprev;
		new->next = nnext;
		new->size = nsize;
		new->free = nfree;

		new->next->prev = new;
		new->prev->next = new;
		new->size -= diff;
		m->size += diff;

		return p;
	}
	printf("moving block %p\n",p);
	void *ptr = malloc(s);
	memcpy(ptr, p, s);
	free(p);
	return ptr;
}

void memFree(void* p, char * fileName, unsigned int lineNum){

	static int initial = 1;
	static MemBlock *head;
	MemBlock *ptr, *prev;

	
	if(initial){
		initial = 0;
		intiMemoryBlock();
		head = (MemBlock*)memory_block;
	}
	
	if(p < (void*)&memory_block || p > (void*)&(memory_block[MEMBLOCK_SIZE])){
		printf("pointer %p was not in range\n",p);
		return;
	}
	ptr = (MemBlock*) ((char*)p - sizeof (MemBlock));
	
	if((void*) ptr->prev < (void*)&memory_block || (void*) ptr->prev > (void*)&(memory_block[MEMBLOCK_SIZE])){
		printf("Invalid Free of block at %p. Pointer not part of block\n",p);
		return;
	}
	
	if(ptr->prev != NULL && ptr->prev->next == ptr && !ptr->free){
		printf("Valid free, freeing block %p\n", p);
		prev = ptr->prev;
		ptr->next = ptr->next;
		ptr->free = 1;
		if(ptr->next != head && ptr->next->free && ptr->next != ptr){
			printf("Merging to the next block\n");
			ptr->size += sizeof(MemBlock) + ptr->next->size;
			ptr->next = ptr->next->next;
			ptr->next->prev = ptr;
		}

		if(ptr != head && prev->free && prev != ptr){
			printf("Merging with previous block\n");
			prev->size += sizeof(MemBlock) + ptr->size;
			prev->next = ptr->next;
			prev->next->prev = prev;
		}
	}else{
		printf("Invalid free or double free\n");
	}


}


void printStats(){
	static int initial = 1;
	MemBlock* head = (MemBlock*) memory_block;
	MemBlock* curr = head;
	MemBlock* next;
	int bytesFree = 0;
	int bytesUsed = 0;
	int blocksFree = 0;
	int blocksUsed = 0;
	
	if(initial){
		initial = 0;
		intiMemoryBlock();
	}
	
	do{
		if(!curr->free){
			bytesUsed += curr->size;
			blocksUsed++;
		}else{
			bytesFree += curr->size;
			blocksFree++;
		}
		
		next = curr->next;
		
		if(next == NULL){
			//data overwrite
			printf("Unable to check all data. Memory blocks lost do to curruption!\n");
			break;
		}
		
		curr= next;
		
	}while(curr != head);
	
	printf("Usage Stats\n");
	printf("\tFree Bytes:\t%d\n",bytesFree);
	printf("\tFree Blocks:\t%d\n",blocksFree);
	printf("\tUsed Bytes:\t%d\n",bytesUsed);
	printf("\tUsed Blocks:\t%d\n",blocksUsed);
	printf("\tData Lost:\t%d\n", (int)(MEMBLOCK_SIZE - bytesFree - (blocksUsed + blocksFree) * sizeof(MemBlock) - bytesUsed));
	
}

void printList(){
	static int initial = 1;
	MemBlock* head;
	MemBlock* curr;
	
	if(initial){
		initial = 0;
		intiMemoryBlock();
	}
	
	head = (MemBlock*) memory_block;
	curr = head;
	printf("\e[1;34mBlock List:\n\e[0m");
	do{
		printf("\e[1;34m\tBlock at %p of size %d with free status of % d (next %p)\n\e[0m",curr, (int) curr->size, curr->free, curr->next);
		curr = curr->next;
	}while(curr != head);

}

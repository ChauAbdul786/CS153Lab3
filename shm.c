#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct shm_page {
    uint id;
    char *frame;
    int refcnt;
  } shm_pages[64];
} shm_table;

void shminit() {
  int i;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
    shm_table.shm_pages[i].id =0;
    shm_table.shm_pages[i].frame =0;
    shm_table.shm_pages[i].refcnt =0;
  }
  release(&(shm_table.lock));
}

int shm_open(int id, char **pointer) {
  struct proc* process = myproc();
  int i = 0;
  uint size = PGROUNDUP(process->sz); //PGROUNDUP is xv6 specific for addresses

  acquire(&(shm_table.lock));

  int validID = 0;
  int index;
  for(i = 0; i < /*sizeof(shm_table.shm_pages) / sizeof(shm_table.shm_pages[0])*/ 64; i++){
    if(shm_table.shm_pages[i].id == id){
      validID = 1;
      index = i;
      break;
    }
  }

  if(validID == 1){
    mappages(process->pgdir, (char*)size, PGSIZE, V2P(shm_table.shm_pages[index].frame), PTE_W|PTE_U);
    shm_table.shm_pages[index].refcnt++;
  }else{
    for(i = 0; i < /*sizeof(shm_table.shm_pages) / sizeof(shm_table.shm_pages[0])*/ 64; i++){
      if(shm_table.shm_pages[i].id == 0){
	shm_table.shm_pages[i].id = id; //Assumption here (per lab session) is that there will always be an available ID
	shm_table.shm_pages[i].frame = kalloc();
	shm_table.shm_pages[i].refcnt++;
	memset(shm_table.shm_pages[i].frame, 0, PGSIZE);
	mappages(process->pgdir, (char*)size, PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W|PTE_U);
	break;
      }
    }
  }

  *pointer = (char*)size;
  process->sz += PGSIZE;

  release(&(shm_table.lock));

  return 0;
}


int shm_close(int id) {
  
  int i = 0;
  acquire(&(shm_table.lock));
  for(i = 0; i < /*sizeof(shm_table.shm_pages) / sizeof(shm_table.shm_pages[0])*/ 64; i++){
      if(shm_table.shm_pages[i].id == id){
	if(shm_table.shm_pages[i].refcnt > 0){
	  shm_table.shm_pages[i].refcnt--;
	}
	if(shm_table.shm_pages[i].refcnt == 0){
	  shm_table.shm_pages[i].id = 0;
	  shm_table.shm_pages[i].frame = 0;
	}
	break;
      }
  }

  release(&(shm_table.lock));

  return 0; 
}

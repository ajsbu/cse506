// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>
// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
	static void
pgfault(struct UTrapframe *utf)
{
	//cprintf("entering pgfault of fork with fault va = %0x, err = %d\n",  utf->utf_fault_va, utf->utf_err);

	//cprintf("reached in pgfault()\n");
	void *addr = (void *) utf->utf_fault_va;
	uint64_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at vpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.


	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 4: Your code here.

	//cprintf("err = %0x, addr = %0x, vpt[VPN(addr)] = %0x\n", err, addr, vpt[VPN(addr)]);

/*	if(vpt[VPN(addr)] == 0 || addr == 0)
	{
		cprintf("In \n");
		return;
	}*/
	if (((err&FEC_WR) && (vpt[VPN(addr)]&PTE_COW))) {
		if ((r = sys_page_alloc(0, PFTEMP, PTE_P|PTE_U|PTE_W)) < 0)
			panic("sys_page_alloc: %e", r);
		void *rounded = ROUNDDOWN(addr, PGSIZE);
		memmove(PFTEMP, rounded, PGSIZE);
		if ((r = sys_page_map(0, PFTEMP, 0, rounded, PTE_P|PTE_U|PTE_W)) < 0)
			panic("sys_page_map: %e", r);
		if ((r = sys_page_unmap(0, PFTEMP)) < 0)
			panic("sys_page_unmap: %e", r);

		return;
	}

	panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
	static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	uint64_t pte = vpt[pn];
	void *addr = (void *)((uint64_t)pn << PGSHIFT);

	int perm = pte & 0xfff;
	//cprintf("Reached....perm = %0x\n", perm);
	int perm_share = pte & PTE_USER;
	// resetting the dirty and access bits
	//perm &= ~PTE_A;
	//perm &= ~PTE_D;

	//cprintf("\npte: %x, perm=%x, PTE_SHARE=%x, perm & PTE_SHARE=%x", pte, perm, PTE_SHARE, perm & PTE_SHARE);
	if(perm_share & PTE_SHARE)
	{
		r = sys_page_map(0,addr, envid, addr, perm_share);

		if(r < 0) {
			//cprintf("error: %e\n", r);
			return r;

		}
		return 0;
	}
	if ((perm&PTE_W || perm&PTE_COW)) {
        //if (pte & (PTE_W|PTE_COW)) {
		perm &= ~PTE_W;
		perm |= PTE_COW;

		r = sys_page_map(0, addr, envid, addr, PTE_P|PTE_U|PTE_COW);
		if (r < 0)
			return r;
		return sys_page_map(0, addr, 0, addr, PTE_P|PTE_U|PTE_COW);
	}

	r = sys_page_map(0, addr, envid, addr, perm);


	return r;
}

/*
static int duppage(envid_t envid, unsigned pn)
{
        unsigned va;
        int r;
        pte_t pte = vpt[pn];
        int perm = pte & 0xfff;
        int perm_share = pte & PTE_USER;
       
        va = pn * PGSIZE;

        if (perm_share & PTE_SHARE)
                return sys_page_map(0, (void *)(uint64_t)va, envid, (void *)(uint64_t)va, perm_share);

        if (pte & (PTE_W|PTE_COW)) {
                //child
                if ((r = sys_page_map(0, (void *)(uint64_t)va, envid, (void *)(uint64_t)va, PTE_P|PTE_U|PTE_COW)) < 0)                        panic("sys_page_map failed: %e\n", r);                // parent                return sys_page_map(0, (void *)(uint64_t)va, 0, (void *)(uint64_t)va, PTE_P|PTE_U|PTE_COW);
        }

        return sys_page_map(0, (void *)(uint64_t)va, envid, (void *)(uint64_t)va, PTE_P|PTE_U|perm);
}

*/

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use vpd, vpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//

	envid_t
fork(void)
{
	int r;
        uint32_t err;
	extern unsigned char end[];
	// LAB 4: Your code here.
	set_pgfault_handler(pgfault);
	envid_t envid = sys_exofork();
	if (envid < 0)
		panic("sys_exofork: %e", envid);
	if (envid == 0) {
		//cprintf("_pgfault_handler in child = %0x\n", _pgfault_handler);
		//set_pgfault_handler(pgfault);
		thisenv = envs + ENVX(sys_getenvid());
		return 0;
	}

	// Parent
	//cprintf("_pgfault_handler in parent = %0x\n", _pgfault_handler);
	int page_table_number, pteno;
	uint32_t pn = 0;

	uint64_t va = 0;

	r = sys_page_alloc(envid, (void*) (UXSTACKTOP - PGSIZE), PTE_P|PTE_U|PTE_W);
        for (va = UTEXT; va < (USTACKTOP-PGSIZE); va += PGSIZE) {

		if (va == ( (UXSTACKTOP - PGSIZE))) {
			continue;
		}

                if((vpml4e[VPML4E(va)] & PTE_P) && (vpde[VPDPE(va)] & PTE_P) && (vpd[VPD(va)] & PTE_P) && (vpt[VPN(va)] & PTE_P)) {
			//cprintf("Reached in duppage()....va = %0x\n");
                        duppage(envid, VPN(va));
                } else if(va == (0xef7fe000)) {
			//cprintf("Reached here..in not duppage().....va = %0x\n", va);
		}
        }

	r = duppage(envid, VPN(USTACKTOP - PGSIZE));

	/*
        //Allocate a new stack for the child and copy the contents of parent on to it.
        uint64_t addr = USTACKTOP-PGSIZE;
        if ((r = sys_page_alloc(0, (void *)PFTEMP, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_alloc failed: %e\n", r);
	memcpy(PFTEMP, (void *) ROUNDDOWN(addr, PGSIZE), PGSIZE);
        void *vaTemp = (void *) ROUNDDOWN(addr, PGSIZE);
        if ((r = sys_page_map(0, (void *)PFTEMP, envid, vaTemp, PTE_P|PTE_U|PTE_W)) < 0)
                panic("sys_page_map failed: %e\n", r);
        if ((r = sys_page_unmap(0, (void *)PFTEMP)) < 0)
		panic("sys_page_unmap failed: %e\n", r);

	if (r)
		panic("sys_page_alloc: %e", r);
	*/

	//cprintf("Reached...setting page fault handler()\n");
	sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall);
	//cprintf("Reached...set page fault handler()\n");
	// Mark the child as runnable and return.
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);


	//cprintf("done with fork()\n");
	return envid;
	panic("fork not implemented");	

}

/*
envid_t
fork(void){
        // LAB 4: Your code here.
        envid_t envid;
        uint64_t addr;
        uint32_t err;
        extern unsigned char end[];
        int r;
        set_pgfault_handler(pgfault);
        envid = sys_exofork();
        if (envid < 0)
                panic("sys_exofork: %e", envid);
        if (envid == 0) {
                // We're the child.
                // The copied value of the global variable 'thisenv'
                // is no longer valid (it refers to the parent!).
                // Fix it and return 0.
                thisenv = &envs[ENVX(sys_getenvid())];
                return 0;
        }

        //Allocate exception stack for the child
        if ((err = sys_page_alloc(envid, (void *) (UXSTACKTOP - PGSIZE), PTE_P|PTE_U|PTE_W)) < 0)
                panic("Error in sys_page_alloc: %e", err);
        // We're the parent.
        // Map our entire address space into the child.
*/
/*
        for (addr = UTEXT; addr < USTACKTOP-PGSIZE; addr += PGSIZE) {
               if((vpml4e[VPML4E(addr)] & PTE_P) && (vpde[VPDPE(addr)] & PTE_P)
                        && (vpd[VPD(addr)] & PTE_P) && (vpt[VPN(addr)] & PTE_P)) {

                       duppage(envid, VPN(addr));
                }
        }
        //Allocate a new stack for the child and copy the contents of parent on to it.
        addr = USTACKTOP-PGSIZE;
        if ((r = sys_page_alloc(0, (void *)PFTEMP, PTE_P|PTE_U|PTE_W)) < 0)
                panic("sys_page_alloc failed: %e\n", r);
        memcpy(PFTEMP, (void *) ROUNDDOWN(addr, PGSIZE), PGSIZE);
        void *vaTemp = (void *) ROUNDDOWN(addr, PGSIZE);
        if ((r = sys_page_map(0, (void *)PFTEMP, envid, vaTemp, PTE_P|PTE_U|PTE_W)) < 0)
                panic("sys_page_map failed: %e\n", r);
        if ((r = sys_page_unmap(0, (void *)PFTEMP)) < 0)
                panic("sys_page_unmap failed: %e\n", r);

        //Set child's page fault handler
        if ((err = sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall) < 0))
                panic("Error in sys_env_set_pgfault_upcall: %e",err);

        //Set the child ready to run
        if ((err = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
                panic("sys_env_set_status: %e", err);

        return envid;
}
*/

// Challenge!
//Challenge Problem Lab 4: sfork
/*	int
sfork(void)
{
	 int r;
        uint32_t err;
        extern unsigned char end[];
        // LAB 4: Your code here.
        set_pgfault_handler(pgfault);
        envid_t envid = sys_exofork();
        if (envid < 0)
                panic("sys_exofork: %e", envid);
        if (envid == 0) {
		thisenv = envs + ENVX(sys_getenvid());
		return 0;
	}

	int page_table_number, pteno;
	uint32_t pn = 0;

	uint64_t va = 0;

	r = sys_page_alloc(envid, (void*) (UXSTACKTOP - PGSIZE), PTE_P|PTE_U|PTE_W);

	for (va = UTEXT; va < UTOP; va += PGSIZE) {

		if (va == ( (UXSTACKTOP - PGSIZE))) {
			continue;
		}
		pn = VPN(va);
		// For STACKTOP do duppage
		if(pn == VPN(USTACKTOP) - 1) {
			r = duppage(envid, pn);
			if (r)
				panic("duppage: %e", r);
		}
		else {	
			void *addr = (void *)((uint64_t)pn << PGSHIFT);
			int perm = vpt[pn] & PTE_USER;
			perm |= PTE_SHARE;
			r = sys_page_map(0, addr, 0, addr, perm);
			if (r)
				panic("sys_page_map: %e", r);
			r = sys_page_map(0, addr, envid, addr, perm);
			if (r)
				panic("sys_page_map: %e", r);
		}
	}

	//Allocate a new stack for the child and copy the contents of parent on to it.
	uint64_t addr = USTACKTOP-PGSIZE;

	if ((r = sys_page_alloc(0, (void *)PFTEMP, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_alloc failed: %e\n", r);
	memcpy(PFTEMP, (void *) ROUNDDOWN(addr, PGSIZE), PGSIZE);
	void *vaTemp = (void *) ROUNDDOWN(addr, PGSIZE);
	if ((r = sys_page_map(0, (void *)PFTEMP, envid, vaTemp, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_map failed: %e\n", r);
	if ((r = sys_page_unmap(0, (void *)PFTEMP)) < 0)
		panic("sys_page_unmap failed: %e\n", r);

	if (r)
		panic("sys_page_alloc: %e", r);


	sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall);
	// Mark the child as runnable and return.
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);


	//cprintf("done with fork()\n");
	return envid;

	panic("sfork not implemented");
	return -E_INVAL;
}*/

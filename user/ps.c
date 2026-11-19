#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/pinfo.h"
#include "user/user.h"

// Define the enum here so that the compiler recognizes the state names
enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Define static array to avoid stack overflow
static struct pinfo procs[NPROC];

// Map enum procstate to strings for readable output
static char *states[] = {
    [UNUSED]   "UNUSED",
    [USED]     "USED",
    [SLEEPING] "SLEEPING",
    [RUNNABLE] "RUNNABLE",
    [RUNNING]  "RUNNING",
    [ZOMBIE]   "ZOMBIE"
};

int
main(int argc, char *argv[])
{
    // Call the custom syscall
    int n = getprocs(procs, NPROC);
    
    if(n < 0){
        fprintf(2, "ps: getprocs failed\n");
        exit(1);
    }

    // Print header
    printf("PID\tPPID\tSTATE\t\tSIZE\tNAME\n");
    
    // Iterate through process information
    for(int i = 0; i < n; i++){
        // Skip unused processes if necessary
        if(procs[i].state == UNUSED) continue;
        
        printf("%d\t%d\t%s\t%d\t%s\n", 
               procs[i].pid, 
               procs[i].ppid, 
               states[procs[i].state], 
               (int)procs[i].sz, 
               procs[i].name);
    }
    
    exit(0);
}

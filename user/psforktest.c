#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/pinfo.h"
#include "user/user.h"

#define NKIDS 4

// Keep these values in sync with enum procstate in kernel/proc.h.
enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

static struct pinfo procs[NPROC];

static const char *const states[] = {
  [UNUSED]   = "UNUSED",
  [USED]     = "USED",
  [SLEEPING] = "SLEEPING",
  [RUNNABLE] = "RUNNABLE",
  [RUNNING]  = "RUNNING",
  [ZOMBIE]   = "ZOMBIE",
};

static const char *
state_name(int state)
{
  // Check the index before looking up the state name.
  if(state < 0 || state >= (int)(sizeof(states) / sizeof(states[0])))
    return "UNKNOWN";

  return states[state];
}

static int
show(int max)
{
  int n = getprocs(procs, max);

  printf("getprocs(max=%d) -> %d\n", max, n);
  if(n < 0 || n > NPROC || n > max){
    fprintf(2, "unexpected result for max=%d: %d\n", max, n);
    return -1;
  }

  for(int i = 0; i < n; i++){
    printf("  pid=%d ppid=%d state=%s name=%s\n",
           procs[i].pid, procs[i].ppid,
           state_name(procs[i].state), procs[i].name);
  }

  return 0;
}

static int
expect_result(const char *label, int actual, int expected)
{
  printf("%s: got %d, expected %d\n", label, actual, expected);
  return actual != expected;
}

int
main(void)
{
  int kids[NKIDS];
  int nkids = 0;
  int failed = 0;

  for(int i = 0; i < NKIDS; i++){
    int pid = fork();

    if(pid < 0){
      fprintf(2, "fork failed for child %d\n", i);
      failed = 1;
      break;
    }

    if(pid == 0){
      if(i % 2 == 0){
        // Sleep, then exit instead of continuing the parent's loop.
        pause(300);
        exit(0);
      }

      // Keep this child runnable until the parent terminates it.
      for(;;){
      }
    }

    // Only the parent stores successfully created child PIDs.
    kids[nkids++] = pid;
  }

  if(!failed){
    // Give the children a chance to run before taking the snapshots.
    pause(10);

    printf("parent pid = %d\n", getpid());

    failed |= (show(NPROC) < 0);
    failed |= (show(2) < 0);
    failed |= (show(NPROC + 1) < 0);

    failed |= expect_result("zero limit", getprocs(procs, 0), 0);
    failed |= expect_result("negative limit", getprocs(procs, -1), -1);

    // Use an address outside the xv6-riscv user address range.
    failed |= expect_result("invalid address",
                            getprocs((struct pinfo *)(1ULL << 63), 1), -1);
  }

  // Signal all created children before waiting for any of them.
  for(int i = 0; i < nkids; i++)
    kill(kids[i]);

  // Reap only the children that were successfully created.
  for(int i = 0; i < nkids; i++){
    if(wait(0) < 0){
      fprintf(2, "wait failed\n");
      failed = 1;
      break;
    }
  }

  printf("getprocstest: %s\n", failed ? "FAIL" : "PASS");
  exit(failed ? 1 : 0);
}

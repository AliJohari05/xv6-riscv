#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
extern struct proc proc[];
// پیاده‌سازی تابع زمان‌بند CFS
void
cfs_scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();

  c->proc = 0;
  for(;;){
    // فعال‌سازی وقفه برای جلوگیری از قفل شدن سیستم
    intr_on();

    struct proc *selected = 0;
    uint64 min_vruntime = (uint64)-1; // مقدار اولیه بسیار بزرگ

    // جستجوی فرآیند RUNNABLE با کمترین vruntime (عادلانه‌ترین انتخاب)
    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        if(selected == 0 || p->vruntime < min_vruntime) {
          if(selected)
            release(&selected->lock);
          selected = p;
          min_vruntime = p->vruntime;
          continue; // حفظ قفل فرآیند انتخاب شده
        }
      }
      release(&p->lock);
    }

    // اگر فرآیندی پیدا شد، آن را اجرا می‌کنیم
    if(selected) {
      selected->state = RUNNING;
      c->proc = selected;
      
      swtch(&c->context, &selected->context);

      // پس از بازگشت از اجرا
      c->proc = 0;
      release(&selected->lock);
    }
  }
}
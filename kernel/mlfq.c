#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
extern struct proc proc[];
// پیاده‌سازی تابع زمان‌بند MLFQ
void
mlfq_scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();

  c->proc = 0;
  for(;;){
    // فعال‌سازی وقفه برای جلوگیری از قفل شدن هسته
    intr_on();

    // 1. مکانیزم Aging: بررسی فرآیندهای RUNNABLE که مدت زیادی انتظار کشیده‌اند
    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE && p->queue > 1) {
        if(p->wait_ticks >= AGING_LIMIT) {
          p->queue--;        // ارتقا به صف بالاتر (اولویت بیشتر)
          p->wait_ticks = 0; // ریست زمان انتظار
          p->queue_enter = ticks;
        }
      }
      release(&p->lock);
    }

    // 2. جستجوی فرآیند با بالاترین اولویت (کوچک‌ترین شماره صف: 1، 2 یا 3)
    struct proc *selected = 0;
    int best_queue = 4; // مقدار اولیه بیشتر از تعداد صف‌ها

    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        int effective_q = p->queue;
        // اگر شرایط Aging برقرار است، در انتخاب اولویت بالاتر در نظر گرفته شود
        if(p->wait_ticks >= AGING_LIMIT && effective_q > 1)
          effective_q--;

        if(effective_q < best_queue) {
          if(selected)
            release(&selected->lock);
          selected = p;
          best_queue = effective_q;
          continue; // قفل فرآیند انتخاب شده را حفظ می‌کنیم
        }
      }
      release(&p->lock);
    }

    // 3. اجرای فرآیند انتخاب شده
    if(selected) {
      selected->queue = best_queue;
      selected->state = RUNNING;
      selected->quantum_ticks = 0; // ریست تیک‌های مصرفی کوانتوم
      selected->wait_ticks = 0;    // ریست انتظار
      c->proc = selected;
      
      swtch(&c->context, &selected->context);

      // پس از بازگشت از پردازش (توسط yield یا پایان کار)
      c->proc = 0;
      release(&selected->lock);
    }
  }
}
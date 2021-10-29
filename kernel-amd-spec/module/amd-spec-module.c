#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <asm/msr.h>

MODULE_LICENSE("GPL");

/*
 * The 'lfence' instruction may not embody an speculation barrier in AMD
 * processors. This can be verified by reading the 'MSR_LFENCE_NOSPEC' MSR.
 *
 * References:
 * https://developer.amd.com/wp-content/resources/Managing-Speculation-on-AMD-Processors.pdf
 */
#define MSR_LFENCE_NOSPEC 0xC0011029

/*
 * allow_speculation
 * -----------------
 * Disable the no-speculation feature of the 'lfence' instruction.
 */
static void allow_speculation(void *info)
{
  uint64_t data;
  rdmsrl(MSR_LFENCE_NOSPEC, data);
  data &= ~0x2UL;
  wrmsrl(MSR_LFENCE_NOSPEC, data);
}

/*
 * prevent_speculation
 * -------------------
 * Check if 'lfence' is a speculation barrier, if not, enable it. In newer OS
 * versions, this is the default behavior to prevent Spectre attacks.
 *
 * - Check the corresponding MSR (MSR_LFENCE_NOSPEC).
 * - If the MSR_LFENCE_NOSPEC[01] bit is not set, set it.
 */
static void prevent_speculation(void *info)
{
  uint64_t data;
  rdmsrl(MSR_LFENCE_NOSPEC, data);
  if (!(data & 0x2UL)) {
    data |= 0x2UL;
    wrmsrl(MSR_LFENCE_NOSPEC, data);
  }
}

static int __init spec_init(void)
{
  on_each_cpu(allow_speculation, NULL, 1);
  return 0;
}

static void __exit spec_exit(void)
{
  on_each_cpu(prevent_speculation, NULL, 1);
}

module_init(spec_init);
module_exit(spec_exit);

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>

/*************** Sysfs functions **********************/
static ssize_t  sysfs_show(struct kobject *kobj, 
                        struct kobj_attribute *attr, char *buf);
static ssize_t  sysfs_store(struct kobject *kobj, 
                        struct kobj_attribute *attr,const char *buf, size_t count);

static ulong watch_address = 0xAAAAAAAA;

struct kobject *sysfs_ref;
struct kobj_attribute watch_address_attr = __ATTR(watch_address, 0660, sysfs_show, sysfs_store);

MODULE_AUTHOR("Oleh Polisan");
MODULE_DESCRIPTION("Memory watchpoint module");
MODULE_LICENSE("GPL");

module_param(watch_address, ulong, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(watch_address, "Physical address to watch");

/*************** Breakpoint functions **********************/
static struct perf_event **rw_hardware_breakpoint, **w_hardware_breakpoint;
static void bp_rw_handler(struct perf_event* bp, struct perf_sample_data* data, struct pt_regs* regs) {
  pr_info("read/write bp_handler - begin\n");
  struct hw_perf_event hw = bp->hw;

  if (hw.interrupts == HW_BREAKPOINT_W) {
    pr_info("rw handler: Write. Skipping.\n");
  } else if (hw.interrupts == HW_BREAKPOINT_R) {
    pr_info("rw handler: Read\n");
    dump_stack();
  } else {
    pr_info("unknown\n");
  }

  pr_info("read/write bp_handler - end\n");
}

static void bp_w_handler(struct perf_event* bp, struct perf_sample_data* data, struct pt_regs* regs) {
  pr_info("write bp_handler - begin\n");
  dump_stack();
  pr_info("write bp_handler - end\n");
}

/*
** This function will be called when we read the sysfs file
*/
static ssize_t sysfs_show(struct kobject *kobj, 
                struct kobj_attribute *attr, char *buf)
{
        pr_info("Sysfs - Read!!!\n");
        return sprintf(buf, "0x%lx", (long unsigned int)watch_address);
}
/*
** This function will be called when we write the sysfsfs file
*/
static ssize_t sysfs_store(struct kobject *kobj, 
                struct kobj_attribute *attr,const char *buf, size_t count)
{
        pr_info("Sysfs - Write!!!\n");
        sscanf(buf, "0x%lx", &watch_address);
        pr_info("New watchpoint address: 0x%lx\n", (unsigned long)watch_address);

        //creating breakpoints
        struct perf_event_attr rwbp_attr, wbp_attr;

        if (rw_hardware_breakpoint)
          unregister_wide_hw_breakpoint(rw_hardware_breakpoint);
        if (w_hardware_breakpoint)
          unregister_wide_hw_breakpoint(w_hardware_breakpoint);
        hw_breakpoint_init(&rwbp_attr);
        hw_breakpoint_init(&wbp_attr);
        rwbp_attr.bp_addr = watch_address;
        rwbp_attr.bp_len = HW_BREAKPOINT_LEN_1;
        rwbp_attr.bp_type = HW_BREAKPOINT_W | HW_BREAKPOINT_R; //R only = invalid parameters (x86 supports only rw (11) or w (01) debug resisters setup)
        
        wbp_attr.bp_addr = watch_address;
        wbp_attr.bp_len = HW_BREAKPOINT_LEN_1;
        wbp_attr.bp_type = HW_BREAKPOINT_W;

        rw_hardware_breakpoint = register_wide_hw_breakpoint(&rwbp_attr, bp_rw_handler, NULL);
        if(IS_ERR(rw_hardware_breakpoint)) {
          pr_err("Read/Write Breakpoint registration failed.\n");
          return PTR_ERR(rw_hardware_breakpoint);
        }
        
        w_hardware_breakpoint = register_wide_hw_breakpoint(&wbp_attr, bp_w_handler, NULL);
        if(IS_ERR(w_hardware_breakpoint)) {
          pr_err("Write Breakpoint registration failed.\n");
          return PTR_ERR(w_hardware_breakpoint);
        }

        pr_info("Created new hardware breakpoint.\n");
        return count;
}

static int __init custom_init(void) {
  sysfs_remove_file(kernel_kobj, &watch_address_attr.attr);
  printk(KERN_INFO "Watchpoint address: 0x%lx\n", (long unsigned int)watch_address);
  sysfs_ref = kobject_create_and_add("watch_address", kernel_kobj);
  if(sysfs_create_file(sysfs_ref, &watch_address_attr.attr)) {
    printk(KERN_ERR "Cannot create sysfs file.\n");
    return -1;
  }
 
  //creating breakpoints
  struct perf_event_attr rwbp_attr, wbp_attr;

  if (rw_hardware_breakpoint)
    unregister_wide_hw_breakpoint(rw_hardware_breakpoint);
  if (w_hardware_breakpoint)
    unregister_wide_hw_breakpoint(w_hardware_breakpoint);
  hw_breakpoint_init(&rwbp_attr);
  hw_breakpoint_init(&wbp_attr);
  rwbp_attr.bp_addr = watch_address;
  rwbp_attr.bp_len = HW_BREAKPOINT_LEN_1;
  rwbp_attr.bp_type = HW_BREAKPOINT_W | HW_BREAKPOINT_R; //R only = invalid parameters (x86 supports only rw (11) or w (01) debug resisters setup)

  wbp_attr.bp_addr = watch_address;
  wbp_attr.bp_len = HW_BREAKPOINT_LEN_1;
  wbp_attr.bp_type = HW_BREAKPOINT_W;

  rw_hardware_breakpoint = register_wide_hw_breakpoint(&rwbp_attr, bp_rw_handler, NULL);
  if(IS_ERR(rw_hardware_breakpoint)) {
    pr_err("Read/Write Breakpoint registration failed.\n");
    return PTR_ERR(rw_hardware_breakpoint);
  }

  w_hardware_breakpoint = register_wide_hw_breakpoint(&wbp_attr, bp_w_handler, NULL);
  if (IS_ERR(w_hardware_breakpoint)) {
    pr_err("Write Breakpoint registration failed.\n");
    return PTR_ERR(w_hardware_breakpoint);
  }

  pr_info("Created new hardware breakpoints.\n");
  return 0;
}

static void __exit custom_exit(void) {
  printk(KERN_INFO "Shutting down kernel module...\n");
  if(rw_hardware_breakpoint)
    unregister_wide_hw_breakpoint(rw_hardware_breakpoint);
  if(w_hardware_breakpoint)
    unregister_wide_hw_breakpoint(w_hardware_breakpoint);
  sysfs_remove_file(kernel_kobj, &watch_address_attr.attr);
}


module_init(custom_init);
module_exit(custom_exit);

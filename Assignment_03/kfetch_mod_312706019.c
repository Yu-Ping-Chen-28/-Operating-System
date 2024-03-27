#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/device.h> 
#include <linux/cdev.h> 
#include <linux/delay.h> 
#include <linux/printk.h> 
#include <linux/uaccess.h> /* for get_user and put_user */
#include <linux/ktime.h> /* for uptime timespec64 & ktime_get_boottime_ts64 */
#include <linux/cpumask.h>
#include<linux/sched.h>
#include<linux/sched/signal.h>
#include <linux/utsname.h>
#include <linux/mm.h>
#include <linux/cpu.h>
#include <asm/cpu.h>
#include <linux/cpumask.h>  /* for num_online_cpus */
#include "kfetch.h"


// Module metadata
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("kfetch_mod");

#define PAGE_SIZE 2048
static int major_num; /* major number assigned to our device driver */ 
static char kfetch_buf[PAGE_SIZE];  // Buffer for storing mask information
//static char msg[BUF_LEN + 1]; /* The msg the device will give when asked */ 
static int info_mask; // control which information should be displayed.
// inforamation set
static char Kernel[50];
static char CPU_name[50];
static char CPU_core[30];
static char Mem[30];
static char Procs[30];
static char Run_time[30];

//enum before CDEV_NOT_USED 
enum { 
    CDEV_NOT_USED = 0, 
    CDEV_EXCLUSIVE_OPEN = 1, 
};

/* Is device open? Used to prevent multiple access to device */ 
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED); 

static struct class *cls; 
//status of the device


//#define SUCCESS 0 
#define DEVICE_NAME "kfetch" /* Dev name as it appears in /proc/devices   */ 
//#define BUF_LEN 80 /* Max length of the message from the device */ 
#define MAXC 2048


// The CPU model name
// static void get_cpu_model(char *buf) {
//     FILE *fp = fopen("/proc/cpuinfo", "r");  /* open CPU info file */

//     if (!fp) {
//         perror("file open failed");
//         return;
//     }

//     while (fgets(buf, MAXC, fp)) {                 /* read each line */
//         const char *cmpstr = "model name";         /* compare string */
//         size_t cmplen = strlen(cmpstr);            /* length to compare */
//         if (strncmp(buf, cmpstr, cmplen) == 0) {   /* does start match? */
//             /* Extract CPU model information using snprintf */
//             snprintf(buf, MAXC, "CPU: %s", buf + cmplen + 2);
//             break;  /* exit loop after finding the first occurrence */
//         }
//     }

//     fclose(fp);      
// }


//The number of processes
static int get_thread_count(void) {
    int thread_count = 0;
    struct task_struct *task;

    for_each_process(task) {
        thread_count += get_nr_threads(task);
    }
    return thread_count;
}

static unsigned long format_uptime(void) {
    struct timespec64 uptime;
    ktime_get_boottime_ts64(&uptime);

    // Convert to minutes
    unsigned long uptime_minutes = uptime.tv_sec / 60;

    return uptime_minutes;
}


// The open and release operations should set up and clean up protections properly.
static int kfetch_open(struct inode *inode, struct file *file)
{
    /* setting up protections */
    if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN)) 
    return -EBUSY; 
 
    pr_info("kfetch_open\n",file); 
    try_module_get(THIS_MODULE); 
    return 0;
}

static int kfetch_release(struct inode *inode, struct file *file)
{
    /* cleaning up protections */
    /* ready for our next caller. */ 
    atomic_set(&already_open, CDEV_NOT_USED); 
    //decrement the reference count of the kernel module
    module_put(THIS_MODULE); 
    return 0;
}
static ssize_t kfetch_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{

    struct new_utsname *uts = utsname();
    int length_read = 0;
    char *hostname = uts->nodename;
    char separator_line[30];
    struct sysinfo si;
    unsigned long totalram, freeram;
    int count=0;
    int flag;
    int temp;
  
    /* A logo of your choosing */
    char logo0[] = "                   ";
    char logo1[] = "        .-.        ";
    char logo2[] = "       (.. |       ";
    char logo3[] = "       <>  |       ";
    char logo4[] = "      / --- \\      ";
    char logo5[] = "     ( |   | |     ";
    char logo6[] = "   |\\\\_)___/\\)/\\   ";
    char logo7[] = "  <__)------(__/   ";

    /* Information */
    /* first line is the machine hostname, which is mandatory and cannot be disabled*/
    length_read += snprintf(kfetch_buf + length_read, PAGE_SIZE - length_read, "%s%s\n", logo0, hostname);
    /* Separator line with a length equal to the hostname */
    memset(separator_line,'-',(strlen(hostname)));  
    separator_line[strlen(hostname)]='\0';
    length_read +=  snprintf(kfetch_buf + length_read, PAGE_SIZE - length_read, "%s%s\n", logo1, separator_line);
    //Kernel: The kernel release
    snprintf(Kernel, sizeof(Kernel), "Kernel:   %s\n",  uts->release);
    //CPU: The CPU model name
    snprintf(CPU_name,sizeof(CPU_core), "CPU:      %s\n",  uts->machine);
    //char *cpu_model = get_cpu_model();
    //length_read += snprintf(kfetch_buf + length_read, PAGE_SIZE - length_read, "%s%s\n", logo3, cpu_model);
    //CPUs: The number of CPU cores, in the format <# of online CPUs> / <# of total CPUs>
    snprintf(CPU_core,sizeof(CPU_name),"CPUs:     %d / %d\n",num_online_cpus(),num_active_cpus());   //cpu cores

    //Mem: The memory information, in the format<free memory> / <total memory> (in MB)
    si_meminfo(&si);
    totalram = (si.totalram * si.mem_unit) >> 20;
    freeram = (si.freeram * si.mem_unit) >> 20;
    snprintf(Mem,sizeof(Mem), "Mem:      %lu / %lu MB\n", freeram, totalram);
    //Procs: The number of processes
    int num_procs = get_thread_count();
    snprintf(Procs,sizeof(Procs), "Procs:    %d\n",  num_procs);
    
    //Uptime: How long the system has been running, in minutes.
    //lu: unsigned long
    unsigned long system_uptime = format_uptime();
    snprintf(Run_time,sizeof(Run_time),  "Uptime:   %lu mins\n",system_uptime );

    //length_read += snprintf(kfetch_buf + length_read, PAGE_SIZE - length_read, "%s%s\n", logo7, uts->nodename);


    struct InfoEntry {
        const char *logo;
        const char *info;
    };
    struct InfoEntry info_entries[] = {
        {logo2, Kernel},
        {logo3, CPU_core},
        {logo4, CPU_name},
        {logo5, Mem},
        {logo6, Run_time},
        {logo7, Procs},
    };

    if (info_mask==0){
        for (int i = 0; i < 6; i++) {
            length_read += snprintf(kfetch_buf + length_read, PAGE_SIZE - length_read, info_entries[i].logo);
            length_read += snprintf(kfetch_buf + length_read, PAGE_SIZE - length_read, "%s",info_entries[i].info);
        }
    }
    else{
        for (int i = 0; i < 6; i++) {
                Again:
                    flag=1<<count;
                    temp=flag & info_mask;
                    if(temp!=0){
                        // output logo
                        length_read += snprintf(kfetch_buf + length_read, PAGE_SIZE - length_read, info_entries[i].logo);
                        // output information
                        length_read += snprintf(kfetch_buf + length_read, PAGE_SIZE - length_read, "%s",info_entries[count].info);
                        count++;
                    }
                    else if (temp == 0){
                        if(count < 6){
                            count++;
                            goto Again; 
                        }
                        else if(count==6){
                            // output logo
                            length_read += snprintf(kfetch_buf + length_read, PAGE_SIZE - length_read, info_entries[i].logo);
                            length_read += snprintf(kfetch_buf + length_read, PAGE_SIZE - length_read, "\n");
                        }
                    }
        }
    }

    ssize_t total_length = length_read;

    // copy_to_user: copy data from kernel space to user space  
    if (copy_to_user(buffer, kfetch_buf, total_length)) {
        pr_alert("Failed to copy data to user\n");
        return -EFAULT;
    }

    return total_length;

}
static ssize_t kfetch_write(struct file *filp, const char __user *buffer, size_t length, loff_t *offset)
{
   int mask_info;
    if (copy_from_user(&mask_info, buffer, length)) {
        pr_alert("Failed to copy data from user\n");
        return 0;
    }
    printk("kfetch_mod: write failed\n");


    /* setting the information mask */
    info_mask = mask_info;



    return 0; 
}



// Device operations
const static struct file_operations kfetch_ops = {
    .owner   = THIS_MODULE,
    .read    = kfetch_read,
    .write   = kfetch_write,
    .open    = kfetch_open,
    .release = kfetch_release,
};

// kfetch init and exit methods
static int __init kfetch_init(void) {
    major_num = register_chrdev(0, DEVICE_NAME, &kfetch_ops);

    if (major_num < 0) {
        pr_alert("Registering char device failed with %d\n", major_num); 
        //printk(KERN_ALERT "kfetch_mod failed to register a major number\n");            return major_num;
        }
    pr_info("Assigned major number %d.\n", major_num); 
    //printk(KERN_INFO "kfetch_mod: registered correctly with major number %d\n", major_num);
    
    //cls = class_create(DEVICE_NAME); 
    cls = class_create(THIS_MODULE, DEVICE_NAME); 
    device_create(cls, NULL, MKDEV(major_num, 0), NULL, DEVICE_NAME); 
    pr_info("Device created on /dev/%s\n", DEVICE_NAME); 
    return 0; 
}

static void __exit kfetch_exit(void) {
    device_destroy(cls, MKDEV(major_num, 0));
    class_destroy(cls);
    /* Unregister the device */ 
    unregister_chrdev(major_num, DEVICE_NAME);
}

module_init(kfetch_init);
module_exit(kfetch_exit);
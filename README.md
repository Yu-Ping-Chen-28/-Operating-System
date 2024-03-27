# Operating-System
NYCU-Institute of Computer Science and Engineering-CSIC30015

## course overview
This course is an advanced version of the undergraduate course "Introduction to Operating System". Generally, most topics are the same as those of the undergraduate counterpart, but we will dive deeper to explore how are those topics implemented in Linux and what is the trend or challenge for each topic. For each topic, we will revisit key concepts provided in the undergraduate OS, and then move to advanced topics, mostly from selected papers. We will also have some hands-on projects on Linux, such as playing with kernel system call and CPU scheduling.


## homework_1 Compiling Linux Kernel and Adding Custom System Calls
[Compiling Linux Kernel and Adding Custom System Calls Spec](https://hackmd.io/@Bmch4MS0Rz-VZWB74huCvw/B1b2S_Kl6)
1. Compiling Linux Kernel
  a. Change kernel suffix
2. Adding Custom System Calls
  a. sys_hello
  b. sys_revstr

## homework_2 Scheduling Policy Demonstration Program
[Scheduling Policy Demonstration Program](https://hackmd.io/@Bmch4MS0Rz-VZWB74huCvw/rJ8OLx6fp)
* The scheduling polices can be divided into four categories:
* Fair scheduing policies
  * `SCHED_NORMAL` (CFS, `SCHED_OTHER` in POSIX), `SCHED_BATCH`
* Real-Time scheduing policies
  * `SCHED_FIFO`, `SCHED_RR`
* The other two are idle scheduling policy (`SCHED_IDLE`) and deadline scheduling policy (`SCHED_DEADLINE`)
1. In this assignment, you are required to implement a program called sched_demo, which lets a user run multiple threads with different scheduling policies and show the working status of each thread.
-n <num_threads>: number of threads to run simultaneously
-t <time_wait>: duration of "busy" period
-s <policies>: scheduling policy for each thread, SCHED_FIFO or SCHED_NORMAL.
The example NORMAL,FIFO,NROMAL,FIFO shown above means to apply SCHED_NORMAL policy to the 1st and 3rd thread and SCHED_FIFO policy to the 2nd and 4nd thread.
-p <priorities>: real-time thread priority for real-time threads
The example -1,10,-1,30 shown above means to set the value 10 to the 2nd thread and value 30 to the 4nd thread.
You should specify the value -1 for threads with SCHED_NORMAL policy.
example command: -n 4 -t 0.5 -s NORMAL,FIFO,NORMAL,FIFO -p -1,10,-1,30
![image](https://github.com/Yu-Ping-Chen-28/-Operating-System/assets/165137119/7686ba02-9a70-4d53-876c-e3bb469ccef4)


## homework_3 System Information Fetching Kernel Module
[System Information Fetching Kernel Module](https://hackmd.io/@a3020008/r1Txj5ES6)
In this assignment, you are going to implement a kernel module that fetches the system information from the kernel.
![image](https://github.com/Yu-Ping-Chen-28/-Operating-System/assets/165137119/77aead73-666a-481d-b758-759ffe72037d)
Here is a list of the information that your kernel module should retrieve:
* Kernel: The kernel release
* CPU: The CPU model name
* CPUs: The number of CPU cores, in the format <# of online CPUs> / <# of total CPUs>
* Mem: The memory information, in the format<free memory> / <total memory> (in MB)
* Procs: The number of processes
* Uptime: How long the system has been running, in minutes.
### Kernel Module: `kfetch_mod`
The kernel module `kfetch_mod` is responsible for retrieving all necessary information and providing it when the device is read. Additionally, users can customize the information that kfetch displays by writing a kfetch information mask to the device. For example, a user could specify that only the CPU model name and memory information should be shown.
#### Kfetch information mask
* A *kfetch information mask* is a bitmask that determines which information to show. Each piece of information is assigned a number, which corresponds to a bit in a specific position.
* The mask is set by using bitwise OR operations on the relevant bits. For example, to show the CPU model name and memory information, one would set the mask like this: mask = KFETCH_CPU_MODEL | KFETCH_MEM.

#### Device operations
* Your device driver must support four operations: `open`, `release`, `read` and `write`.
* For the `read` operation, you need to return a buffer that contains the content of the logo and information to the user space. This allows the user to access and use the data from the device.
* For the `write` operation, a single integer representing the information mask that the user wants to set is passed to the device driver. Subsequent read operations will use this mask to determine which information to return to the user. This allows the user to specify which information they want to receive, and the device driver can use the mask to ensure that only the specified information is returned.
* For the `open` and `release` operations, you need to set up and clean up protections, since in a multi-threaded environment, concurrent access to the same memory can lead to race conditions. These protections can take the form of locks, semaphores, or other synchronization mechanisms that ensure that only one thread can access the variables at a time.






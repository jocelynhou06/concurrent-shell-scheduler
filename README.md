# Custom Shell and Scheduler

A C-based shell implementing script execution with support for multiple CPU scheduling policies, including First-Come-First-Serve (FCFS), Shortest Job First (SJF), Round Robin (RR/RR30), and SJF with Aging. The shell also supports background execution of the batch script using the `#` option.  

## Features

- Execute multiple scripts via the `exec` command.
- Support for **background batch script execution** using `#`.
- Scheduling policies:
  - **FCFS** – First-Come-First-Serve
  - **SJF** – Shortest Job First
  - **RR** – Round Robin with 2-instruction time slice
  - **RR30** – Round Robin with 30-instruction time slice
  - **AGING** – Shortest Job First with Aging to prevent starvation
- Processes managed via **PCBs** stored in shared memory.
- Ready queue management with proper insertion according to policy.

## Technologies

- C programming language  
- File I/O for script handling  
- Linked lists for ready queue management  
- Custom CPU scheduling algorithms  

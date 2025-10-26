# TypeTwo-Core  
A Minimal AMD SVM-Based Type-2 Hypervisor  

---

## Overview  
**TypeTwo-Core** is an educational **Type-2 (hosted) hypervisor** built from scratch using **AMD’s Secure Virtual Machine (SVM)** extensions.  
The goal of this project is to gain a hands-on understanding of **virtualization internals**, **CPU state management**, and **world switching** on x86-64 platforms without relying on existing hypervisor frameworks like KVM or VirtualBox.  

TypeTwo-Core is written primarily in **C/C++** with minimal inline Assembly and currently runs as a Windows driver for learning and experimentation purposes.  

---

## Current Status  
As of now, **TypeTwo-Core** performs:
- Initialization of the **VMCB (Virtual Machine Control Block)**  
- Setup of the **Guest Memory Layout** (currently 10 pages, 4KB each)  
- Basic **World Switching** between Host and Guest  
- Handling of fundamental **intercepts** (e.g., `HLT`, `VMMCALL`)  
- Logging and debugging of **guest state transitions**
- Added **VMEXIT handlers**

However, the project currently contains a **known bug** that can cause **system instability and crashes** after the first world switch. Debugging efforts are ongoing to isolate and resolve the issue.  

---

## Architecture  
- **Host Environment:** Windows (User-mode + Kernel driver)  
- **Processor:** AMD Ryzen (SVM enabled)  
- **Core Components:**
  - VMCB setup and initialization  
  - Nested paging structures (planned)  
  - Guest RIP/RSP memory mapping  
  - Intercept handling and control flow management  
- **Language:** C, C++, Assembly  
- **Build System:** Visual Studio (MSVC)  

---

## Future Roadmap  
- [ ] Fix current instability and ensure reliable guest transitions  
- [ ] Implement **Nested Paging (NPT)** for virtual address translation  
- [ ] Enhance / Modify current **VMEXIT handlers** for common intercepts (I/O, CPUID, MSR, etc.)  
- [ ] Build a minimal **Guest OS boot sequence**  
- [ ] Introduce a **lightweight CLI debugger** for monitoring guest state  
- [ ] Expand documentation for educational use  

---

## Educational Intent  
This project is designed purely for **educational and research purposes**, to explore the inner workings of AMD’s virtualization stack, system-level programming, and hardware interaction. It’s not production-ready, nor is it intended for deployment on critical systems.  

---

## Disclaimer  
TypeTwo-Core operates at a low system level and may cause **system crashes, BSODs, or hardware instability**.  
Please use only on **non-critical, test systems** with proper backups.  

---

## Acknowledgments  
- AMD Architecture Programmer’s Manual (Volumes 2 & 3)  
- [Sina Karvandi’s Hypervisor Development Tutorials](https://rayanfam.com/topics/hypervisor-from-scratch-part-1/)  
- Open-source hypervisor research and documentation communities  

---

## Author  
**Ritesh Tiwari**  
- Systems Programmer | Virtualization Enthusiast | Rust & C++ Developer  
- [LinkedIn](https://www.linkedin.com/in/ritesh-tiwari-67871124a)  

---

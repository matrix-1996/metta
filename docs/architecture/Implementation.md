General implementation considerations
-------------------------------------

Separate hardware-related interfaces from conceptual interfaces.
Similar to Pistachio, try to keep conceptual interfaces higher level and independent of hardware implementation details.


Stretch Allocator: implementation details
-----------------------------------------

`stretch_allocator` [privileged]
User API via: `stretch_allocator_v1::allocate_stretch()`/`release_stretch()`
Kernel impl: `protection_domain_t`
- range lists of available virtual memory ranges
+ allocate_stretch
+ release_stretch
* there's a VA_quota implied on all stretches owned by a domain

`frame_allocator` [privileged]
User API via: `frame_allocator_v1`
Kernel impl: `frame_allocator_t`
- range lists of available physical memory ranges
+ allocate_frame()
+ free_frame()
* There's a frame quota implied on a domain

`protection_domain` (via stretch_allocator?) [kernel]
- stretches list
+ get stretch for given va?

`stretch_driver` (bound to stretch) [userspace]
* 1. `map(va, pa, attr)`: arrange that the virtual address "va" maps onto the physical address "pa" with the (machine-dependent) attributes "attr".
* 2. `unmap(va)`: remove the mapping of the virtual address "va". Any further access to the address should cause some form of memory fault.
* 3. `mapping(va) -> (pa, attr)`: retrieve the current mapping of the virtual address "va", if any.

[DRM/GEM memory management details in Linux][drmgem]


IDC
---

Portals - sections of JIT generated code, that performs argument marshalling and request PD switch. Portals are generated by portal manager, which retrieves portal specifications from Interface Definition files. Portal manager is free to perform some specific optimizations, like portal short-circuiting, which in some cases can save a network roundtrip.


Protection Domain
-----------------

PD: a set of virtual addresses and access rights

in a domain: list of stretches and access rights define PD


Every application (or a process) is a domain.
Newly created domain possesses at least two stretches for text and data.

By default, all loaded code has RX and data R global stretch permissions. This could be improved.
Code is mapped RXM and data RWM to the corresponding new PD.

```
modules/Builder
modules/DomainMgrOffer
```

New PDs created by MMU!

SDom - scheduling domain.


Loading modules creates names in `modules>` context e.g. `modules>Gatekeeper` (GatekeeperMod in nemesis).


Structure of the boot process
-----------------------------

Since the system is highly modular, explicit code does minimal necessary initialization to start BP and APs. Most of the initialization is done by components during bootup module loading (which includes dependencies resolution).

First code loaded by the bootloader tries to figure out the bootloader type and prepare for further boot. During preparation the loader creates bootpage - it contains information about bootstrap in custom format. Preparations obtain entry point address, which is then called to perform bootstrap, address of bootpage is passed in.

Bootable images can be in different formats. Default GRUB image consists of first stage bootloader as kernel and the second stage loader within the binary blob containing all other components together as modules.

E.g.,
```
kernel /kickstart
module /system-bootimage
```


### GRUB boot model ###

#### First stage loader: /kickstart

Starts in protected mode, linear addresses equal physical, paging is off.

```
GRUB (or other bootloader)
+--loader
   +--setup stack
   +--find modules in bootimage - `nucleus` and `root_domain`
   +--`loader->prepare()`
      +--create bootpage
      +--relocate nucleus binary
      +--perform privileged nucleus initialization (interrupt and exception handlers)
      +--relocate `root_domain` binary
      +--obtain `root_domain` entry point address
   +--run `root_domain` entry point in ring3
```

First stage loader does not allocate any memory, except the bootpage at a fixed memory address, which can later be freed or put into memory map.


#### Root domain initialization

Second stage loader inits CPUs, memory system, paging, interrupts, publicly accessible information and boots the primary domain, which will be the privileged domain during runtime and will start up all other domains as necessary.

Please note, the privileged domain still runs in ring3. It is privileged in the types of operations it can perform with other domains.

> @todo Relate Pistachio SMP startup routines here.

Primary domain "materializes system domain out of thin air".
It reads the list of system "applications" and components from the boot image and adds them to the domain list.
It also holds the default pervasives list which system domains use during startup.

Uses namespace lookups in nexus - TBD how this works.

Primal runs only on Boot Processor.


#### Physical memory layout during kickstart:

              physical memory
       0  +----------------------+ 0x0000_0000
          +----------------------+ 0x0000_1000
          | kernel info page     |
          +----------------------+ 0x0000_2000
          |                      |
          +----------------------+ 0x0000_8000
          | bootinfo page        |
          +----------------------+ 0x0000_9000
          | .................... |
     1Mb  +----------------------+ 0x0010_0000 --------> 0x0010_0000 identity mapped
          | kickstart code       |
          +----------------------+ 0x0010_8000
          | kickstart data       |
          +----------------------+ 0x0010_9000 -------->
          |                      |
          | /kernel-startup      |
          |  module              |
          |                      |
          +----------------------+ 0x0011_1000 --------> 0x0100_0000 mapped at 16mb
          |                      |
          | /system-bootimage    |
          |  module              |
          |                      |
          +----------------------+ 0x001f_0000 --------> placement alloc starts here
          |                      |                       do we need it at all? try to avoid.
          | .................... |
          |                      |


#### Applications startup

From the starting application process viewpoint:

 * load application code and data
 * read library dependencies
 * for missing libraries, load them and their dependencies
 * link calls from app to used libraries


Library viewpoint:
 * library implements a component interface
 * this means per-client data is allocated by calling constructors of the interface
 * libraries which do not have per-client data may implement interfaces directly, but i presume this is rare.

A typical application:
 * loads trader library
 * loads memory library
 * instantiates trader interface
 * instantiates memory interface
 * allocates something
 * uses trader to locate a peer
 * instantiates a peer interface
 * sends something to peer via interface instance


Other random notes
------------------

All Oberon needs is Single Level Storage (SLS); another innovative feature of the System i5. There is a "Persistent Oberon" variant that offers an SLS-like framework for those people that want to explore that avenue.

Oberon the language allows direct manipulation of the hardware registers, so Oberon the OS is written entirely in a high-level language, without the need for assembly code.

----

Specialization instead of plain inheritance?
Template specialization removes need for virtual functions (?).

```cpp
struct arch_x86 {};

template<typename arch_t>
class virtual_address_space_t;

template <>
class virtual_address_space_t<arch_x86>
{
    // Specialized for x86
};
```
----

Try and have separate ABI and APIs for the kernel, much better for portability and writing new software.


  [drmgem]: http://lwn.net/Articles/283798/
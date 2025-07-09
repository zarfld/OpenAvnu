.. image:: avnu_logo.png

|
.. image:: https://github.com/zarfld/OpenAvnu/actions/workflows/travis_sh.yml/badge.svg?branch=master
   :target: https://github.com/zarfld/OpenAvnu/actions/workflows/travis_sh.yml
   :alt: Build status


OpenAvnu
========

The OpenAvnu project is sponsored by the Avnu Alliance.

📋 **Project Status**: ⚠️ **DEVELOPMENT/TESTING** - Core gPTP Clock Quality Testing Framework implemented, hardware validation required

⚠️ **Hardware Testing Required**: Intel i210/i219 NIC validation needed on Windows 10/11 and Linux

📚 **Documentation**: **FULLY ORGANIZED** - All documentation cleaned up and properly categorized

🎯 **Quick Start**: See `PROJECT_OVERVIEW.md <PROJECT_OVERVIEW.md>`_ for current status and `docs/README.md <docs/README.md>`_ for organized documentation

📁 **Documentation Structure**:
- **docs/completed/** - Production-ready documentation (25 files)
- **docs/work-in-progress/** - Active development (2 files)  
- **docs/status/** - Current project status (1 file)
- **docs/archive/** - Obsolete/redundant documentation (3 files)

The intent is to provide components for building AVB/TSN systems. The repository
contains primarily network building block components - drivers, libraries,
\example applications  and daemon source code - required to build an AVB/TSN system.
It is planned to eventually include the various packet encapsulation types,
protocol discovery daemons, libraries to convert media clocks to AVB clocks
and vice versa), and drivers.

This repository does not include all components required to build a full
production AVB/TSN system (e.g. a turnkey solution to stream stored or live audio
or video content). Some simple example applications are provided which
illustrate the flow - but a professional Audio/Video system requires a full media stack
- including audio and video inputs and outputs, media processing elements, and
various graphical user interfaces. Various companies provide such integrated
solutions.

For more information about AVB/TSN, see also the Avnu Alliance webpage at
www.avnu.org.

BACKGROUND
===========

Intel created the OpenAvnu repository to encourage collaborative source code
development for AVB/TSN technology enabling. By publishing the source code, our
intent is to encourage standardization, stability and inter-operability between
multiple vendors. This repository - created by the Intel LAN Access Division -
is open for contributions from other vendors. 

LICENSING AND CONTRIBUTION GUIDELINES
======================================
To the extent possible, content is licensed under BSD licensing terms. Linux 
kernel mode components are provided under a GPLv2 license. The specific license 
information is included in the various directories to eliminate confusion. We 
encourage you to review the ‘LICENSE’ file included in the head of the 
various subdirectories for details.

Third party submissions are welcomed. Our intent for third party content 
contributions is to enable derivative products with minimal licensing 
entanglements. Practically speaking, this means we would enforce (a) an 
original-source attestation for any contributed content, and (b) rejecting 
patches with GPL content into existing “BSD” licensed components. Third 
party copyrights can be included provided they do not narrow the licensing 
terms of an existing component.

The OpenAvnu project has a development mailing list. To subscribe, visit
https://lists.sourceforge.net/lists/listinfo/open-avb-devel to sign up.

WEBSITE
=======

A github based website for OpenAvnu can be found here.

+ http://avnu.github.io/OpenAvnu

GIT SUBMODULES
==============

After checking out the OpenAvnu git repository submodules should be
configured by going::

    git submodule init
    git submodule update

MAKE on debian
==============

``bash
sudo apt install libpci-dev libpcap-dev libsndfile-dev libjack-dev
make all
make -C lib/igb_avb all
``

CMAKE
=====

cmake is a cross-platform build system generator. cmake build files are
currently available to build mrpd in Windows and linux and CppUTest unit
tests for mrpd. The recommended usage for cmake is to create an out-of-tree
directory for cmake output of generated makefiles or MSVC project files.

Starting from the OpenAvnu dir, one would go::

    mkdir tmp
    cd tmp
    cmake .. -G "Unix Makefiles"
    make

to build in OpenAvnu/tmp

gPTP TIME SYNCHRONIZATION
==========================

OpenAvnu requires a gPTP (IEEE 802.1AS) daemon for proper time synchronization
in AVB/TSN networks. This repository includes the zarfld/gptp implementation as
a submodule, which provides cross-platform gPTP support.

The gPTP daemon is automatically built when building OpenAvnu with cmake. After
building, you can start the gPTP daemon using the provided scripts:

**Linux:**::

    sudo ./run_gptp.sh eth0

**Windows:**::

    .\run_gptp.ps1 -NetworkInterface "Ethernet"

For detailed information about gPTP configuration and troubleshooting, see
``docs/gPTP_Integration.md``.

**VS Code Users:** Several gPTP-related tasks are available in VS Code:

- Apply gPTP Windows Build Fix (fixes config file path issues)
- Run gPTP Daemon (Windows/Linux)
- Build gPTP Only
- Verify gPTP Integration
- Open gPTP Documentation

**Important:** gPTP must be running before starting any AVTP applications, as
proper time synchronization is required for standards-compliant operation.

Intel Ethernet Hardware Abstraction Layer (HAL)
===============================================

OpenAvnu includes a comprehensive Intel Ethernet HAL that provides native hardware
support for Intel I210, I219, I225, and I226 network adapters across Windows and Linux.

**Supported Hardware:**

- **Intel I210** (0x1533, 0x1536, 0x1537) - Basic IEEE 1588, MMIO, DMA
- **Intel I219** (0x15B7, 0x15D7, 0x0DC7, etc.) - Basic IEEE 1588, MDIO PHY access  
- **Intel I225** (0x15F2, 0x15F3) - Enhanced timestamping, TSN TAS/FP, PCIe PTM, 2.5G
- **Intel I226** (0x125B, 0x125C) - Next-gen 2.5G with all I225 features + improvements

**Platform Integration:**

- **Windows**: Native NDIS integration with ``NDIS_TIMESTAMP_CAPABILITIES``
- **Linux**: PTP Hardware Clock (``/dev/ptp*``) and ethtool integration

**Key Features:**

- Precise IEEE 1588 hardware timestamping
- TSN Time Aware Shaping and Frame Preemption (I225/I226)
- Cross-platform capability detection
- Native OS API integration for optimal performance

**Usage:**

The Intel HAL is automatically built with OpenAvnu::

    cmake .. -DOPENAVNU_BUILD_INTEL_HAL=ON

For detailed integration examples, see ``thirdparty/intel-ethernet-hal/OPENAVNU_INTEGRATION.md``.

Apple Vendor PTP Profile
========================

Support for the Apple Vendor PTP Profile can be found on the
feature-aptp-experimental branch of the OpenAvnu repository.

These changes allow interaction with Apple proprietary PTP clocks. This 
implementation has been tested with the Apple AirPlay SDK on a Raspberry Pi 3 
running within a group of devices playing the same music stream.

RELATED OPEN SOURCE PROJECTS
============================

AVDECC
------
Jeff Koftinoff maintains a repository of AVDECC example open 
source code. AVDECC is a management layer, similar to SNMP MIB formats, 
which enables remote devices to detect, enumerate and configure AVB/TSN-related
devices based on their standardized management properties.

+ https://github.com/jdkoftinoff/jdksavdecc-c

AudioScience has created a 1722.1 C++ controller library which builds on jdkadvecc-c.

+ https://github.com/audioscience/avdecc-lib

XMOS
----
XMOS is a semiconductor company providing a reference design for AVB/TSN
endpoints in pro audio and automotive. XMOS endpoint source code is open source 
and available on Github - https://github.com/xcore/sw_avb

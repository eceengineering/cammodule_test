# cammodule_test

This is a test application to check if CAMMODULE is running properly on the platform.

Please see CAMMODULE at https://github.com/eceengineering/cammodule

Description: 

    Programming Language:   C
    Compiler:               GCC
    Test Platform(s):       ARM Cortex-A8 h/w
                            ARM Cortex-A9 h/w
    Test OS:                Linux Ubuntu
                            Linux Debian
                            
    Input(s):
          - Camera resolution
          - Camera device name

    Output(s):
          - JPEG File

    Sequence:

          See the test application source code available in main.c file.
          The module interface details are available in cammodule.h file.
          - Init() 	- To initialize the module with device name and type of camera
          - Start() - To start capturing of raw video frames
          - Stop()	- To stop capturing process
          - SaveFrame() - To save the video frame into a JPEG File

How to Build:

    - Makefile available in the repository is generated regarding to Arm Linux Gnueabihf Gcc.
    - $make 

How to Run:

    - $./Test.sh


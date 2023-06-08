# CSE306 Team Project - RC Car only for computer engineers

## Execution
- **Raspberry Pi**: make clean, make. On separate shell, run ```sudo ./main```, ```./cam```. 
- **PC**: make clean, make. On separate shell, run ```./main easy``` or ```./main hard```, ```./cam``` 
  - make sure to configure the IP address correctly

## Files

- **Server**: Files to be run on Raspberry pi
  - [server_main.c](server/server_main.c): Main controlling file (uses port 5000)
  - [server_camera.c](server/server_camera.c): Server file for camera (uses port 8888)
  - [makefile](server/makefile): Makefile with appropriate linkers
- **Client**: Files to be run on the controlling PC
  - [client_main.c](client/client_main.c): RC car controller. Execute with controlling mode (easy or hard) (uses 
    port 5000)
  - [client_camera.c](client/client_camera.c): Shows live camera video with SDL window. SDL2 must be installed (uses 
    port 8888)
  - [makefile](client/makefile): Makefile with appropriate linkers

## Required packages
- **Raspberry Pi**:
  - **Wiring pi**: ```$ sudo apt-get install wiringpi```
- **PC**:
  - **SDL**: ```$ sudo apt-get install libsdl2-dev```


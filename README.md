# Program_Schedular
**Use the following commands in order to compile and run our program on a Linux machine or WSL terminal:**  
— sudo apt update  
— sudo apt upgrade -y  
— sudo apt install build-essential gcc g++ make  
— sudo apt install libgtk-3-dev pkg-config  
— gcc Queue.c OS_MS2.c scheduler_gui2.c -o scheduler_gui ‘pkg-config –cflags –libs gtk+-
3.0‘ && sudo ./scheduler_gui

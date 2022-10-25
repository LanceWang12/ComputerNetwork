# Implement TCP transportation by using UDP protocal

## Goal

1. Three-Way Handshake (Start connection)
    <img src="https://i.imgur.com/9jSKdwy.jpg" style="zoom:50%;" />

2. Four-Way Wavehand (Close connection)
    <img src="https://i.imgur.com/dZoRoLl.png" style="zoom:50%;" />

3. Flow Control
    <img src="https://i.imgur.com/kB4kfnY.png" style="zoom:50%;" />

4. Congestion Control

    <img src="https://i.imgur.com/avgF2NW.png" style="zoom:80%;" />

## Code explanation

<img src="https://i.imgur.com/HSbe6f0.png" style="zoom:90%;" />

1. TCP.h
   ```c++
   class CORE{
     sockaddr_in my_info; // my socket info
     sockaddr_in your_info; // your socket info
     char BUFFER[MAX_BUF]; // Buffer for data transportation
     int rwnd, cwnd; // window size for flow control
     int seq_number, ack_number // my seq and ack number
   };
   ```

2. server.cpp
   ```c++
   class Server{
     CORE *reception; // serve to receive client's connection requirement
     CORE *core[ClientNum]; // some record in core
     pthread_t core_thread[ClientNum]; // transport data
   };
   ```

3. client.cpp
   ```c++
   class Client{
     CORE core; // just need one core to receive data from server
     int connect_to_server(char * ip, int port_num); // Three-Way Handshake
     int close_connection(); // Four-Way Wavehand
   };
   ```

   


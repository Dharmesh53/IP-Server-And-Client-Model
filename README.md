## Basic Introduction

Most interprocess communication uses the client server model. These terms refer to the two processes which will be communicating with each other. One of the two processes, the client, connects to the other process, the server, typically to make a request for information. A good analogy is a person who makes a phone call to another person.

The system calls for establishing a connection are somewhat different for the client and the server, but both involve the basic construct of a socket.

A socket is one end of an interprocess communication channel. The two processes
each establish their own socket.

The steps involved in establishing a socket on the client side are as follows:
- Create a socket with the socket() system call
- Connect the socket to the address of the server using the connect() system call
- Send and receive data. There are a number of ways to do this, but the simplest is to use the read() and write() system calls.


The steps involved in establishing a socket on the server side are as follows:
- Create a socket with the socket() system call
- Bind the socket to an address using the bind() system call. For a server socket on the Internet, an address consists of a port number on the host machine.
- Send and receive data

When a socket is created, the program has to specify the address domain and the socket type.
There are two widely used address domains, the unix domain, in which two processes which share a common file system communicate, and the Internet domain, in which two processes running on any two hosts on the Internet communicate.

The address of a socket in the Internet domain consists of the Internet address of the host machine (every computer on the Internet has a unique 32 bit address, often referred to as its IP address).
In addition, each socket needs a port number on that host.
Port numbers are 16 bit unsigned integers.
The lower numbers are reserved in Unix for standard services. For example, the port number for the FTP server is 21. It is important that standard services be at the same port on all computers so that clients will know their addresses.

Port numbers above 2000 to 65535 are generally available.

A virtual network interface (VNI) is an abstract virtualized representation of a computer network interface that may or may not correspond directly to a network interface controller.

#### These codes can only run in UNIX environment

To run the code first setup two virtual interfaces:

These virtual network interfaces only exists till your next reboot
```
sudo ifconfig wlan0:1 192.168.1.6 up
sudo ifconfig wlan0:2 192.168.1.7 up
```
- Change the `wlan0` with your network interfaces. 
- These interfaces will be gone after a reboot.

Then you need to compile the program
```
g++ -o server server.cpp
g++ -o client client.cpp
```

./server 5000 
// telling server to run on which port number

./client
```


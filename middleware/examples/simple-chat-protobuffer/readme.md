# Simple chat protobuffer

The purpose of this application is to give you an example of how a very simple client/server application
can be built using casual and google protobuffers.

Several clients on the same computer can exchange messages in an old fasioned terminal window.

## Instructions

First you will have to compile and install casual. Using the same environment you can build and install this application by 
 $ casual-make install

If you have used the example environment cd to the installation directory
 $ cd ~/usr/local/casual/simple-chat-protobuffer
 
Source the domain environment
 $ source domain.env
 
Start the server 
 $ casual-admin domain -b
 
Start the client
 $ simple-chat-protobuffer-client
 
Hit h for help
 @> h

## Directories and files

### makefile.cmk
Is sits right here beside the readme. 

### commmon
The directory common contains code common to both server and client, most notably the protobuffer source. The file make.sh is
used to generate c++ source code from the protobuffer source. 

### client
The directory holds the client code.

### server
The server code lives here. The file 'simple-chat-protobuffer.services' contains the bindings from service strings to functions inside
the server code.

### configuration
Here is the domain.yaml file shipped with the installation

### env
The domain.env file shipped with the istallation lives here. The casual.env file from the example directory is also copied during
installation.



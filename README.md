# Chat
Chat en C implementando sockets y protobuf
### Ejecución
En terminal:
c++ -std=c++11 ChatServer.cpp new.pb.cc -o [object_file_name] ``pkg-config --cflags --libs protobuf``

Luego con cada ejecutable:
#### Server
`./[object_file_name] [port]`

#### Client
`./[object_file_name] [username] [IP address] [port]`

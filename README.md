# Chat
Chat en C implementando sockets y protobuf
### Ejecución
En terminal:
<pre>
c++ -std=c++11 ChatServer.cpp new.pb.cc -o [object_file_name] `pkg-config --cflags --libs protobuf`
</pre>

Luego con cada ejecutable:
#### Server
<pre>./[object_file_name] [port]</pre>

#### Client
<pre>./[object_file_name] [username] [IP address] [port]</pre>

### Comandos disponibles para el cliente
<pre>
- Enviar mensajes (general o privado): MESSAGE [recipiente] [mensaje]
- Listar usuarios conectados:          CONNECTED
- Cambio de estado:                    STATUS [nuevo_status]
- Información acerca de un usuario:    INFO [nombre_de_usuario]
- Ayuda y comandos:                    HELP
- Salida del chat room:                EXIT
</pre>

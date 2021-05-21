#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "new.pb.h"

#define CLIENTS    8192
#define OK_CODE     200
#define ERROR_CODE  500
#define IDLE_TIME    30

#define clrsrc() printf("\e[1;1H\e[2J")

using namespace std;


//global client count
int client_count = 0;

//structure for storing the clients
struct client{

	int index;
	int socket_ID;
	struct sockaddr_in client_addr;
	socklen_t len;
	char username[256];
	char ip[256];
	char status[32];

};

//storage of client structures and client threads
struct client clients[CLIENTS];
pthread_t threads[CLIENTS];

//function for client threads
void * requestListening(void * CD){

	struct client* client_detail = (struct client*) CD;

	//idle time
	int ELAPSED = 0;
	bool CHANGED = false;
	
	//client's index and socket
	int index = client_detail -> index;
	int client_socket = client_detail -> socket_ID;

	//server printing of client IDs
	printf("[Client %d] 200 Connected in socket number %d\n", index + 1, client_socket);

	//loop for attending calls
	while(1){

		//receive client request
		chat::ClientPetition client_request;
	
		void* buffer;
		buffer = malloc(1024);

		//read and receive requests 
		int read = recv(client_socket, buffer, 1024, MSG_DONTWAIT);

		//parse request from array
		client_request.ParseFromArray(buffer, 1024);

		//options for the requests
		/*
		2. Usuarios conectados
		3. Cambio de estado
		4. Mensajes
		5. Informacion de un usuario en particular
		*/
		if(read > 1)
		{
			ELAPSED = 0;

			if(client_request.option() == 2){
				//unpack message communication
				chat::ServerResponse response;
				chat::ConnectedUsersResponse* user_list;

				//code and option
				response.set_option(2);
				response.set_code(OK_CODE);

				//user list	
				user_list = response.mutable_connectedusers();
				
				int i;
				for(i = 0; i < client_count; i++)
				{
					//fill user list with each user connected
					if(strcmp(clients[i].status, "") != 0)
					{
						chat::UserInfo* users;
						users = user_list -> add_connectedusers();
		
						users -> set_username(clients[i].username);
						users -> set_status(clients[i].status);
						users -> set_ip(clients[i].ip);
					}
				}

				//user list to be sent
				size_t size = response.ByteSizeLong();
				buffer = malloc(size);
	
				response.SerializeToArray(buffer, size);
				send(client_socket, buffer, size, 0);

				cout << "[Client " << index + 1 << "] " << OK_CODE << " GET Connected users" << endl;

				continue;

			}
			if(client_request.option() == 3){
				//	unpack change status
				chat::ChangeStatus status;

				status = client_request.change();

				//	string to char
				char char_status[status.status().size() + 1];
				strcpy(char_status, status.status().c_str());

				//	create server response
				chat::ServerResponse response;
				chat::ChangeStatus* tb_status;

				//	send if valid
				if(strcmp(char_status, "ACTIVO") == 0 | strcmp(char_status, "INACTIVO") == 0 | strcmp(char_status, "OCUPADO") == 0)
				{
					//	code, option, server message
					response.set_code(OK_CODE);
					response.set_servermessage("Status actualizado con exito");

					strcpy(clients[index].status, char_status);
					
					cout << "[Client " << index + 1 << "] " << OK_CODE << " POST Status change" << endl;
				}
				else
				{
					//	code, option, server message
					response.set_code(ERROR_CODE);

					response.set_servermessage("El nuevo status no es valido");

					strcpy(clients[index].status, char_status);

					cout << "[Client " << index + 1 << "] " << ERROR_CODE << " POST Status change" << endl;
				}

				response.set_option(3);

				//	mutable
				tb_status = response.mutable_change();

				//	set values
				tb_status -> set_username(status.username());
				tb_status -> set_status(status.status());

				//	send response
				size_t size = response.ByteSizeLong();
				buffer = malloc(size);
	
				response.SerializeToArray(buffer, size);
				send(client_socket, buffer, size, 0);

				continue;
	
			}
			if(client_request.option() == 4){
				//unpack message communication
				chat::MessageCommunication message;
	
				message = client_request.messagecommunication();
	
				//string to char
				char char_recipient[message.recipient().size() + 1];
				strcpy(char_recipient, message.recipient().c_str());
	
				//create server responser
				chat::ServerResponse response;
				chat::MessageCommunication* tb_message;
	
				//code and option
				response.set_option(4);
				response.set_code(OK_CODE);
	
				//set mutable
				tb_message = response.mutable_messagecommunication();
	
				//set values for message
				tb_message -> set_sender(message.sender());
				tb_message -> set_recipient(message.recipient());
				tb_message -> set_message(message.message());
	
				//message communication to be sent
				size_t size = response.ByteSizeLong();
				buffer = malloc(size);
	
				response.SerializeToArray(buffer, size);
	
				bool sent = false;
				int i;

				//send to every connected client
				for(i = 0; i < client_count; i++)
				{	
					if(strcmp("everyone", char_recipient) == 0 | strcmp(char_recipient, clients[i].username) == 0){
						send(clients[i].socket_ID, buffer, size, 0);
						sent = true;
					}
				}

				if(!sent)
				{
					cout << "[Client " << index + 1 << "] " << ERROR_CODE << " POST Message communication" << endl;
					char msg[] = "No existen usuarios con el nombre ";
					strcat(msg, char_recipient);

					response.set_code(ERROR_CODE);
					response.set_servermessage(msg);

					size_t size = response.ByteSizeLong();
					buffer = malloc(size);
	
					response.SerializeToArray(buffer, size);
					send(client_socket, buffer, size, 0);
				}
				else
				{
					cout << "[Client " << index + 1 << "] " << OK_CODE << " POST Message communication" << endl;
					if(strcmp("everyone", char_recipient) != 0)
					{
						send(client_socket, buffer, size, 0);
					}
				}
			}
			if(client_request.option() == 5){
				//	unpack user request
				chat::UserRequest user;

				user = client_request.users();

				//	string to char
				char char_username[user.user().size() + 1];
				strcpy(char_username, user.user().c_str());

				//	server response
				chat::ServerResponse response;
				chat::UserInfo* user_info;

				response.set_option(5);

				user_info = response.mutable_userinforesponse();

				int i;
				bool found = false;
				for(i = 0; i < client_count; i++)
				{
					//fill user list with each user connected
					if(strcmp(clients[i].username, char_username) == 0)
					{
						user_info -> set_username(clients[i].username);
						user_info -> set_status(clients[i].status);
						user_info -> set_ip(clients[i].ip);

						response.set_code(OK_CODE);
						found = true;

						cout << "[Client " << index + 1 << "] " << OK_CODE << " GET User information" << endl;
					}
				}

				if(!found)
				{
					response.set_code(ERROR_CODE);

					char msg[] = "No existen usuarios con el nombre ";
					strcat(msg, char_username);

					response.set_servermessage(msg);

					cout << "[Client " << index + 1 << "] " << ERROR_CODE << " GET User information" << endl;
				}

				size_t size = response.ByteSizeLong();
				buffer = malloc(size);
	
				response.SerializeToArray(buffer, size);
				send(client_socket, buffer, size, 0);

				continue;
	
			}
		}
		else
		{
			if(read == 0)
			{
				//clear the user disconnected index
				memset(&clients[index], 0, sizeof(clients[index]));
				printf("Client %d disconnected\n", index + 1);
			
				return NULL;
			}
			if(read == -1)
			{
				sleep(1);
				
				if(ELAPSED < IDLE_TIME)
				{
					ELAPSED = ELAPSED + 1;
				}
				else
				{
					if(!CHANGED)
					{
						cout << "Client " << index +1 << " has gone idle" << endl;
						strcpy(clients[index].status, "INACTIVO");

						chat::ServerResponse response;
						chat::ChangeStatus* tb_status;

						response.set_code(OK_CODE);
						response.set_servermessage("Status actualizado con exito");

						response.set_option(3);

						//	mutable
						tb_status = response.mutable_change();

						//	set values
						tb_status -> set_username(clients[index].username);
						tb_status -> set_status("INACTIVO");
						

						size_t size = response.ByteSizeLong();
						buffer = malloc(size);
	
						response.SerializeToArray(buffer, size);
						send(client_socket, buffer, size, 0);

						CHANGED = true;
					}
				}
			}
		}
	}

	return NULL;

}

int main(int argc, char* argv[]){

	//good protobuf practices
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	clrsrc();

	//variables
	int i;
	void *buffer;

	//get port definition from terminal
	uint16_t PORT = atoi(argv[1]);

	//create socket
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);

	//define the address for server binding
	struct sockaddr_in server_addr;

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = htons(INADDR_ANY);

	//bind socket to addres
	if(bind(server_socket, (struct sockaddr *) &server_addr , sizeof(server_addr)) == -1) return 0;

	//listening to client connections
	if(listen(server_socket, CLIENTS) == -1) return 0;

	//server initialization success
	printf("Server listening en el puerto: %d ...........\n", PORT);

	//loop for listening client connections
	while(1){
		/* USER REGISTRATION */
		// accept client connection 
		clients[client_count].socket_ID = accept(server_socket, (struct sockaddr*) &clients[client_count].client_addr, &clients[client_count].len);
		clients[client_count].index = client_count;
	
		//protobuf classes objects
		chat::UserRegistration user;
		chat::ClientPetition client_request;

		//receive user's data
		buffer = malloc(1024);
		int read = recv(clients[client_count].socket_ID, buffer, 1024, 0);

		client_request.ParseFromArray(buffer, 1024);

		user = client_request.registration();

		//string to char parsing
		char c[user.username().size() + 1];
		strcpy(c, user.username().c_str());

		bool exists = false;

		//check if username exists in user table
		for(i = 0; i < client_count ; i++)
		{
			if(strcmp(clients[i].username, c) == 0)
			{
				exists = true;
			}
		}

		//prepare response to client
		chat::ServerResponse response;
		response.set_option(1);

		size_t size;

		//error if username exists OK if else
		if(exists)
		{
			//set error response
			response.set_code(500);
			response.set_servermessage("Ya existe un usuario con ese nombre, intente con uno nuevo\n");
			printf("Failed to register user with username '%s'\n", c);		
		}
		else
		{
			//set success response
			response.set_code(200);
			response.set_servermessage("Usuario registrado correctamente");

			//save user's data in table
			strcpy(clients[i].username, user.username().c_str());
			strcpy(clients[i].ip, user.ip().c_str());
			strcpy(clients[i].status, "ACTIVO");
		}
		
		//send server response
		size = response.ByteSizeLong();
		buffer = malloc(size);
		response.SerializeToArray(buffer, size);
		send(clients[client_count].socket_ID, buffer, size, 0);

		if(!exists)
		{
			//create pthread for user actions
			pthread_create(&threads[client_count], NULL, requestListening, (void *) &clients[client_count]);
			//increment user count
			client_count ++;
		}
		
		/* ----------------- */
	}

	//join for pthreads
	for(i = 0 ; i < client_count ; i++) pthread_join(threads[i], NULL);

	//good protobuf practices
	google::protobuf::ShutdownProtobufLibrary();

}

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

#define CLIENTS 8192

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

	//client's index and socket
	int index = client_detail -> index;
	int client_socket = client_detail -> socket_ID;

	//server printing of client IDs
	printf("Client %d connected in socket number %d\n", index + 1, client_socket);

	//loop for attending calls
	while(1){

		//receive client request
		chat::ClientPetition client_request;

		void* buffer;
		buffer = malloc(1024);

		//read and receive requests
		int read = recv(client_socket, buffer, 1024, 0);

		//parse request from array
		client_request.ParseFromArray(buffer, 1024);

		//options for the requests
		/*
		2. Usuarios conectados
		3. Cambio de estado
		4. Mensajes
		5. Informacion de un usuario en particular
		*/

		if(read > 0)
		{
			if(client_request.option() == 2){
				//unpack message communication
				chat::ServerResponse response;
				chat::ConnectedUsersResponse* user_list;

				//code and option
				response.set_option(2);
				response.set_code(200);

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

				continue;

			}
			if(client_request.option() == 3){
				// printf("POST Cambio de estado");

				//	unpack change status
				chat::ChangeStatus status:

				status = client_request.changestatus();

				//	string to char
				chat char_user_status[status.status().size() + 1];
				strcpy(char_user_status, status.status().c_str());

				//	create server response
				chat::ServerResponse response;
				chat::ChangeStatus* tb_status;

				//	code, option and serverMessage
				response.set_option(3);
				response.set_code(200);
				response.set_servermessage("El status se ha cambiado con éxito!");

				//	ser mutable
				tb_status = response.mutable_changestatus();

				//	set values for status
				tb_status -> set_username(status.username());
				tb_status -> set_status(status.status());

				//	change status to be sent
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
				response.set_code(200);

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
			}
			if(client_request.option() == 5){
				// printf("GET Usuario especifico");

				//	unpack user request
				chat::UserRequest user;

				user = client_request.userrequest();

				//	string to char
				char char_username[user.username().size() + 1];
				strcpy(chat_username, user.username().c_str());

				//	 create Server Response
				chat::ServerResponse response;
				chat::UserInfoRequest* user_info;

				//	option and code
				response.set_option(5);
				response.set_code(200);

				//	set mutable
				user_info = response.mutable_connectedusers();

				int i;
				for(i = 0; i < client_count; i++)
				{
						if(strcmp(char_username, clients[i].username) == 0) {
							chat::UserInfo* user_details;

							user_details = user_info -> add_connectedusers();

							user_details -> set_username(clients[i].username);
							user_details -> set_status(clients[i].status);
							user_details -> set_ip(clients[i].ip);
						}
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
			//clear the user disconnected index
			memset(&clients[index], 0, sizeof(clients[index]));
			printf("Client %d disconnected\n", index + 1);

			return NULL;
		}
	}

	return NULL;

}

int main(int argc, char* argv[]){

	//good protobuf practices
	GOOGLE_PROTOBUF_VERIFY_VERSION;


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

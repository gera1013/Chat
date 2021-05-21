#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "new.pb.h"

#define clrsrc() printf("\e[1;1H\e[2J")
#define clrln()  printf("\33[2K\r")

using namespace std;

//global username declaration
char* USERNAME;
char STATUS[] = "ACTIVO";

//input line sim
void print_namestatus()
{
	cout << "(@" << USERNAME << " " << STATUS << "): ";
}

//print help menu funtion
void print_help()
{
	printf("\nEstos son los comandos utilizados dentro de la sala:\n");
	printf("  * Envio de mensajes:         MESSAGE [USERNAME] [MENSAJE]\n");
	printf("  * Informacion de usuarios:   CONNECTED\n");
	printf("  * Informacion de un usuario: INFO [USERNAME]\n");
	printf("  * Cambio de status:          STATUS [NEW STATUS]\n");
	printf("  * Informacion de comandos:   HELP\n");
	printf("  * Salida del chat:           EXIT\n");

	printf("\nUtilice el comando 'HELP' para volver a ver la informacion acerca de los demas comandos\n\n");
}


//funcion que realiza el thread para recibir comunicaciones del servidor
void * listenResponses(void * socket_ID){
	
	int socket = *((int *) socket_ID);

	while(1){
		//receive client request
		chat::ServerResponse server_response;
	
		void* buffer;
		buffer = malloc(1024);

		//receive from socket
		int read = recv(socket, buffer, 1024, 0);

		//parse response to object
		server_response.ParseFromArray(buffer, 1024);

		//options for the requests
		/*
		2. Usuarios conectados
		3. Cambio de estado
		4. Mensajes
		5. Informacion de un usuario en particulas
		*/
		if(server_response.option() == 2){
			//unpack the user list received
			chat::ConnectedUsersResponse user_list;
			
			//set user list from response
			user_list = server_response.connectedusers();
			
			//print the user list contents
			if(server_response.code() == 200)
			{
				int i;

				cout << "\nLISTADO DE USUARIOS CONECTADOS\n";

				for(i = 0; i < user_list.connectedusers_size(); i++)
				{
					chat::UserInfo user = user_list.connectedusers(i);
	
					cout << "* Username -> " << user.username() << endl;
					cout << "* Status   -> " << user.status() << endl << endl;
				}
			}
			else
			{
				cout << "\n" << server_response.servermessage() << endl << endl;
			}

			continue;

		}
		if(server_response.option() == 3){
			
			chat::ChangeStatus status;

			status = server_response.change();

			if(server_response.code() == 200)
			{
				cout << "Cambio de status: " << status.status() << endl;

				char char_new_status[status.status().size() + 1];
				strcpy(char_new_status, status.status().c_str());

				strcpy(STATUS, char_new_status);
			}
			else
			{
				cout << "Error en el cambio de status\n" << server_response.servermessage() << endl;
			}
		
			continue;

		}
		if(server_response.option() == 4){
			//protobuf message communication
			chat::MessageCommunication message;

			//set message
			message = server_response.messagecommunication();

			//string to char
			char char_recipient[message.recipient().size() + 1];
			strcpy(char_recipient, message.recipient().c_str());

			char char_sender[message.sender().size() + 1];
			strcpy(char_sender, message.sender().c_str());

			if(server_response.code() == 200)
			{
				clrln();

				//print del mensaje segun sender
				if(strcmp(char_recipient, "everyone") == 0)
				{
					if(strcmp(char_sender, USERNAME) == 0)
					{
						cout << "[GENERAL] YOU:" << message.message() << endl;
					}
					else
					{
						cout << "[GENERAL] " << message.sender() << ":" << message.message() << endl;
					}
				}
				else
				{
					if(strcmp(char_recipient, USERNAME) == 0)
					{
						cout << "[PRIVADO - " << message.sender() << "] " << message.sender() << ":" << message.message() << endl;
					}
					else
					{
						cout << "[PRIVADO - " << message.recipient() << "] YOU:" << message.message() << endl;
					}
				}
				
				continue;
			}
			else
			{
				cout <<  "\n" << server_response.servermessage() << endl << endl;
			}

		}
		if(server_response.option() == 5){
			chat::UserInfo user;

			user = server_response.userinforesponse();

			if(server_response.code() == 200)
			{
				cout << "\n Informacion de Usuario" << endl;

				cout << "* Username -> " << user.username() << endl;
				cout << "* Status   -> " << user.status() << endl;
				cout << "* IP       -> " << user.ip() << endl << endl;
			}
			else
			{
				cout <<  "\n" << server_response.servermessage() << endl << endl;
			}
			continue;

		}
	}

}


//Main function para el programa
int main(int argc, char* argv[]){

	clrsrc();

	//good protobuf practices
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	
	//get user data from command line
	USERNAME = argv[1];
	char* IP = argv[2];
	uint16_t PORT = atoi(argv[3]);	
	strcpy(STATUS, "ACTIVO");

	//var
	void *buffer;

	//create client socket
	int client_socket = socket(AF_INET, SOCK_STREAM, 0);

	//server address, connect to IP given by user
	struct sockaddr_in server_addr;

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = inet_addr(IP);

	//connect to the server, message and exit if failed
	if(connect(client_socket, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1)
	{
		printf("Fallo en la conexion al servidor ............\n");
		return 0;
	}

	//vars for client's IP details
	char host[256];
	char *IPbuffer;
	int hostname;
	struct hostent *host_entry;

	/* USER REGSITRATION */

	//get the user IP
	hostname = gethostname(host, sizeof(host));
	host_entry = gethostbyname(host);
	IPbuffer = inet_ntoa(*((struct in_addr*) host_entry -> h_addr_list[0]));

	//create petition and user from protobuf class
	chat::UserRegistration* user;
	chat::ClientPetition request;

	//initialize values
	request.set_option(1);
	user = request.mutable_registration();

	user -> set_username(USERNAME);
	user -> set_ip(IPbuffer);

	//send data to server
	size_t size = request.ByteSizeLong();
	buffer = malloc(size);

	request.SerializeToArray(buffer, size);
	send(client_socket, buffer, size, 0);
	/* ------------------- */

	/* SERVER RESPONSE */
	chat::ServerResponse response;

	buffer = malloc(1024);
	int read = recv(client_socket, buffer, 1024, 0);

	//parse server response
	response.ParseFromArray(buffer, 1024);

	//exit if failure continue if everything OK
	if(response.code() == 200)
	{
		
		printf("Estableciendo conexion............\n");
	
		//pthread for receiving server responses
		pthread_t thread;
		pthread_create(&thread, NULL, listenResponses, (void *) &client_socket );
	}
	else
	{
		//error message and exit
		cout << response.servermessage() << endl;
		return 0;
	}
	// --------------- //

	sleep(1);
	clrsrc();

	//--------------------------------------------//
	printf("Bienvenido al CHAT ROOM\n");

	print_help();
	//--------------------------------------------//

	//loop for client requests
	while(1){

		//client request
		chat::ClientPetition client_request;

		//client input
		char input[1024];
		
		sleep(1);

		clrln();
		print_namestatus();

		scanf(" %s", input);

		//options for client requests
		/*
		2. CONNECTED - GET  Usuarios conectados
		3. STATE     - POST Cambio de estado
		4. MESSAGE   - POST Mensajes privados y broadcast
		5. INFO      - GET  Informacion de un usuario en particular
		*/
		if(strcmp(input, "CONNECTED") == 0){
			//protobuf user request creation
			chat::UserRequest* request;

			//set option and users
			client_request.set_option(2);
			request = client_request.mutable_users();

			//set users to "everyone"
			request -> set_user("everyone");

		}
		if(strcmp(input, "STATUS") == 0){
			//	protobuf change status object
			chat::ChangeStatus* status;

			//	set object and mutable change status
			client_request.set_option(3);
			status = client_request.mutable_change();

			//	set username
			status -> set_username(USERNAME);

			//	read from input and set status
			scanf("%s%*c", input);
			status -> set_status(input);

		}
		if(strcmp(input, "MESSAGE") == 0){
			//protobuf message communication object
			chat::MessageCommunication* message;

			//set object and mutable message communication
			client_request.set_option(4);
			message = client_request.mutable_messagecommunication();

			//set username
			message -> set_sender(USERNAME);
			
			//read from input and set recipient
			scanf("%s", input);
			message -> set_recipient(input);
			
			//read from input and set message to be sent
			scanf("%[^\n]s", input);
			message -> set_message(input);
		}
		if(strcmp(input, "INFO") == 0){
			//	protobuf user request object
			chat::UserRequest* request;

			//	set option and users
			client_request.set_option(5);
			request = client_request.mutable_users();

			//	set username to be sent
			scanf("%s", input);
			request -> set_user(input);

		}
		if(strcmp(input, "HELP") == 0){
			print_help();

		}
		if(strcmp(input, "EXIT") == 0){
			//close socket
			printf("Conexion terminada ............\n");
			close(client_socket);

			return 0;

		}

		
		//serialize the client request 
		size_t size = client_request.ByteSizeLong();
		void *buffer = malloc(size);

		client_request.SerializeToArray(buffer, size);

		//send client request
		send(client_socket, buffer, size, 0);

	}

	//good protobuf practices
	google::protobuf::ShutdownProtobufLibrary();
}

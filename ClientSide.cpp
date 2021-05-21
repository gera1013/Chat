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

using namespace std;

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
			int i;
			cout << "\nLISTADO DE USUARIOS CONECTADOS";
			for(i = 0; i < user_list.connectedusers_size(); i++)
			{
				chat::UserInfo user = user_list.connectedusers(i);

				cout << "\n* Username -> " << user.username() << endl;
				cout << "* Status   -> " << user.status() << endl;
				cout << "* IP       -> " << user.ip() << endl;
			}

			continue;

		}
		if(server_response.option() == 3){
			// printf("POST Cambio de estado");

			chat::ChangeStatus status;

			status = server_response.changestatus();

			cout << "CAMBIO DE STATUS " << status.username() << ": " << status.status() << endl;

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

			//print del mensaje segun sender
			if(strcmp(char_recipient, "everyone") == 0)
			{
				cout << "[TODOS] " << message.sender() << ":" << message.message() << endl;
			}
			else
			{
				cout << "[PRIVADO] " << message.sender() << ":" << message.message() << endl;
			}

			continue;

		}
		if(server_response.option() == 5){
			// printf("GET Usuario especifico");

			chat::UserInfoRequest user;

			user = server_response.userrequest();

			cout << "\nINFORMACIÓN DE USUARIO";

			int i;
			for(i = 0; i < user.connectedusers_size(); i++)
			{
				chat::UserInfo user_detail = user_info.connectedusers(i);

				cout << "\n* Username -> " << user_detail.username() << endl;
				cout << "* Status 		-> " << user_detail.status() << endl;
				cout << "* IP					-> " << user_detail.ip() << endl;
			}

			continue;

		}
	}

}

//Main function para el programa
int main(int argc, char* argv[]){

	//good protobuf practices
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	//get user data from command line
	char* USERNAME = argv[1];
	char* IP = argv[2];
	uint16_t PORT = atoi(argv[3]);

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

		printf("Conexion establecida ............\n");

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


	//loop for client requests
	while(1){

		//client request
		chat::ClientPetition client_request;

		//client input
		char input[1024];
		scanf("%s",input);


		//options for client requests
		/*
		2. CONNECTED - GET  Usuarios conectados
		3. STATE     - POST Cambio de estado
		4. MESSAGE   - POST Mensajes privados y broadcast
		5. INFO      - GET  Informacion de un usuario en particular
		*/

		//	GET ALL CONNECTED USERS
		if(strcmp(input, "CONNECTED") == 0){
			//protobuf user request creation
			chat::UserRequest* request;

			//set option and users
			client_request.set_option(2);
			request = client_request.mutable_users();

			//set users to "everyone"
			request -> set_user("everyone");

		}

		//	POST A CHANGE OD STATUS
		if(strcmp(input, "STATE") == 0){
			// printf("POST Cambio de estado actual");

			//	protobuf change status object
			chat::ChangeStatus* status;

			//	set object and mutable change status
			client_request.set_option(3);
			status = client_request.mutable_changestatus();

			//	set username
			status -> set_username(USERNAME);

			//	read from input and set status to be sent
			printf("Ingrese su nuevo status");
			scanf("%s", input);
			status -> set_status(input);
		}

		//	POST A PRIVATE OR A BROADCAST MESSAGE
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

		//	GET A USER'S INFO
		if(strcmp(input, "INFO") == 0){
			// printf("GET Usuario especifico conectado");
			//send(client_socket, input, 1024, 0);

			//	protobuf user request object
			chat::UserRequest* request;

			//	set option and users
			client_request.set_option(5);
			request = mutable_request.mutable_users();

			//	read from input and set username to be sent
			printf("Ingrese el usuario del que desea obtener información");
			scanf("%s", input);

			request -> set_user(input);
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

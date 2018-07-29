#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(char *msg)
{
	perror(msg);
	exit(0);
}

void login(int sockfd)
{
	char buffer[256];
	int n;

	n = read(sockfd,buffer,255);
	if(n < 0)
		error("ERROR reading from socket");

	if(buffer[0] == '2')
	{
		error("Max limit for client requests reached\n");
	}

	printf("Please enter the Username: ");
	bzero(buffer,256);
	fgets(buffer,255,stdin);
	buffer[strlen(buffer) - 1] = '\0';

	n = write(sockfd,buffer,strlen(buffer));

	if (n < 0)
	{
		error("ERROR writing to socket");
	}
	bzero(buffer,256);

	n = read(sockfd,buffer,255);
	
	if (n < 0)
	{
		error("ERROR reading from socket");
	}

	if(buffer[0] == '0')
	{
		error("invalid username\n");
	}
	else if(buffer[0] == '3')
		error("This trader already logged in\n");	
	else
	{
		printf("Please enter the Password: ");
		bzero(buffer,256);
		fgets(buffer,255,stdin);

		buffer[strlen(buffer) - 1] = '\0';
		n = write(sockfd,buffer,strlen(buffer));

		if (n < 0)
		{
			error("ERROR writing to socket");
		}

		bzero(buffer,256);
		n = read(sockfd,buffer,255);
		if (n < 0)
		{
			error("ERROR reading from socket");
		}

		if(buffer[0] == '0')
			error("invalid password\n");
	}
}

void send_request(int x, int sockfd){
	int n;
	int item_code, qty, price;
	printf("enter item code, quantity, price (separated by space):\n ");
	scanf("%d %d %d",&item_code, &qty, &price);
	if(item_code < 1 || item_code > 10){
		printf("invalid item code\n");
	}
	else if(qty <= 0 || price < 0){
		printf("invalid qty and price\n");
	}
	else{
		char send_msg[20];
		sprintf(send_msg,"%d %d %d %d",x,item_code,qty,price);
		n = write(sockfd,send_msg, strlen(send_msg));
		if(n < 0){
			error("error in writing to socket");
		}
	}
}

void view_status(int x,int sockfd)
{
	int n;
	char buffer[1024];
	if(x == 3)
		n = write(sockfd,"3",1);
	else
		n = write(sockfd,"4",1);
	if(n < 0)
	{
		error("error in writing to socket");
	}

	bzero(buffer,1024);
	n = read(sockfd,buffer,1023);
	if(n < 0)
		error("error reading from socket");
	printf("%s",buffer);
} 

int main(int argc, char *argv[])
{
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	char buffer[256];

	if (argc < 3)
	{
		fprintf(stderr,"usage %s hostname port\n", argv[0]);
		exit(0);
	}

	portno = atoi(argv[2]);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		error("ERROR opening socket");
	}

	server = gethostbyname(argv[1]);

	if(server == NULL)
	{
		fprintf(stderr, "ERROR, no such host\n");
	}

	bzero((char *) &serv_addr, sizeof(serv_addr) );
	serv_addr.sin_family = AF_INET;

	bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);

	serv_addr.sin_port = htons(portno);

	if (connect(sockfd,&serv_addr,sizeof(serv_addr)) < 0)
	{
		error("ERROR connecting");
	} 

	login(sockfd);
	
	printf("Login Done\n");
	printf("To send a buy request press 1\n");
	printf("To send a sell request press 2\n");
	printf("To view order status press 3\n");
	printf("To view your trade status press 4\n");
	
	char c;
	while(1)
	{	
		printf("enter your option: ");
		c = getchar();
		getchar();
		switch(c){
			case '1':
				send_request(1,sockfd);
				getchar();
				break;
			case '2':
				send_request(2,sockfd);
				getchar();
				break;

			case '3':
				view_status(3,sockfd);
				break;

			case '4':
				view_status(4,sockfd);
				break;
			default:
				printf("enter valid option press ctrl-c to exit\n");
		}
	}
	return 0;
}
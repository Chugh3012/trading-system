/* Creates a datagram server. The port number is passed as an argument. This server runs forever */
#include <stdio.h> 
#include <string.h>   //strlen 
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h>   //close 
#include <arpa/inet.h>    //close 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros

struct node
{
	int trader_id;
	int quantity;
	int price;
	struct node* next;
};

struct trade_data
{
	int item_no;
	struct node* buy_request;
	struct node* sell_request; 
};

char* file_names[] = {"item1.txt","item2.txt","item3.txt","item4.txt","item5.txt","item6.txt",
					"item7.txt","item8.txt","item9.txt","item10.txt"};

char* trade_files[] = {"trader1.txt","trader2.txt","trader3.txt","trader4.txt","trader5.txt"};

struct trade_data trade_array[10];

int trader_id_sd[5] = {-1,-1,-1,-1,-1}; 

struct node* getNode(int quantity, int price,int trader_id)
{
	struct node * new_node = (struct node*) malloc(sizeof(struct node));
	new_node->next = NULL;
	new_node->trader_id = trader_id;
	new_node->quantity = quantity;
	new_node->price = price;
	return new_node;
}

void print_list(struct node * head)
{
	while(head != NULL)
	{
		printf("%d,%d ",head->quantity, head->price);
		head = head->next;
	}
	printf("\n");
}

void view_trade_status(int sd,int trader_id)
{
	int n;
	FILE* fp = fopen(trade_files[trader_id],"r");
	char* buffer = (char*)malloc(40*sizeof(char));
	char* order_str = (char*)malloc(1024*sizeof(char));
	strcpy(order_str,"\ntrade-type(item_no) qty ,price - trader_id ,qty ,price\n");
	while(fgets(buffer,39,fp) != NULL)
	{
		strcat(order_str,buffer);
	}
	n = write(sd,order_str,strlen(order_str));
	if(n < 0)
		perror("Error writing to socket");
	fclose(fp);
}

void view_order_status(int sd)
{
	struct node* head;
	int i=0,n;
	char* buffer = (char*)malloc(30*sizeof(char));
	char* order_str = (char*)malloc(1024*sizeof(char));
	strcpy(order_str,"");
	int quantity,price;
	for(i=0;i<10;i++)
	{
		sprintf(buffer,"\n%d:\n",i+1);
		strcat(order_str,buffer);
		head = trade_array[i].buy_request;
		if(head != NULL)
		{
			quantity = head->quantity;
			price = head->price;
			while(head->next && head->next->price == price)
			{
				head = head->next;
				quantity+=head->quantity;
			}
			sprintf(buffer,"buy: %d @ %d\n",quantity,price);
			strcat(order_str,buffer);
		}
		else
		{
			sprintf(buffer,"buy: \n");
			strcat(order_str,buffer);
		}

		head = trade_array[i].sell_request;
		if(head != NULL)
		{
			quantity = head->quantity;
			price = head->price;
			while(head->next && head->next->price == price)
			{
				head = head->next;
				quantity+=head->quantity;
			}
			sprintf(buffer,"sell: %d @ %d\n",quantity,price);
			strcat(order_str,buffer);
		}
		else
		{
			sprintf(buffer,"sell: \n");
			strcat(order_str,buffer);
		}
	}
	n = write(sd,order_str,strlen(order_str));
	if(n < 0)
		perror("Error writing to socket");
}

void insert_into_buy_queue(int item_no,int quantity,int price,int trader_id)
{
	struct node* newnode = getNode(quantity,price,trader_id);
	struct node *temp = trade_array[item_no-1].buy_request;
	if(temp == NULL){
		trade_array[item_no-1].buy_request = newnode;
	}
	else{
		if(temp->price < price){
			newnode->next = temp;
			trade_array[item_no-1].buy_request = newnode;
			return;
		}
		while(temp->next!=NULL){
			
			if(temp->next->price < price){
				newnode->next = temp->next;
				temp->next = newnode;
				break;
			}

			temp = temp->next;
		}
		if(temp->next == NULL)
		{
			temp->next = newnode;
		}
	}
}

void insert_into_sell_queue(int item_no,int quantity,int price,int trader_id)
{
	struct node* newnode = getNode(quantity,price,trader_id);	
	struct node *temp = trade_array[item_no-1].sell_request;
	if(temp == NULL){
		trade_array[item_no-1].sell_request = newnode;
	}
	else{
		if(temp->price > price){
			newnode->next = temp;
			trade_array[item_no-1].sell_request = newnode;
			return;
		}
		while(temp->next!=NULL){
			
			if(temp->next->price > price){
				newnode->next = temp->next;
				temp->next = newnode;
				break;
			}

			temp = temp->next;
		}
		if(temp->next == NULL){
			temp->next = newnode;
		}
	}
}

void update_file(int fileindex)
{
	FILE* fp = fopen(file_names[fileindex],"w");
	struct node* buy_head = trade_array[fileindex].buy_request;
	struct node* sell_head = trade_array[fileindex].sell_request;

	if(fp)
	{
		fprintf(fp,"buy");
		while(buy_head != NULL)
		{
			fprintf(fp," %d,%d,%d",buy_head->trader_id,buy_head->quantity,buy_head->price);
			buy_head = buy_head->next;
		}
		fprintf(fp,"\nsell");
		while(sell_head != NULL)
		{
			fprintf(fp," %d,%d,%d",sell_head->trader_id,sell_head->quantity,sell_head->price);
			sell_head = sell_head->next;
		}
		fclose(fp);
	}
	else
		printf("File %s cannot be opened\n",file_names[fileindex]);
}

void initialise()
{
	int i=0;
	FILE* fp;
	char* type = (char*)malloc(10*sizeof(char));
	char c;
	int quantity,price,trader_id;
	for(i=0;i<10;i++)
	{
		fp = fopen(file_names[i],"r");
		if(fp)
		{
			if(fscanf(fp,"%s",type) != EOF)
			{
				if(strcmp(type,"buy") == 0)
				{
					c = fgetc(fp);
					while(c != '\n' && fscanf(fp,"%d,%d,%d",&trader_id,&quantity,&price) != EOF)
					{
						insert_into_buy_queue(i+1,quantity,price,trader_id);
						c = fgetc(fp);
					}
				}
				fscanf(fp,"%s",type);
				if(strcmp(type,"sell") == 0)
				{
					c = fgetc(fp);
					while(c != '\n' && fscanf(fp,"%d,%d,%d",&trader_id,&quantity,&price) != EOF)
					{
						insert_into_sell_queue(i+1,quantity,price,trader_id);
						c = fgetc(fp);
					}
				}
			}
			fclose(fp);
		}
		else
			printf("File %s does not exist\n",file_names[i]);
	}
}

void handle_buy_request(int item_no,int quantity,int price,int trader_id)
{
	FILE* gp;
	struct node* temp = trade_array[item_no-1].sell_request;
	FILE* fp = fopen(trade_files[trader_id],"a");
	while(quantity > 0)
	{
		temp = trade_array[item_no-1].sell_request;
		if(temp != NULL)
		{
			if(temp->price <= price)
			{
				gp = fopen(trade_files[temp->trader_id],"a");
				if(temp->quantity <= quantity)
				{

					fprintf(fp,"buy%d: %d,%d %d,%d,%d\n",item_no,quantity,price,temp->trader_id+1,temp->quantity,temp->price);
					fprintf(gp,"sell%d: %d,%d %d,%d,%d\n",item_no,temp->quantity,temp->price,trader_id+1,quantity,price);
					
					quantity = quantity - temp->quantity;
					trade_array[item_no-1].sell_request = temp->next;
					free(temp);
				}
				else
				{
					fprintf(fp,"buy%d: %d,%d %d,%d,%d\n",item_no,quantity,price,temp->trader_id+1,temp->quantity,temp->price);
					fprintf(gp,"sell%d: %d,%d %d,%d,%d\n",item_no,temp->quantity,temp->price,trader_id+1,quantity,price);
					
					temp->quantity = temp->quantity - quantity;
					quantity = 0;
				}
				fclose(gp);
				continue;
			}
			else
			{
				insert_into_buy_queue(item_no,quantity,price,trader_id);
				break;
			}
		}
		else
		{
			insert_into_buy_queue(item_no,quantity,price,trader_id);
			break;
		}
	}
	fclose(fp);
	update_file(item_no-1);
}

void handle_sell_request(int item_no,int quantity,int price,int trader_id)
{
	FILE* gp;
	struct node* temp = trade_array[item_no-1].buy_request;
	FILE* fp = fopen(trade_files[trader_id],"a");
	while(quantity > 0)
	{
		temp = trade_array[item_no-1].buy_request;
		if(temp != NULL)
		{
			if(temp->price >= price)
			{
				gp = fopen(trade_files[temp->trader_id],"a");

				if(temp->quantity <= quantity)
				{
					fprintf(fp,"sell%d: %d,%d %d,%d,%d\n",item_no,quantity,price,temp->trader_id+1,temp->quantity,temp->price);
					fprintf(gp,"buy%d: %d,%d %d,%d,%d\n",item_no,temp->quantity,temp->price,trader_id+1,quantity,price);
					quantity = quantity - temp->quantity;
					trade_array[item_no-1].buy_request = temp->next;
					free(temp);
				}
				else
				{
					fprintf(fp,"sell%d: %d,%d %d,%d,%d\n",item_no,quantity,price,temp->trader_id+1,temp->quantity,temp->price);
					fprintf(gp,"buy%d: %d,%d %d,%d,%d\n",item_no,temp->quantity,temp->price,trader_id+1,quantity,price);
					temp->quantity = temp->quantity - quantity;
					quantity = 0;
				}
				fclose(gp);
				continue;
			}
			else
			{
				insert_into_sell_queue(item_no,quantity,price,trader_id);
				break;
			}
		}
		else
		{
			insert_into_sell_queue(item_no,quantity,price,trader_id);
			break;
		}
	}
	fclose(fp);
	update_file(item_no-1);
}

void error(char *msg)
{
	perror(msg);
	exit(1);
}

int verifyLogin(int newsockfd,char* LoginDetails[10])
{
	int n,i;
	char buffer[256];
	bzero(buffer,256);
	char* id = (char*) malloc(64*sizeof(char));
	char* password = (char*) malloc(64*sizeof(char));
	int flag=0;

	n = read(newsockfd,buffer,255);
	if (n < 0)
	{
		error("ERROR reading from socket");
	}

	strcpy(id,buffer);
	for(i=0;i<10;i+=2)
	{
		if(strcmp(id,LoginDetails[i]) == 0)
		{
			if(trader_id_sd[i/2] != -1)
			{
				n = write(newsockfd,"3",1);
				if(n < 0)
					perror("error writing to socket");
				return 0;
			}
			else
			{
				flag = 1;
				strcpy(password,LoginDetails[i+1]);
				break;
			}
		}
	}
	if(flag == 0)
	{
		n = write(newsockfd,"0",1);

		if (n < 0)
		{
			error("ERROR writing to socket");
		}	
		return 0;
	}	
	else
	{
		n = write(newsockfd,"1",1);

		if (n < 0)
		{
			error("ERROR writing to socket");
		}	
		bzero(buffer,256);
		n = read(newsockfd,buffer,255);

		if (n < 0)
		{
			error("ERROR reading from socket");
		}
		
		if(strcmp(buffer,password) != 0)
		{
			n = write(newsockfd,"0",1);

			if (n < 0)
			{
				error("ERROR writing to socket");
			}	
			return 0; 
		}
		else
		{
			n = write(newsockfd,"1",1);

			if (n < 0)
			{
				error("ERROR writing to socket");
			}
			trader_id_sd[i/2] = newsockfd;
			return 1;	
		}
	}
}

void getLoginDetails(char *LoginDetails[10])
{
	FILE *fp;
	int i=0;
	fp = fopen("pass.txt","r");
	char *id, *password;
	id = (char*) malloc(64*sizeof(char));
	password = (char*) malloc(64*sizeof(char));

	if(!fp)
	{
		printf("password file does not exist\n");
		return;
	}

	while(fscanf(fp,"%[^,],%s\n",id,password) != EOF)
	{
		strcpy(LoginDetails[i++],id);
		strcpy(LoginDetails[i++],password);
	}
	fclose(fp);
}

int main(int argc, char *argv[])
{
	initialise();
	int opt = 1;
	int portno, clilen;
	struct sockaddr_in serv_addr, cli_addr;
	int master_socket,serv_len;
	int client_socket[5],max_clients = 5, activity,sd, new_socket;
	int max_sd;

	int login_done = 0;
	char buffer[256];

	int n,i,j;

	fd_set readfds;

	for (i = 0; i < max_clients; i++)  
    {  
        client_socket[i] = 0;  
    }	

	if (argc < 2)
	{
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}

	master_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (master_socket < 0)
	{
		error("ERROR opening socket");
	}

	//set master socket to allow multiple connections , 
    //this is just a good habit, it will work without this 
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )  
    {  
        error("setsockopt");    
    }  

	bzero((char *) &serv_addr, sizeof(serv_addr));

	portno = atoi(argv[1]);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(master_socket, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
	{
		error("ERROR on binding");
	}

	printf("Listener on port %d \n", portno);

	char* LoginDetails[10];
	
	for (i = 0; i < 10; ++i)
	{
		LoginDetails[i] = (char *) malloc(64*sizeof(char));
	}
	
	getLoginDetails(LoginDetails);

	if (listen(master_socket, 3) < 0)  
    {  
        error("listen");  
    }

    //accept the incoming connection 
    serv_len = sizeof(serv_addr);  
    puts("Waiting for connections ...");

    while(1)
    {
    	//clear the socket set 
        FD_ZERO(&readfds);  
    
        //add master socket to set 
        FD_SET(master_socket, &readfds);  
        max_sd = master_socket;  
            
        //add child sockets to set 
        for ( i = 0 ; i < max_clients ; i++)  
        {  
            //socket descriptor 
            sd = client_socket[i];  
                
            //if valid socket descriptor then add to read list 
            if(sd > 0)  
                FD_SET( sd , &readfds);  
                
            //highest file descriptor number, need it for the select function 
            if(sd > max_sd)  
                max_sd = sd;  
        }

        //wait for an activity on one of the sockets , timeout is NULL , 
        //so wait indefinitely 
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);  
      
        if ((activity < 0) && (errno!=EINTR))  
        {  
            printf("select error");  
        }  

        //If something happened on the master socket , 
        //then its an incoming connection 

        if (FD_ISSET(master_socket, &readfds))  
        {  
            if ((new_socket = accept(master_socket,(struct sockaddr *)&cli_addr, (socklen_t*)&clilen))<0)  
            {  
                error("accept");  
            }  
            //inform user of socket number - used in send and receive commands 
            printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_socket , inet_ntoa(cli_addr.sin_addr) , ntohs(cli_addr.sin_port));  
                        
            //add new socket to array of sockets 
            for (i = 0; i < max_clients; i++)  
            {  
                //if position is empty 
                if( client_socket[i] == 0)  
                {  
                	n = write(new_socket,"1",1);
                	if(n < 0)
                		perror("error in writing\n");

                	login_done = verifyLogin(new_socket,LoginDetails);
					if(!login_done)
					{
						n = close(new_socket);
						if (n < 0) 
				        	perror("socket cannot be closed");
					}
					else
					{
						printf("Login Done\n");
						client_socket[i] = new_socket;  
                    	printf("Adding to list of sockets as %d\n" , i);
					}    
                    break;  
                }  
            } 
            if(i == 5)
            {
            	n = write(new_socket,"2",1);
            	if(n < 0)
            		perror("Error writing\n");
            	n = close(new_socket);
				if (n < 0) 
		        	perror("socket cannot be closed");
            }
        }

        //else its some IO operation on some other socket
        for (i = 0; i < max_clients; i++)  
        {  
            sd = client_socket[i];  
                
            if (FD_ISSET( sd , &readfds))  
            {  
                //Check if it was for closing , and also read the 
                //incoming message 
				bzero(buffer,256);
                if ((n = read( sd , buffer, 256)) == 0)  
                {  
                    //Somebody disconnected , get his details and print 
                    getpeername(sd , (struct sockaddr*)&cli_addr,(socklen_t*)&cli_addr);  
                   
                    printf("Host disconnected , ip %s , port %d \n",inet_ntoa(cli_addr.sin_addr) , ntohs(cli_addr.sin_port));  
                        
                    //Close the socket and mark as 0 in list for reuse 
                    close(sd);  
                    for(j = 0;j < 5;j++)
                    {
                    	if(trader_id_sd[j] == sd)
                    		trader_id_sd[j] = -1;
                    }
                    client_socket[i] = 0;  
                }  
                    
                //Echo back the message that came in 
                else
                {  
                    //set the string terminating NULL byte on the end 
                    //of the data read 
                    int trader_id;
                    for(j = 0;j < 5;j++)
                    {
                    	if(trader_id_sd[j] == sd){
                    		trader_id = j;
                    		break;
                    	}
                    }
                    int option,item_no,quantity,price;
                    buffer[n] = '\0';  
                    switch(buffer[0]){
                    	case '1':
                    		sscanf(buffer,"%d %d %d %d",&option,&item_no,&quantity,&price);
                    		handle_buy_request(item_no,quantity,price,trader_id);		
                    		break;
                    	case '2':
                    		sscanf(buffer,"%d %d %d %d",&option,&item_no,&quantity,&price);
                    		handle_sell_request(item_no,quantity,price,trader_id);
                    		break;
                    	case '3':
                    		view_order_status(sd);
                    		break;
                		case '4':
                			view_trade_status(sd,trader_id);
                			break;
                    	default:
                    		break;	
                    }
                }  
            }
        } 
    }
	return 0;
}
/* Program for Client*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h> 
#include <sys/stat.h>
#include <fcntl.h>

/* Length of variable*/
#define PASSWORD_LEN 16
#define USERNAME_LEN 20
#define PORT_NO 5000
#define ADMIN_ACC_TYPE 0
#define NORMAL_ACC_TYPE 1
#define AGENT_ACC_TYPE 2
#define NORMAL 1
#define CANCEL 0

void ERR_EXIT(const char * msg) {perror(msg);exit(EXIT_FAILURE);}

typedef struct customer{
	int acc_no;
	int type;
	char cust_username[USERNAME_LEN];
	char cust_password[PASSWORD_LEN];
	int status;
}struct_customer;

static struct_customer CURRENT_USER;

/*	Implementation of login functionality.
	@return
	0 on failure, 
	1 on success for NORMAL/AGENT account,
	2 on success for ADMIN ACCOUNT.
*/
int login(int conn_fd){
	int choice=0;int response_flg;
	int acc_no;char username[USERNAME_LEN];char password[PASSWORD_LEN];
	
	int count=3;int fd;
	while(count>0){
		if(count!=3) printf("%d more try remaining\n",count);
		if(write(conn_fd, &choice, sizeof(int)) == -1)	ERR_EXIT("write()");

		printf("Account No:");scanf("%d",&acc_no);
		printf("Username :");scanf("%s",username);
    	printf("Password:");scanf("%s",password);

    	struct_customer input={acc_no,-1,"","",1};
    	strcpy(input.cust_username,username);
    	strcpy(input.cust_password,password);

    	//input = htonl(input);
		if(write(conn_fd, &input, 1*sizeof(struct_customer)) == -1)	ERR_EXIT("write()");

		if(read(conn_fd, &response_flg, sizeof(int))==-1)	ERR_EXIT("read()");
		
		if(response_flg==3){
			printf("Already logged in from different session. Only 1 login per user.\n");
			return 0;
		}
		if(response_flg!=0){
			CURRENT_USER=input;
			return response_flg;
		}
		count--;
	}
    
    return 0;
}

int menu_user(int conn_fd){
	printf("1. Book Ticket\n2. View Booking History\n3. Cancel Booking\n4. Exit\n\n");
	int choice;
	scanf("%d",&choice);
    	switch(choice){
    		case 1:
    			break;
    		case 2:
    			break;
    		case 3:
    			break;
    		case 4:
    			if(write(conn_fd, &choice, sizeof(int)) == -1)	ERR_EXIT("write()");
    			//if(write(conn_fd, &CURRENT_USER, 1*sizeof(struct_customer)) == -1)	ERR_EXIT("write()");
    			close(conn_fd);
    			return 0;
    		default:
    			printf("Invalid choice.Enter again\n");
    			sleep(2);
    			break;
    	}
    return 1;
}


int menu_admin(int conn_fd){
	printf("1. Add User\n2. Search User \n3. Delete User\n4. Add Train\n5. Search Train \n6. Delete Train\n7. Exit\n\n");
	int choice;
	scanf("%d",&choice);
    switch(choice){
    	case 1:
    		break;
   		case 2:
   			break;
   		case 3:
   			break;
    	case 4:
    		break;
    	case 5:
    		break;
    	case 6:
    		break;
    	case 7:
    		if(write(conn_fd, &choice, sizeof(int)) == -1)	ERR_EXIT("write()");
    		close(conn_fd);
   			return 0; 			
    	default:
    		printf("Invalid choice.Enter again\n");
    		sleep(2);
    		break;
    	}
    return 1;
}

int main(int argc,char* argv[])
{
    int socket_fd;int response_flg=0;int choice=10;
    ssize_t n;
    char recvBuff[1024];
    struct sockaddr_in server_addr; 
    
    if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        ERR_EXIT("socket()");

    memset(&server_addr, '0', sizeof(server_addr)); 
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT_NO); 

    if(inet_pton(AF_INET, "0.0.0.0", &server_addr.sin_addr) <= 0)
        ERR_EXIT("inet_pton()"); 
    if(connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        ERR_EXIT("connect()");

    response_flg=login(socket_fd);
    if(response_flg==0){
    	if(write(socket_fd, &choice, sizeof(int)) == -1)	ERR_EXIT("write()");	
    }
    
    printf("Inside main %d",response_flg);
    while(response_flg){
    	system("clear");

    	printf("***********************************************\n");
    	printf("	   WELCOME TO RAILWAY BOOKING SYSTEM       \n");
    	printf("***********************************************\n\n");
    	if(response_flg!=2)//not an ADMIN account
    		response_flg=menu_user(socket_fd);
    	else
    		response_flg=menu_admin(socket_fd);
    }
}


/*int fd = open("customer",O_CREAT | O_RDWR,0744);
	init(fd);
	readfile();
void init(int fd) {
	struct_customer customer;
	customer.acc_no=1;
	customer.type=ADMIN_ACC_TYPE;
	customer.status=NORMAL;
	strcpy(customer.cust_username,"admin");
	strcpy(customer.cust_password,"admin1234");
	write(fd, &customer, 1*sizeof(customer));

	lseek(fd, 1*sizeof(struct_customer), SEEK_SET);
	customer.acc_no=2;
	customer.type=NORMAL_ACC_TYPE;
	customer.status=NORMAL;
	strcpy(customer.cust_username,"utsav");
	strcpy(customer.cust_password,"utsav1234");
	write(fd, &customer, 1*sizeof(customer));

	lseek(fd, 2*sizeof(struct_customer), SEEK_SET);
	customer.acc_no=3;
	customer.type=AGENT_ACC_TYPE;
	customer.status=NORMAL;
	strcpy(customer.cust_username,"agent");
	strcpy(customer.cust_password,"agent1234");
	write(fd, &customer, 1*sizeof(customer));
}
void readfile(){
	int fd;
	if((fd = open("customer", O_RDONLY))==-1)   ERR_EXIT("open()");
    struct_customer actual_cust;
    int pos=0;
    while(read(fd,&actual_cust,(pos+1)*sizeof(struct_customer))>0){
    	printf("pos:%d %d %d %s %s %d\n",pos,actual_cust.acc_no,actual_cust.type ,actual_cust.cust_username,actual_cust.cust_password,actual_cust.status);   //DELETE
    	//pos++;
    	//lseek(fd, pos*sizeof(struct_customer), SEEK_SET);
    	sleep(1);
    }
    
}*/

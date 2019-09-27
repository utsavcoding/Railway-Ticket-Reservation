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
#define TRNAME_LEN 30
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

typedef struct train{
	int trn_no;
	char trn_name[TRNAME_LEN];
	int trn_avl_seats;
	int trn_book_seats;
	int status;
}struct_train;

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
	int choice;int option;
	scanf("%d",&choice);
    	switch(choice){
    		case 1:
    			break;
    		case 2:
    			break;
    		case 3:
    			break;
    		case 4:
    			option=htonl(choice);
    			if(write(conn_fd, &option, sizeof(int)) == -1)	ERR_EXIT("write()");
    			if(write(conn_fd, &CURRENT_USER, 1*sizeof(struct_customer)) == -1)	ERR_EXIT("write()");
    			close(conn_fd);
    			return 0;
    		default:
    			printf("Invalid choice.Enter again\n");
    			sleep(2);
    			break;
    	}
    return 1;
}

struct_customer get_customer(){
	printf("Inside get_customer\n");
	struct_customer new;
	printf("Enter account type(0 for ADMIN, 1 for NORMAL, 2 for AGENT) : ");
	scanf("%d",&new.type);
	printf("Enter username :");
	scanf("%s",new.cust_username);
	printf("Enter password :");
	scanf("%s",new.cust_password);
	new.status=NORMAL;
	return new;
}

void display_customer(struct_customer customer){
	printf(" \t%d\t \t%d\t \t%s\t\t \t%s\t\t \t%d\n",customer.acc_no,customer.type,customer.cust_username,customer.cust_password,customer.status);
}

void add_customer(int conn_fd){
	//printf("Inside add_customer\n");  //DELETE
	struct_customer new=get_customer();struct_customer response;
	if(write(conn_fd, &new, 1*sizeof(struct_customer)) == -1)	ERR_EXIT("write()");
	if(read(conn_fd, &response, sizeof(struct_customer))==-1)	ERR_EXIT("read()");
	printf("User has been added with following details:\n\n");
	printf("      Account No |    Account Type |    Account Username   |  Account Password   \t|    Account Status \t\n");
	display_customer(response);	
}

void show_all_customer(int conn_fd){
	struct_customer response;int moreflg;
	if(read(conn_fd, &moreflg, sizeof(int))==-1)	ERR_EXIT("read()");
	moreflg=ntohl(moreflg);
	printf("      Account No |    Account Type |    Account Username   |  Account Password   \t|    Account Status \t\n");
	while(moreflg){
		if(read(conn_fd, &response, sizeof(struct_customer))==-1)	ERR_EXIT("read()");
		display_customer(response);
		if(read(conn_fd, &moreflg, sizeof(int))==-1)	ERR_EXIT("read()");
		moreflg=ntohl(moreflg);
	}
}

void search_user(int conn_fd){
	int accno;struct_customer response;int presentflg=0;

	printf("Enter Account no: ");
	scanf("%d",&accno);
	accno=htonl(accno);
	if(write(conn_fd, &accno, 1*sizeof(int)) == -1)	ERR_EXIT("write()");

	if(read(conn_fd, &presentflg, 1*sizeof(int)) == -1)	ERR_EXIT("write()");
	
	presentflg=ntohl(presentflg);
	if(presentflg){
		if(read(conn_fd, &response, sizeof(struct_customer))==-1)	ERR_EXIT("read()");
		printf("      Account No |    Account Type |    Account Username   |  Account Password   \t|    Account Status \t\n");
		display_customer(response);
		return;
	}

	printf("No such record with this account no!!\n\n");
}

void delete_user(int conn_fd){
	int accno;struct_customer response;int presentflg=0;

	printf("Enter Account no: ");
	scanf("%d",&accno);
	accno=htonl(accno);
	if(write(conn_fd, &accno, 1*sizeof(int)) == -1)	ERR_EXIT("write()");

	if(read(conn_fd, &presentflg, 1*sizeof(int)) == -1)	ERR_EXIT("write()");
	
	presentflg=ntohl(presentflg);
	if(presentflg){
		printf("Account has been deleted.\n\n");
		return;
	}

	printf("No such record with this account no!!\n\n");	
}

struct_train get_train(){
	struct_train new;char train_name[30];
	printf("Enter train name : ");
	scanf(" %[^\n]",train_name);
	strcpy(new.trn_name,train_name);
	printf("Enter total number of seats :");
	scanf("%d",&new.trn_avl_seats);
	new.trn_book_seats=0;
	new.status=NORMAL;
	return new;
}

void display_train(struct_train train){
	printf(" \t%d \t%s\t \t%d\t\t \t%d\t \t%d\n",train.trn_no,train.trn_name,train.trn_avl_seats,train.trn_book_seats,train.status);
}

void add_train(int conn_fd){
	//printf("Inside add_customer\n");  //DELETE
	struct_train new=get_train();struct_train response;
	if(write(conn_fd, &new, 1*sizeof(struct_train)) == -1)	ERR_EXIT("write()");
	if(read(conn_fd, &response, sizeof(struct_train))==-1)	ERR_EXIT("read()");
	printf("Train has been added with following details:\n\n");
	printf("     Train No |    Train Name\t     |    Available Seats   |     Booked Seats   |    Status \t\n");
	display_train(response);	
}

void show_all_train(int conn_fd){
	struct_train response;int moreflg;
	if(read(conn_fd, &moreflg, sizeof(int))==-1)	ERR_EXIT("read()");
	moreflg=ntohl(moreflg);
	printf("     Train No |    Train Name\t     |    Available Seats   |     Booked Seats   |    Status \t\n");
	while(moreflg){
		if(read(conn_fd, &response, sizeof(struct_train))==-1)	ERR_EXIT("read()");
		display_train(response);
		if(read(conn_fd, &moreflg, sizeof(int))==-1)	ERR_EXIT("read()");
		moreflg=ntohl(moreflg);
	}
}

void search_train(int conn_fd){
	int trn_no;struct_train response;int presentflg=0;

	printf("Enter Train no: ");
	scanf("%d",&trn_no);
	trn_no=htonl(trn_no);
	if(write(conn_fd, &trn_no, 1*sizeof(int)) == -1)	ERR_EXIT("write()");

	if(read(conn_fd, &presentflg, 1*sizeof(int)) == -1)	ERR_EXIT("write()");
	
	presentflg=ntohl(presentflg);
	if(presentflg){
		if(read(conn_fd, &response, sizeof(struct_train))==-1)	ERR_EXIT("read()");
		printf("     Train No |    Train Name\t     |    Available Seats   |     Booked Seats   |    Status \t\n");
		display_train(response);
		return;
	}

	printf("No such train record with this train no!!\n\n");
}

void delete_train(int conn_fd){
	int trn_no;struct_train response;int presentflg=0;

	printf("Enter Train no: ");
	scanf("%d",&trn_no);
	trn_no=htonl(trn_no);
	if(write(conn_fd, &trn_no, 1*sizeof(int)) == -1)	ERR_EXIT("write()");

	if(read(conn_fd, &presentflg, 1*sizeof(int)) == -1)	ERR_EXIT("write()");
	
	presentflg=ntohl(presentflg);
	if(presentflg){
		printf("Account has been deleted.\n\n");
		return;
	}

	printf("No such train record with this train no!!\n\n");
}

int menu_admin(int conn_fd){
	printf("1. Add User\n2. Search User \n3. Delete User\n4. View Users\n");
	printf("5. Add Train\n6. Search Train \n7. Delete Train\n8. View Trains\n9. Exit\n\n");
	int choice;int option;
	scanf("%d",&choice);
    switch(choice){
    	case 1:
    		option=htonl(choice+4);
			if(write(conn_fd, &option, 1*sizeof(int)) == -1)	ERR_EXIT("write()");    		
    		add_customer(conn_fd);
    		break;
   		case 2:
   			option=htonl(choice+4);
   			if(write(conn_fd, &option, 1*sizeof(int)) == -1)	ERR_EXIT("write()");
   			search_user(conn_fd);
   			break;
   		case 3:
   			option=htonl(choice+4);
   			if(write(conn_fd, &option, 1*sizeof(int)) == -1)	ERR_EXIT("write()");
   			delete_user(conn_fd);
   			break;
    	case 4:
    		option=htonl(choice+4);
			if(write(conn_fd, &option, 1*sizeof(int)) == -1)	ERR_EXIT("write()");
			show_all_customer(conn_fd);
    		break;
    	case 5:
    		option=htonl(choice+4);
			if(write(conn_fd, &option, 1*sizeof(int)) == -1)	ERR_EXIT("write()");    		
    		add_train(conn_fd);
    		break;
    	case 6:
    		option=htonl(choice+4);
   			if(write(conn_fd, &option, 1*sizeof(int)) == -1)	ERR_EXIT("write()");
   			search_train(conn_fd);
    		break;
    	case 7:
    		option=htonl(choice+4);
   			if(write(conn_fd, &option, 1*sizeof(int)) == -1)	ERR_EXIT("write()");
   			delete_train(conn_fd);
    		break;
    	case 8:
    		option=htonl(choice+4);
			if(write(conn_fd, &option, 1*sizeof(int)) == -1)	ERR_EXIT("write()");
			show_all_train(conn_fd);
    		break;
    	case 9:
    		option=htonl(choice+4);
    		if(write(conn_fd, &option, sizeof(int)) == -1)	ERR_EXIT("write()");
    		close(conn_fd);
   			return 0; 			
    	default:
    		printf("Invalid choice.Enter again\n");
    		sleep(2);
    		break;
    	}

    printf("\nPress enter to get back to main menu\n");
    getchar();getchar();
    return 2;
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
    printf("conn_fd: %d\n",socket_fd);
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

void init(int fd) {
	struct_train train;
	train.trn_no=1;
	strcpy(train.trn_name,"JANSHATABDI EXPRESS");
	train.trn_avl_seats=1400;
	train.trn_book_seats=0;
	train.status=NORMAL;
	write(fd, &train, 1*sizeof(train));
}

void readfile(){
	int fd;
	if((fd = open("train", O_RDONLY))==-1)   ERR_EXIT("open()");
    struct_train actual_trn;
    int pos=0;
    while(read(fd,&actual_trn,(pos+1)*sizeof(struct_train))>0){
    	printf("%d %s %d %d %d\n",actual_trn.trn_no,actual_trn.trn_name ,actual_trn.trn_avl_seats,actual_trn.trn_book_seats,actual_trn.status);   //DELETE
    	//pos++;
    	//lseek(fd, pos*sizeof(struct_customer), SEEK_SET);
    	sleep(1);
    }
    close(fd);
}


void init(int fd) {
	struct_train train;
	train.trn_no=1;
	train.trn_avl_seats=1400;
	train.trn_book_seats=0;
	train.status=NORMAL;
	strcpy(train.trn_name,"JANSHATABDI EXPRESS");
	write(fd, &train, 1*sizeof(train));
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

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
#include <pthread.h>

/* Length of variable*/
#define PASSWORD_LEN 16
#define USERNAME_LEN 20
#define PORT_NO 5000
#define MAX_CUSTOMER 1000
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

struct flock get_writelock_customer(int start){
	struct flock lock;
	lock.l_start = start*sizeof(struct_customer);
	lock.l_len = sizeof(struct_customer);
	lock.l_pid = 
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	return lock;
}

int compare_struct_customer(struct_customer actual,struct_customer input){
    if(strcmp(actual.cust_password,input.cust_password)==0 && strcmp(actual.cust_username,input.cust_username)==0
        && strcmp(actual.cust_password,input.cust_password)==0)
        return 1;
    return 0;
}


/*	Implementation of authenticate functionality.
	@return
    0 on failure, 
    1 on success for NORMAL/AGENT account,
    2 on success for ADMIN ACCOUNT.
*/
int authenticate(int conn_fd){
    struct_customer input;int fd;   

    if(read(conn_fd, &input, 1*sizeof(struct_customer)) == -1) ERR_EXIT("read()");

    //printf("%d %s %s\n",input.cust_no,input.cust_username,input.cust_password);   //DELETE

    if((fd = open("customer", O_RDONLY))==-1)   ERR_EXIT("open()");

    printf("Before acquiring write lock...\n");
    struct flock lock=get_writelock_customer(input.acc_no-1);
    fcntl(fd, F_SETLKW, &lock);
    printf("Locking the %d record to update\n",input.acc_no);
    
    lseek(fd, (input.acc_no-1)*sizeof(struct_customer), SEEK_SET);
    struct_customer actual_cust;
    read(fd,&actual_cust,1*sizeof(struct_customer));
    if(compare_struct_customer(actual_cust,input)){
        if(actual_cust.type==ADMIN_ACC_TYPE || actual_cust.type==AGENT_ACC_TYPE ){
            lock.l_type = F_UNLCK;
            fcntl(fd, F_SETLKW, &lock);
            printf("Lock Released\n");
        }
        if(actual_cust.type==ADMIN_ACC_TYPE){
            return 2;
        }else{
            return 1;
        }
    }
    /*struct_customer actual_cust;
    while(read(fd,&actual_cust,1*sizeof(struct_customer))){
        if(compare_struct_customer(actual_cust,input))
        {
            if(close(fd)==-1) ERR_EXIT("close()");
            return 1;
        }
    }*/

	if(close(fd)==-1) ERR_EXIT("close()");
    return 0;
}


void * service(void * fd)
{
    int conn_fd = *((int *) fd);
    printf("Connection accepted %d\n",conn_fd);
    while(1){
        int option;
        if(read(conn_fd, &option, sizeof(option)) == -1) ERR_EXIT("read()");
        option = ntohl(option);
        if(option == 0){    //login
            printf("Inside option 0\n");
            int ret=authenticate(conn_fd);
            printf("ret: %d\n",ret);
            if(write(conn_fd, &ret, sizeof(ret)) == -1) ERR_EXIT("write()");
        }else{
            break;
        }
    }

    /*int size;
    if(read(fd, &size, sizeof(size)) == -1) ERR_EXIT("read()");
    size = ntohl(size);
    int sum = htonl(0), var;
    for (int i = 0; i < size; ++i)
    {
        if(read(fd, &var, sizeof(int)) == -1) ERR_EXIT("read()");
        sum += ntohl(var);
        printf("fd: %d size: %d sum:%d\n",fd,size,sum);
    }
    sum = htonl(sum);
    printf("\nAddition of numbers completed\n");
    if(write(fd, &sum, sizeof(sum)) == -1) ERR_EXIT("write()");
    if(close(fd) == -1) ERR_EXIT("close()");*/

    if(close(conn_fd) == -1) ERR_EXIT("close()");
}



int main(int argc,char* argv[])
{
    int socket_fd, conn_fd;
    struct sockaddr_in serv_addr;
    
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)  ERR_EXIT("socket()");
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT_NO);

    if(bind(socket_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)  ERR_EXIT("bind()");
    
    if(listen(socket_fd, 10) == -1) ERR_EXIT("listen()");

    pthread_t th[5];
    int i=0;
    while(1)
    {
        if((conn_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL)) == -1)
            ERR_EXIT("accept()");
        if(pthread_create(&th[i++], NULL, service, &conn_fd) != 0)
            ERR_EXIT("pthread_create()");
    }
    close(socket_fd);
}


/*int count=3;int fd;
    while(count>0){
        if(count!=3) printf("%d more try remaining\n",count);
        printf("Username:");scanf("%s",username);
        printf("Password:");scanf("%s",password);   

        if((fd = open("customer", O_RDONLY))==-1)   ERR_EXIT("open()");

        struct_customer actual_cust;
        read(fd,&actual_cust,1*sizeof(struct_customer));

        if(strcmp(actual_cust.cust_username,username)==0 && strcmp(actual_cust.cust_password,password)==0)
            return 1;
        count--;
    }*/

/*typedef struct cust_details{
    struct_customer cust_item;
    int session=0;
}struct_session_customer;

static struct_session_customer customer_table[MAX_CUSTOMER];
static int cacheFlag=0;static int total_customer=0;
struct_session_customer get_customer_info(char ip_user[],char ip_pwd[]){
    if(cacheFlag==0){
        cacheFlag=1;
        if((fd = open("customer", O_RDONLY))==-1)   ERR_EXIT("open()");
        int count=0;
        while(1){
            struct_customer actual_cust;
            read(fd,&actual_cust,1*sizeof(struct_customer));
            lseek(fd, count*sizeof(tkt), SEEK_SET);
            struct_session_customer tmp;
            tmp.cust_item=actual_cust;
            customer_table[count++]=tmp;
        }
        total_customer=count;
    }
    
    for(int i=0;i<total_customer;i++){
        struct_session_customer tmp=customer_table[i];
        char *user=tmp.cust_item.cust_username;
        char *pwd=tmp.cust_item.cust_password;
        if(strcmp(user,ip_user)==0 && strm)
    }
}*/

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
#include <sys/ipc.h>
#include <sys/sem.h>


/* Length of variable*/
#define PASSWORD_LEN 16
#define USERNAME_LEN 20
#define TRNAME_LEN 30
#define TRAVELLER_LEN 20
#define PORT_NO 5000
#define MAX_CUSTOMER 1000
#define ADMIN_ACC_TYPE 0
#define NORMAL_ACC_TYPE 1
#define AGENT_ACC_TYPE 2
#define NORMAL 1
#define CANCEL 0
#define S_ADD_CUSTOMER 0
#define S_DEL_CUSTOMER 1
#define S_ADD_TRAIN 2
#define S_DEL_TRAIN 3
#define S_MOD_TRAIN 4
#define S_BOOK_TICKET 5
#define S_MOD_CUSTOMER 6
#define S_CANCEL_BOOKING 7


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

typedef struct booking{
    int book_no;
    int trn_no;
    int acc_no;
    char name[TRAVELLER_LEN];
    int age;
    int seats;
    int status;
}struct_booking;

typedef union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short int *array;
}struct_semap;

static int NO_OF_SEMAPHORE=10;static int G_SEMID=0;

static int LOGGED_IN_USER[MAX_CUSTOMER];static int LOGGED_IN_COUNT=0;


/* Prepares the semaphore set and sets the semaphore setid in a global variable
*/
void init_semaphore_set(){
    int semid, semflag = 0644 | IPC_CREAT | IPC_EXCL;
    key_t key;
    if((key = ftok(".", (int)'s')) == (key_t)-1)
        ERR_EXIT("ftok()");
    if((semid = semget(key, NO_OF_SEMAPHORE , semflag)) == -1){
        if((semid = semget(key, NO_OF_SEMAPHORE, 0)) == -1)
                ERR_EXIT("semget()");
    }
    G_SEMID=semid;

    struct_semap arg;arg.val=1;
    if(semctl(G_SEMID, S_ADD_CUSTOMER, SETVAL, arg) == -1) ERR_EXIT("semctl()");
    if(semctl(G_SEMID, S_DEL_CUSTOMER, SETVAL, arg) == -1) ERR_EXIT("semctl()");
    if(semctl(G_SEMID, S_ADD_TRAIN, SETVAL, arg) == -1) ERR_EXIT("semctl()");
    if(semctl(G_SEMID, S_DEL_TRAIN, SETVAL, arg) == -1) ERR_EXIT("semctl()");
    if(semctl(G_SEMID, S_MOD_TRAIN, SETVAL, arg) == -1) ERR_EXIT("semctl()");
    if(semctl(G_SEMID, S_BOOK_TICKET, SETVAL, arg) == -1) ERR_EXIT("semctl()");
    if(semctl(G_SEMID, S_MOD_CUSTOMER, SETVAL, arg) == -1) ERR_EXIT("semctl()");
    if(semctl(G_SEMID, S_CANCEL_BOOKING, SETVAL, arg) == -1) ERR_EXIT("semctl()");
}

/* Holding the semaphore*/
void wait(int semaphore_no){
    struct sembuf buf;buf.sem_num = semaphore_no;buf.sem_op = -1;
    buf.sem_flg = 0 | SEM_UNDO;//here sem_flg 0 represents, process will wait until the critical section is available
    size_t no_of_sembufs = 1;
    if(semop(G_SEMID, &buf, no_of_sembufs) == -1) ERR_EXIT("semop()");
}

/* Releasing the semaphore*/
void release(int semaphore_no){
    struct sembuf buf;buf.sem_num = semaphore_no;buf.sem_op = 1;
    buf.sem_flg = 0 | SEM_UNDO;//here sem_flg 0 represents, process will wait until the critical section is available
    size_t no_of_sembufs = 1;
    if(semop(G_SEMID, &buf, no_of_sembufs) == -1) ERR_EXIT("semop()");   
}

/* check for already logged in normal users
   @return -1 if not logged in, index if logged in already
*/
int check_for_logged(int acc_no){
    for(int i=0;i<LOGGED_IN_COUNT;i++){
        printf("array: %d accno: %d",LOGGED_IN_USER[i],acc_no);
        if(LOGGED_IN_USER[i]==acc_no){
            return i;
        }
    }
    return -1;
}


int compare_struct_customer(struct_customer actual,struct_customer input){
    if(actual.status==NORMAL && strcmp(actual.cust_password,input.cust_password)==0 && strcmp(actual.cust_username,input.cust_username)==0
        && strcmp(actual.cust_password,input.cust_password)==0)
        return 1;
    return 0;
}

/*  Implementation of authenticate functionality.
    @return
    0 on failure, 
    1 on success for NORMAL/AGENT account,
    2 on success for ADMIN ACCOUNT
    3 on already logged in.
*/
int authenticate(int conn_fd){
    struct_customer input;int fd;   

    if(read(conn_fd, &input, 1*sizeof(struct_customer)) == -1) ERR_EXIT("read()");

    printf("%d %s %s\n",input.acc_no,input.cust_username,input.cust_password);   //DELETE

    if(check_for_logged(input.acc_no)!=-1){
        return 3;
    }
    if((fd = open("customer", O_RDONLY))==-1)   ERR_EXIT("open()");

    lseek(fd, (input.acc_no-1)*sizeof(struct_customer), SEEK_SET);
    struct_customer actual_cust;
    read(fd,&actual_cust,1*sizeof(struct_customer));
    if(compare_struct_customer(actual_cust,input)){
        if(actual_cust.type==NORMAL_ACC_TYPE ){
            LOGGED_IN_USER[LOGGED_IN_COUNT++]=actual_cust.acc_no;
        }
        if(actual_cust.type==ADMIN_ACC_TYPE){
            return 2;
        }else{
            return 1;
        }
    }

    if(close(fd)==-1) ERR_EXIT("close()");
    return 0;
}

void release_logged_in_user(int conn_fd){
    struct_customer exiting_user;
    if(read(conn_fd, &exiting_user, sizeof(struct_customer)) == -1) ERR_EXIT("read()");
    int index;
    if((index=check_for_logged(exiting_user.acc_no))!=-1){
        for(int i=index;i<LOGGED_IN_COUNT-1;i++){
            LOGGED_IN_USER[i]= LOGGED_IN_USER[i+1];
        }
        LOGGED_IN_COUNT--;
    }
}

void modify_train(int flow,int trn_no,int booked_seats,char name[],int seats_to_inc){
    wait(S_MOD_TRAIN);
    sleep(30);
    int fd;
    if((fd = open("train", O_RDWR))==-1)   ERR_EXIT("open()");
    struct_train actual_trn;
    lseek(fd,(trn_no-1)*(sizeof(struct_train)),SEEK_SET);
    read(fd,&actual_trn,1*sizeof(struct_train));

    if(flow==0){//for booking
        actual_trn.trn_avl_seats-=booked_seats;
        actual_trn.trn_book_seats+=booked_seats;
    }else if(flow==2){//for cancel booking
        actual_trn.trn_avl_seats+=seats_to_inc;
        actual_trn.trn_book_seats-=seats_to_inc;
    }else{
        actual_trn.trn_avl_seats+=seats_to_inc;
        strcpy(actual_trn.trn_name,name);
    }
    
    lseek(fd,-1*sizeof(struct_train),SEEK_CUR);
    write(fd,&actual_trn,1*sizeof(struct_train));
    release(S_MOD_TRAIN);
}

void show_all_train(int conn_fd){
    int moreflg=htonl(1);int fd;
    
    if((fd = open("train", O_RDONLY))==-1)   ERR_EXIT("open()");    
    struct_train actual_trn;
    while(read(fd,&actual_trn,1*sizeof(struct_train))){
        if(write(conn_fd, &moreflg, sizeof(int)) == -1) ERR_EXIT("write()");
        if(write(conn_fd, &actual_trn, sizeof(struct_train)) == -1) ERR_EXIT("write()");
    }
    moreflg=htonl(0);
    if(write(conn_fd, &moreflg, sizeof(int)) == -1) ERR_EXIT("write()");        
        
    if(close(fd)==-1) ERR_EXIT("close()");//closing file                
}

int show_all_booking(int conn_fd){
    int moreflg=htonl(1);int fd;int acc_no;
    
    if(read(conn_fd, &acc_no, 1*sizeof(int)) == -1) ERR_EXIT("read()");
    char filename[10], accno[4];
    sprintf(accno, "%d", acc_no);
    strcpy(filename,accno);strcat(filename,"bh");

    if((fd = open(filename, O_RDONLY))!=-1){
        struct_booking actual_book;
        while(read(fd,&actual_book,1*sizeof(struct_booking))){
            if(write(conn_fd, &moreflg, sizeof(int)) == -1) ERR_EXIT("write()");
            if(write(conn_fd, &actual_book, sizeof(struct_booking)) == -1) ERR_EXIT("write()");
        }
    }
    if(fd!=-1)
    if(close(fd)==-1) ERR_EXIT("close()");//closing file                
    printf("Here: %d",moreflg);
    moreflg=htonl(0);
    if(write(conn_fd, &moreflg, sizeof(int)) == -1) ERR_EXIT("write()");
    if(fd==-1) return 0;
    return 1;
}

void book_ticket(int conn_fd){
    show_all_train(conn_fd);
    struct_booking new;
    int presentflg=0;int fd;
    if(read(conn_fd, &new, 1*sizeof(struct_booking)) == -1) ERR_EXIT("read()");

    if((fd = open("train", O_RDONLY))==-1)   ERR_EXIT("open()");

    struct_train actual_trn;
    lseek(fd,(new.trn_no-1)*(sizeof(struct_train)),SEEK_SET);
    read(fd,&actual_trn,1*sizeof(struct_train));
    if(actual_trn.trn_no==new.trn_no && actual_trn.status==NORMAL && actual_trn.trn_avl_seats>=new.seats){
        presentflg=1;
        presentflg=htonl(presentflg);
        modify_train(0,actual_trn.trn_no,new.seats,"",0);
    }

    if(close(fd)==-1) ERR_EXIT("close()");//closing file 
    
    presentflg=htonl(presentflg);
    if(write(conn_fd, &presentflg, sizeof(int)) == -1) ERR_EXIT("write()");
                    
    if(presentflg){
        wait(S_BOOK_TICKET);

        char filename[10], accno[4];int bookfd;
        sprintf(accno, "%d", new.acc_no);
        strcpy(filename,accno);strcat(filename,"bh");
        if((bookfd = open(filename, O_RDWR | O_CREAT, 0744))==-1)   ERR_EXIT("open()");

        int size=lseek(bookfd,0,SEEK_END);
        if(size==0){
            new.book_no=1;
        }else{
            struct_booking last;
            lseek(bookfd,-1*sizeof(struct_booking),SEEK_END);
            read(bookfd,&last,sizeof(struct_booking));
            new.book_no=last.book_no+1;
        }
        new.status=NORMAL;

        lseek(bookfd,0,SEEK_END);
        write(bookfd,&new,sizeof(struct_booking));

        if(write(conn_fd, &new, sizeof(struct_booking)) == -1) ERR_EXIT("write()");
        release(S_BOOK_TICKET);   
    }
}


struct_customer add_customer(int conn_fd){
    struct_customer new;int fd;
    if(read(conn_fd, &new, 1*sizeof(struct_customer)) == -1) ERR_EXIT("read()");

    printf("Before entering into critical section.\n");
    printf("Waiting for lock...\n");
    wait(S_ADD_CUSTOMER);

    if((fd = open("customer", O_RDWR))==-1)   ERR_EXIT("open()");    
    lseek(fd, -1*sizeof(struct_customer), SEEK_END);
    struct_customer last;
    if(read(fd, &last, 1*sizeof(struct_customer)) == -1) ERR_EXIT("read()");
    new.acc_no=last.acc_no+1;
    write(fd, &new, 1*sizeof(new));

    printf("Inside critical section.\n");

    if(close(fd)==-1) ERR_EXIT("close()");//closing file    

    release(S_ADD_CUSTOMER);
    return new;
}

void show_all_customer(int conn_fd){
    int moreflg=htonl(1);int fd;
    
    if((fd = open("customer", O_RDONLY))==-1)   ERR_EXIT("open()");    
    struct_customer actual_cust;
    while(read(fd,&actual_cust,1*sizeof(struct_customer))){
        if(write(conn_fd, &moreflg, sizeof(int)) == -1) ERR_EXIT("write()");
        if(write(conn_fd, &actual_cust, sizeof(struct_customer)) == -1) ERR_EXIT("write()");
    }
    moreflg=htonl(0);
    if(write(conn_fd, &moreflg, sizeof(int)) == -1) ERR_EXIT("write()");        
        
    if(close(fd)==-1) ERR_EXIT("close()");//closing file                
}

void search_user(int conn_fd){
    int  accno;int presentflg=0;int fd;
    if(read(conn_fd, &accno, sizeof(int)) == -1) ERR_EXIT("write()");
    accno=ntohl(accno);
    if((fd = open("customer", O_RDONLY))==-1)   ERR_EXIT("open()");    

    struct_customer actual_cust;
    while(read(fd,&actual_cust,1*sizeof(struct_customer))){
        if(actual_cust.acc_no==accno && actual_cust.status==NORMAL){
            presentflg=1;
            presentflg=htonl(presentflg);
            if(write(conn_fd, &presentflg, sizeof(int)) == -1) ERR_EXIT("write()");
            if(write(conn_fd, &actual_cust, sizeof(struct_customer)) == -1) ERR_EXIT("write()");
            if(close(fd)==-1) ERR_EXIT("close()");//closing file                
            return;       
        }
    }
    presentflg=htonl(presentflg);
    if(write(conn_fd, &presentflg, sizeof(int)) == -1) ERR_EXIT("write()");
    if(close(fd)==-1) ERR_EXIT("close()");//closing file                
}

void delete_user(int conn_fd){
    int  accno;int presentflg=0;int fd;
    if(read(conn_fd, &accno, sizeof(int)) == -1) ERR_EXIT("write()");
    accno=ntohl(accno);
    printf("Before entering into critical section.\n");
    printf("Waiting for lock...\n");
    wait(S_DEL_CUSTOMER);
    printf("Inside critical section.\n");

    if((fd = open("customer", O_RDWR))==-1)   ERR_EXIT("open()"); 
    struct_customer actual_cust;
    while(read(fd,&actual_cust,1*sizeof(struct_customer))){
        if(actual_cust.acc_no==accno && actual_cust.status==NORMAL){
            presentflg=1;
            lseek(fd, -1*sizeof(struct_customer), SEEK_CUR);
            actual_cust.status=CANCEL;
            write(fd, &actual_cust, 1*sizeof(struct_customer));            
            break;
        }
    }
    if(close(fd)==-1) ERR_EXIT("close()");//closing file
    release(S_DEL_CUSTOMER);
    printf("Lock Released\n");
    presentflg=htonl(presentflg);
    if(write(conn_fd, &presentflg, sizeof(int)) == -1) ERR_EXIT("write()");
}

struct_train add_train(int conn_fd){
    struct_train new;int fd;
    if(read(conn_fd, &new, 1*sizeof(struct_train)) == -1) ERR_EXIT("read()");

    printf("Before entering into critical section.\n");
    printf("Waiting for lock...\n");
    wait(S_ADD_TRAIN);

    if((fd = open("train", O_RDWR))==-1)   ERR_EXIT("open()");    
    lseek(fd, -1*sizeof(struct_train), SEEK_END);
    struct_train last;
    if(read(fd, &last, 1*sizeof(struct_train)) == -1) ERR_EXIT("read()");
    new.trn_no=last.trn_no+1;
    write(fd, &new, 1*sizeof(new));

    printf("Inside critical section.\n");

    if(close(fd)==-1) ERR_EXIT("close()");//closing file    

    release(S_ADD_TRAIN);
    return new;
}

void search_train(int conn_fd){
    int trn_no;int presentflg=0;int fd;
    if(read(conn_fd, &trn_no, sizeof(int)) == -1) ERR_EXIT("write()");
    trn_no=ntohl(trn_no);
    if((fd = open("train", O_RDONLY))==-1)   ERR_EXIT("open()");    

    struct_train actual_trn;
    while(read(fd,&actual_trn,1*sizeof(struct_train))){
        if(actual_trn.trn_no==trn_no && actual_trn.status==NORMAL){
            presentflg=1;
            presentflg=htonl(presentflg);
            if(write(conn_fd, &presentflg, sizeof(int)) == -1) ERR_EXIT("write()");
            if(write(conn_fd, &actual_trn, sizeof(struct_train)) == -1) ERR_EXIT("write()");
            if(close(fd)==-1) ERR_EXIT("close()");//closing file                
            return;       
        }
    }
    presentflg=htonl(presentflg);
    if(write(conn_fd, &presentflg, sizeof(int)) == -1) ERR_EXIT("write()");
    if(close(fd)==-1) ERR_EXIT("close()");//closing file                
}

void delete_train(int conn_fd){
    int  trn_no;int presentflg=0;int fd;
    if(read(conn_fd, &trn_no, sizeof(int)) == -1) ERR_EXIT("write()");
    trn_no=ntohl(trn_no);
    printf("Before entering into critical section.\n");
    printf("Waiting for lock...\n");
    wait(S_DEL_TRAIN);
    printf("Inside critical section.\n");

    if((fd = open("train", O_RDWR))==-1)   ERR_EXIT("open()"); 
    struct_train actual_trn;
    while(read(fd,&actual_trn,1*sizeof(struct_train))){
        if(actual_trn.trn_no==trn_no && actual_trn.status==NORMAL){
            presentflg=1;
            lseek(fd, -1*sizeof(struct_train), SEEK_CUR);
            actual_trn.status=CANCEL;
            write(fd, &actual_trn, 1*sizeof(struct_train));            
            break;
        }
    }
    if(close(fd)==-1) ERR_EXIT("close()");//closing file
    release(S_DEL_TRAIN);
    printf("Lock Released\n");
    presentflg=htonl(presentflg);
    if(write(conn_fd, &presentflg, sizeof(int)) == -1) ERR_EXIT("write()");
}

void cancel_booking(int conn_fd){
    int presentflg=show_all_booking(conn_fd);
    
    presentflg=htonl(presentflg);
    if(write(conn_fd, &presentflg, sizeof(int))==-1)   ERR_EXIT("read()");
    presentflg=ntohl(presentflg);
    printf("flag: %d",presentflg);
    if(presentflg==0){
        return;
    }
    
    int  booking_no;int fd;int acc_no;
    if(read(conn_fd, &acc_no, sizeof(int)) == -1) ERR_EXIT("write()");
    if(read(conn_fd, &booking_no, sizeof(int)) == -1) ERR_EXIT("write()");
    printf("Acc No: %d Booking No: %d\n",acc_no,booking_no);
    booking_no=ntohl(booking_no);
    acc_no=ntohl(acc_no);
    printf("Acc No: %d Booking No: %d\n",acc_no,booking_no);
    printf("Before entering into critical section.\n");
    printf("Waiting for lock...\n");
    wait(S_CANCEL_BOOKING);
    printf("Inside critical section.\n");

    char filename[10], accno[4];int bookfd;
    sprintf(accno, "%d", acc_no);
    strcpy(filename,accno);strcat(filename,"bh");
    if((fd = open(filename, O_RDWR | O_CREAT, 0744))==-1)   ERR_EXIT("open()");

    struct_booking actual_book;
    lseek(fd, (booking_no-1)*sizeof(struct_booking), SEEK_CUR);
    read(fd,&actual_book,sizeof(struct_booking));
    struct_train actual_trn;
    if(actual_book.book_no==booking_no && actual_book.status==NORMAL){
        actual_book.status=CANCEL;
        lseek(fd, -1*sizeof(struct_booking), SEEK_CUR);
        write(fd,&actual_book,sizeof(struct_booking));
        modify_train(2,actual_book.trn_no,0,"",actual_book.seats);
        presentflg=1;
    }else{
        presentflg=0;
    }
    if(close(fd)==-1) ERR_EXIT("close()");//closing file
    release(S_CANCEL_BOOKING);
    printf("Lock Released\n");
    presentflg=htonl(presentflg);
    if(write(conn_fd, &presentflg, sizeof(int)) == -1) ERR_EXIT("write()");
}

void modify_customer(int conn_fd){
    printf("Before entering into critical section.\n");
    printf("Waiting for lock...\n");
    
    wait(S_MOD_CUSTOMER);
    printf("Inside critical section.\n");
    int fd;
    if((fd = open("customer", O_RDWR))==-1)   ERR_EXIT("open()");
    struct_customer mod_cust;
    if(read(conn_fd, &mod_cust, 1*sizeof(struct_train)) == -1) ERR_EXIT("read()");
    lseek(fd,(mod_cust.acc_no-1)*(sizeof(struct_customer)),SEEK_SET);
    write(fd,&mod_cust,1*sizeof(struct_customer));
    release(S_MOD_CUSTOMER);
    printf("Lock Released\n");
}

void modify_user(int conn_fd){
    int  accno;int presentflg=0;int fd;
    if(read(conn_fd, &accno, sizeof(int)) == -1) ERR_EXIT("write()");
    accno=ntohl(accno);

    if((fd = open("customer", O_RDONLY))==-1)   ERR_EXIT("open()"); 
    struct_customer actual_cust;
    while(read(fd,&actual_cust,1*sizeof(struct_customer))){
        if(actual_cust.acc_no==accno && actual_cust.status==NORMAL){
            presentflg=1;
            if(write(conn_fd, &presentflg, sizeof(int)) == -1) ERR_EXIT("write()");
            if(write(conn_fd, &actual_cust, sizeof(struct_customer)) == -1) ERR_EXIT("write()");
            if(close(fd)==-1) ERR_EXIT("close()");//closing file        
            modify_customer(conn_fd);
            return;
        }
    }
    if(close(fd)==-1) ERR_EXIT("close()");//closing file
    presentflg=htonl(presentflg);
    if(write(conn_fd, &presentflg, sizeof(int)) == -1) ERR_EXIT("write()");
}

void modify_train_by_admin(int conn_fd){
    int  trn_no;int presentflg=0;int fd;
    if(read(conn_fd, &trn_no, sizeof(int)) == -1) ERR_EXIT("write()");
    trn_no=ntohl(trn_no);

    if((fd = open("train", O_RDONLY))==-1)   ERR_EXIT("open()"); 
    struct_train actual_train;
    while(read(fd,&actual_train,1*sizeof(struct_train))){
        if(actual_train.trn_no==trn_no && actual_train.status==NORMAL){
            presentflg=1;
            if(write(conn_fd, &presentflg, sizeof(int)) == -1) ERR_EXIT("write()");
            if(write(conn_fd, &actual_train, sizeof(struct_train)) == -1) ERR_EXIT("write()");
            if(close(fd)==-1) ERR_EXIT("close()");//closing file    

            char up_trname[TRNAME_LEN];int up_seats;
            if(read(conn_fd, &up_trname, sizeof(up_trname)) == -1) ERR_EXIT("write()");
            puts(up_trname);
            if(read(conn_fd, &up_seats, sizeof(int)) == -1)    ERR_EXIT("write()");    
            modify_train(1,trn_no,0,up_trname,up_seats);
            return;
        }
    }
    if(close(fd)==-1) ERR_EXIT("close()");//closing file
    presentflg=htonl(presentflg);
    if(write(conn_fd, &presentflg, sizeof(int)) == -1) ERR_EXIT("write()");
}


void * service(void * fd)
{
    int conn_fd = *((int *) fd);
    printf("Connection accepted %d\n",conn_fd);
    while(1){
        int option;
        if(read(conn_fd, &option, sizeof(option)) == -1) ERR_EXIT("read()");
        option = ntohl(option);
        printf("option from client:%d\n",option);
        if(option == 0){    //login
            printf("Inside option 0\n");
            int ret=authenticate(conn_fd);
            printf("ret: %d\n",ret);
            if(write(conn_fd, &ret, sizeof(ret)) == -1) ERR_EXIT("write()");
        }else if(option==1){
            printf("Inside option 1\n");
            book_ticket(conn_fd);
        }else if(option==2){
            printf("Inside option 2\n");
            show_all_booking(conn_fd);
        }else if(option==3){
            printf("Inside option 3\n");
            cancel_booking(conn_fd);
        }else if(option==5){
            printf("Inside option 5\n");
            struct_customer new=add_customer(conn_fd);
            if(write(conn_fd, &new, sizeof(struct_customer)) == -1) ERR_EXIT("write()");
        }else if(option==6){
            printf("Inside option 6\n");
            search_user(conn_fd);
        }else if(option==7){
            printf("Inside option 7\n");
            delete_user(conn_fd);
        }else if(option==8){
            printf("Inside option 8\n");
            modify_user(conn_fd);
        }else if(option==9){
            printf("Inside option 9\n");
            show_all_customer(conn_fd);
        }else if(option==10){
            printf("Inside option 10\n");
            struct_train new=add_train(conn_fd);
            if(write(conn_fd, &new, sizeof(struct_train)) == -1) ERR_EXIT("write()");
        }else if(option==11){
            printf("Inside option 11\n");
            search_train(conn_fd);
        }else if(option==12){
            printf("Inside option 12\n");
            delete_train(conn_fd);
        }else if(option==13){
            printf("Inside option 13\n");
            modify_train_by_admin(conn_fd);
        }else if(option==14){
            printf("Inside option 14\n");
            show_all_train(conn_fd);
        }else if(option==15){
            break;
        }else{
            release_logged_in_user(conn_fd);
            break;
        }
        //sleep(2); //DELETE
    }

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

    init_semaphore_set();
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


/*struct_customer actual_cust;
    while(read(fd,&actual_cust,1*sizeof(struct_customer))){
        if(compare_struct_customer(actual_cust,input))
        {
            if(close(fd)==-1) ERR_EXIT("close()");
            return 1;
        }
    }*/

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

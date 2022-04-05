#include "includes.h"       
#define BUFSIZE 512     
#define PORT_NUM 50001

void* display_login(void* arg);
void* send_msg(void* arg);
void* recv_msg(void* arg);

void menu(int a);
void menuOptions() ;

pthread_mutex_t mutx;
int chat_status=0;
int login_status = 0;
int dis_sock =-1;
char my_time[BUFSIZE];
char my_id[11];
char IP[20];

int main(int argc, char **argv)
{
    int sock, user_num;
    struct sockaddr_in serv_addr;
    char message[BUFSIZE];
    int str_len;
    pthread_t threads, send_thread, receive_thread; int result_code;

    if (argc != 2) {    printf("Usage : %s <IP> \n", argv[0]);  exit(1);    }
    strcpy(IP, argv[1]);
    printf("1");
    struct tm *t;
    time_t timer = time(NULL);
    t=localtime(&timer);
    sprintf(my_time, "%d-%d-%d %d:%d", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour,t->tm_min);
    printf("2");
    pthread_mutex_init(&mutx, NULL);
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1) { perror("socket() error"); exit(1); }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(IP);
    serv_addr.sin_port = htons(PORT_NUM);
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) { perror("connect() error"); }
    sleep(0.5);
    write(sock, "message\n", 16);         
    menu(0);
    while (1) {
        fputs("input text(q to quit) : ", stdout);      
        fgets(message, BUFSIZE, stdin);    
        sleep(0.12);     
        write(sock, message, strlen(message)+1);         
        if (!strcmp(message, "q\n")) {
            close(dis_sock);
            close(sock);
            exit(1);
            break;
        }
        strcpy(my_id, message);
        str_len = read(sock, message, BUFSIZE - 1);     /* read message from server */
        switch(login_status)
        {
            case 0:
                if (!strcmp(message,"") || !strcmp(message,"\n")) menu(0);
                else printf("\nReceived message : %s\n", message);
                if(strcmp(message,"*****successful login****\n")==0)
                    {
                        //write(sock, "login",10);
                        //str_len= read(sock, message,BUFSIZE-1);
                        login_status=1; 
                        menu(1);
                        
                        user_num= atoi(message);
                        result_code = pthread_create(&threads, NULL, display_login, (void*)&user_num);  // create client thread
                        assert(!result_code);
                        result_code = pthread_detach(threads);      // detach echo thread
                        assert(!result_code);
                    }
                    else if(!strcmp(message,"3 time wrong \n"))
                    {
                        exit(1);
                    }
            break;

            case 1:
                printf("%s",message);
                if (!strcmp(message,"\n") || !strcmp(message,"")) menu(1);
                else if(!strcmp(message,"startchat")) {
                    system("clear");
                    printf("\n*               JOINED ROOM             *\n");
                    printf("* PLEASE ENTER 'q'  IF YOU WANT GO MENU *\n\n");
                    chat_status=1;
                    sleep(0.1);
                    result_code = pthread_create(&send_thread, NULL, send_msg, (void*)&sock);   // create client thread
                    assert(!result_code);
                    result_code = pthread_detach(send_thread);      // detach echo thread
                    assert(!result_code);
                    result_code = pthread_create(&receive_thread, NULL, recv_msg, (void*)&sock);    // create client thread
                    assert(!result_code);
                    result_code = pthread_detach(receive_thread);       // detach echo thread
                    assert(!result_code);
                    while(chat_status==1){;;;}
                    sleep(1);
                    menu(1);
                }
            break;
                
        }
    }
    while(1);
    return 0;
}

void* display_login(void* arg){
    int sock2, str_len;
    int user_num = *(int*)arg;
    char message[BUFSIZE];
    struct sockaddr_in serv_addr;
    sock2 = socket(PF_INET, SOCK_STREAM, 0);        /* socket type을  stream socket으로 setting */
    if (sock2 == -1) { perror("socket() error"); exit(1); }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(IP);
    serv_addr.sin_port = htons(PORT_NUM);
    sleep(0.5);
    if (connect(sock2, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) { 
        perror("connect() error");
        exit(1); 
    }
    dis_sock=sock2;
    
    write(sock2, "dis\n",30);

    while(1){
        pthread_mutex_lock(&mutx);
        str_len = read(sock2, message, 200);
        sleep(0.3);
        printf("\n ---------------- login----------------\n%s --------------------------------------\n", message);
        pthread_mutex_unlock(&mutx);
    }
    close(sock2);
    return;
}

void* send_msg(void* arg) //메세지 송신 쓰레드
{
    int sock=*((int*)arg);
    char msg[BUFSIZE];
    write(sock,"joined in", 10);
    while(1)
    {
        strcpy(msg,"");
        fgets(msg, BUFSIZE, stdin);
        msg[strlen(msg)] = '\0';

        // menu_mode command -> !menu
        if (!strcmp(msg, "!menu\n"))
        {
            menuOptions();
            continue;
        }
 
        else if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
        {
            write(sock, msg, 4);
            chat_status=0;
            return NULL;
        }
 
        else
        {
            write(sock,msg,strlen(msg)+1);
        }
        // send message
        if (chat_status == 0 )
            printf(msg, "%s",msg);
        
    }
    return NULL;
}
void* recv_msg(void* arg) //메세지 수신 쓰레드
{
    int sock=*((int*)arg);
    char msg[BUFSIZE];
    int str_len;
    while(1)
    {   
        str_len=read(sock, msg, BUFSIZE-1);
        if (str_len==-1 || !strcmp(msg,"q\n")){
            chat_status = 0;
            
            return (void*)-1;
        }
        //if(chat_status == 0){return NULL;}

        printf("%s\n",msg);
        sleep(0.1);
    }
    return NULL;
}

 
void menuOptions() 
{
    int select;
    // print menu
    printf("\n\t**** menu mode ****\n");
    printf("\t1. make chating room\n");
    printf("\t2. enter chating room\n");
    printf("\t3. check login user list\n");
    printf("\t4. check chating room list\n\n");
    printf("\tthe other key is cancel");
    printf("\n\t*******************");
    printf("\n\t>> ");
    return ;
}
 
 
void menu(int a)
{
    system("clear");
    system("clear");
    printf(" ----- Linux talk chatting client -----\n");
    printf(" server port : %d \n", PORT_NUM);
    printf(" client IP   : %s \n", IP);
    printf(" server time : %s \n", my_time);
    printf(" ---------------- menu ----------------\n");
    if(a==1)
    {
        printf("\t1. make chating room\n");
        printf("\t2. enter chating room\n");
        printf("\t3. check login user list\n");
        printf("\t4. check chating room list\n\n");
    }   
    else if (a==0)
    {
        printf("\t1. enter 'join' to join us\n");
        printf("\t2. enter 'login' to login us\n");
    }
    printf(" --------------------------------------\n");
    printf(" Exit -> q & Q\n\n");
}
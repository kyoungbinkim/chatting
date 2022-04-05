#include "includes.h"
#include <signal.h>
#define BUFSIZE 512
#define PORT_NUM 50001
#define ON '1'
#define OFF '0'
#define CHATROOMSIZE 10
#define DBSIZE 20

struct User
{
    char ID[11];
    char PW[11];
    char login_status; // 로그인 상태 ON OFF
    char chat_status;  // 채팅 상태 ON OFF
    int my_sock;       // 내 소캣 번호
    int joined_num;    // 채팅방 번호
    int dis_sock;      // display _thread 소캣번호
};

pthread_mutex_t mutx;
void *menu_thread(void *argument); //
void *Send_login_list(void *arg);
int login_service(int clnt_sock); // 로그인 함수
int join_service(int clnt_sock);  // 회원가입 함수
void stanby_screen(int clnt_sock, int user_num);
void chat_room(int sock, int User_num, int Chat_num);
int Check_ID(char id[]);
int chat_room_cnt = 0; // 만들어진 채팅룸 수
int chat_room_sock[CHATROOMSIZE][5] = {
    0,
};                                     // 접속되어 있는 소캣 번호
char chat_room_name[CHATROOMSIZE][20]; // 채팅방 이름
int chat_room_joinednum[CHATROOMSIZE] = {
    0,
};                      // 접속되어있는 수
struct User DB[DBSIZE]; // DB 사용자 불러오기
int db_fd, db_num = 0;
char logined_list[DBSIZE + 1]; // 로그인되어 있는사람

int main(int argc, char **argv)
{
    int serv_sock, clnt_sock;
    char message[BUFSIZE];
    char db_buffer[10];
    int str_len, thread_args, result_code;
    struct sockaddr_in serv_addr, clnt_addr;
    int clnt_addr_size;
    pthread_t threads;
    int nread, nwrite, strLen;
    int i = 0;
    signal(SIGPIPE, SIG_IGN);
    if (argc != 1)
    {
        printf("Usage : %s\n", argv[0]);
        exit(1);
    }
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
    {
        perror("socket() error");
        exit(1);
    }
    for (i = 0; i < CHATROOMSIZE; i++)
    {
        strcpy(chat_room_name[i], "");
    }
    for (i = 0; i < DBSIZE; i++) //DB 초기화
    {
        strcpy(DB[i].ID, "");
        strcpy(DB[i].PW, "");
        DB[i].login_status = OFF;
        DB[i].chat_status = OFF;
        logined_list[i] = OFF;
        DB[i].dis_sock = -1;
        DB[i].my_sock = -1;
        DB[i].joined_num = -1;
    }
    logined_list[DBSIZE + 1] = '\0';
    if ((db_fd = open("DB", O_RDWR | O_CREAT)) < 0) //DB 파일 오픈
    {
        printf("open DB error\n");
        return -1;
    }
    i = 0;
    while ((nread = read(db_fd, db_buffer, 9)) > 0) // DB읽어오기
    {
        if (i % 2 == 0)
        {
            strcpy(DB[i / 2].ID, db_buffer);
            DB[i / 2].ID[9] = '\0';
        }
        else
        {
            strcpy(DB[i / 2].PW, db_buffer);
            DB[i / 2].PW[9] = '\0';
        }
        db_num = i;
        i++;

        if (i == 40)
            break;
    }
    printf("num : %d\n", db_num);
    for (i = 0; i < 20; i++)
    {
        printf("%d ID = %s  PW = %s \n", i, DB[i].ID, DB[i].PW);
    }
    pthread_mutex_init(&mutx, NULL);

    //result_code = pthread_create(&threads, NULL, display_login, NULL);
    //assert(!result_code);
    //result_code = pthread_detach(threads);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT_NUM);

    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        perror("bind() error");
        exit(1);
    }
    if (listen(serv_sock, 5) == -1)
    {
        perror("listen() error");
        exit(1);
    }
    while (1)
    {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1)
        {
            perror("accept() error");
            exit(1);
        }
        pthread_mutex_lock(&mutx);
        printf("Connection from: %s:%d\n", inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));

        strLen = read(clnt_sock, message, BUFSIZE - 1); // user '메세지'  'dis'
        printf("%d %s", clnt_sock, message);
        thread_args = clnt_sock;
        if (!strcmp(message, "dis\n")) // send login list 실행
        {
            result_code = pthread_create(&threads, NULL, Send_login_list, (void *)&thread_args);
            assert(!result_code);
            result_code = pthread_detach(threads);
            assert(!result_code);
        }
        else // menu thread
        {
            result_code = pthread_create(&threads, NULL, menu_thread, (void *)&thread_args);
            assert(!result_code);
            result_code = pthread_detach(threads);
            assert(!result_code);
        }
        pthread_mutex_unlock(&mutx);
    }
    close(db_fd);
    return 0;
}

// user가 접속하여 login 또는 join 선택하는 thread
void *menu_thread(void *arg)
{
    int result_code;
    int clnt_sock;
    clnt_sock = *(int *)arg;
    char message[BUFSIZE];
    char error[BUFSIZE];
    char DB_tmp[25]; // DB에 입력하기 위한 변수
    int strLen, user_num, chat_num ,stat;
    strcpy(error, "you must write join or login\n");

    while ((strLen = read(clnt_sock, message, BUFSIZE - 1)) != 0)
    {
        /* join 입력시 회원가입 해준다.*/
        if (strcmp(message, "join\n") == 0 || !strcmp(message, "1\n"))
        {
            if(join_service(clnt_sock)==1)return;
        }
        /*user가 로그인 입력시 로그인 실행*/
        else if (strcmp(message, "login\n") == 0 || !strcmp(message, "1\n"))
        {
            user_num = login_service(clnt_sock);
            printf("user num %d",user_num);
            if(user_num == -1)
            { 
                return;
            }
            else if(user_num == -2)
            {
                continue;
            }
            else
            {
                stanby_screen(clnt_sock, user_num);
                return;
            }
        }
        else if (!strcmp(message, "q\n")) // 로그아웃
        {
            close(clnt_sock);
            return;
        }
        else
        {
            write(clnt_sock, "", 10);
        }
    }
    return;
}
void *Send_login_list(void *arg)
{
    int i;
    char message[BUFSIZE];
    struct User DB_tmp[20];
    char logined_list_tmp[21];
    char display_tmp[200];
    int display_sock = *(int *)arg;
    sleep(0.5);
    for (i = 0; i < 20; i++)
    {
        DB_tmp[i] = DB[i];
        logined_list_tmp[i] = logined_list[i];
    }
    logined_list_tmp[20] = '\0';
    printf("%s\n", logined_list_tmp);
    sleep(0.5);

    for (i = 0; i < 21; i++)
    {
        logined_list_tmp[i] = logined_list[i];
        if (logined_list[i] == ON)
        {
            strncat(display_tmp, "\t\t", 2);
            strncat(display_tmp, DB[i].ID, 10);
            printf("%s", display_tmp);
        }
    }
    write(display_sock, display_tmp, 200);

    strcpy(display_tmp, "");
    while (1)
    {

        if (strcmp(logined_list, logined_list_tmp) != 0) // 새로운 사용자가 로그인 했을 경우
        {
            printf("someone login\n");
            printf("%s\n", logined_list);

            for (i = 0; i < 21; i++)
            {
                logined_list_tmp[i] = logined_list[i];
                if (logined_list[i] == ON)
                {
                    strncat(display_tmp, "\t\t", 2);
                    strncat(display_tmp, DB[i].ID, 10);
                    printf("%s", display_tmp);
                }
            }
            write(display_sock, display_tmp, 200);
            strcpy(display_tmp, "");
        }
        sleep(0.1);
    }
    close(display_tmp);
    return;
}
int join_service(int clnt_sock)
{
    char DB_tmp[25]; // DB에 입력하기 위한 변수
    char message[BUFSIZE];
    int strLen, tmp;

    write(clnt_sock, "******write ID less than 9 char****** \n", 100);
    strLen = read(clnt_sock, message, BUFSIZE - 1);
    if (!strcmp(message, "q\n"))
    { // q
        write(clnt_sock, "\n", 100);
        return 1;
    }
    if (!strcmp(message, "login\n"))
    {
        write(clnt_sock, "\n", 100);
        return 2;
    }
    while (Check_ID(message) > -1 || strLen > 9) // 곂치는 id 체크
    {
        write(clnt_sock, "**ID is already joined or length is over 9** \n", 100);
        strLen = read(clnt_sock, message, BUFSIZE - 1);
        if (!strcmp(message, "q\n"))
        { // q
            write(clnt_sock, "\n", 100);
            return 1;
        }
        if (!strcmp(message, "login\n"))
        {
            write(clnt_sock, "\n", 100);
            return 2;
        }
    }
    tmp = db_num;
    db_num += 2;
    write(db_fd, message, 9);            // db 파일 에 입력
    strncpy(DB[tmp / 2].ID, message, 9); // DB 행열에 쓰기
    write(clnt_sock, "******write PW less than 9 char****** \n", 100);
    strncpy(message, "\0", 1);
    strLen = read(clnt_sock, message, BUFSIZE - 1);
    write(db_fd, message, 9);
    strncpy(DB[tmp / 2].PW, message, 9);
    write(clnt_sock, "OK\n", 9);
    return 0;
}

int login_service(int clnt_sock)
{
    char message[BUFSIZE];
    int strLen, i, user_num, chat_num;
    write(clnt_sock, "******write login ID****** \n", 100);
    strLen = read(clnt_sock, message, BUFSIZE - 1);
    if (!strcmp(message, "q\n"))
    {
        write(clnt_sock, "\n", 100);
        return -1;
    }
    if (!strcmp(message, "join\n"))
    {
        write(clnt_sock, "\n", 100);
        return -2;
    }
    while ((user_num = Check_ID(message)) <= -1)
    { // 아이디가 DB에 있는지 확인
        printf("%s", message);
        write(clnt_sock, "***write login ID(no ID or logined ID)*** \n", 100);
        strLen = read(clnt_sock, message, BUFSIZE - 1);
        if (!strcmp(message, "q\n"))
        {
            write(clnt_sock, "\n", 100);
            return -1;
        }
        if (!strcmp(message, "join\n"))
        {
            write(clnt_sock, "\n", 100);
            return -2;
        }
    }
    write(clnt_sock, "******write login PW****** \n", 100);
    strLen = read(clnt_sock, message, BUFSIZE - 1);
    for (int i = 0; i <= 3; i++)
    { // 비밀번호 확인
        if (strcmp(message, DB[user_num].PW) == 0)
        {
            break;
        }
        else
        {
            if (i == 3) // 3차례 틀리면 프로그램 종료
            {
                write(clnt_sock, "3 time wrong \n", 100);
                return-1;
            }
            write(clnt_sock, "******write login PW (error)****** \n", 100);
            strLen = read(clnt_sock, message, BUFSIZE - 1);
        }
    }

    write(clnt_sock, "*****successful login****\n", 50);
    pthread_mutex_lock(&mutx);
    DB[user_num].login_status = ON;
    DB[user_num].my_sock = clnt_sock;
    logined_list[user_num] = ON;
    pthread_mutex_unlock(&mutx);
    printf("%s\n", logined_list);
    return user_num;
}

int Check_ID(char id[]) // 아이디 있는지 확인 DB 번호 return
{
    int i = 0;
    printf("IN Check ID db_num = %d\n\n", db_num);
    for (i = 0; i <= db_num / 2; i++)
    {
        if (strcmp(id, DB[i].ID) == 0)
        {
            if (DB[i].login_status == ON) // 로그인 확인할 때 사용 음수번호를
                return ((-1 * i) - 1);
            return i; // 회원가입 할떄 아이디 이쓰면 번호
        }
    }
    return -100;
}


// 대기화면 함수
void stanby_screen(int clnt_sock, int user_num)
{
    int result_code, i, chat_num;
    int strLen;
    pthread_t chat_room_tread;
    char message[BUFSIZE];
    char TMP[BUFSIZE];
    char DB_tmp[500];
    char tmp[100];

    printf("start standby screen \n");
    while ((strLen = read(clnt_sock, message, BUFSIZE - 1)) != 0)
    {
        strcpy(TMP, "");
        strcpy(DB_tmp, "");
        if (!strcmp(message, "make\n") || !strcmp(message, "1\n")) // 채팅방 생성
        {
            write(clnt_sock, "******write room name******\n", 50);
            sleep(0.3);
            strLen = read(clnt_sock, message, BUFSIZE - 1);
            for (i = 0; i < 10; i++)
            {
                printf("%d make room\n", i);
                if (strcmp(chat_room_name[i], "") == 0)
                {
                    strcpy(chat_room_name[i], message);
                    sleep(0.3);
                    chat_room_cnt++;
                    write(clnt_sock, "", strlen(message) + 1);
                    break;
                }
            }
            if (i == 10)
                write(clnt_sock, "Cannot make chating room\n", 25);
        }
        else if (!strcmp(message, "chat\n") || !strcmp(message, "2\n")) // 채팅방 입장
        {
            memcpy(DB_tmp, "\0", 1);
            memcpy(TMP, "\0", 1);
            printf("chat\n");
            if (chat_room_cnt == 0)
            {
                write(clnt_sock, "no room\n", 10);
            }
            else
            {
                printf("chat room cnt %d \n", chat_room_cnt);
                for (i = 0; i < 10; i++) // 채팅방 이름 알려주기
                {
                    printf("%s", chat_room_name[i]);
                    sprintf(DB_tmp, "%d. ", i);
                    strcat(TMP, DB_tmp);
                    strcat(TMP, chat_room_name[i]);
                    if (!strcmp(chat_room_name[i], ""))
                        strcat(TMP, "\n");
                    sleep(0.1);
                }
                strcat(TMP, "\n");
                printf("%d, %s\n", strlen(TMP), TMP);
                write(clnt_sock, TMP, strlen(TMP));
                sleep(0.1);
                strLen = read(clnt_sock, message, BUFSIZE - 1); // 채팅방 번호를 읽어
                chat_num = atoi(message);
                while (strcmp(chat_room_name[chat_num], "") == 0) // 채팅방이 존재하지 않으면 다시 입력받기
                {
                    for (i = 0; i < chat_room_cnt; i++)
                    {
                        printf("%s", chat_room_name[i]);
                        sprintf(DB_tmp, "%d. ", i);
                        strcat(TMP, DB_tmp);
                        strcat(TMP, chat_room_name[i]);
                        if (!strcmp(chat_room_name[i], ""))
                            strcat(TMP, "\n");
                        sleep(0.3);
                    }
                    strcat(TMP, "\n");
                    write(clnt_sock, TMP, strlen(TMP) + 30);
                    strLen = read(clnt_sock, message, BUFSIZE - 1);
                    chat_num = atoi(message);
                }
                DB[user_num].joined_num = chat_num; //방번호 저장
                for (i = 0; i < 5; i++)             // 사용자가 나갔을 경우를 생각해서 짠 코드
                {
                    if (chat_room_sock[chat_num][i] == 0)
                    {
                        chat_room_sock[chat_num][i] = clnt_sock;
                        chat_room_joinednum[chat_num]++;
                        break;
                    }
                }

                write(clnt_sock, "startchat", 20);
                //chating start
                chat_room(clnt_sock, user_num, chat_num);
            }
        }
        else if (!strcmp(message, "3\n")) // 로그인 목록 확인
        {
            strcpy(TMP, "");
            for (i = 0; i < 21; i++)
            {
                if (logined_list[i] == ON)
                {
                    strncat(TMP, "\t", 1);
                    strncat(TMP, DB[i].ID, 10);
                    printf("%s", TMP);
                }
            }
            write(clnt_sock, TMP, 180);
            strcpy(TMP, "");
        }
        else if (!strcmp(message, "q\n")) // 메인메뉴 로그아웃
        {
            DB[user_num].login_status = OFF;
            logined_list[user_num] = OFF;
            close(clnt_sock);
            return;
        }
        else if (!strcmp(message, "4\n")) // 채팅방 목록 확인
        {
            strcpy(DB_tmp, "");
            if (chat_room_cnt == 0)
            {
                write(clnt_sock, "no room\n", 10);
            }
            else
            {
                printf("chat room cnt %d \n", chat_room_cnt);
                for (i = 0; i < 10; i++)
                {
                    printf("%s", chat_room_name[i]);
                    sprintf(DB_tmp, "%d. ", i);
                    strcat(TMP, DB_tmp);
                    strcat(TMP, chat_room_name[i]);
                    if (!strcmp(chat_room_name[i], ""))
                        strcat(TMP, "\n");
                    sleep(0.3);
                }
                write(clnt_sock, TMP, strlen(TMP));
            }
        }
        else
        {
            write(clnt_sock, "", 10); // 메인메뉴에서 올바르지 않은 값을 입력했을 경우
        }
    }
}
// 채팅하는 함수
void chat_room(int sock, int User_num, int Chat_num)
{
    int i, result_code;
    int str_len;
    char message[BUFSIZE];
    char tmp[BUFSIZE];
    char id[22];
    DB[User_num].chat_status = ON;
    strcpy(id, DB[User_num].ID);
    id[strlen(id) - 1] = '\0';

    while ((str_len = read(sock, message, BUFSIZE - 1)) != 0)
    {
        if (!strcmp(message, "q\n")) // q누르면 채팅방 나가기
        {
            printf("QUIT\n");
            DB[User_num].joined_num = -1;
            DB[User_num].chat_status = OFF;
            chat_room_joinednum[Chat_num]--;
            for (i = 0; i < 5; i++)
            {
                printf("%d  ", chat_room_sock[Chat_num][i]);
                if (DB[User_num].my_sock == chat_room_sock[Chat_num][i])
                {
                    chat_room_sock[Chat_num][i] = 0;
                }
            }
            write(sock, "q\n", 3);
            // 대기화면으로 나가기
            return;
        }
        strcpy(tmp, id);
        strcat(tmp, " : ");
        strncat(tmp, message, strlen(message));
        strcat(tmp, "\0");
        printf("%s", tmp);
        printf("%s", message);

        for (i = 0; i < 5; i++) // 채팅방에 들어온 사람들에게 매세지 보내기
        {
            //printf("%d\n",chat_room_sock[Chat_num][i]);
            if (chat_room_sock[Chat_num][i] != 0)
            {
                write(chat_room_sock[Chat_num][i], tmp, strlen(id) + str_len + 3);
            }
        }

        sleep(0.1);
        strcpy(tmp, "");
        strcpy(message, "");
    }
    return;
}
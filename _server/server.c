/*
	파일전송이 가능한 채팅 프로그램의 서버 소스 코드입니다.
	구현된 기능: 메시지 통신, 파일 수신
	작성자: 이준영, 양시현
	수정이력:
	- 2023-11-26: 초기버전 생성, 파일 수신 기능 추가, port 번호를 변수로 변경, 
    FILE_SIZE 정의 추가, 닉네임 중복 처리 추가
    - 2023-11-27: 닉네임 배열을 NULL로 초기화 하도록 수정, 클라이언트가 접속 해제할 때 닉네임을 지우도록 수정
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>

#define MAX_CLIENTS 10      // 최대 클라이언트 수 정의
#define BUFFER_SIZE 1024    // 메시지 버퍼 크기 정의
#define NICKNAME_LEN 32     // 닉네임 최대 길이 정의
#define FILE_SIZE 256       // 파일 사이즈

void *handle_client(void *arg);
void exit_routine();

char g_nickname[MAX_CLIENTS][NICKNAME_LEN] = {0};           // 현재 접속한 클라이언트의 닉네임을 저장할 배열
int clients[MAX_CLIENTS];                                   // 연결된 클라이언트 소켓 저장 배열
int n_clients = 0;                                          //  현재 연결된 클라이언트 수
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;  // 스레드 간 동기화를 위한 뮤텍스
pthread_mutex_t nicknameMutex = PTHREAD_MUTEX_INITIALIZER;  // 닉네임 저장을 위한 동기화

int main() {
    int server_sock, client_sock;                   // 서버 및 클라이언트 소켓
    struct sockaddr_in server_addr, client_addr;    // 소켓 주소 구조체
    pthread_t thread;                               // 스레드 ID
    socklen_t client_addr_size;                     // 클라이언트 주소 크기
    char user_input[10];
    int port = 50001;// 포트 번호

    for(int i=0; i<MAX_CLIENTS; i++){
        clients[i] = -1;
    }

    // 닉네임 배열 초기화
    memset(g_nickname, 0, sizeof(g_nickname));

    // 서버 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("socket");
        return -1;
    }

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    // 프로그램 종료 루틴 추가
    if (atexit(exit_routine) != 0){
        fprintf(stderr, "종료 루틴 등록 실패!\n");
        exit(1);
    }


    // 소켓에 주소 바인딩
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        return -1;
    }

    // 소켓 리스닝 시작
    if (listen(server_sock, 5) == -1) {
        perror("listen");
        return -1;
    }

    printf("서버가 시작되었습니다... 다른 탭에서 클라이언트를 실행해 주세요\n");

    // 클라이언트 연결 수락 반복문
    while (1) {
        client_addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_size);
        if (client_sock == -1) {
            perror("accept");
            continue;
        }

        // 클라이언트 관리를 위한 뮤텍스 잠금
        pthread_mutex_lock(&clients_mutex);
        if (n_clients < MAX_CLIENTS) {
            clients[n_clients++] = client_sock;
            // 각 클라이언트에 대한 스레드 생성 및 실행
            pthread_create(&thread, NULL, handle_client, (void *)&client_sock);
            pthread_detach(thread); // 스레드 분리 (스레드 종료 시 자동 정리)
        } else {
            printf("최대 클라이언트 수를 초과했습니다.\n");
            close(client_sock); // 초과된 클라이언트 연결 닫기
        }
        pthread_mutex_unlock(&clients_mutex);   // 뮤텍스 잠금 해제
    }

    return 0;
}

/* 클라이언트를 처리하는 스레드 함수 */
void *handle_client(void *arg) {
    int sock = *((int *)arg);           // 클라이언트 소켓
    char nickname[NICKNAME_LEN] = {0};  // 닉네임 저장 배열
    char buffer[BUFFER_SIZE];
    int read_len;
    int name_index = -1;

    // 클라이언트로부터 닉네임 수신
    while(1){
        if ((read_len = recv(sock, nickname, NICKNAME_LEN, 0)) > 0) {
            nickname[read_len] = '\0';
            // 뮤텍스를 이용한 닉네임 중복 확인 및 저장
            pthread_mutex_lock(&nicknameMutex);
            
            int isDuplicate = 0;
            // 중복 확인
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (strcmp(g_nickname[i], nickname) == 0) {
                    // 중복된 닉네임이 이미 존재
                    pthread_mutex_unlock(&nicknameMutex);;
                    isDuplicate = 1;
                    break;
                }
            }
            
            // 중복이 존재한다면 1전송
            if(isDuplicate){
                send(sock, &isDuplicate, sizeof(int), 0);  //성공 여부 전송
                pthread_mutex_unlock(&nicknameMutex);
            }
            else{
                // 중복이 없으면 닉네임 저장
                for(int i=0; i<MAX_CLIENTS; i++){
                    if(g_nickname[i][0] == '\0'){
                        name_index = i;
                        break;
                    }
                }

                strcpy(g_nickname[name_index], nickname);
                pthread_mutex_unlock(&nicknameMutex);
                printf("%s가 연결되었습니다.\n", nickname);
                send(sock, &isDuplicate, sizeof(int), 0);  //성공 여부 전송
                break;
            }
        }
    }

    // 클라이언트로부터 메시지 수신
    while ((read_len = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_len] = '\0';

        if(strcmp(buffer, "/upload") == 0){
            int isnull = 0, success = 0, nbyte;
            char filename[BUFFER_SIZE], buf[FILE_SIZE];
            FILE* file;
            size_t fsize;

			recv(sock, &isnull, sizeof(isnull), 0); // 명령어를 전달받은 후 의 처리
			if(isnull ==1){
				continue;
			}
			
			recv(sock, filename, BUFFER_SIZE, 0);  //파일 이름 수신
			
            char newFilename[BUFFER_SIZE + 6];  // "files/"를 추가하기 때문에 6을 더함
            sprintf(newFilename, "files/%s", filename);
            
			file = fopen(newFilename, "wb");  //쓰기 전용, 이진모드로 파일 열기
			recv(sock, &fsize, sizeof(fsize), 0);  //파일 크기 수신

			nbyte = FILE_SIZE;
			while(nbyte >= FILE_SIZE){
				nbyte = recv(sock, buf, FILE_SIZE, 0);  //256씩 수신
				success = fwrite(buf, sizeof(char), nbyte, file);  //파일 쓰기
				if(nbyte < FILE_SIZE) success =1;
			}
			send(sock, &success, sizeof(int), 0);  //성공 여부 전송
			fclose(file);		
        } else{
            printf("서버가 받은 채팅 [%s]: %s\n", nickname, buffer); // 서버가 받은 메시지 출력
            // send(sock,buffer, BUFFER_SIZE, 0);  //성공 여부 전송
            for(int i = 0; i< MAX_CLIENTS;i++){
                if(clients[i]!= -1)
                    send(clients[i],buffer, BUFFER_SIZE, 0);  //성공 여부 전송
            }
        }
        // printf("%s: %s\n", nickname, buffer);
    }
    
    if (name_index != -1){
        // 접속한 클라이언트의 닉네임 제거
        pthread_mutex_lock(&nicknameMutex);
        g_nickname[name_index][0] = '\0';
        pthread_mutex_unlock(&nicknameMutex);
    }
    
    // 클라이언트와의 연결 종료 처리
    close(sock);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < n_clients; i++) {
        if (clients[i] == sock) {
            clients[i] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    printf("%s가 연결을 종료했습니다.\n", nickname);

    return NULL;
}

// 프로그램 종료 루틴
void exit_routine() {
    for (int i = 0; i < MAX_CLIENTS; i++){
        close(clients[i]);
    }
}
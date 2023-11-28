/*
	파일전송이 가능한 채팅 프로그램의 클라이언트 소스 코드입니다.
	구현된 기능: 메시지 전송, 파일 전송
	작성자: 이준영, 양시현
	수정이력:
	- 2023-11-26: 초기버전 생성, 파일 전송 기능 추가, <arpa/inet.h> 헤더 추가
    port 번호를 변수로 변경, FILE_SIZE 정의 추가, 닉네임 중복 처리 추가
    - 2023-11-27: 프로그램 종료 루틴 추가 
    - 2023-11-28: ip 변수로 수정
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024    // 메시지 버퍼 크기 정의
#define NICKNAME_LEN 32     // 닉네임 최대 길이 정의
#define FILE_SIZE 256 // 파일 사이즈

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;

int sock; // 클라이언트 소켓

void *receive_message(void *socket);
void exit_routine();

int main() {
    struct sockaddr_in server_addr; // 서버 주소 구조체
    pthread_t recv_thread;          // 수신 스레드
    char nickname[NICKNAME_LEN];    // 사용자 닉네임
    char message[BUFFER_SIZE];      // 메시지 입력 버퍼
    char ip[100] = "127.0.0.1\0";
    int port = 50001;                // 포트번호

    // 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return -1;
    }

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);   // 서버의 IP 주소
    server_addr.sin_port = htons(port);                     // 서버의 포트 번호

    // 서버에 연결
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        return -1;
    }

    // 프로그램 종료 루틴 추가
    if (atexit(exit_routine) != 0){
        fprintf(stderr, "종료 루틴 등록 실패!\n");
        exit(1);
    }

    // 닉네임 중복이 존재하는지 확인하는 코드
    while (1){
        // 닉네임 입력 및 서버로 전송
        printf("닉네임을 입력하세요: ");

        fgets(nickname, NICKNAME_LEN, stdin);
        nickname[strcspn(nickname, "\n")] = 0;  // 개행 문자 제거
        fflush(stdin);

        send(sock, nickname, strlen(nickname), 0);  // 닉네임 서버로 전송

        int isDuplicate = -1;
        recv(sock, &isDuplicate, sizeof(int), 0);
        if(isDuplicate == 0){
            break;
        }
        printf("닉네임이 중복됩니다!\n");
    }

    // 수신 스레드 생성
    pthread_create(&recv_thread, NULL, receive_message, (void *)&sock);

    // 사용자가 메시지 입력 및 서버로 전송
    while (1) {
        fgets(message, BUFFER_SIZE, stdin);
        message[strcspn(message, "\n")] = 0; // 개행 문자 제거
        fflush(stdin);
        // "/exit" 입력 시 종료
        if (strcmp(message, "/exit") == 0) {
            break;
        } else if(strcmp(message, "/upload") == 0){
            send(sock, message, strlen(message), 0); // 명령어 전송
            char buf[FILE_SIZE], path[BUFFER_SIZE], filename[BUFFER_SIZE]; // 파일 경로를 저장할 변수
            FILE* file;
	        size_t fsize;
            int success; // 파일전송 성공 여부

            printf("업로드할 파일명 : ");
			scanf("%s", path);  //파일 이름 입력
			getchar();

            char *lastSlash = strrchr(path, '/');
    
            if (lastSlash != NULL) {
                // '/' 이후의 문자열을 복사
                strcpy(filename, lastSlash + 1);
                // printf("추출된 파일 이름: %s\n", filename);
            } else {
                // '/'가 없을 경우 전체 경로를 그대로 사용
                strcpy(filename, path);
                // printf("추출된 파일 이름: %s\n", filename);
            }

            file = fopen(path, "rb");  //읽기 전용, 이진 모드로 파일 열기\

			int isnull =0;
			if(file ==NULL){
				isnull =1;
				send(sock, &isnull, sizeof(isnull), 0);
				printf("해당 파일이 존재하지 않습니다.\n");
				continue;			
			}	
			send(sock, &isnull, sizeof(isnull), 0);  //파일 존재 여부 전송
			
			send(sock, filename, BUFFER_SIZE, 0);  //파일 이름 전송
			
			/*파일 크기 계산*/
			fseek(file, 0, SEEK_END);  //파일 포인터를 끝으로 이동
			fsize = ftell(file);  //파일 크기 계산
			fseek(file, 0, SEEK_SET);  //파일 포인터를 다시 처음으로 이동
			
			size_t size = htonl(fsize);
			send(sock, &size, sizeof(fsize), 0);  //파일 크기 전송
			
			int nsize =0;
			/*파일 전송*/
			while(nsize != fsize){  //256씩 전송
				int fpsize = fread(buf, 1, FILE_SIZE, file);
				nsize += fpsize;
				send(sock, buf, fpsize, 0);
			}
			fclose(file);
			recv(sock, &success, sizeof(int), 0);  //업로드 성공 여부 수신
			
			if (success) printf("업로드가 완료되었습니다.\n");
			else printf("업로드를 실패했습니다.\n");	
        } 
        else if(strcmp(message, "/download") == 0){
            send(sock, message, strlen(message), 0);          // 메시지 서버로 전송
            int isnull = 0, success = 0, nbyte;
            char filename[BUFFER_SIZE], buf[FILE_SIZE];
            FILE* file;
            size_t fsize;

            printf("전송할 파일이름: ");
            scanf("%s",filename);

            send(sock, filename, BUFFER_SIZE, 0); // 파일명 전송

			recv(sock, &isnull, sizeof(isnull), 0); // 명령어를 전달받은 후 의 처리
			if(isnull ==1){
                printf("파일이 존재하지 않습니다.\n");
				continue;
			}

            char newFilename[BUFFER_SIZE + 6];  // "files/"를 추가하기 때문에 6을 더함
            sprintf(newFilename, "files/%s", filename)
            ;
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

            if (success) printf("다운로드가 완료되었습니다.\n");
			else printf("다운로드에 실패했습니다.\n");
            pthread_mutex_lock(&mutex);
            pthread_cond_signal(&condition);
            pthread_mutex_unlock(&mutex);
        }   
        else{
            send(sock, message, strlen(message), 0);          // 메시지 서버로 전송
        }
    }
    close(sock);    // 소켓 닫기
    return 0;
}

/* 서버로부터 메시지를 수신하는 스레드 함수 */
void *receive_message(void *socket) {
    int sock = *((int *)socket);    // 클라이언트 소켓
    char message[BUFFER_SIZE];      // 메시지 저장 버퍼
    int length;

    // 서버로부터 메시지를 계속 수신
    while ((length = recv(sock, message, BUFFER_SIZE * 2, 0)) > 0) {
        message[length] = '\0';
        if(strcmp(message, "/Q") == 0){
            pthread_mutex_lock(&mutex);
            pthread_cond_wait(&condition, &mutex);
            pthread_mutex_unlock(&mutex);
        }  
        else{
            printf("%s\n", message);    // 수신된 메시지 출력
        }
    }

    return NULL;
}

// 프로그램 종료 루틴
void exit_routine(){
    close(sock);
}
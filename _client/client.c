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

void *receive_message(void *socket);

int main() {
    int sock;                       // 클라이언트 소켓
    struct sockaddr_in server_addr; // 서버 주소 구조체
    pthread_t recv_thread;          // 수신 스레드
    char nickname[NICKNAME_LEN];    // 사용자 닉네임
    char message[BUFFER_SIZE];      // 메시지 입력 버퍼
    int port = 1234;                // 포트번호
    
    // 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return -1;
    }

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");   // 서버의 IP 주소
    server_addr.sin_port = htons(port);                     // 서버의 포트 번호

    // 서버에 연결
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        return -1;
    }

    // 닉네임 입력 및 서버로 전송
    printf("닉네임을 입력하세요: ");
    fgets(nickname, NICKNAME_LEN, stdin);
    nickname[strcspn(nickname, "\n")] = 0;  // 개행 문자 제거

    send(sock, nickname, strlen(nickname), 0);  // 닉네임 서버로 전송

    // 수신 스레드 생성
    pthread_create(&recv_thread, NULL, receive_message, (void *)&sock);

    // 사용자가 메시지 입력 및 서버로 전송
    while (1) {
        fgets(message, BUFFER_SIZE, stdin);
        message[strcspn(message, "\n")] = 0; // 개행 문자 제거

        // "/exit" 입력 시 종료
        if (strcmp(message, "/exit") == 0) {
            break;
        } else if(strcmp(message, "/upload") == 0){
            send(sock, message, strlen(message), 0); // 명령어 전송
            char temp[5], buf[FILE_SIZE], filename[BUFFER_SIZE]; // 파일 경로를 저장할 변수
            FILE* file;
	        size_t fsize;
            int success; // 파일전송 성공 여부
            printf("업로드할 파일명 : ");
			scanf("%s", filename);  //파일 이름 입력
			fgets(temp, 5, stdin);  //버퍼에 남은 \n 제거
			
			file = fopen(filename, "rb");  //읽기 전용, 이진 모드로 파일 열기
			
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
        else{
            printf("클라이언트가 보낸 채팅: %s\n", message);    // 사용자가 보낸 메시지 출력
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
    while ((length = recv(sock, message, BUFFER_SIZE, 0)) > 0) {
        message[length] = '\0';
        printf("%s\n", message);    // 수신된 메시지 출력
    }

    return NULL;
}
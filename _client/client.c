/*
	파일전송이 가능한 채팅 프로그램의 클라이언트 소스 코드입니다.
	구현된 기능: 메시지 전송, 파일 전송
	작성자: 이준영, 양시현
	수정이력:
	- 2023-11-26: 초기버전 생성, 파일 전송 기능 추가, <arpa/inet.h> 헤더 추가
    port 번호를 변수로 변경, FILE_SIZE 정의 추가, 닉네임 중복 처리 추가
    - 2023-11-27: 프로그램 종료 루틴 추가 
    - 2023-11-28: ip 변수로 수정
    - 2023-11-30: 채팅 및 파일전송 방송 추가, UI 추가
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
// UI
#include <gtk/gtk.h>



#define BUFFER_SIZE 1024    // 메시지 버퍼 크기 정의
#define NICKNAME_LEN 32     // 닉네임 최대 길이 정의
#define FILE_SIZE 256 // 파일 사이즈

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;

// UI 변수
GtkWidget *connect_button;
GtkWidget *ip_entry;            // ip 입력창
GtkWidget *port_entry;          // port 입력창
GtkWidget *room_entry;          // 방번호 입력창
GtkWidget *window;              // 메인창
GtkWidget *send_button;         // 보내기 버튼
GtkWidget *text_view;           // 채팅 로그 박스
GtkWidget *chat_entry;          // 메시지 입력창
GtkWidget *file_chooser_button;  // 파일 업로드 버튼
GtkWidget *download_button;  // 파일 업로드 버튼
// 클라이언트 변수 선언
int sock; // 클라이언트 소켓
struct sockaddr_in server_addr; // 서버 주소 구조체
pthread_t recv_thread;          // 수신 스레드
char nickname[NICKNAME_LEN];    // 사용자 닉네임
char ip[100] = "127.0.0.1\0";   // IP번호
int port = 50001;               // 포트번호
int room_no;                    // 방번호

void *receive_message(void *socket);
void exit_routine();

// UI함수
void show_nickname_dialog(GtkWidget *parent);                               // 닉네임 입력창
void show_warning_dialog(GtkWidget *parent, const gchar *message);          // 경고창 UI
void connect_button_clicked(GtkWidget *widget, gpointer data);              // 연결 버튼 함수
void send_button_clicked(GtkWidget *widget, gpointer data);                 // 보내기 버튼 함수
void append_text_to_textview(GtkTextView *textview, const gchar *text);     // 채팅 삽입 함수
void on_entry_activate(GtkEntry *entry, gpointer user_data);                        // 엔터 처리
void on_file_selected(GtkFileChooserButton *filechooserbutton, gpointer user_data); // 파일 선택기
void download_button_clicked(GtkWidget *widget, gpointer data);             // 다운로드 버튼

int main(int argc, char *argv[]) {
    // 프로그램 종료 루틴 추가
    if (atexit(exit_routine) != 0){
        fprintf(stderr, "종료 루틴 등록 실패!\n");
        exit(1);
    }

    gtk_init(&argc, &argv);

    // 윈도우 생성
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "chatting program");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_widget_set_size_request(window, 520, 780);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // GtkFixed 위젯 생성
    GtkWidget *fixed = gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(window), fixed);

    // IP 텍스트 박스 및 레이블
    GtkWidget *ip_label = gtk_label_new("IP:");
    gtk_fixed_put(GTK_FIXED(fixed), ip_label, 10, 17);

    // IP 입력칸
    ip_entry = gtk_entry_new();
    gtk_fixed_put(GTK_FIXED(fixed), ip_entry, 40, 10);
    gtk_entry_set_width_chars(GTK_ENTRY(ip_entry), 8);
    gtk_widget_set_size_request(ip_entry, 140, 30);
    // 기본값 설정
    gtk_entry_set_text(GTK_ENTRY(ip_entry), "127.0.0.1");

    // Port 텍스트 박스 및 레이블
    GtkWidget *port_label = gtk_label_new("Port:");
    gtk_fixed_put(GTK_FIXED(fixed), port_label, 200, 17);

    port_entry = gtk_entry_new();
    gtk_fixed_put(GTK_FIXED(fixed), port_entry, 245, 10);
    gtk_entry_set_width_chars(GTK_ENTRY(port_entry), 8);
    gtk_widget_set_size_request(port_entry, 50, 30);
    // 기본값 설정
    gtk_entry_set_text(GTK_ENTRY(port_entry), "50001");

    // '연결' 버튼
    connect_button = gtk_button_new_with_label("Connect");
    gtk_fixed_put(GTK_FIXED(fixed), connect_button, 345, 10);
    gtk_widget_set_size_request(connect_button, 70, -1);
    // 이벤트 함수 연결
    g_signal_connect(connect_button, "clicked", G_CALLBACK(connect_button_clicked), NULL);

    // 업로드 라벨
    GtkWidget *upload_label = gtk_label_new("upload:");
    gtk_fixed_put(GTK_FIXED(fixed), upload_label, 155, 77);
    // '업로드' 버튼
    file_chooser_button = gtk_file_chooser_button_new("업로드할 파일 선택", GTK_FILE_CHOOSER_ACTION_OPEN);
    gtk_fixed_put(GTK_FIXED(fixed), GTK_WIDGET(file_chooser_button), 215, 70);
    gtk_widget_set_size_request(GTK_WIDGET(file_chooser_button), 100, -1);
    // 이벤트 함수 연결
    g_signal_connect(file_chooser_button, "file-set", G_CALLBACK(on_file_selected), NULL);
    gtk_widget_set_sensitive(file_chooser_button, FALSE);
    // '다운로드' 버튼
    download_button = gtk_button_new_with_label("Download");
    gtk_fixed_put(GTK_FIXED(fixed), GTK_WIDGET(download_button), 330, 70);
    gtk_widget_set_size_request(GTK_WIDGET(download_button), 100, -1);
    // 클릭 이벤트에 대한 콜백 함수 연결
    g_signal_connect(download_button, "clicked", G_CALLBACK(download_button_clicked), NULL); // text_view는 GtkTextView
    gtk_widget_set_sensitive(download_button, FALSE);

    // 채팅창 번호 텍스트 박스 및 레이블
    GtkWidget *room_label = gtk_label_new("Room:");
    gtk_fixed_put(GTK_FIXED(fixed), room_label, 10, 77);

    room_entry = gtk_entry_new();
    gtk_fixed_put(GTK_FIXED(fixed), room_entry, 60, 70);
    gtk_entry_set_width_chars(GTK_ENTRY(room_entry), 8);
    gtk_widget_set_size_request(room_entry, 50, 30);



    text_view = gtk_text_view_new(); // GtkTextView 위젯 생성

    // 텍스트 뷰의 스크롤 가능 여부 설정 (선택 사항)
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                GTK_POLICY_AUTOMATIC,
                                GTK_POLICY_AUTOMATIC);

    // GtkTextView에 사용자 입력 비활성화 및 커서 숨김 설정
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE); // 입력 비활성화
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE); // 커서 숨김

    
    // 부모 위젯에 추가 (예시: fixed라는 GtkFixed 위젯을 부모로 추가)
    gtk_fixed_put(GTK_FIXED(fixed), scrolled_window, 10, 120);

    gtk_widget_set_size_request(scrolled_window, 430, 500);


    // 채팅창 텍스트 박스 및 레이블
    GtkWidget *chat_label = gtk_label_new("chat:");
    gtk_fixed_put(GTK_FIXED(fixed), chat_label, 10, 637);

    chat_entry = gtk_entry_new();
    gtk_fixed_put(GTK_FIXED(fixed), chat_entry, 55, 630);
    gtk_entry_set_width_chars(GTK_ENTRY(chat_entry), 8);
    gtk_widget_set_size_request(chat_entry, 300, 30);

    // '보내기' 버튼
    send_button = gtk_button_new_with_label("↵");
    gtk_fixed_put(GTK_FIXED(fixed), send_button, 370, 630);
    gtk_widget_set_size_request(send_button, 70, -1);
    gtk_widget_set_sensitive(send_button, FALSE);

    // 클릭 이벤트에 대한 콜백 함수 연결
    g_signal_connect(send_button, "clicked", G_CALLBACK(send_button_clicked), text_view); // text_view는 GtkTextView

    // 데이터 전달을 위해 chat_entry를 'send_button'에 연결
    g_object_set_data(G_OBJECT(send_button), "ENTRY", chat_entry);


    // 화면 표시 및 메인 루프 시작
    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}

/* 채팅 삽입 함수 */
void append_text_to_textview(GtkTextView *textview, const gchar *text) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview);
    GtkTextIter iter;

    // 텍스트 버퍼의 끝 위치 얻기
    gtk_text_buffer_get_end_iter(buffer, &iter);

    // 텍스트 끝에 줄바꿈 추가
    gchar *text_with_newline = g_strdup_printf("%s\n", text);

    // 텍스트 삽입
    gtk_text_buffer_insert(buffer, &iter, text_with_newline, -1);

    // 메모리 해제
    g_free(text_with_newline);
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
            // printf("%s\n", message);    // 수신된 메시지 출력
            append_text_to_textview(GTK_TEXT_VIEW(text_view), message);
        }
    }

    return NULL;
}

// 프로그램 종료 루틴
void exit_routine(){
    close(sock);
}

// 경고창
void show_warning_dialog(GtkWidget *parent, const gchar *message) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_WARNING,
                                               GTK_BUTTONS_OK,
                                               "%s", message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// '연결' 버튼 클릭 시 호출되는 콜백 함수
void connect_button_clicked(GtkWidget *widget, gpointer data) {
    const gchar *ip_text = gtk_entry_get_text(GTK_ENTRY(ip_entry));
    const gchar *port_text = gtk_entry_get_text(GTK_ENTRY(port_entry));
    const gchar *room_no_text = gtk_entry_get_text(GTK_ENTRY(room_entry));
    
    if (!(g_utf8_strlen(ip_text, -1) > 0) || !(g_utf8_strlen(port_text, -1) > 0) ||
        !(g_utf8_strlen(room_no_text, -1) > 0)) {
        show_warning_dialog(GTK_WIDGET(window), "엔트리에 값이 입력되지 않았습니다.");
        return;
    }
        
    // 여기에 연결하는 동작을 추가할 수 있습니다
    // g_print("연결 버튼이 클릭되었습니다!\n");
    
    /* 서버 연결부 */
    strcpy(ip, ip_text);
    // g_print("입력된 IP: %s\n", ip);

    // 텍스트를 정수로 변환하여 port 변수에 저장
    // port = atoi(port_text);

    // port 변수에 저장된 값을 출력
    // g_print("입력된 Port: %d\n", port);

     // 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        show_warning_dialog(GTK_WIDGET(window), "소켓생성에 실패하였습니다.");
        perror("socket");
        return;
    }
    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);   // 서버의 IP 주소
    server_addr.sin_port = htons(port);                     // 서버의 포트 번호

    // 서버에 연결
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        show_warning_dialog(GTK_WIDGET(window), "서버연결에 실패하였습니다.");
        perror("connect");
        return;
    }else{
        // 연결 성공시 버튼 비활성화
        gtk_widget_set_sensitive(connect_button, FALSE);
        // IP, PORT, 방번호 입력창 비활성화
        gtk_widget_set_sensitive(ip_entry, FALSE);
        gtk_widget_set_sensitive(port_entry, FALSE);
        gtk_widget_set_sensitive(room_entry, FALSE);
        // upload, download 활성화
        gtk_widget_set_sensitive(file_chooser_button, TRUE);
        gtk_widget_set_sensitive(download_button, TRUE);
        // 보내기 버튼 활성화
        gtk_widget_set_sensitive(send_button, TRUE);
        // 엔터를 누르면 보내기 버튼이 눌리도록 설정
        g_signal_connect(G_OBJECT(chat_entry), "activate", G_CALLBACK(on_entry_activate), send_button);
    }

    /* 방번호, 닉네임 설정부 */
    room_no = atoi(room_no_text);
    // g_print("입력된 방번호: %d\n", room_no);
    if (send(sock, &room_no, sizeof(int), 0) < 0) {
        show_warning_dialog(GTK_WIDGET(window), "방번호 전송에 실패하였습니다.");
        perror("send failed");
        // 오류 처리
    }

    // 닉네임 중복이 존재하는지 확인하는 코드
    while (1){
        // 닉네임 입력창 생성
        show_nickname_dialog(window);
        
        // 닉네임 서버로 전송
        if (send(sock, nickname, strlen(nickname), 0) < 0) {
            show_warning_dialog(GTK_WIDGET(window), "닉네임 전송에 실패하였습니다.");
            perror("send failed");
            // 오류 처리
        }

        int isDuplicate = -1;   
        recv(sock, &isDuplicate, sizeof(int), 0);
        if(isDuplicate == 0){
            break;
        } else{
            show_warning_dialog(GTK_WIDGET(window), "닉네임이 중복됩니다.");
            printf("닉네임이 중복됩니다!\n");
        }
        
    }
    
    // 수신 스레드 생성
    pthread_create(&recv_thread, NULL, receive_message, (void *)&sock);

}

/* 파일 선택기 */
void on_file_selected(GtkFileChooserButton *filechooserbutton, gpointer user_data) {
    char message[BUFFER_SIZE] = "/upload";
    char buf[FILE_SIZE], path[BUFFER_SIZE], filename[BUFFER_SIZE]; // 파일 경로를 저장할 변수
    FILE* file;
    size_t fsize;
    int success; // 파일전송 성공 여부
    const gchar *select_filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filechooserbutton));
    
    strcpy(path, select_filename);
    // g_print("선택된 파일: %s\n%s\n", select_filename, path);

    send(sock, message, strlen(message), 0); // 명령어 전송

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

// '다운로드' 버튼 클릭 시 호출되는 콜백 함수
void download_button_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog, *content_area, *entry, *label;
    const gchar *select_file;
    int isnull = 0, success = 0, nbyte;
    char filename[BUFFER_SIZE], buf[FILE_SIZE];
    FILE* file;
    size_t fsize;
    
    // 다이얼로그 생성
    dialog = gtk_dialog_new_with_buttons("다운로드할 파일",
                                         GTK_WINDOW(window),
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "확인",
                                         GTK_RESPONSE_ACCEPT,
                                         "취소", GTK_RESPONSE_CANCEL,
                                         NULL);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    // 라벨 추가
    label = gtk_label_new("다운로드할 파일의 이름:");
    gtk_container_add(GTK_CONTAINER(content_area), label);

    // 엔트리 추가
    entry = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(content_area), entry);

    // 다이얼로그 표시
    gtk_widget_show_all(dialog);

    // 다이얼로그 실행
    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_ACCEPT) {
        select_file = gtk_entry_get_text(GTK_ENTRY(entry));
        strcpy(filename, select_file);
        // g_print("입력된 파일: %s\n%s\n", select_file,filename);
    }
    else if (result == GTK_RESPONSE_CANCEL) {
        // 취소 버튼이 눌렸을 때의 처리
        gtk_widget_destroy(dialog);  // 다이얼로그 파괴
        return;  // 함수 종료
    }
    char message[BUFFER_SIZE] = "/download";
    send(sock, message, strlen(message), 0);          // 메시지 서버 다운로드 요청

    // 다이얼로그 파괴
    gtk_widget_destroy(dialog);

    

    send(sock, filename, BUFFER_SIZE, 0); // 파일명 전송

    recv(sock, &isnull, sizeof(isnull), 0); // 파일이 있는지 없는지 확인
    if(isnull ==1){
        show_warning_dialog(GTK_WIDGET(window), "파일이 존재하지 않습니다.");
        printf("파일이 존재하지 않습니다.\n");
        pthread_mutex_lock(&mutex);
        pthread_cond_signal(&condition);
        pthread_mutex_unlock(&mutex);
        return;
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

    if (success){
        printf("다운로드가 완료되었습니다.\n");
        show_warning_dialog(GTK_WIDGET(window), "다운로드가 완료되었습니다.");
    } 
    else{ 
        printf("다운로드에 실패했습니다.\n");
        show_warning_dialog(GTK_WIDGET(window), "다운로드에 실패했습니다.");
    }
    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&condition);
    pthread_mutex_unlock(&mutex);
}

// '보내기' 버튼 클릭 시 호출되는 콜백 함수
void send_button_clicked(GtkWidget *widget, gpointer data) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(data)); // data는 GtkTextView
    GtkWidget *entry_widget = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "ENTRY")); // chat_entry

    char message[BUFFER_SIZE]; // 메시지 입력 버퍼
    const gchar *message_text = gtk_entry_get_text(GTK_ENTRY(entry_widget)); // chat_entry에서 텍스트 가져오기

    strcpy(message, message_text);
    send(sock, message, strlen(message), 0); 
    gtk_entry_set_text(GTK_ENTRY(chat_entry), "");
}

void on_entry_activate(GtkEntry *entry, gpointer user_data) {
    // 엔터 키가 눌렸을 때 send_button 클릭 시그널을 발생시킴
    g_signal_emit_by_name(user_data, "clicked");
}

void show_nickname_dialog(GtkWidget *parent) {
    GtkWidget *dialog, *content_area, *entry, *label;
    const gchar *name;

    // 다이얼로그 생성
    dialog = gtk_dialog_new_with_buttons("닉네임 입력",
                                         GTK_WINDOW(parent),
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "확인",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    // 라벨 추가
    label = gtk_label_new("닉네임을 입력하세요:");
    gtk_container_add(GTK_CONTAINER(content_area), label);

    // 엔트리 추가
    entry = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(content_area), entry);

    // 다이얼로그 표시
    gtk_widget_show_all(dialog);

    // 다이얼로그 실행
    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_ACCEPT) {
        name = gtk_entry_get_text(GTK_ENTRY(entry));
        // g_print("입력된 닉네임: %s\n", name);
        strcpy(nickname, name);
    }

    // 다이얼로그 파괴
    gtk_widget_destroy(dialog);
}
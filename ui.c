#include <gtk/gtk.h>

// '연결' 버튼 클릭 시 호출되는 콜백 함수
void connect_button_clicked(GtkWidget *widget, gpointer data) {
    // 여기에 연결하는 동작을 추가할 수 있습니다
    g_print("연결 버튼이 클릭되었습니다!\n");
}

// '보내기' 버튼 클릭 시 호출되는 콜백 함수
void send_button_clicked(GtkWidget *widget, gpointer data) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(data)); // data는 GtkTextView
    GtkWidget *entry_widget = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "ENTRY")); // chat_entry

    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry_widget)); // chat_entry에서 텍스트 가져오기

    // 텍스트 추가
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(buffer, &iter);
    gtk_text_buffer_insert(buffer, &iter, text, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1); // 한 줄씩 쌓이도록 줄 바꿈 추가
    
    gtk_entry_set_text(GTK_ENTRY(entry_widget), "");
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // 윈도우 생성
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
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


    GtkWidget *ip_entry = gtk_entry_new();
    gtk_fixed_put(GTK_FIXED(fixed), ip_entry, 40, 10);
    gtk_entry_set_width_chars(GTK_ENTRY(ip_entry), 8);
    gtk_widget_set_size_request(ip_entry, 140, 30);

    // Port 텍스트 박스 및 레이블
    GtkWidget *port_label = gtk_label_new("Port:");
    gtk_fixed_put(GTK_FIXED(fixed), port_label, 200, 17);

    GtkWidget *port_entry = gtk_entry_new();
    gtk_fixed_put(GTK_FIXED(fixed), port_entry, 245, 10);
    gtk_entry_set_width_chars(GTK_ENTRY(port_entry), 8);
    gtk_widget_set_size_request(port_entry, 50, 30);

    // '연결' 버튼
    GtkWidget *connect_button = gtk_button_new_with_label("Connect");
    gtk_fixed_put(GTK_FIXED(fixed), connect_button, 345, 10);
    gtk_widget_set_size_request(connect_button, 70, -1);

    // 채팅창 번호 텍스트 박스 및 레이블
    GtkWidget *room_label = gtk_label_new("Room Num:");
    gtk_fixed_put(GTK_FIXED(fixed), room_label, 10, 77);

    GtkWidget *room_entry = gtk_entry_new();
    gtk_fixed_put(GTK_FIXED(fixed), room_entry, 105, 70);
    gtk_entry_set_width_chars(GTK_ENTRY(room_entry), 8);
    gtk_widget_set_size_request(room_entry, 50, 30);



    GtkWidget *text_view = gtk_text_view_new(); // GtkTextView 위젯 생성

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

    GtkWidget *chat_entry = gtk_entry_new();
    gtk_fixed_put(GTK_FIXED(fixed), chat_entry, 55, 630);
    gtk_entry_set_width_chars(GTK_ENTRY(chat_entry), 8);
    gtk_widget_set_size_request(chat_entry, 300, 30);

    // '보내기' 버튼
    GtkWidget *send_button = gtk_button_new_with_label("↵");
    gtk_fixed_put(GTK_FIXED(fixed), send_button, 370, 630);
    gtk_widget_set_size_request(send_button, 70, -1);

    // 클릭 이벤트에 대한 콜백 함수 연결
    g_signal_connect(send_button, "clicked", G_CALLBACK(send_button_clicked), text_view); // text_view는 GtkTextView

    // 데이터 전달을 위해 chat_entry를 'send_button'에 연결
    g_object_set_data(G_OBJECT(send_button), "ENTRY", chat_entry);


    // 화면 표시 및 메인 루프 시작
    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}

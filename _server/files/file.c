#include <gtk/gtk.h>

void on_file_selected(GtkFileChooserButton *filechooserbutton, gpointer user_data) {
    const gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filechooserbutton));
    char *filename_utf8 = g_strdup(filename);
    g_print("선택된 파일: %s\n", filename_utf8);

    // 메모리 해제
    g_free(filename_utf8);
}

void show_entry_dialog(GtkWidget *widget, gpointer window) {
    GtkWidget *dialog, *content_area, *entry;
    const gchar *text;

    dialog = gtk_dialog_new_with_buttons("입력 받기",
                                         GTK_WINDOW(window),
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "확인",
                                         GTK_RESPONSE_ACCEPT,
                                         "취소",
                                         GTK_RESPONSE_CANCEL,
                                         NULL);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry), 50);
    gtk_container_add(GTK_CONTAINER(content_area), entry);

    gtk_widget_show_all(dialog);

    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_ACCEPT) {
        text = gtk_entry_get_text(GTK_ENTRY(entry));
        g_print("입력된 텍스트: %s\n", text);
    }

    gtk_widget_destroy(dialog);
}


int main(int argc, char *argv[]) {
    GtkWidget *button_box, *button;
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // 다이얼로그
    button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_container_add(GTK_CONTAINER(window), button_box);

    button = gtk_button_new_with_label("입력 받기");
    g_signal_connect(button, "clicked", G_CALLBACK(show_entry_dialog), window);
    gtk_container_add(GTK_CONTAINER(button_box), button);

    GtkWidget *file_chooser_button = gtk_file_chooser_button_new("파일 선택", GTK_FILE_CHOOSER_ACTION_OPEN);
    g_signal_connect(file_chooser_button, "file-set", G_CALLBACK(on_file_selected), NULL);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(box), file_chooser_button, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(window), box);

    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}

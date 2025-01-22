
void form_get_error(int status_code, int newsockfd);

int read_file(const char *filepath, int client_socket, const char *method);

void form_success_response(int newsockfd, int content_length);

int get_file_size(const char *filepath);

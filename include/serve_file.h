
void form_get_error(int status_code, int newsockfd, const char *method);

int read_file(const char *filepath, int client_socket);

void form_success_response(int newsockfd, int content_length, const char *content_type);

int get_file_size(const char *filepath);

const char *get_content_type(const char *filename);

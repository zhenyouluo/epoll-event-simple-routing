
#ifndef _REQUEST_H
#define _REQUEST_H


int request_data_parse(char *buf);
void request_save_srcfd_to_hash(int socket_fd,void **data);
int request_get_destfd_from_hash(void);
int request_remove_fd(char *name);

#endif
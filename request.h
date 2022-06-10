#ifndef __REQUEST_H__

#define STATIC_REQ 1
#define ERROR_REQ 0
#define DYN_REQ -1

int requestHandle(int fd, double a, double dma, int t_id, int req, int req_s, int req_d);

#endif

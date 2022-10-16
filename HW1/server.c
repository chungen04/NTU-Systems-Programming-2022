#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>

#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)

#define OBJ_NUM 3
#define FOOD_INDEX 0
#define CONCERT_INDEX 1
#define ELECS_INDEX 2
#define RECORD_PATH "./bookingRecord"

#define BOOKING_BOUND 15

static char* obj_names[OBJ_NUM] = {"Food", "Concert", "Electronics"};

typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

typedef struct request {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
    int id;
    int wait_for_write;  // used by handle_read to know if the header is read or not.
} request;

server svr;  // server
request* requestP = NULL;  // point to a list of requests
int maxfd;  // size of open file descriptor table, size of request list

const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";
const char* ask_for_id = "Please enter your id (to check your booking state):\n";
const unsigned char IAC_IP[3] = "\xff\xf4";

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

typedef struct record{
    int id;          // 902001-902020
    int bookingState[OBJ_NUM]; // 1 means booked, 0 means not.
} record;

int handle_read(request* reqP) {
    /*  Return value:
     *      1: read successfully
     *      0: read EOF (client down)
     *     -1: read failed
     */
    int r;
    char buf[512] = {};

    // Read in request from client
    r = read(reqP->conn_fd, buf, sizeof(buf));
    // fprintf(stderr, "buffer: %s\n", buf);
    if (r < 0) return -1;
    if (r == 0) return 0;
    char* p1 = strstr(buf, "\015\012");
    int newline_len = 2;
    if (p1 == NULL) {
       p1 = strstr(buf, "\012");
        if (p1 == NULL) {
            if (!strncmp(buf, IAC_IP, 2)) {
                // Client presses ctrl+C, regard as disconnection
                fprintf(stderr, "Client presses ctrl+C....\n");
                return 0;
            }
            ERR_EXIT("this really should not happen...");
        }
    }
    size_t len = p1 - buf + 1;
    memmove(reqP->buf, buf, len);
    reqP->buf[len - 1] = '\0';
    reqP->buf_len = len-1;
    return 1;
}

int handle_id_data(request* reqP){ // check whether the id is in range.
    if(strlen(reqP->buf) == 6 &&
        reqP->buf[0] == '9' &&
        reqP->buf[1] == '0' && 
        reqP->buf[2] == '2' && 
        reqP->buf[3] == '0' && 
        (( reqP->buf[4] == '0' && reqP->buf[5] >= '1' && reqP->buf[5] <= '9')|| 
        (reqP->buf[4] == '1' && reqP->buf[5] >= '0' && reqP->buf[5] <= '9') ||
        (reqP->buf[4] == '2' && reqP->buf[5] == '0'))
    ){
        int id = (reqP->buf[4]-'0')*10+(reqP->buf[5]-'0')-1;
        reqP->id = id;
        return 1;
    }
    return 0;
}

int handle_output_status(request* requestP, int conn_fd, int file_fd){ 
    /* 
        handle the cases which they want to ask for the booking status.
    */

    char buf[512];
    sprintf(buf, "response: %s : %s \n", accept_read_header, requestP[conn_fd].buf);

    record* data = calloc(1, sizeof(record));
    lseek(file_fd, requestP[conn_fd].id*sizeof(record), SEEK_SET);
    read(file_fd, data, sizeof(record));
    
    //now try to obtain the data and write to buf
    sprintf(buf, "Food: %d booked\nConcert: %d booked\nElectronics: %d booked\n", data->bookingState[FOOD_INDEX], data->bookingState[CONCERT_INDEX], data->bookingState[ELECS_INDEX]);
    write(requestP[conn_fd].conn_fd, buf, strlen(buf)); //handle write in buffer data
    free(data);
    return 0;
}

int is_int(char* p){
    if(strcmp(p, "-0") == 0) return 1;
    int num = atoi(p);
    char str[32];
    sprintf(str, "%d", num);
    return strcmp(str, p) == 0;
}

int handle_writing_data(request* requestP, int file_fd){
    /*
        return 1 means success
        return 0 means invalid input
        return -1 means invalid operation
    */
    // when calling this, request id should be set
    char buf[512];
    char* amount_FOOD_str;
    char* amount_CONCERT_str;
    char* amount_ELEC_str;
    amount_FOOD_str = strtok(requestP->buf, " ");
    amount_CONCERT_str = strtok(NULL, " ");
    amount_ELEC_str = strtok(NULL, " ");
    if(!(is_int(amount_FOOD_str) && is_int(amount_CONCERT_str) && is_int(amount_ELEC_str))){
        return 0;
    }
    int amount_FOOD = atoi(amount_FOOD_str);
    int amount_CONCERT = atoi(amount_CONCERT_str);
    int amount_ELEC = atoi(amount_ELEC_str);
    
    record* data = calloc(1, sizeof(record));
    lseek(file_fd, requestP->id*sizeof(record), SEEK_SET);
    read(file_fd, data, sizeof(record));
    if(data->bookingState[FOOD_INDEX] + amount_FOOD + data->bookingState[CONCERT_INDEX] + amount_CONCERT + data->bookingState[ELECS_INDEX] + amount_ELEC >15){
        strcpy(buf, "[Error] Sorry, but you cannot book more than 15 items in total.\n");
        write(requestP->conn_fd, buf, strlen(buf));
        free(data);
        return -1;
    }
    if(data->bookingState[FOOD_INDEX] + amount_FOOD < 0 || data->bookingState[CONCERT_INDEX] + amount_CONCERT < 0 || data->bookingState[ELECS_INDEX] + amount_ELEC < 0){
        strcpy(buf, "[Error] Sorry, but you cannot book less than 0 items.\n");
        write(requestP->conn_fd, buf, strlen(buf));
        free(data);
        return -1;
    }
    //manipulate new data on data
    data->bookingState[FOOD_INDEX] += amount_FOOD;
    data->bookingState[CONCERT_INDEX] += amount_CONCERT;
    data->bookingState[ELECS_INDEX] += amount_ELEC;
    lseek(file_fd, requestP->id*sizeof(record), SEEK_SET);
    write(file_fd, data, sizeof(record));

    sprintf(buf, "Bookings for user %d are updated, the new booking state is:\nFood: %d booked\nConcert: %d booked\nElectronics: %d booked\n", 902000+requestP->id+1, data->bookingState[FOOD_INDEX], data->bookingState[CONCERT_INDEX], data->bookingState[ELECS_INDEX]);
    write(requestP->conn_fd, buf, strlen(buf));
    free(data);
    return 1;
}

#define read_lock(fd, offset) lock_reg((fd), F_SETLK, F_RDLCK, (offset))
#define write_lock(fd, offset) lock_reg((fd), F_SETLK, F_WRLCK, (offset))
#define unlock(fd, offset) lock_reg((fd), F_SETLK, F_UNLCK, (offset))
// for getlock, directly call fcntl(fd, F_GETLK, &lock)

int lock_reg(int fd, int cmd, int type, off_t offset){
    struct flock lock;
    lock.l_type = type;
    lock.l_start = offset;
    lock.l_whence = SEEK_SET;
    lock.l_len = sizeof(record); // always lock these bytes in every scene
    return(fcntl(fd, cmd, &lock));
}

int main(int argc, char** argv) {

    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;

    int conn_fd;  // fd for a new connection with client
    int file_fd;  // fd for file that we open for reading
    char buf[512] = {};
    
    // Initialize server
    init_server((unsigned short) atoi(argv[1]));

    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);

    file_fd = open(RECORD_PATH, O_RDWR); // BLOCK or NONBLOCK?
    if (file_fd == 0){
        fprintf(stderr, "\nError opening file\n");
        exit (1);
    }
    fd_set master_fds; 
    fd_set read_fds;
    FD_ZERO(&master_fds); // init
    FD_ZERO(&read_fds); // init

    FD_SET(svr.listen_fd, &master_fds); // add server.listen_fd to master

    int* read_lock_ref_count = calloc(20, sizeof(int));
    int* write_lock_ref_count = calloc(20, sizeof(int));

    maxfd = svr.listen_fd + 1;
    // fprintf(stderr, "%d\n", maxfd);
    while (1) {
        // TODO: Add IO multiplexing
        
        memcpy(&read_fds, &master_fds, sizeof(master_fds));
        if(select(maxfd, &read_fds, NULL, NULL, NULL) < 0){
            fprintf(stderr, "exit due to select error");
            break;
        }
        
        //check every fds listening to
        for(int i = 0; i<maxfd; i++){
	        if(FD_ISSET(i, &read_fds)){ // select detects event
                if(i == svr.listen_fd){ // new connection
                    clilen = sizeof(cliaddr);
                    conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
                    if (conn_fd < 0) {
                        if (errno == EINTR || errno == EAGAIN) continue;  // try again
                        if (errno == ENFILE) {
                            (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                            continue;
                        }
                        ERR_EXIT("accept");
                    }
                    // start handle client
                    requestP[conn_fd].conn_fd = conn_fd;
                    if(conn_fd >= maxfd){
			            maxfd = conn_fd + 1;
                    }
                    write(conn_fd, ask_for_id, strlen(ask_for_id)); 

                    strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
                    fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);
                    FD_SET(conn_fd, &master_fds);
                }else{ // previous connection
                    int ret = handle_read(&requestP[i]); // parse data from client to requestP[conn_fd].buf
                    fprintf(stderr, "ret = %d\n", ret);
                    if (ret < 0) {
                        fprintf(stderr, "bad request from %s\n", requestP[i].host);
                        FD_CLR(requestP[i].conn_fd, &master_fds);
                        close(requestP[i].conn_fd);
                        free_request(&requestP[i]);
                        continue; // handle bad request while read fd error
                    }
                    else if (ret == 0){
                        fprintf(stderr, "disconnected from %s\n", requestP[i].host);
#ifdef READ_SERVER 
                        if(requestP[i].wait_for_write != 0){ // inputed id; needs to release read lock
                            read_lock_ref_count[requestP[i].id] -= 1;
                            if(read_lock_ref_count[requestP[i].id] == 0){
                                unlock(file_fd, (off_t)requestP[i].id*sizeof(record));
                            }
                        }
#elif defined WRITE_SERVER
                        write_lock_ref_count[requestP[i].id] = 0;
                        unlock(file_fd, (off_t)requestP[i].id*sizeof(record));
#endif
                        FD_CLR(requestP[i].conn_fd, &master_fds);
                        close(requestP[i].conn_fd);
                        free_request(&requestP[i]);
                        continue;
                    }
#ifdef READ_SERVER 
                    //first check id
                    if(handle_id_data(&requestP[i]) != 1 && requestP[i].wait_for_write == 0){ // invalid input
                        strcpy(buf, "[Error] Operation failed. Please try again.\n");
                        write(requestP[i].conn_fd, buf, strlen(buf));
                        FD_CLR(requestP[i].conn_fd, &master_fds);
                        close(requestP[i].conn_fd);
                        free_request(&requestP[i]);
                    }else if(handle_id_data(&requestP[i]) == 1 && requestP[i].wait_for_write == 0){
                        //first handle lock
                        if(read_lock(file_fd, (off_t)requestP[i].id*sizeof(record))<0){ // lock get by other process' write process
                            strcpy(buf, "Locked.\n");
                            write(requestP[i].conn_fd, buf, strlen(buf));
                            FD_CLR(requestP[i].conn_fd, &master_fds);
                            close(requestP[i].conn_fd);
                            free_request(&requestP[i]);
                            continue;
                        }else{
                            read_lock_ref_count[requestP[i].id] += 1; // lock get
                            handle_output_status(requestP, requestP[i].conn_fd, file_fd);
                            strcpy(buf, "\n(Type Exit to leave...)\n");
                            write(requestP[i].conn_fd, buf, strlen(buf));
                            requestP[i].wait_for_write = 1;
                        }  
                    }else if(requestP[i].wait_for_write == 1){ // handle final output (upon receiving EXIT)
                        if(strcmp(requestP[i].buf, "Exit") != 0){
                            continue;
                        }
                        //free lock.
                        read_lock_ref_count[requestP[i].id] -= 1;
                        if(read_lock_ref_count[requestP[i].id] == 0){
                            unlock(file_fd, (off_t)requestP[i].id*sizeof(record));
                        }
                        FD_CLR(requestP[i].conn_fd, &master_fds);
                        close(requestP[i].conn_fd);
                        free_request(&requestP[i]);
                    }
#elif defined WRITE_SERVER
                    if(requestP[i].wait_for_write == 0){
                        if(handle_id_data(&requestP[i]) != 1){ // invalid input
                            strcpy(buf, "[Error] Operation failed. Please try again.\n");
                            write(requestP[i].conn_fd, buf, strlen(buf));
                            FD_CLR(requestP[i].conn_fd, &master_fds);
                            close(requestP[i].conn_fd);
                            free_request(&requestP[i]);
                        }else{
                            int status = write_lock(file_fd, (off_t)requestP[i].id*sizeof(record));
                            if(status<0 || write_lock_ref_count[requestP[i].id] != 0){ // already obtained lock in this process, or lock is occupied by other read 
                                strcpy(buf, "Locked.\n");
                                write(requestP[i].conn_fd, buf, strlen(buf));
                                FD_CLR(requestP[i].conn_fd, &master_fds);
                                close(requestP[i].conn_fd);
                                free_request(&requestP[i]);
                                continue;
                            }else{
                                write_lock_ref_count[requestP[i].id] = 1;
                            }
                            sprintf(buf,"acccept message: %s : %s \n", accept_write_header, requestP[conn_fd].buf);
                            handle_output_status(requestP, requestP[i].conn_fd, file_fd);
                            strcpy(buf, "\nPlease input your booking command. (Food, Concert, Electronics. Positive/negative value increases/decreases the booking amount.):\n");
                            write(requestP[i].conn_fd, buf, strlen(buf));
                            requestP[i].wait_for_write = 1;
                        }
                    }else{
                        int write_status = handle_writing_data(&requestP[i], file_fd);
                        if(write_status == 0){
                            // fprintf(stderr, "booking failed; invalid input");
                            strcpy(buf, "[Error] Operation failed. Please try again.\n");
                            write(requestP[i].conn_fd, buf, strlen(buf));
                        }else if(write_status == -1){ //invalid operation
                            // fprintf(stderr, "booking failed; invalid operation");
                        }else{
                            // fprintf(stderr, "booking success");
                        }
                        //unlock.
                        write_lock_ref_count[requestP[i].id] = 0;
                        unlock(file_fd, (off_t)requestP[i].id*sizeof(record));

                        FD_CLR(requestP[i].conn_fd, &master_fds);
                        close(requestP[i].conn_fd);
                        free_request(&requestP[i]);
                    }
#endif
                }
            }
        }
    }
    free(requestP);
    free(read_lock_ref_count);
    free(write_lock_ref_count);
    close(file_fd);
    return 0;
}

// ======================================================================================================
// You don't need to know how the following codes are working

static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->id = 0;
    reqP->wait_for_write = 0;
}

static void free_request(request* reqP) {
    init_request(reqP);
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    // Get file descripter table size and initialize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    return;
}

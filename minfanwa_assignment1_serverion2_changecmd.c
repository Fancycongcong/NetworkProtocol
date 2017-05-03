/**
 * @minfanwa_assignment1
 * @author  Minfan Wang <minfanwa@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
 #include <stdio.h>
 #include <stdlib.h>
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <netdb.h>
 #include <arpa/inet.h>
 #include <sys/unistd.h>//get hostname
 #include <string.h>
 #include <fcntl.h>

 #include "../include/global.h"
 #include "../include/logger.h"
 
 #define MAX_NAME_SZ 256
 char *command_str;

 char hostname[100];
 char ip_addr[INET_ADDRSTRLEN];
 struct addrinfo hints, *res, *p;
 void get_ipaddr(){
        int result;
        gethostname(hostname, sizeof hostname);
        memset(&hints, 0, sizeof hints); //make sure the struct is empty
        hints.ai_family = AF_INET; // IP type IPv4
        hints.ai_socktype = SOCK_STREAM; //TCP stream sockets
        hints.ai_flags = AI_PASSIVE;
        if((result = getaddrinfo(hostname, NULL, &hints, &res)) !=0){
                exit(1);
        }  
        for(p=res;p!=NULL;p=p->ai_next){ 
                // //get ip_addr
                // //convert the IP into a string
                strcpy(ip_addr , inet_ntoa(((struct sockaddr_in *) p->ai_addr)->sin_addr ) ); 
        }
        freeaddrinfo(res);
 }
 //check if an ip adress is valid
 //reference:http://stackoverflow.com/questions/791982/determine-if-a-string-is-a-valid-ip-address-in-c
 int is_valid_ip(const char *s)
{
    int len = strlen(s);
    if (len < 7 || len > 15)
        return 0;
    char tail[16];
    tail[0] = 0;
    unsigned int d[4];
    int c = sscanf(s, "%3u.%3u.%3u.%3u%s", &d[0], &d[1], &d[2], &d[3], tail);
    if (c != 4 || tail[0])
        return 0;
    for (int k = 0; k < 4; k++)
        if (d[k] > 255)
            return 0;
    return 1;
}
int is_valid_port(const int po){
        if( po>=0||po<65535)
                return 1;
        else
                return 0;
}

void *get_in_addr(struct sockaddr *sa){
        return &(((struct sockaddr_in*)sa)->sin_addr);
}

char buf[1024];
fd_set master; //master file descriptor lisr
fd_set read_fds; //temp file descriptor list for select()
int fdmax; //maximum file descriptor number

int listener; //listening socket descriptor 
int newfd;//newly accept()ed socket descriptor
struct sockaddr_in server_addr, remoteaddr; //client address
socklen_t addrlen;
// char buf[256];//buffer for client data
char remoteIP[INET_ADDRSTRLEN];
int port;
char PORT[10];
int yes=1;//for setsockopt() SO_REUSEADDR, below
int i,j,rv;
//serv_socket create
 void create_socket(){
        // struct addrinfo hints, *res, *p;
        //has been global declared
        FD_ZERO(&master); //clear all the master and temp sets
        FD_ZERO(&read_fds);
        //get a socket and bind it 
        listener = socket(AF_INET, SOCK_STREAM, 0);
        if(listener < 0){
            cse4589_print_and_log("cannot create_socket\n");
            exit(1);
        }

        memset(&server_addr,0,sizeof server_addr);
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        server_addr.sin_port = htons(port);
        if((rv = getaddrinfo(NULL, PORT, &hints, &res))!=0){
                cse4589_print_and_log("getaddrinfo\n");
                exit(1);
        }
        if (bind(listener,(struct sockaddr *)&server_addr,sizeof(server_addr))<0){
            cse4589_print_and_log("bind fail\n");
            exit(1);
        }
        //listen
        if(listen(listener,5)<0){//maxium client 4
                cse4589_print_and_log("unablelisten\n");
                exit(1);
        }
        //add the listener into the master set
        FD_SET(listener, &master);
        //keep track of the biggest file descriptor
        fdmax=listener;
 }

 //serv read from client, part of select
 //refer: http://www.gnu.org/software/libc/manual/html_node/Server-Example.html
 int nbytes;
 int read_from_client (int list_id)
{
  // char serv_buffer[256];
  nbytes = read (list_id, buf, strlen(buf));
  if (nbytes < 0){//error
        return -1;
  }
  else if (nbytes == 0){//file end
        return 0;
  }
  else{ // data read
      return 1;
}
}

/*client connect, inform the server with client command
* the basic structure refer from: 
* http://www.tutorialspoint.com/unix_sockets/socket_client_example.htm*/
// struct hostent *server;
// int serv_socket, n, serv_port;
// struct sockaddr_in serv_addr;
// char client_buffer[1024];
// void connect_serv(){
//         /* Create a socket point */
//         serv_socket = socket(AF_INET, SOCK_STREAM, 0);
//         if ( serv_socket< 0) {
//                 cse4589_print_and_log("[%s:ERROR]\n", command_str);
//                 cse4589_print_and_log("[%s:END]\n", command_str);
//                 exit(1);
//         }
//         bzero((char *) &serv_addr, sizeof(serv_addr));
//         serv_addr.sin_family = AF_INET;
//         bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
//         serv_addr.sin_port = htons(serv_port);
//          //Now connect to the server 
//         //int connect (int sockfd, const struct sockaddr *servaddr, socklen_t addrlen);
//         if ( connect(serv_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
//                 cse4589_print_and_log("[%s:ERROR]\n", command_str);
//                 cse4589_print_and_log("[%s:END]\n", command_str);
//                 exit(1);
//         }
//         /* Send message to server ,port number
//         Write in form : PORT portno*/
//         bzero(client_buffer,1024);
//         fgets(client_buffer,1023,stdin);
//         /* Send message to the server */

//         n = write(serv_socket, client_buffer, strlen(client_buffer));
//         bzero(client_buffer,1024);
//         // free(client_buffer);
//  }
//
int serv_socket, numbytes;
char client_buffer[1024];
struct addrinfo hints, *servinfo, *p;
char SERV_IP[20];
char SERV_PORT[10];
void connect_serv(){
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        if((rv = getaddrinfo(SERV_IP, SERV_PORT, &hints, &servinfo)) != 0){
                cse4589_print_and_log("[%s:ERROR]\n", command_str);
                cse4589_print_and_log("getaddrinfo\n");
                cse4589_print_and_log("[%s:END]\n", command_str);
                exit(1);
        }
        //loop through all the results and connect to the first we can
        for (p = servinfo; p != NULL; p = p->ai_next) {
                if ((serv_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
                        continue;
                }

                if (connect(serv_socket, p->ai_addr, p->ai_addrlen) == -1){
                        close(serv_socket);
                        continue;
                }
                break;
        }

        if( p == NULL){
                cse4589_print_and_log("[%s:ERROR]\n", command_str);
                cse4589_print_and_log("fail to connect\n");
                cse4589_print_and_log("[%s:END]\n", command_str);
                exit(1);
        }

        freeaddrinfo(servinfo); //all done with this structure
}
//extract information from client msg
 char cmd[4][200];
 char *seg;
 int mark = 0;
 void extract_msg(char *m){
        seg = strtok( m," ");//seperate cmd with space
        while(seg){
                strcpy(cmd[mark],seg);
                mark +=1;
                seg = strtok(NULL," ");
        }
 }

 //structure for list information
 struct list
 {
     int list_id;
     char hostname[1024];
     char ip_addr[30];
     int port_num;
     int num_msg_sent;
     int num_msg_rcv;
     int state;
     char status[10];    
 }client[4],temp;
 
/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv)
{
        /*Init. Logger*/
        cse4589_init_log(argv[2]);

        /*Clear LOGFILE*/
        fclose(fopen(LOGFILE, "w"));

        /*Start Here*/
        if(argc!=3)
                return -1;
        //process commands
        int port = 0;
        strcpy(PORT,argv[2]);
        port = atoi(argv[2]);
        char *command_str = malloc(MAX_NAME_SZ);

        if(!(strcmp(argv[1], "s"))) //run as server
        {
                while(1){
                        fgets(command_str, MAX_NAME_SZ, stdin);
                        /* Remove trailing newline, if there. */
                        if ((strlen(command_str)>0) && (command_str[strlen (command_str) - 1] == '\n')){
                                command_str[strlen (command_str) - 1] = '\0';
                        }
                        char *arg;
                        argc = 0;
                        arg = strtok( command_str, " "); // seperate cmd with space
                        while(arg){
                                strcpy(argv[argc], arg);
                                argc += 1;
                                arg = strtok(NULL, " ");
                        }

                        if(!(strcmp(argv[0], "AUTHOR"))){
                                char *ubit = "minfanwa";
                                cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                                cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", ubit);
                                cse4589_print_and_log("[%s:END]\n", command_str);
                        } 
                            
                        if(!(strcmp(argv[0], "IP")))
                        {
                                get_ipaddr(); 
                                cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                                cse4589_print_and_log("IP:%s\n", ip_addr);
                                cse4589_print_and_log("[%s:END]\n", command_str);
                        }

                        if(!(strcmp(argv[0], "PORT")))
                        {
                                cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                                cse4589_print_and_log("PORT:%d\n", port);
                                cse4589_print_and_log("[%s:END]\n", command_str);
                        }
                        //create socket and listen, select()
                        //state fot mark connection of client
                        int state[4];
                        struct list *client;
                        create_socket();
                        while(1){
                                read_fds = master;
                                if(select(fdmax+1, &read_fds, NULL, NULL,NULL)==-1){
                                        exit(1);
                                }
                                //run through the exiting connections looking for data to read
                                for(i=0;i<fdmax;i++){
                                        if(FD_ISSET(i, &read_fds))//new connection
                                        {
                                                addrlen = sizeof remoteaddr;
                                                newfd = accept(listener, (struct sockaddr *)&remoteaddr,&addrlen);
                                                if(newfd == -1){
                                                        cse4589_print_and_log("accept\n");
                                                }else{
                                                        FD_SET(newfd, &master);
                                                        if(newfd > fdmax){
                                                                fdmax = newfd;
                                                        }
                                                        client[i].state = 1;
                                                        //new connection from newfd, listid
                                                        //remoteIP
                                                        inet_ntop(AF_INET, (struct sockaddr*)&remoteaddr,
                                                        remoteIP, INET_ADDRSTRLEN);
                                                        struct hostent *hp;
                                                        hp = gethostbyaddr((const void *)(get_in_addr((struct sockaddr*)&remoteaddr)),
                                                         sizeof addrlen, AF_INET);
                                                        strcpy( client[i].hostname,hp->h_name );
                                                        strcpy( client[i].ip_addr,remoteIP );
                                                }
                                        }else{ //handle data from client
                                                if(read_from_client(i)<0){//error
                                                        cse4589_print_and_log("error or connectin cliose\n");
                                                }
                                                if(read_from_client(i)==0){
                                                        //connection hung up, logout
                                                        close (i);
                                                        FD_CLR (i, &master);
                                                        client[i].state = 0;
                                                }
                                                if(read_from_client(i)>0){
                                                        //read the data from client and handle the cmd
                                                        extract_msg(buf);
                                                        if(!(strcmp(cmd[0], "PORT"))){
                                                                client[i].port_num = atoi(cmd[1]);
                                                        }
                                                        if(!(strcmp(cmd[0], "LIST")))
                                                        if(!(strcmp(cmd[0], "SEND")))
                                                        if(!(strcmp(cmd[0], "BROADCAST"))){
                                                                strcpy(buf,cmd[1]);
                                                                for(j = 0;j<=fdmax; j++){
                                                                        if(FD_ISSET(j, &master)){
                                                                                if(j!=listener&&j!=i){
                                                                                        if(send(j,buf,nbytes,0)==-1){

                                                                                        }
                                                                                }
                                                                        }
                                                                }
                                                                // free(buf);
                                                        }
                                                        // if(!(strcmp(cmd[0], "BLOCK")))
                                                        // if(!(strcmp(cmd[0], "UNBLOCK")))
                                                }
                                        }
                                        // free(buf);
                                }

                        }

                        if(!(strcmp(argv[0], "LIST"))){
                                cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                                //refer:http://www.c4learn.com/c-programs/sorting-elements-of-structure.html
                                int a,b;
                                for (a = 1; a < 4; a++)
                                        for (b = 0; b < 4 - i; b++) {
                                                if (client[b].port_num > client[b + 1].port_num) {
                                                        temp = client[b];
                                                        client[b] = client[b + 1];
                                                        client[b + 1] = temp;
                                                }
                                        }
                                for (a = 0; a < 4; a++) {
                                        if(client[i].state){
                                                cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", client[i].list_id, 
                                                        client[i].hostname, client[i].ip_addr, client[i].port_num);
                                        }
                                        
                                }
                                cse4589_print_and_log("[%s:END]\n", command_str);
                        }

                        // if(!(strcmp(argv[0], "STATISTIC")))
                        // if(!(strcmp(argv[0], "BLOCKED")))
                        // if(!(strcmp(argv[0], "RELAYED"))
                        //handle the request from client, seperate the string first

                 }	
        }


        if(!(strcmp(argv[1], "c"))) //run as client
        {
                while(1){
                        fgets(command_str, MAX_NAME_SZ, stdin);
                        /* Remove trailing newline, if there. */
                        if ((strlen(command_str)>0) && (command_str[strlen (command_str) - 1] == '\n')){
                                command_str[strlen (command_str) - 1] = '\0';
                        }

                        char *arg;
                        argc = 0;
                        arg = strtok( command_str, " "); // seperate cmd with space
                        while(arg){
                                strcpy(argv[argc], arg);
                                argc += 1;
                                arg = strtok(NULL, " ");
                        }

                        if(!(strcmp(argv[0], "AUTHOR"))){
                                char *ubit = "minfanwa";
                                cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                                cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", ubit);
                                cse4589_print_and_log("[%s:END]\n", command_str);
                        }

                        int login = 0; //state for other options
                        if(!(strcmp(argv[0], "LOGIN"))){
                                //handleexceptions
                                //invalid IP address/port number(out of range)
                                //valid but incorrest/non-existent IP address/port number(not between the five ip)
                                strcpy( SERV_IP,argv[1]);
                                strcpy( SERV_PORT,argv[2]);
                                int serv_port;
                                serv_port = atoi(argv[2]);
                                cse4589_print_and_log("%s\n", argv[0]);
                                cse4589_print_and_log("%s\n", argv[1]);
                                cse4589_print_and_log("%s\n", argv[2]);

                                if(!is_valid_port(serv_port)){
                                        cse4589_print_and_log("[%s:ERROR]\n", command_str);
                                        cse4589_print_and_log("invalid port\n");
                                        cse4589_print_and_log("[%s:END]\n", command_str);
                                        exit(1);
                                }
                                if(!is_valid_ip(SERV_IP)){
                                        cse4589_print_and_log("[%s:ERROR]\n", command_str);
                                        cse4589_print_and_log("invalid ip\n");
                                        cse4589_print_and_log("[%s:END]\n", command_str);
                                        exit(1);
                                }
                                connect_serv();
                                login = 1;
                                cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                                cse4589_print_and_log("[%s:END]\n", command_str);
                                //sent port to the server

                                // struct info *client=malloc(sizeof(struct info));
                                // strcpy(client->name, hostname);
                                // strcpy(client->ip_addr, ip_addr)
                                // client->port=port;
                                // FILE * file= fopen("output", "wb");
                                // if (file != NULL) {
                                //         fwrite(object, sizeof(struct date), 1, file);
                                //         fclose(file);
                                // }

                                /* Send message to server */
                                // bzero(client_buffer,256);
                                // fgets(client_buffer,255,stdin);
                                // n = send(serv_socket, client_buffer, strlen(client_buffer));
                                // if (n < 0) {
                                //         exit(1);
                                // }
                                // /* read server response */
                                // bzero(client_buffer,256);
                                // n = read(serv_socket, client_buffer, 255);
                                // if (n < 0) {
                                //         exit(1);
                                // }
                                
                                // //get current log information from server
                                // char list_info[256];
                                // strncpy(list_info, client_buffer, 256); 
                        }
                        while(login == 1){
                                if(!(strcmp(argv[0], "IP")))
                                {
                                        get_ipaddr();
                                        cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                                        cse4589_print_and_log("IP:%s\n", ip_addr);
                                        cse4589_print_and_log("[%s:END]\n", command_str);
                                }
                                if(!(strcmp(argv[0], "PORT")))
                                {
                                        cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                                        cse4589_print_and_log("PORT:%d\n", port);
                                        cse4589_print_and_log("[%s:END]\n", command_str);
                                }
                              // if(!(strcmp(cmd[0], "LIST"))){
                        //         cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                        //         cse4589_print_and_log("LIST:%d\n", list_info);
                        //         cse4589_print_and_log("[%s:END]\n", command_str);
                        // }
                        
                        // if(!(strcmp(cmd[0], "REFRESH"))){

                        // }
                        // if(!(strcmp(cmd[0], "SEND"))){
                        //         //exception handle
                        //         //invalid IP adress
                        //         // Valid but incorrect/non-existent IP adress
                        //         if(){}
                        //         //information to store in buffer 
                        //         //remote client IP, host IP for printing and login msg to send
                        //         client_buffer = 
                        // }
                        // if(!(strcmp(cmd[0], "BROADCAST"))){
                        //         char message[256] = argv[1];

                        // }
                        // // if(!(strcmp(cmd[0], "BLOCK"))){shutdown()}
                        // // if(!(strcmp(cmd[0], "UNBLOCK")))
                        if(!(strcmp(cmd[0], "LOGOUT"))){
                                login = 0;
                                close(serv_socket);
                        }
                        // // if(!(strcmp(cmd[0], "RECEIVED")))
                         
                        }
                        if(!(strcmp(cmd[0], "EXIT"))){
                                cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                                cse4589_print_and_log("[%s:END]\n", command_str);
                                exit(0);
                        } 

                }
        }
        return 0;
}

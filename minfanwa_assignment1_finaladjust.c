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
 #include <netinet/in.h>//socket_in addr
 #include <netdb.h>
 #include <arpa/inet.h>
 #include <sys/unistd.h>//get hostname
 #include <string.h>
 #include <fcntl.h>

 #include "../include/global.h"
 #include "../include/logger.h"
 
 #define MAX_NAME_SZ 256
 #define STDIN 0
 #define CMD_SIZE 100
 #define BUFSIZE 256
 #define BACKLOG 5
 char *command_str;
 char *command_str_copy;

 char hostname[100];
 char ip_addr[INET_ADDRSTRLEN];
 struct addrinfo hints, *res, *p;
 int result;
 void get_ipaddr(){
        
        gethostname(hostname, sizeof hostname);
        memset(&hints, 0, sizeof hints); //make sure the struct is empty
        hints.ai_family = AF_INET; // IP type IPv4
        hints.ai_socktype = SOCK_STREAM; //TCP stream sockets
        hints.ai_flags = AI_PASSIVE;
        result = getaddrinfo(hostname, NULL, &hints, &res);
        for(p=res;p!=NULL;p=p->ai_next){ 
                // //get ip_addr
                // //convert the IP into a string
                strcpy(ip_addr , inet_ntoa(((struct sockaddr_in *) p->ai_addr)->sin_addr ) ); 
        }
        freeaddrinfo(res);
 }

 
 int is_correct_ip(const char *r){
    char *ip[5] = {"128.205.36.46","128.205.36.35","128.205.36.33","128.205.36.34","128.205.36.36"};
    int t =0 ;
    for(t=0;t<5;t++){
        if(strcmp(r,ip[t])==0)
            return 1;
    }
    return 0;
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
char *msg;
char *buf;
fd_set master; //master file descriptor lisr
fd_set read_fds; //temp file descriptor list for select()
int fdmax; //maximum file descriptor number

int listener; //listening socket descriptor 
int newfd;//newly accept()ed socket descriptor
struct sockaddr_in local_addr, remoteaddr; //client address
socklen_t addrlen;
// char buf[256];//buffer for client data
char remoteIP[INET_ADDRSTRLEN];
int port;
char PORT[10];
int yes=1;//for setsockopt() SO_REUSEADDR, below
int i,j,rv;
//refer from Beej's and the recitation code
//serv_socket create
 void create_socket(char *SPO, int spo){
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

        memset(&local_addr,0,sizeof local_addr);
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        local_addr.sin_port = htons(spo);
        // cse4589_print_and_log("%s\n",local_addr.sin_addr);
        // cse4589_print_and_log("%d\n",local_addr.sin_port);
        // cse4589_print_and_log("%d\n",spo);
        if((rv = getaddrinfo(NULL, SPO, &hints, &res))!=0){
                cse4589_print_and_log("getaddrinfo\n");
                exit(1);
        }
        if (bind(listener,(struct sockaddr *)&local_addr,sizeof(local_addr))<0){
            cse4589_print_and_log("bind fail\n");
            exit(1);
        }
        //listen
        if(listen(listener,BACKLOG)<0){//maxium client 4
                cse4589_print_and_log("unablelisten\n");
                exit(1);
        }
        //add the listener into the master set
        FD_SET(listener, &master);
        //register STDIN
        FD_SET(STDIN, &master);
        //keep track of the biggest file descriptor
        fdmax=listener;
 }

/*client connect, inform the server with client command
* the basic structure refer from: 
* http://www.tutorialspoint.com/unix_sockets/socket_client_example.htm*/
// refer from : recitation

char *client_buffer;
#define BUFFER_SIZE 1024
#define MSG_SIZE 512

int client_socket;
int CONNEC;
struct sockaddr_in server_addr, client_addr;
void connect_serv(char *serv_ip, int serv_port)
{
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if(client_socket<0)
            printf("fail to create socket\n");
        bzero(&server_addr, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        inet_pton(AF_INET, serv_ip, &server_addr.sin_addr);
        server_addr.sin_port = htons(serv_port);
        CONNEC = connect(client_socket, (struct sockaddr*)&server_addr,sizeof(server_addr));
        if( CONNEC < 0)
            printf("connect fail\n");

}
//extract information from client msg
 char client_cmd[20][20];
 char *segm;
 int msg_index = 0;
 void extract_msg(char *m){
        segm = strtok( m, " ");//seperate cmd with space
        while(segm){
                strcpy(client_cmd[msg_index],segm);
                msg_index +=1;
                segm = strtok(NULL," ");
        }
 }

 //structure for list information
 int counter =0;
 struct list
 {
     int socket_num;
     int list_id;
     char hostname[200];
     char ip_addr[30];
     int port_num;
     char PORT_NUM[10];
     int num_msg_sent;
     int num_msg_rcv;
     int state;
     int ever_log;
     int block;
     char status[10];    
 }client[8],temp;
 
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
        //initialize the structure as zero
        int w;
        for(w=0;w<7;w++){
            memset(&client[i], 0, sizeof(client[i]));
        }
        
        memset(&temp, 0, sizeof(temp));

        if(!(strcmp(argv[1], "s"))) //run as server
        {
            //create socket and listen, select()
            //state fot mark connection of client
            create_socket(PORT,port);
            while(1){
                printf("\n[PA1-Server@CSE489/589]$ ");
                fflush(stdout);
                memcpy(&read_fds, &master, sizeof(master));
                // read_fds = master;
                if(select(fdmax+1, &read_fds, NULL, NULL,NULL)<0){
                    printf("select fail\n");
                    exit(1);
                }
                // else{
                //     cse4589_print_and_log("SELECT SUCCESS\n");
                // }
                // struct list *client, temp;                
                //run through the exiting connections looking for data to read
                for(i=0;i<=fdmax;i++){//i is the socket index
                if(FD_ISSET(i, &read_fds)){
                    //new connection
                    //check if new command on STDIN
                    if( i == STDIN){//deal with cmd
                        char *command_str = (char *)malloc(sizeof(char)*CMD_SIZE);
                        char buf_list[1024];
                        memset(command_str, '\0', CMD_SIZE);
                        fgets(command_str, CMD_SIZE-1, stdin);
                        if ((strlen(command_str)>0) && (command_str[strlen (command_str) - 1] == '\n')){
                            command_str[strlen (command_str) - 1] = '\0';
                        }
                        char cmd[4][200];
                        char *seg;
                        int mark = 0;
                        seg = strtok( command_str," ");//seperate cmd with space
                        while(seg){
                            strcpy(cmd[mark],seg);
                            mark +=1;
                            seg = strtok(NULL," ");
                        }
                        
                        // extract_msg(command_str);
                        if(!(strcmp(cmd[0], "AUTHOR"))){
                            char *ubit = "minfanwa";
                            cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                            cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", ubit);
                            cse4589_print_and_log("[%s:END]\n", command_str);
                        } 
                        if(!(strcmp(cmd[0], "IP"))){
                            get_ipaddr(); 
                            cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                            cse4589_print_and_log("IP:%s\n", ip_addr);
                            cse4589_print_and_log("[%s:END]\n", command_str);
                        }

                        if(!(strcmp(cmd[0], "PORT"))){
                            cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                            cse4589_print_and_log("PORT:%d\n", port);
                            cse4589_print_and_log("[%s:END]\n", command_str);
                        }
                        if(!(strcmp(cmd[0], "LIST"))){
                            cse4589_print_and_log("[%s:SUCCESS]\n", command_str);

                            //refer:http://www.c4learn.com/c-programs/sorting-elements-of-structure.html
                            int a,b;
                            // cse4589_print_and_log("fdmax:%d\n", fdmax);
                            // cse4589_print_and_log("listener:%d\n", listener);
                            for (a = 1; a <= 7; a++)
                                for (b = 0; b <= 7 - a; b++) {
                                    if (client[b].port_num > client[b + 1].port_num) {
                                        temp = client[b];
                                        client[b] = client[b + 1];
                                        client[b + 1] = temp;
                                    }
                                }
                                b = 1;
                                for (a = 4; a <= 7; a++) {
                                    if(client[a].state == 1){
                                        cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", b, 
                                        client[a].hostname, client[a].ip_addr, client[a].port_num);
                                        b++;
                                    }
                                }
                               
                            cse4589_print_and_log("[%s:END]\n", command_str);
                        }
                        if(!(strcmp(cmd[0], "STATISTICS"))){
                            cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                            int a,b;
                            for (a = 1; a <= 7; a++)
                                for (b = 0; b <= 7 - a; b++) {
                                    if (client[b].port_num > client[b + 1].port_num) {
                                        temp = client[b];
                                        client[b] = client[b + 1];
                                        client[b + 1] = temp;
                                    }
                                }
                                b = 1;
                                for (a = 4; a <= 7; a++) {
                                    if(client[a].ever_log == 1){
                                        if(client[a].state == 0)
                                            strcpy(client[a].status, "offline");
                                        else
                                            strcpy(client[a].status, "online");
                                        cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", 
                                        b, client[a].hostname, client[a].num_msg_sent, 
                                        client[a].num_msg_rcv, client[a].status);
                                        b++;
                                    }   
                                }
                                // cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                            cse4589_print_and_log("[%s:END]\n", command_str);
                        }
                        if(!(strcmp(cmd[0], "BLOCKED"))){
                            //exception handle
                            if(!is_correct_ip(cmd[1])||!is_valid_ip(cmd[1])){
                                cse4589_print_and_log("[%s:ERROR]\n", command_str);
                                cse4589_print_and_log("[%s:END]\n", command_str);
                                // exit(1);
                            }
                            else{
                                // cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                                int a,b;
                            for (a = 1; a <= 7; a++)
                                for (b = 0; b <= 7 - a; b++) {
                                    if (client[b].port_num > client[b + 1].port_num) {
                                        temp = client[b];
                                        client[b] = client[b + 1];
                                        client[b + 1] = temp;
                                    }
                                }
                                b = 1;
                                for (a = 4; a <= 7; a++) {
                                    if(client[a].block == 1){
                                        cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", a-3, 
                                        client[a].hostname, client[a].ip_addr, client[a].port_num);
                                        b++;
                                    }   
                                }
                                cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                            cse4589_print_and_log("[%s:END]\n", command_str);
                            }
                            
                        }
                        
                        free(command_str);
                    }else if( i == listener){//if a new client is requesiting connection
                        client[i].state = 1;
                        
                        addrlen = sizeof(remoteaddr);
                        newfd = accept(listener, (struct sockaddr *)&remoteaddr,&addrlen);
                        if(newfd == -1){
                            printf("accept fail\n");
                        }else{
                            printf("\nRemote Host connected\n");
                            FD_SET(newfd, &master);
                            if(newfd > fdmax){
                                fdmax = newfd;
                            }
                        }
                    }else{//recv data form client
                        char *buf = (char *)malloc(sizeof(char)*BUFSIZE);
                        memset(buf, '\0', BUFSIZE);
                        client[i].ever_log = 1;
                        if(recv(i, buf, BUFSIZE,0)<=0){
                            // struct list client[i];
                            client[i].state = 0;
                            close(i);
                            printf("Client Host terminate connection\n");
                            FD_CLR(i, &master);
                        }else{
                            //processing income data from existing clients
                            printf("client send me:%s\n", buf);  
                            client[i].num_msg_rcv = strlen(buf);          
                            //process data from client
                            char client_cmd[4][200];
                            char *segm;
                            int msg_index = 0;
                            segm = strtok( buf, " ");//seperate cmd with space
                            while(segm){
                                strcpy(client_cmd[msg_index],segm);
                                msg_index +=1;
                                segm = strtok(NULL," ");
                            }
                            char buf_list[1024];
                            // memset(buf_list,'\0',1024);

                            if(!(strcmp(client_cmd[0], "PORT"))){
                                // struct list client[i];
                                strcpy(client[i].PORT_NUM,client_cmd[1]);
                                client[i].port_num = atoi(client_cmd[1]);
                                // struct list client[newfd];
                                client[i].state = 1;
                                addrlen = sizeof(remoteaddr);
                                strcpy(remoteIP,inet_ntoa(remoteaddr.sin_addr));
                                inet_pton(AF_INET, inet_ntoa(remoteaddr.sin_addr), &remoteaddr);
                                struct hostent *he;
                                he = gethostbyaddr(&remoteaddr, addrlen, AF_INET);
                                strcpy( client[i].hostname,he->h_name );
                                strcpy( client[i].ip_addr,remoteIP );
                            
                                printf("\n%-5d%-35s%-20s%-8d\n", i, 
                                        client[i].hostname, client[i].ip_addr, client[i].port_num);
                                
                                htonl(client[i].port_num);
                            }
                            
                            send(i, client , sizeof(client),0);
                            
                            if(!(strcmp(client_cmd[0], "SEND"))){
                                char *buf = (char*)malloc(sizeof(char)*BUFSIZE);
                                memset(buf, '\0', BUFSIZE);
                                //source IP address--remoteIP
                                getpeername(i,(struct sockaddr*)&remoteaddr,&addrlen);
                                strcpy(remoteIP,inet_ntoa(remoteaddr.sin_addr));
                                for(j=0;j<=fdmax;j++){
                                    if(FD_ISSET(j,&master)){
                                        getpeername(j,(struct sockaddr*)&remoteaddr,&addrlen);
                                        if(!strcmp(client_cmd[1],inet_ntoa(remoteaddr.sin_addr))){
                                            //add source ip --remoteIP
                                            // char *source_ip = inet_ntoa(remoteaddr.sin_addr);
                                            //EVENT deal
                                            cse4589_print_and_log("[%s:SUCCESS]\n", "RELAYED");
                                            cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", remoteIP, 
                                                client_cmd[1], client_cmd[2]);
                                            cse4589_print_and_log("[%s:END]\n", "RELAYED");

                                            memcpy(buf, remoteIP, strlen(remoteIP));
                                            memcpy(buf+strlen(remoteIP)," ", strlen(" "));
                                            memcpy(buf + strlen(remoteIP)+strlen(" "), client_cmd[2], strlen(client_cmd[2])+1);
                                            // client[j].num_msg_sent = strlen(buf);
                                            // cse4589_print_and_log("Buf: %s\n", buf); 
                                            // cse4589_print_and_log("remoteIP: %s\n", remoteIP); 
                                            if(send(j, buf, strlen(buf),0) == -1)
                                                printf("Send fail\n");
                                            client[j].num_msg_sent = strlen(buf);
                                        }
                                    }
                                }  
                            }
                            if(!(strcmp(client_cmd[0], "BROADCAST"))){
                                char *buf = (char*)malloc(sizeof(char)*BUFSIZE);
                                memset(buf, '\0', BUFSIZE);
                                getpeername(i,(struct sockaddr*)&remoteaddr,&addrlen);
                                strcpy(remoteIP,inet_ntoa(remoteaddr.sin_addr));
                                for(j = 4;j<=fdmax; j++){
                                    if(FD_ISSET(j, &master)){
                                        if(j!=listener&&j!=i){
                                            //EVENT
                                            cse4589_print_and_log("[%s:SUCCESS]\n", "RELAYED");
                                            cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", remoteIP, 
                                                "255.255.255.255", client_cmd[1]);
                                            cse4589_print_and_log("[%s:END]\n", "RELAYED");

                                            //write the source ip and the msg into the buf
                                            memcpy(buf, remoteIP, strlen(remoteIP));
                                            memcpy(buf+strlen(remoteIP)," ", strlen(" "));
                                            memcpy(buf + strlen(remoteIP)+strlen(" "), client_cmd[1], strlen(client_cmd[1])+1);
                                            // cse4589_print_and_log("%s\n", buf); 
                                            // cse4589_print_and_log("%s\n", client_cmd[1]);
                                            // cse4589_print_and_log("%s\n", client_cmd[2]);  
                                            // memcpy(buf, )
                                            if( send(j,buf,strlen(buf),0) == -1 ){
                                                printf("Broadcast fail\n");
                                                }
                                                client[j].num_msg_sent = strlen(buf);
                                            // send(j,buf,strlen(buf),0);
                                            }
                                        }
                                    }
                            }
                            if(!(strcmp(client_cmd[0], "BLOCK"))){
                                for(j=4;j<=fdmax;j++){
                                    if(FD_ISSET(j,&master)){
                                        getpeername(j,(struct sockaddr*)&remoteaddr,&addrlen);
                                        if(!strcmp(client_cmd[1],inet_ntoa(remoteaddr.sin_addr))){
                                            // shutdown(j, SHUT_RD);
                                            client[j].block = 1 ;
                                        }
                                    }
                                }  
                            }
                            if(!(strcmp(client_cmd[0], "UNBLOCK"))){
                                for(j=4;j<=fdmax;j++){
                                    if(FD_ISSET(j,&master)){
                                        getpeername(j,(struct sockaddr*)&remoteaddr,&addrlen);
                                        if(!strcmp(client_cmd[1],inet_ntoa(remoteaddr.sin_addr))){
                                            // shutdown(j, SHUT_RD);
                                            client[j].block = 0 ;
                                        }
                                    }
                                }  
                            }
                                
                            if(!(strcmp(client_cmd[0], "REFRESH"))){
                                send(i, client , sizeof(client),0);
                            }
                        }
                        free(buf);
                    }
                                                
                }
                }
            }
        }


        if(!(strcmp(argv[1], "c"))) //run as client
        {
                int client_socket, CONNEC;
                struct sockaddr_in server_addr;
                fd_set master,read_fds;
                // memcpy(&read_fds, &master, sizeof(master));
                client_socket = socket(AF_INET,SOCK_STREAM,0);
                if(client_socket<0)
                    printf("fail to craete socket\n");
                FD_ZERO(&master);
                FD_ZERO(&read_fds);
                FD_SET(STDIN,&master);
                fdmax = 0;

                while(1){
                    memcpy(&read_fds, &master, sizeof(master));
                    printf("\n[PA1-Client@CSE489/589]$ ");
                    fflush(stdout);

                    if(select(fdmax+1,&read_fds,NULL, NULL,NULL)<0){
                        printf("select fail\n");
                        exit(1);
                    }

                    for(i = 0; i<=fdmax; i++){
                        if(FD_ISSET(i,&read_fds)){
                            if( i == STDIN){
                                char *command_str = (char *)malloc(sizeof(char)*CMD_SIZE);
                                // char buf_list[1024];
                                memset(command_str, '\0', CMD_SIZE);
                                fgets(command_str, CMD_SIZE-1, stdin);
                                if ((strlen(command_str)>0) && (command_str[strlen (command_str) - 1] == '\n')){
                                command_str[strlen (command_str) - 1] = '\0';
                                }
                                char cmd[4][200];
                                char *seg;
                                int mark = 0;
                                seg = strtok( command_str," ");//seperate cmd with space
                                while(seg){
                                    strcpy(cmd[mark],seg);
                                    mark +=1;
                                    seg = strtok(NULL," ");
                                }
                                if(!(strcmp(cmd[0], "AUTHOR"))){
                                    char *ubit = "minfanwa";
                                    cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                                    cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", ubit);
                                    cse4589_print_and_log("[%s:END]\n", command_str);
                                }

                                int login = 0; //state for other options
                                if(!(strcmp(cmd[0], "LOGIN"))){
                                //handleexceptions
                                //invalid IP address/port number(out of range)
                                //valid but incorrest/non-existent IP address/port number(not between the five ip)
        
                                    if(!is_valid_port(atoi(cmd[2]))){
                                        cse4589_print_and_log("[%s:ERROR]\n", command_str);
                                        printf("invalid port\n");
                                        cse4589_print_and_log("[%s:END]\n", command_str);
                                        // exit(1);
                                    }else{
                                        if(!is_valid_ip(cmd[1])){
                                            cse4589_print_and_log("[%s:ERROR]\n", command_str);
                                            printf("invalid ip\n");
                                            cse4589_print_and_log("[%s:END]\n", command_str);
                                        // exit(1);
                                        }else{
                                            bzero(&server_addr, sizeof(server_addr));
                                            server_addr.sin_family = AF_INET;
                                            inet_pton(AF_INET, cmd[1], &server_addr.sin_addr);
                                            server_addr.sin_port = htons(atoi(cmd[2]));
                                            CONNEC = connect(client_socket, (struct sockaddr*)&server_addr,sizeof(server_addr));
                                            if( CONNEC < 0)
                                                printf("connect fail\n");

                                            FD_SET(client_socket,&master);
                                            if(client_socket>fdmax)
                                                fdmax = client_socket;
                                            // connect_serv(cmd[1],atoi(cmd[2]));        
                                            if(!CONNEC){
                                                login = 1;
                                                cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                                                cse4589_print_and_log("[%s:END]\n", command_str);
                                                //send the client port number to the server
                                                //PORT portno
                                                char *msg = (char*) malloc(sizeof(char)*MSG_SIZE);
                                                memset(msg, '\0', MSG_SIZE);
                                            
                                                memcpy(msg,"PORT ",strlen("PORT "));
                                                memcpy(msg+strlen("PORT "),PORT,strlen(PORT));
                                                send(client_socket, msg, strlen(msg), 0); 
                                                recv(client_socket, &client, sizeof(client), 0);
                                            
                                            }
                                            else{
                                                cse4589_print_and_log("[%s:ERROR]\n", command_str);
                                                cse4589_print_and_log("[%s:END]\n", command_str);
                                            }
                                        }//end of connect operation 
                                    }  
                                }  //end of login loop

                            while(login == 1){
                            printf("\n[PA1-Client@CSE489/589]$ ");
                            fflush(stdout);
                            
                            fgets(command_str, MAX_NAME_SZ, stdin);
                            /* Remove trailing newline, if there. */
                            if ((strlen(command_str)>0) && (command_str[strlen (command_str) - 1] == '\n')){
                                command_str[strlen (command_str) - 1] = '\0';
                            }
                            char *command_str_copy = malloc(MAX_NAME_SZ);
                            // char *client_buffer_copy = malloc(1024);
                            strcpy(command_str_copy, command_str);
                            char cmd[4][200];
                            char *seg;
                            int mark = 0;
                            seg = strtok( command_str_copy," ");//seperate cmd with space
                            while(seg){
                                strcpy(cmd[mark],seg);
                                mark +=1;
                                seg = strtok(NULL," ");
                            }

                            if(!(strcmp(cmd[0], "IP"))){
                                get_ipaddr();
                                if(result){
                                    cse4589_print_and_log("[%s:ERROR]\n", cmd[0]);
                                    cse4589_print_and_log("[%s:END]\n", cmd[0]);
                                }
                                cse4589_print_and_log("[%s:SUCCESS]\n", cmd[0]);
                                cse4589_print_and_log("IP:%s\n", ip_addr);
                                cse4589_print_and_log("[%s:END]\n", cmd[0]);
                            }
                            if(!(strcmp(cmd[0], "PORT"))){
                                cse4589_print_and_log("[%s:SUCCESS]\n", cmd[0]);
                                cse4589_print_and_log("PORT:%d\n", port);
                                cse4589_print_and_log("[%s:END]\n", cmd[0]);
                            }

                            /* Initialize buffer to receieve response */
                                // char *client_buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
                                // memset(client_buffer, '\0', BUFFER_SIZE);
                                // recv(client_socket, client_buffer, strlen(client_buffer),0);
                                // cse4589_print_and_log("buffer:%s\n", client_buffer);
                            if(!(strcmp(cmd[0], "LIST"))){
                                cse4589_print_and_log("[%s:SUCCESS]\n", command_str);

                            //refer:http://www.c4learn.com/c-programs/sorting-elements-of-structure.html
                            int a,b;
                            // cse4589_print_and_log("fdmax:%d\n", fdmax);
                            // cse4589_print_and_log("listener:%d\n", listener);
                            for (a = 1; a <= 7; a++)
                                for (b = 0; b <= 7 - a; b++) {
                                    if (client[b].port_num > client[b + 1].port_num) {
                                        temp = client[b];
                                        client[b] = client[b + 1];
                                        client[b + 1] = temp;
                                    }
                                }
                                b = 1;
                                for (a = 4; a <= 7; a++) {
                                    if(client[a].state == 1){
                                        cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", b, 
                                        client[a].hostname, client[a].ip_addr, atoi(client[a].PORT_NUM));
                                        b++;
                                    }
                                }
                                
                            cse4589_print_and_log("[%s:END]\n", command_str);
                            }
                        
                            if(!(strcmp(cmd[0], "REFRESH"))){
                                send(client_socket, "REFRESH", strlen("REFRESH"), 0);
                                if(recv(client_socket, &client, sizeof(client), 0)>=0){
                                    // cse4589_print_and_log("[%s:SUCCESS]\n", cmd[0]);
                                    int a,b;
                                    for (a = 1; a <= 7; a++)
                                        for (b = 0; b <= 7 - a; b++) {
                                            if (client[b].port_num > client[b + 1].port_num) {
                                                temp = client[b];
                                                client[b] = client[b + 1];
                                                client[b + 1] = temp;
                                            }
                                        }
                                    b = 1;
                                    for (a = 4; a <= 7; a++) {
                                        if(client[a].state == 1){
                                            cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", b, 
                                            client[a].hostname, client[a].ip_addr, atoi(client[a].PORT_NUM));
                                            b++;
                                        }
                                    }
                                    cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                                    cse4589_print_and_log("[%s:END]\n", cmd[0]);
                                }else{
                                    cse4589_print_and_log("[%s:ERROR]\n", cmd[0]);
                                    cse4589_print_and_log("[%s:END]\n", cmd[0]);
                                    // exit(1);
                                }
                            }
                            if(!(strcmp(cmd[0], "SEND"))){
                                //exception handle
                                //invalid IP adress
                                // Valid but incorrect/non-existent IP adress
                                if(!is_valid_ip(cmd[1])||!is_correct_ip(cmd[1])){
                                    cse4589_print_and_log("[%s:ERROR]\n", cmd[0]);
                                    printf("invalid ip\n");
                                    cse4589_print_and_log("[%s:END]\n", cmd[0]);
                                    // exit(1);
                                }
                                else{
                                    printf("%s\n", cmd[1]);
                                    printf("%s\n", cmd[2]);
                                    printf("%s\n", command_str);
                                    
                                    if(send(client_socket, command_str, strlen(command_str), 0) == strlen(command_str)){
                                        cse4589_print_and_log("[%s:SUCCESS]\n", cmd[0]);
                                        cse4589_print_and_log("[%s:END]\n", cmd[0]);
                                    }else{
                                        cse4589_print_and_log("[%s:ERROR]\n", cmd[0]);
                                        cse4589_print_and_log("[%s:END]\n", cmd[0]);
                                    }
                                }
                                
                            }
                            if(!(strcmp(cmd[0], "BROADCAST"))){
                                if(send(client_socket, command_str, strlen(command_str), 0) == strlen(command_str)){
                                    cse4589_print_and_log("[%s:SUCCESS]\n", cmd[0]);
                                    cse4589_print_and_log("[%s:END]\n", cmd[0]);
                                }else{
                                    cse4589_print_and_log("[%s:ERROR]\n", cmd[0]);
                                    cse4589_print_and_log("[%s:END]\n", cmd[0]);
                                }
                            }
                            if(!(strcmp(cmd[0], "BLOCK"))){
                                //shutdown()
                                if(!is_valid_ip(cmd[1])||!is_correct_ip(cmd[1])){
                                    cse4589_print_and_log("[%s:ERROR]\n", cmd[0]);
                                    printf("invalid ip\n");
                                    cse4589_print_and_log("[%s:END]\n", cmd[0]);
                                    // exit(1);
                                }else{
                                    if(send(client_socket, command_str, strlen(command_str), 0) == strlen(command_str)){
                                        cse4589_print_and_log("[%s:SUCCESS]\n", cmd[0]);
                                        cse4589_print_and_log("[%s:END]\n", cmd[0]);
                                    }else{
                                        cse4589_print_and_log("[%s:ERROR]\n", cmd[0]);
                                        cse4589_print_and_log("[%s:END]\n", cmd[0]);
                                    }
                                }
                                
                            }
                            if(!(strcmp(cmd[0], "UNBLOCK"))){
                                if(!is_valid_ip(cmd[1])||!is_correct_ip(cmd[1])){
                                    cse4589_print_and_log("[%s:ERROR]\n", cmd[0]);
                                    printf("invalid ip\n");
                                    cse4589_print_and_log("[%s:END]\n", cmd[0]);
                                }else{
                                    if(send(client_socket, command_str, strlen(command_str), 0) == strlen(command_str)){
                                        cse4589_print_and_log("[%s:SUCCESS]\n", cmd[0]);
                                        cse4589_print_and_log("[%s:END]\n", cmd[0]);
                                    }else{
                                        cse4589_print_and_log("[%s:ERROR]\n", cmd[0]);
                                        cse4589_print_and_log("[%s:END]\n", cmd[0]);
                                    }
                                }
                                
                            }
                            if(!(strcmp(cmd[0], "LOGOUT"))){
                                login = 0;
                                close(client_socket);
                                cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                                cse4589_print_and_log("[%s:END]\n", command_str);
                            }
                            
                            if(!(strcmp(cmd[0], "EXIT"))){
                                close(client_socket);
                                cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                                cse4589_print_and_log("[%s:END]\n", command_str);
                                exit(0);
                            }
                        
                        }
                        free(client_buffer);

                        if(!(strcmp(cmd[0], "EXIT"))){
                            cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
                            cse4589_print_and_log("[%s:END]\n", command_str);
                            exit(0);
                        } 

                            }//end of keyboard in command operation
                            else if(i == client_socket){
                                // /* Initialize buffer to receieve response */
                                char *client_buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
                                memset(client_buffer, '\0', BUFFER_SIZE);

                                if(recv(client_socket, client_buffer, BUFFER_SIZE, 0) >= 0){
                                    printf("Server responded: %s\n", client_buffer);
                                    fflush(stdout);
                                    
                                    char client_cmd[4][200];
                                    char *segm;
                                    int msg_index = 0;
                                    segm = strtok( client_buffer, " ");//seperate cmd with space
                                    while(segm){
                                        strcpy(client_cmd[msg_index],segm);
                                        msg_index +=1;
                                        segm = strtok(NULL," ");
                                    }
                                    cse4589_print_and_log("[%s:SUCCESS]\n", "RECEIVED");
                                    cse4589_print_and_log("msg from:%s\n[msg]:%s\n", client_cmd[0], client_cmd[1]);
                                    cse4589_print_and_log("[%s:END]\n", "RECEIVED");
                                    
                                }else{
                                    cse4589_print_and_log("[%s:ERROR]\n", "RECEIVED");
                                    cse4589_print_and_log("[%s:END]\n", "RECEIVED");
                                }
                            }//end of dealing with the buffer
                        }// end of FD_ISSET(i,&read_fds)
                    }//end of loop fdmax
                    

                       
                }//end of while loop
                free(command_str);
        }

    return 0;
}

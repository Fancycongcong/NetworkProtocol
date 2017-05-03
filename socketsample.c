//http://stackoverflow.com/questions/24055234/socket-programming-client-server-message-read-write-in-c
int service_count, sockfd, d1;

// Socket create
int sock_create()
{
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0)
  {
    printf("Could not create socket");
    return 1;
  }
  puts("Socket created");
  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(2100);

}
// Bind
int sock_bind()
{
  int b = bind(sockfd, (struct sockaddr *) &server, sizeof(server));
  if (b < 0)
  {
    perror("Bind failed. Error");
    return 1;
  }
  puts("Bind");

}
// Listen
int sock_listen()
{
  listen(sockfd, 10);
}
//Connection accept
int sock_accept()
{
  int s = sizeof(struct sockaddr_in);
  d1 = accept(sockfd, (struct sockaddr *) &client, (socklen_t*) &s);

  if (d1 < 0)
  {
    perror("accept failed");
    return 1;
  }
  puts("Connection accepted");
}
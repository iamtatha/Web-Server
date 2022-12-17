/* run using ./server <port> */

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include "http_server.hh"
#include <vector>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include<bits/stdc++.h>

using namespace std;


#define Thread_max 200
#define Max_client 4000


int taskcount;
pthread_mutex_t mutexlock;
pthread_cond_t condvar;
queue<int> clients;


vector<string> split(const string &s, char delim) {
  vector<string> elems;

  stringstream ss(s);
  string item;

  while (getline(ss, item, delim)) {
    if (!item.empty())
      elems.push_back(item);
  }

  return elems;
}






HTTP_Request::HTTP_Request(string request) {
  vector<string> lines = split(request, '\n');
  vector<string> first_line = split(lines[0], ' ');

  this->HTTP_version = "1.0"; // first_line here

  this->method = first_line[0];
  this->url = first_line[1];
  // cout<<this->url<<endl;

  if (this->method != "GET") {
    cerr << "Method '" << this->method << "' not supported" << endl;
    exit(1);
  }
}






HTTP_Response *handle_request(string req) {
  HTTP_Request *request = new HTTP_Request(req);

  HTTP_Response *response = new HTTP_Response();
  
  string url = string("html_files") + request->url;


  response->HTTP_version = "1.0";

  struct stat sb;
  if (stat(url.c_str(), &sb) == 0) // requested path exists
  {
    response->status_code = "200";
    response->status_text = "OK";
    response->content_type = "text/html";

    string body;

    if (S_ISDIR(sb.st_mode)) {
      url = url + string("/index.html");
    }
    
    std::ifstream ifs(url);
    std::string html( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );

    int content_size = html.size();   

    response->body = html;
    response->content_length = to_string(content_size);
  }

  else {
    response->status_code = "404";
    response->status_text = "Not Found";
    response->content_type = "text/html";
    response->body = "<!DOCTYPE html>\n<html>\n<h1>404 NOT FOUND</h1>\n</html>";
    response->content_length = to_string((response->body).size());
  }

  // cout << "\nThe URL is: " << url << "\n";

  delete request;
  return response;
}





void error(char *msg) {
  printf("In error");
  perror(msg);
  // exit(1);
}




void* handle_client(void  *sock)
{
  int newsockfd = *((int *)sock);
  char buffer[2048];
  int n;

  while(1)
  {

    bzero(buffer, 2048);
    n = read(newsockfd, buffer, 2048);

    if (n <= 0)
      break;

    string y(buffer);
    if (buffer[0] == 'G'){
        // cout << y << endl;
    HTTP_Response *temp = handle_request(y);
    
    string data = "HTTP/" + temp->HTTP_version + " " + temp->status_code + " " + temp->status_text + "\nContent-Type: " + temp->content_type + "\nContent-Length:  " + temp->content_length + "\n\n";
    data += temp->body;


    int len = data.length();
    char char_array[len + 1];
    strcpy(char_array, data.c_str());

    // cout << "\nHTTP Response:\n" << char_array << "\n";

    n = write(newsockfd, char_array, len+1);
    if (n < 0)
      error("ERROR writing to socket");
    }
    
    close(newsockfd);
  }
  return NULL;
}










void* initiatethread(void* args) {

  int newsockfd;
  char buffer[1024];
  int n;
  while(1){
  pthread_mutex_lock(&mutexlock);
  
  while(taskcount == 0) {
    pthread_cond_wait(&condvar, &mutexlock);
  }

  newsockfd = clients.front();
  int i;
  clients.pop();
  taskcount--;
  pthread_mutex_unlock(&mutexlock);

  handle_client(&newsockfd);
  }
}





int main(int argc, char *argv[]) {

  
  int sockfd, newsockfd, portno, i;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  
  pthread_t threads[Thread_max];




  if (argc < 2) {
    fprintf(stderr, "ERROR, no port provided\n");
    exit(1);
  }

  
  pthread_mutex_init(&mutexlock, NULL);
  pthread_cond_init(&condvar, NULL);


  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0)
    error("ERROR Openning Socket");



  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  


  int bind_value = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (bind_value < 0)
    error("ERROR on binding");


  for(i=0; i<Thread_max; i++)
  {
    int n = pthread_create(&threads[i], NULL, initiatethread, NULL);
    if(n != 0)
      perror("Failed to create the thread");
  }


  while(1) {
    
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);


    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0)
      error("ERROR on accept");


    pthread_mutex_lock(&mutexlock);

    if(taskcount < Max_client) {
      clients.push(newsockfd);
      taskcount++;
    }
    else {
      close(newsockfd);
      printf("[CONNECTION DROPPED]\n");
    }

    pthread_mutex_unlock(&mutexlock);
    pthread_cond_signal(&condvar);
  }

  return 0;
}

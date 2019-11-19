#include <dirent.h>
#include <fnmatch.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#define BACKLOG (10)

void serve_request(int);
/*
char * request_str = "HTTP/1.0 200 OK\r\n"
        "Content-type: text/html; charset=UTF-8\r\n\r\n";
*/

char * index_hdr = "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\"><html>"
        "<title>Directory listing for %s</title>"
"<body>"
"<h2>Directory listing for %s</h2><hr><ul>";

// snprintf(output_buffer,4096,index_hdr,filename,filename);


char * index_body = "<li><a href=\"%s\">%s</a>";

char * index_ftr = "</ul><hr></body></html>";

/* char* parseRequest(char* request)
 * Args: HTTP request of the form "GET /path/to/resource HTTP/1.X" 
 *
 * Return: the resource requested "/path/to/resource"
 *         0 if the request is not a valid HTTP request 
 * 
 * Does not modify the given request string. 
 * The returned resource should be free'd by the caller function. 
 */
char* parseRequest(char* request) {
  //assume file paths are no more than 256 bytes + 1 for null. 
  char *buffer = malloc(sizeof(char)*257);
  memset(buffer, 0, 257);
  
  if(fnmatch("GET * HTTP/1.*",  request, 0)) return 0; 

  sscanf(request, "GET %s HTTP/1.", buffer);
  return buffer; 
}

void serve_request(int);

struct thread_arg {
    int client_fd; //client_fd
    int server_sock;
};

/* The function that each new thread will execute when it begin. To make this
 * generic, it returns a (void *) and takes a (void *) as it's argument.  In C,
 * (void *) is essentially a typeless pointer to anything that you can cast to
 * anything else when necessary. For example, look at the argument to this
 * thread function.  Its type is (void *), but since we KNOW that the type of
 * the argument is really a (struct thread_arg *), the first thing we do is cast
 * it to that type, to make it usable. */
void *thread_function(void *argument_value) {
    struct thread_arg* my_argument = (struct thread_arg *) argument_value;
    pthread_detach(pthread_self());
    int server_sock = my_argument->server_sock;

    while(1) {
        /* Declare a socket for the client connection. */ 
        int sock;
        char buffer[256];
        /* Another address structure.  This time, the system will automatically
         * fill it in, when we accept a connection, to tell us where the 
         * connection came from. */
         struct sockaddr_in remote_addr;
         unsigned int socklen = sizeof(remote_addr);
         /* Accept the first waiting connection from the server socket and  
          * populate the address information.  The result (sock) is a socket  
          * descriptor for the conversation with the newly connected client.  If 
          * there are no pending connections in the back log, this function will
          * block indefinitely while waiting for a client connection to be made.
          * */
         sock = accept(server_sock, (struct sockaddr*) &remote_addr, &socklen); 
         if(sock < 0) {
           perror("Error accepting connection");
           exit(1);
           }
        serve_request(sock);
        /* Tell the OS to clean up the resources associated with that client
         * connection, now that we're done with it. */
    }
    close(server_sock);
 //     }
    return NULL;
}



void get_filetype(char* filename, char** request_str)
  {
  if(strstr(filename, ".gif"))
    *request_str = "HTTP/1.0 200 OK\r\n"
      "Content-type: image/gif\r\n\r\n";

  else if(strstr(filename, ".png"))
    *request_str = "HTTP/1.0 200 OK\r\n"
      "Content-type: image/png\r\n\r\n";

  else if(strstr(filename, ".jpg"))
    *request_str = "HTTP/1.0 200 OK\r\n"
      "Content-type: image/jpeg\r\n\r\n";

  else if(strstr(filename, ".pdf"))
    *request_str = "HTTP/1.0 200 OK\r\n"
      "Content-type: application/pdf\r\n\r\n";
  else if(strstr(filename, ".html"))
    *request_str = "HTTP/1.0 200 OK\r\n"
      "Content-type: text/html\r\n\r\n";

  else
    {
    *request_str = "HTTP/1.0 200 OK\r\n"
      "Content-type: text/plain; charset=UTF-8\r\n\r\n";
    }
  }

void serve_dynamic(int fd, char* uri) { //uri is just the filename
char* ptr;
char filename[4096];
char cgiargs[4096];
ptr = index(uri, '?');
//uri+=4; //remove the /cgi part from the URL
if(ptr) {
  strcpy(cgiargs, ptr+1);
  *ptr = '\0';
  }
else
  strcpy(cgiargs, "");
strcpy(filename, ".");
strcat(filename, uri);
//above is the textbook version of parsing the dynamic request

//below is the textbook version of serve_dynamic
char* request_str;
char buf[4096];
char* emptyList[20]; //allow for 2 arguments
struct stat isDir;
stat(filename, &isDir);
if(S_ISREG(isDir.st_mode))
 {
 request_str = "HTTP/1.0 200 OK\r\n"
        "Content-type: text/plain; charset=UTF-8\r\n\r\n";
 }
else
 {
 request_str = "HTTP/1.0 404 Not Found\r\n"
        "Content-type: text/html; charset=UTF-8\r\n\r\n";
 strcat(buf, request_str); //a little redundancy never hurt anyone
 send(fd, buf, strlen(buf),0);
 send(fd, "<html>404 not found</html>", strlen("<html>404 not found</html>"),0);
 close(fd);
 return;
 }

strcat(buf, request_str); //a little redundancy never hurt anyone
send(fd, buf, strlen(buf),0);
if(fork() == 0) //if we're in the child
  {
  char* token = strtok(cgiargs, "&");
  int ifYouAreReadingThisGoFuckYourself = 0;
  while(token != NULL)
    {
    emptyList[ifYouAreReadingThisGoFuckYourself] = token;
    ifYouAreReadingThisGoFuckYourself++;
    token = strtok(NULL, "&");
    }
  dup2(fd, STDOUT_FILENO); //redirect stdout to the client
  execve(filename, emptyList, environ);
  if(1)
    {
    printf("error running command\n");
    close(fd);
    return;
    }
  }
wait(NULL); //parent waits for child
}
void serve_request(int client_fd){
  int read_fd;
  int bytes_read;
  int file_offset = 0;
  char client_buf[4096];
  char send_buf[4096];
  char filename[4096];
  //below, use a local version of request_str because multiple threads using a global variable is probably bad
  char* request_str = "HTTP/1.0 200 OK\r\n"
        "Content-type: text/html; charset=UTF-8\r\n\r\n";
  char* requested_file;
  memset(client_buf,0,4096);
  memset(filename,0,4096);
  while(1){
    file_offset += recv(client_fd,&client_buf[file_offset],4096,0);
    if(strstr(client_buf,"\r\n\r\n"))
      break;
    if(file_offset < 0) return;
  }
  requested_file = parseRequest(client_buf);
  if(strstr(requested_file, "format"))
    {
    //serve dynamic
    serve_dynamic(client_fd, requested_file);
    close(client_fd);
    return;
    }

  if(requested_file[strlen(requested_file)-1] == '/' && strcmp(requested_file, "/") != 0) //if the file is the initial page, dont redirect
    {
    char indexListing[4096];
    requested_file[strlen(requested_file)-1] = '\0'; //the slash breaks a lot of my shit
    request_str = "HTTP/1.0 200 OK\r\n"
        "Content-type: text/html; charset=UTF-8\r\n\r\n";
    send(client_fd,request_str,strlen(request_str),0);

    strcat(indexListing, "<html><meta http-equiv = \"refresh\" content = \"0; url = "); //start the redirect string
    strcat(indexListing, requested_file); //redirect to the page with no slash
    strcat(indexListing, "\" /> </html>"); 
    send(client_fd, indexListing, strlen(indexListing), 0);
    close(client_fd);
    return;
    }


  get_filetype(requested_file, &request_str);
  // take requested_file, add a . to beginning, open that file
  filename[0] = '.';
  strncpy(&filename[1],requested_file,4095);
  //check to see if it's a dynamic request, and serve if it is

  struct stat isDir;
  stat(filename, &isDir);
  if(S_ISREG(isDir.st_mode))
    {
    read_fd = open(filename,0,0);
    send(client_fd,request_str,strlen(request_str),0);
    }

  else if(S_ISDIR(isDir.st_mode)) //if we're in a directory
    {
    char tempName[4096];
    memcpy(tempName, filename, strlen(filename)); //create a backup string without the /index.html
    strcat(filename, "/index.html"); //open the index of the directory
    read_fd = open(filename,0,0);

    if(read_fd != -1) //if index is there, behave normally
      {
      request_str = "HTTP/1.0 200 OK\r\n"
        "Content-type: text/html; charset=UTF-8\r\n\r\n";
      send(client_fd, request_str, strlen(request_str),0);
      char indexListing[4096];
      if(requested_file[strlen(requested_file)-1] != '/')
        strcat(requested_file, "/index.html"); //if you're not in the parent directory, add /index.html
      else
      strcat(requested_file, "index.html"); //if you're in the parent directory, the / is already included
      strcat(indexListing, "<html><meta http-equiv = \"refresh\" content = \"0; url = "); //start the redirect string
      strcat(indexListing, requested_file); //redirect to the index page
      strcat(indexListing, "\" /> </html>"); //redirect the user to the page with the index
      send(client_fd, indexListing, strlen(indexListing), 0);
      close(read_fd);
      close(client_fd);
      return;
      }

    else //list the entire directory
      {
      request_str = "HTTP/1.0 200 OK\r\n"
        "Content-type: text/html; charset=UTF-8\r\n\r\n";
      send(client_fd,request_str,strlen(request_str),0); //send the notification that we're gonna list it early on
      char indexListing[4096];
      if(tempName[strlen(tempName)-1] != '/') //add the slash
        strcat(tempName, "/");

      strcat(indexListing, "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\"><html>"
                           "<title>Directory listing for ");

      strcat(indexListing, tempName);
      strcat(indexListing, "</title>"
                           "<body>"
                           "<h2>Directory listing for ");
      strcat(indexListing, tempName);
      strcat(indexListing, "</h2><hr><ul>");


      strcat(indexListing, "<html><ul>"); //start the html file
      DIR* myDir = opendir(tempName);
      if(myDir == NULL) {
        printf("Shouldn't be here\n");
        return; //replace with 404
        }
      struct dirent* fileStruct;
      while((fileStruct = readdir(myDir)) != NULL) //created a listing for every file in the directory
        {
        char newFileName[4096];
        memcpy(newFileName, tempName, strlen(tempName)+1);
        strcat(newFileName, fileStruct->d_name);
        strcat(indexListing, "<li><a href=\"");
        strcat(indexListing, fileStruct->d_name); //insert the link
        struct stat isDir2;
        stat(newFileName, &isDir2);
        if(S_ISDIR(isDir2.st_mode))
          strcat(indexListing, "/");//cheese
        strcat(indexListing, "\">" );//name the dir
        strcat(indexListing, fileStruct->d_name);
        strcat(indexListing, "</a>");
        }
      strcat(indexListing, "</ul><hr></body></html>");
      strcat(indexListing, "</ul></html>");
      send(client_fd,indexListing, strlen(indexListing),0);
      close(read_fd);
      close(client_fd);
      return;
      }

    send(client_fd,request_str,strlen(request_str),0);
    }

  else //if the file is unreadable, or non-existent, throw a 404
    {
    //read_fd = open("404.html", 0, 0);
    request_str = "HTTP/1.0 404 Not Found\r\n"
      "Content-type: text/html; charset=UTF-8\r\n\r\n";
    send(client_fd,request_str,strlen(request_str),0);
    send(client_fd, "<html>404 not found</html>", strlen("<html>404 not found</html>"),0);
    close(client_fd);
    return;
    }

  while(1){ //this should send the 404 or the regular file
    bytes_read = read(read_fd,send_buf,4096);
    if(bytes_read <= 0)
      break;
    send(client_fd,send_buf,bytes_read,0);
  }
  close(read_fd);
  close(client_fd);
  return;
}

/* Your program should take two arguments:
 * 1) The port number on which to bind and listen for connections, and
 * 2) The directory out of which to serve files.
 */
int main(int argc, char** argv) {
    /* For checking return values. */
    int retval;
    if(argc < 3)
      {
      perror("Not enough arguments");
      exit(1);
      }
    /* Read the port number from the first command line argument. */
    int port = atoi(argv[1]);
    char* directory = argv[2];
    int succ = chdir(directory);
    if(succ == -1)
      {
      perror("Can't find directory specified");
      exit(1);
      }
    /* Create a socket to which clients will connect. */
    int server_sock = socket(AF_INET6, SOCK_STREAM, 0);
    if(server_sock < 0) {
        perror("Creating socket failed");
        exit(1);
    }

    /* A server socket is bound to a port, which it will listen on for incoming
     * connections.  By default, when a bound socket is closed, the OS waits a
     * couple of minutes before allowing the port to be re-used.  This is
     * inconvenient when you're developing an application, since it means that
     * you have to wait a minute or two after you run to try things again, so
     * we can disable the wait time by setting a socket option called
     * SO_REUSEADDR, which tells the OS that we want to be able to immediately
     * re-bind to that same port. See the socket(7) man page ("man 7 socket")
     * and setsockopt(2) pages for more details about socket options. */
    int reuse_true = 1;
    retval = setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &reuse_true,
                        sizeof(reuse_true));
    if (retval < 0) {
        perror("Setting socket option failed");
        exit(1);
    }

    /* Create an address structure.  This is very similar to what we saw on the
     * client side, only this time, we're not telling the OS where to connect,
     * we're telling it to bind to a particular address and port to receive
     * incoming connections.  Like the client side, we must use htons() to put
     * the port number in network byte order.  When specifying the IP address,
     * we use a special constant, INADDR_ANY, which tells the OS to bind to all
     * of the system's addresses.  If your machine has multiple network
     * interfaces, and you only wanted to accept connections from one of them,
     * you could supply the address of the interface you wanted to use here. */

    struct sockaddr_in6 addr;   // internet socket address data structure
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port); // byte order is significant, so we use htons(port)
    addr.sin6_addr = in6addr_any; // listen to all interfaces


    /* As its name implies, this system call asks the OS to bind the socket to
     * address and port specified above. */
    retval = bind(server_sock, (struct sockaddr*)&addr, sizeof(addr));
    if(retval < 0) {
        perror("Error binding to port");
        exit(1);
    }

    /* Now that we've bound to an address and port, we tell the OS that we're
     * ready to start listening for client connections.  This effectively
     * activates the server socket.  BACKLOG (#defined above) tells the OS how
     * much space to reserve for incoming connections that have not yet been
     * accepted. */
    retval = listen(server_sock, BACKLOG);
    if(retval < 0) {
        perror("Error listening for connections");
        exit(1);
    }
    int i = 0;
    while(i < BACKLOG) {
        i++;
        int sock = -1;
        struct thread_arg* argument = malloc(sizeof(struct thread_arg));
        argument->client_fd = sock;
        argument->server_sock = server_sock;
        pthread_t* new_thread = malloc(sizeof(pthread_t));
        int retval = pthread_create(new_thread, NULL, thread_function, (void *) argument);
        if (retval) {
          printf("pthread_create() failed\n");
          exit(1);
          }

        //free(new_thread);
        //serve_request(sock);
        /* Tell the OS to clean up the resources associated with that client
         * connection, now that we're done with it. */
        //close(sock) is called in serve_request
    }
    while(1){}
    close(server_sock);
}


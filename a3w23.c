#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <search.h>
#include <unistd.h> // read(), write(), close()
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/times.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>


#define MAXBUFF 512

#define FILEAMOUNT 50
#define SA struct sockaddr
#define NCLIENT 1

//deal with client handler
void signal_callback_handler(int signum){
    printf("Client has exited\n");
    exit(0);
}


int main(int argc, char *argv[])
{
    
    /*
    init vars, 
    *p is for converting our argv portnumber to an int
    files[AMOUNT] is meant to store all files
    */
    float time;
    char *p;

    //these are for hashtables, helps us store who is able to get and delete
    //we insert into the hashtable in put 
    ENTRY e;
    ENTRY *ep;
    //create 25 files to store
    //https://man7.org/linux/man-pages/man3/hsearch.3.html
    hcreate(FILEAMOUNT);
    if (strncmp(argv[1], "-s", 2) == 0) {
        
        //create files
        //https://stackoverflow.com/questions/1088622/how-do-i-create-an-array-of-strings-in-c
        char files[FILEAMOUNT][MAXBUFF];
        int i = 0;

        //start the clock here
        clock_t start = clock();

        /*
        init vars,
        done[] is for seeing if clients are finished in the program
            0 for connected, -1 for disconnenected, +1 for finished
        pfd contains all polls for clients
        cliLen contains the length of the client socket
        */
        int sockfd, connfd, len, timeout, rval;
        int done[NCLIENT + 1], cliSocks[NCLIENT + 1];
        struct sockaddr_in servaddr, cli;
        struct pollfd pfd[NCLIENT + 1];
        socklen_t cliLen;

        for (int i = 0; i <= NCLIENT; i++){
            done[i] = -1;
        }
       
       //https://stackoverflow.com/questions/9748393/how-can-i-get-argv-as-int
        long portNumL = strtol(argv[2], &p, 10);
        int portNum = portNumL;
        int helloCheck = 0;
        

        // socket is created and verified
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            printf("socket creation failed...\n");
            exit(0);
        }
        else
            printf("Socket successfully created..\n");
        
        

        //https://man7.org/linux/man-pages/man3/bzero.3.html
        bzero(&servaddr, sizeof(servaddr));
    
        // assign the Port and IP
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(portNum);
    
        // Binding creates socket to the IP and verifies
        if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
            printf("socket bind failed...\n");
            exit(0);
        }
        else
            printf("Socket successfully binded..\n");
    
        //  Server listens and verifies
        int listenfd = listen(sockfd, NCLIENT);
        if ((listen(sockfd, 5)) != 0) {
            printf("Listen failed...\n");
            exit(0);
        }
        else
            printf("Server listening..\n");
            
        len = sizeof(cli);

        // Accept the data packet from client and verifies 
        connfd = accept(sockfd, (SA*)&cli, &len);
        if (connfd < 0) {
            printf("Server accept failed...\n");
            exit(0);
        }
        else
            printf("Server accepted the client...\n");
    


        
        char buffer[MAXBUFF];
        char *errorM = "Received (src = 0) ERROR: Not greeted.\n";
        int check;

        while(1) {
            // read the message from client and copy it in buffer
            // print buffer which contains the client contents

            bzero(buffer, MAXBUFF);

            signal(SIGINT, signal_callback_handler);
            
            check = read(connfd, buffer, sizeof(buffer));
            
            //check if the client said hello 
            if (strncmp(buffer, "HELLO", 5) != 0 && helloCheck == 0){
                printf("From client: %s \n", buffer);
                
                //if not ask the client 

                printf("Please greet the server with HELLO\n");
                bzero(buffer, MAXBUFF);
                strcpy(buffer, errorM);
                write(connfd, buffer, sizeof(buffer));
            } else {

                //else we start reading the files 
                helloCheck = 1;

                //copy the buffer to ensure there is no issue with changing 
                char bufferC[MAXBUFF];
                strcpy(bufferC, buffer);

                //tokenize the copy 
                char *tokens[4];
                char* token = strtok(bufferC, " ");
                int i = 0;
                while (token != NULL){
                    tokens[i] = token;
                    token = strtok(NULL, " ");
                    i++;
                }

                //double check and see if the client said hello again
                if (strncmp(tokens[0], "HELLO" , 5) == 0)
                {
                    printf("%s\n", buffer);

                    char *OK = " Received (src = 0): Hello Client!";
                    bzero(buffer, MAXBUFF);

                    strcpy(buffer, OK);
                    write(connfd, buffer, sizeof(buffer));

                }

                //get the time by calling clock again and sending it back to the client
                else if(strncmp(tokens[1], "gtime", 5) == 0)
                {

                    bzero(buffer, MAXBUFF);
                    clock_t end = clock();
                    time += (float)(end - start)/CLOCKS_PER_SEC;
                    //as taken from my a2
                    gcvt(time, 2, buffer);
                    printf("Received (src = client %s) (%s)\n", tokens[0], tokens[1]);

                    printf("Transmitting (src = server) %s\n", buffer);

                    write(connfd, buffer, sizeof(buffer));
                }

                //if we have put
                else if (strncmp(tokens[1], "put", 3) == 0)
                {
                    
                    //create a string for keys and data
                    char key[30];
                    char data[MAXBUFF];

                    printf("Received (src = client %s) (%s: %s)\n", tokens[0], tokens[1], tokens[2]);
                    //remove the new line and create the key
                    //https://aticleworld.com/remove-trailing-newline-character-from-fgets/
                    tokens[2][strcspn(tokens[2], "\n")] = '\0';
                    strcpy(key, tokens[2]);
                    strcpy(data, buffer);
                    //add it into ENTRY e
                    e.key = key;
                    e.data = data;
                    
                    //verify its in the hashtable
                    ep = hsearch(e, ENTER);
                    if(ep == NULL){
                        printf("ERROR COULD NOT BE STORED\n");
                    }

                    //reset buffer
                    bzero(buffer, MAXBUFF);
                    
                    //let the client know its in the server
                    char *OK = "Received (src = 0) OK\n";
                    strcpy(buffer, OK);

                    //add it into the list of files we have
                    strcpy(files[i], data);
                    i++;
                    
                    // and send that buffer to client
                    write(connfd, buffer, sizeof(buffer));
        
                }
                else if (strncmp(tokens[1], "get", 3) == 0){
                    
                    //do the same as put for the first part
                    char key[30];
                    char data[MAXBUFF];
                    char OK[MAXBUFF];
                    printf("Received (src = client %s) (%s:%s)\n", tokens[0], tokens[1], tokens[2]);

                    tokens[2][strcspn(tokens[2], "\n")] = '\0';
                    strcpy(key, tokens[2]);

                    //this time assign only the key to ENTRY e
                    e.key = key;
                    
                    //find it in the hashtable
                    ep = hsearch(e, FIND);
                    
                    //if its there we print the data, else it DNE
                    if (ep == NULL){
                        strcpy(OK, "Received (src = 0): Client does not have permission or the file does not exist");
                    } else {
                        char *s = (char *)ep->data;

                        strcat(OK, "Received (src = 0): ");
                        strcat(OK, s);
                    }

                    bzero(buffer, MAXBUFF);
                    strcpy(buffer, OK);
        
                    //and send that buffer to client
                    write(connfd, buffer, sizeof(buffer));
                }
                
                else if (strncmp(tokens[1], "delete", 6) == 0){
                   
                    //as done in put
                    char key[30];
                    char data[MAXBUFF];
                    char OK[MAXBUFF];
                    printf("Received (src = client %s) (%s:%s)\n", tokens[0], tokens[1], tokens[2]);
                    tokens[2][strcspn(tokens[2], "\n")] = '\0';

                    strcat(key, tokens[2]);
                    e.key = key;


                    ep = hsearch(e, FIND);
                    
                    //if we find it we set it to NULL
                    if (ep == NULL){
                        strcpy(OK, "Received (src = 0): Client does not have permission");
                    } else {
                        strcat(OK,"Received (src = 0): Delete ");
                        char *s = (char *)ep->data;

                        strcat(OK, s);
                        ep -> key = NULL;
                        ep -> data = NULL;
                        
                    }
                    bzero(buffer, MAXBUFF);
                    strcpy(buffer, OK);
        
                    //and send that buffer to client
                    write(connfd, buffer, sizeof(buffer));   
                }
                else if (strncmp(tokens[1], "quit", 4) == 0){
                    
                    //verify that a client has left
                    printf("Client %s has left the server.\n", tokens[0]);
                    while(1){
                        //create the server loop and find what stdin wants
                        char input[20] = "";
                        fgets(input, 20, stdin);
                        if (strncmp(input, "list", 4) == 0){
                            
                            //if list print out files
                            for(i = 0; i < FILEAMOUNT; i++){
                                printf("%s\n",files[i]);
                            }
                    
                        } else if (strncmp(input, "exit", 4)== 0){
                            //else terminate
                            printf("Terminating\n");
                            close(sockfd);
                            exit(0);
                        }
                    }
                } 
            }
            
        }


    } else if (strncmp(argv[1], "-c", 2 ) ==0){
        
        //do th same thing as the server
        int sockfd, connfd;
        struct sockaddr_in servaddr, cli;

        //create a file pointer
        FILE *fp;
        
        //obtain the id, open the file, get the server address and portnumber
        char *id = argv[2];
        fp = fopen(argv[3], "r");
        char *servAddr = argv[4];
        long portNumL = strtol(argv[5], &p, 10);
        int portNum = portNumL;


        //check if the file exists
        if(fp == NULL){
            perror("Unable to open file");
            exit(1);
        }

        //create socket and verify 
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            printf("socket creation failed...\n");
            exit(0);
        }
        else
            printf("Socket successfully created..\n");
        bzero(&servaddr, sizeof(servaddr));
    
        // assign IP and the portnumber
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(servAddr);
        servaddr.sin_port = htons(portNum);


    
        // connect the client socket to the server socket
        if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
            printf("connection with the server failed...\n");
            exit(0);
        }
        else
            printf("connected to the server..\n");
    
        
        char buffer[MAXBUFF];
        int helloCheck = 0;
        printf("Main: do_client (ID Number = %s, inputFile = '%s',) (server = '%s', port = %d)\n" , id, argv[3], servAddr, portNum);
        while(1) {
            bzero(buffer, sizeof(buffer));

            //ensure the client says hello the server first
            if (helloCheck == 0){
                fgets(buffer, MAXBUFF, stdin);
                write(sockfd, buffer, sizeof(buffer));

                if(strncmp(buffer, "HELLO", 5) == 0){
                    helloCheck = 1;
                }
                
                bzero(buffer, sizeof(buffer));
                read(sockfd, buffer, sizeof(buffer));
                bzero(buffer, MAXBUFF);
            } else {
                //reset the buffer
                bzero(buffer, sizeof(buffer));
                
                //get the buffer from the file and check if there is anything

                fgets(buffer, MAXBUFF, fp);
                if (buffer == NULL){
                    close(sockfd);
                    exit(0);
                }
                
                //if its a comment or a new line continue
                if (buffer[0] == '#' || buffer[0] == '\n')
                    continue;
                if (strncmp(buffer, id, 1) == 0){ 
                    
                    //tokenize
                    char bufferC[MAXBUFF];
                    strcpy(bufferC, buffer);
                    char *tokens[4];
                    char* token = strtok(bufferC, " ");
                    int i = 0;
                    while (token != NULL){
                        tokens[i] = token;
                        token = strtok(NULL, " ");
                        i++;
                    }

                    //if put
                    if (strncmp(tokens[1], "put", 3) == 0){
                        char bufferCC[MAXBUFF];
                        bzero(bufferCC, MAXBUFF);
                        
                        //while the new buffer copy does not equal }
                        while (strncmp(bufferCC, "}", 1) != 0){
                            //add the entire put into buffer
                            bzero(bufferCC, MAXBUFF);
                            fgets(bufferCC, MAXBUFF, fp);
                            strcat(buffer, " ");
                            strcat(buffer, bufferCC);
                        }

                        //transmit write and read
                        printf("Transmitting (src client = %s) (%s: %s)\n", tokens[0], tokens[1], tokens[2]);
                        write(sockfd, buffer, sizeof(buffer));
                        bzero(buffer, sizeof(buffer));
                        read(sockfd, buffer, sizeof(buffer));
                        printf("%s\n", buffer);
                    }
                    if (strncmp(tokens[1], "get", 3 ) == 0){

                        //transmit, write and read
                        printf("Transmitting (src client = %s) (%s: %s)\n", tokens[0], tokens[1], tokens[2]);
                        write(sockfd, buffer, sizeof(buffer));
                        bzero(buffer, sizeof(buffer));
                        read(sockfd, buffer, sizeof(buffer));
                        printf("%s\n", buffer);
                    }
                    if (strncmp(tokens[1], "delete", 6) == 0){

                        //transmit, write and read
                        printf("Transmitting (src client = %s) (%s: %s)\n", tokens[0], tokens[1], tokens[2]);
                        write(sockfd, buffer, sizeof(buffer));
                        bzero(buffer, sizeof(buffer));
                        read(sockfd, buffer, sizeof(buffer));
                        printf("%s\n", buffer);
                    }

                    if (strncmp(tokens[1], "gtime", 5) == 0){

                        //transmit write and read
                        printf("Transmitting (src client = %s) (%s)\n", tokens[0], tokens[1]);
                        write(sockfd, buffer, sizeof(buffer));
                        
                        bzero(buffer, sizeof(buffer));

                        read(sockfd, buffer, sizeof(buffer));
                        
                        printf("Received (src = server): %s\n", buffer);
                    }
                    if (strncmp(tokens[1], "delay", 5) == 0){

                        //delays on the client end
                        printf("Delaying program for %s milliseconds\n", tokens[2]);
                        int sleepM = atoi(tokens[2]);
                        sleep(sleepM/1000);
                        printf("Ending delay\n");
                    }
                    if (strncmp(tokens[1], "quit", 4) == 0) {
                        //lets the client quit
                        printf("Client Exit...\n");
                        write(sockfd, buffer, sizeof(buffer));
                        close(sockfd);
                        break;
                    }
                    sleep(2);
                }
                
                
            }

 
        }
        // close the socket
        close(sockfd);
    }
}
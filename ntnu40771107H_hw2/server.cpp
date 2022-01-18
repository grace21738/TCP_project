#include<stdio.h>
#include<linux/tcp.h>
#include<linux/tcp.h>
#include<linux/ip.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/ioctl.h>
#include<net/if.h>
#include<unistd.h>
#include<string.h>
#include<dirent.h>
#include<errno.h>
#include<vector>
#include<string>
#include<iostream>
#include<fstream>
#include<stdlib.h>
#include<algorithm>
#include<signal.h>
#include<sys/wait.h>
#include <opencv/cv.h>
#include <opencv2/core/mat.hpp>
#include "opencv2/opencv.hpp"
#include <opencv2/core/types_c.h>

using namespace std;
using namespace cv;

#define BUFF_SIZE 1024
#define PORT 8787
#define GET_ALL_FILE_NAME 1
#define PUT_FILE 2
#define GET_FILE 3
#define PLAY_VIDEO 4

#define ERR_EXIT(a){ perror(a); exit(1); }


void sig_catcher(int n){
    //cout << "child process dies" << endl;
    cout << "The client "<< " is unconnected." << endl;
    while(waitpid(-1,NULL,WNOHANG)>0);
}
int client_instruction( int client_sockfd );

int get_files( string &files,vector <string> &vector_files  ){

    DIR *pf;
    struct dirent *pdir;
    if( (pf = opendir( "./40771107H_server_folder" )) == NULL ){
        printf("ERROR: %d\n",errno);
        return -1;
    }
    while( (pdir = readdir(pf)) != NULL ){
        if( string(pdir->d_name)!="." &&  string(pdir->d_name)!=".." ){
            files = files + string(pdir->d_name) + "\n";
            vector_files.push_back(string(pdir->d_name));
        }
        //cout << string(pdir->d_name) << endl;
    }
    closedir(pf);
    return 0;
}

void get_size( int &tmp_size,long long int &file_size ){
    tmp_size = 0;
    if( file_size > BUFF_SIZE ) {
        tmp_size = BUFF_SIZE;
        file_size -= BUFF_SIZE;
    }
    else {
        tmp_size = file_size;
        file_size = 0;
    }
}

int main(int argc, char *argv[]){
    int server_sockfd, client_sockfd;
    struct sockaddr_in server_addr, client_addr;
    pthread_t thread_id;
    int addrLen = sizeof(struct sockaddr_in);
    int client_addr_len = sizeof(client_addr);
    char message[BUFF_SIZE] = "Hello World from Server";

    vector<int> client_connection;

    // mkdir folder
    if( access("40771107H_server_folder",F_OK) != 0 )
        system("mkdir 40771107H_server_folder");
    // Get socket file descriptor
    if((server_sockfd = socket(AF_INET , SOCK_STREAM , 0)) < 0){
        ERR_EXIT("socket failed\n");
        return -1;
    }

    int port=atoi(argv[1]);
    // Set server address information
    bzero(&server_addr, sizeof(server_addr)); // erase the data
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    int server_addr_len = sizeof(server_addr);
    
    //Set socket to allow multiple connections
    if( (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &server_addr, server_addr_len)) < 0 )  {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }  
    //setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &server_addr, server_addr_len);
    // Bind the server file descriptor to the server address
    if(bind(server_sockfd, (struct sockaddr *)&server_addr , sizeof(server_addr)) < 0){
        ERR_EXIT("bind failed\n");
        return -1;
    }
        
    // Listen on the server file descriptor,specify maximum of 3 pending connections for the master socket
    if(listen(server_sockfd , 3) < 0){
        ERR_EXIT("listen failed\n");
        return -1;
    }

    puts("Waiting for connections ...");
    signal(SIGCHLD,sig_catcher);
    int pid;
    while(1){
        // Accept the client and get client file descriptor
        //如果 queue 裡都沒有 client 的話， accept() 會一直等。
        if((client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, (socklen_t*)&client_addr_len)) < 0){
            ERR_EXIT("accept failed\n");
            return -1;
        }
        cout << "The client "<< client_sockfd << " is connected." << endl;
        pid = fork();
        if( pid<0 ){
            ERR_EXIT("fork failed\n");
            return -1;
        }
        //Child process
        else if( pid==0 ){
            close(server_sockfd);
            client_instruction( client_sockfd );
            close( client_sockfd );
            exit(EXIT_SUCCESS);

        }
        //Parent process
        else{
            close(client_sockfd);
        }
        
    }
    return 0;
    
}



//=====================instruction========================================
int client_instruction( int client_sockfd ){

    while( 1 ){
    
        char instruction[BUFF_SIZE];
        int  write_byte, read_byte;
        // Receive message from client
        if((read_byte = read(client_sockfd, instruction, 2)) < 0){
            ERR_EXIT("receive failed\n");
            return -1;
        }
        if( read_byte==0 ) {
            //cout << "The client "<< client_sockfd << " is unconnected." << endl;
            break;
        }
        //cout<<"instruction: "<< instruction<<endl;
        string files = "";
        char *file_content;
        char file_name[BUFF_SIZE] = {};
        long long int file_size = 0;
        FILE *new_file;
        char file_path[3*BUFF_SIZE];
        char buffer[2 * BUFF_SIZE] = {};
        int amount_of_file = 0 ,tmp_size = 0;
        vector <string> vector_files;


        //Server has got message
        if( read_byte > 0 ){
            memset(buffer,0,sizeof(buffer));
            //cout<<"instruction: "<<instruction<<endl;
            if(atoi(instruction) == GET_ALL_FILE_NAME ){
                vector_files.clear();
                //files.clear();
                //Send message to client
                get_files( files,vector_files );
                file_size = files.size();
                //cout <<"file_size: "<<file_size<<endl;
                write_byte = send(client_sockfd,&file_size, sizeof(file_size), 0 );
                if( file_size != 0 ) write_byte = send(client_sockfd,files.c_str(), file_size, 0 );
             
            }
            else if(atoi(instruction) == PUT_FILE){

                //Get how much files
                read_byte = read(client_sockfd, &amount_of_file, sizeof(amount_of_file));
                //-------------------------------------------------------
                for( int i=0; i<amount_of_file; ++i ){
                    //Get file Name and hole file size
                    memset(buffer,0,sizeof(buffer));
                    memset(file_name,'\0',sizeof(file_name));
                    read_byte = read(client_sockfd, &file_size, sizeof(file_size));
                    read_byte = read(client_sockfd, file_name, sizeof(file_name));
                    write_byte = send(client_sockfd,"OK", 3, 0 );
                    //puts(buffer);
                    //puts(file_name);
                    //printf("file_size: %lld\n",file_size);

                    sprintf(file_path,"./40771107H_server_folder/%s",file_name);
                    new_file = fopen(file_path, "w" );
                    if( ! new_file){
                        printf("File could not be opened\n");
                        perror( "ERROR" );
                        return -1;
                    }
                    file_content = new char[BUFF_SIZE]();
                    //Recieve hole file
                    while( file_size>0  ){
                        int tmp_packet;
                        memset(buffer,'\0',sizeof(buffer));
                        if( file_size < BUFF_SIZE ){
                            tmp_packet = read(client_sockfd, file_content, file_size );
                        }
                        else{
                            tmp_packet = read(client_sockfd, file_content, sizeof(file_content) );
                        }
                        //===Write in file===
                        file_size -= tmp_packet;
                        tmp_packet = fwrite( file_content,sizeof(char),tmp_packet,new_file );
                    }
                    fclose( new_file );
                    free(file_content);

                }
                //-------------------------------------------------------
            }
            else if(atoi(instruction) == GET_FILE){
                //printf("GET_FILE\n");
                memset(buffer,0,sizeof(buffer));
             
                //Get how much files
                read_byte = read(client_sockfd, &amount_of_file, sizeof(amount_of_file));
                //cout <<"amount_of_file: "<<amount_of_file<<endl;
                for( int i=0; i<amount_of_file; ++i ){
                    memset(buffer,'\0',sizeof(buffer));
                    memset(file_path,'\0',sizeof(file_path));
                    memset(file_name,'\0',sizeof(file_name));
                    vector_files.clear();
                    //Get file name
                    read_byte = read(client_sockfd, file_name, sizeof(file_name)-1);
                    //printf("filename: %s\n",file_name);
                    //find file exist and send file size
                    get_files( files,vector_files );
                    vector<string>::iterator it = std::find(vector_files.begin(),vector_files.end(),file_name);
                    if( it != vector_files.end() ){
                        //passing file path
                        sprintf(file_path,"./40771107H_server_folder/%s",file_name);
                        new_file = fopen(file_path, "r");
                        if( ! new_file ){
                            printf("File could not be opened\n");
                            perror( "ERROR" );
                            return -1;
                        }
                        //send file size to client
                        //new_file.seekg(0,ios::end);
                        fseek(new_file,0,SEEK_END);
                        file_size = ftell(new_file);
                    }
                    else{
                        file_size = -1;
                    }
             
                    write_byte = send(client_sockfd, &file_size, sizeof(file_size), 0);
                    if( file_size < 0 ) continue;
                    //cout<<"file_size"<<file_size<<endl;
                    fseek(new_file,0,SEEK_SET);
                    file_content = new char[BUFF_SIZE]();
                    //Sending file to client -> sending 1024 bytes once a time
                    int cnt =0;
                    while( file_size > 0  ){

                        memset(file_content,'\0',sizeof(file_content));
                        int tmp_packet = fread(file_content,sizeof(char),sizeof(file_content),new_file);
                        tmp_packet = send(client_sockfd, file_content, tmp_packet, 0);
                        file_size -= tmp_packet;
                        //read_byte = read(client_sockfd, buffer, 3);
                    }
                    //cout <<"cnt: "<<cnt <<endl;
                    free(file_content);
                    fclose(new_file);

                }

            }
            else if(atoi(instruction) == PLAY_VIDEO ){
                sleep(0.1);
                memset(buffer,0,sizeof(buffer));
                memset(file_path,0,sizeof(file_path));
                vector_files.clear();

                //get video name
                if((read_byte = read(client_sockfd, buffer, sizeof(buffer) - 1)) < 0){
                    ERR_EXIT("receive failed\n");
                    return -1;
                }
                sscanf(buffer,"%s",file_name);
                //cout<<"file_name: "<< file_name<<endl;
                get_files( files,vector_files );
                vector<string>::iterator it = std::find(vector_files.begin(),vector_files.end(),file_name);
                if( it != vector_files.end() ){
                    //passing file path
                    sprintf(file_path,"./40771107H_server_folder/%s",file_name);
                    sprintf(buffer,"OK");
                }
                else{
                    sprintf(buffer,"NO");
                }

                //Give reaponse of file exist
                if((write_byte = send(client_sockfd, buffer, 3, 0)) < 0){
                    ERR_EXIT("write failed\n");
                    return -1;
                }
                sleep(0.1);
                if( strcmp(buffer,"OK")==0 ){

                    VideoCapture cap(file_path); // open the default camera
                    //double rate = cap.get(CV_CAP_PROP_FPS);
                    if( !cap.isOpened() ){
                        cout << "Read video Failed." << endl;
                    }

                    Mat frame;
                    //namedWindow("video_test");
                    long long int frame_num = cap.get(CV_CAP_PROP_FRAME_COUNT);
                    //printf("mpg size: %d\n",frame_num);
                    sprintf(buffer,"%lld",frame_num);
                    //give fram_num
                    if((write_byte = send(client_sockfd, buffer, strlen(buffer), 0)) < 0){
                        ERR_EXIT("write failed\n");
                        return -1;
                    }
                    //Get Client Ready to get videos
                    if((read_byte = read(client_sockfd, buffer, sizeof(buffer) - 1)) < 0){
                        ERR_EXIT("receive failed\n");
                        return -1;
                    }

                    for( int i=0; i<frame_num-1; ++i ){
                        vector<uchar> data_encode;
                        cap.read(frame);
                        imencode(".jpg",frame,data_encode);
                        //cout<<"data encode size:"<<data_encode.size()<<endl;
                        string str_encode(data_encode.begin(), data_encode.end());
                        //cout<<"str_encode size:"<<str_encode.size()<<endl;
                        memset(buffer,0,sizeof(buffer));
                        sprintf(buffer,"%ld%c",str_encode.size(),'\0');
             
                        //sending frame size
                        if((write_byte = send(client_sockfd, buffer, strlen(buffer), 0)) < 0){
                            ERR_EXIT("write failed\n");
                            return -1;
                        }
                        //Get Client Ready to get videos
                        if((read_byte = read(client_sockfd, buffer, sizeof(buffer) - 1)) < 0){
                            ERR_EXIT("receive failed\n");
                            return -1;
                        }

                        if( strcmp(buffer,"NO")==0 ) break;

                        file_size = str_encode.size();
                        int time = 0;
                        int bytes = 0;
                        while( file_size > 0  ){
                            //printf("AAAAAAAAAAAAAAA\n");
                            get_size( tmp_size,file_size );
                            for( int j=0; j<tmp_size; ++j ){
                                buffer[j] = str_encode[time*BUFF_SIZE + j];
                                //printf("%d",int(buffer[j]));
                            }
                            //printf("\n\n");
                            //sending frame size
                            if((write_byte = send(client_sockfd, buffer, tmp_size, 0)) < 0){
                                ERR_EXIT("write failed\n");
                                return -1;
                            }
                            bytes += write_byte;
                            if((read_byte = read(client_sockfd, buffer, 3)) < 0){
                                ERR_EXIT("receive failed\n");
                                return -1;
                            }
                            //sleep(1);
                            time += 1;
                        }
                        //cout << "bytes: " << bytes << endl;
                        //imshow("video_test",frame);
                        waitKey(10);
                    }
                    //destroyWindow("video_test");
                    //cap.release();
                }
            
            }
            else{
                cout<<instruction;
                printf("DEFAULT\n");
            }
            
        }
    }
        

    return 0;
}
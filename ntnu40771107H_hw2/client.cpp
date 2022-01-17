#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/ioctl.h>
#include<net/if.h>
#include<unistd.h>
#include<string.h>
#include<dirent.h>
#include<errno.h>
#include<stdio.h>
#include<vector>
#include<string>
#include<iostream>
#include<sstream>
#include<fstream>
#include<stdlib.h>
#include<iostream>
#include<algorithm>
#include <opencv/cv.h>
#include <opencv2/core/mat.hpp>
#include "opencv2/opencv.hpp"
#include <opencv2/core/types_c.h>


using namespace std;
using namespace cv;

#define BUFF_SIZE 1024
#define PORT 8787
#define GET_ALL_FILE_NAME "1"
#define PUT_FILE "2"
#define GET_FILE "3"
#define PLAY_VIDEO "4"
#define ERR_EXIT(a){ perror(a); exit(1); }

//============================fix esc ====================
//reference:  http://www.cplusplus.com/forum/unices/11910/
//referenc: cpp.sh/4wb2zt
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

void clean_stdin()
{
        int stdin_copy = dup(STDIN_FILENO);
        /* remove garbage from stdin */
        tcdrain(stdin_copy);
        tcflush(stdin_copy, TCIFLUSH);
        close(stdin_copy);
}

void keyPress( bool &stop ){

    struct termios oldSettings, newSettings;

    tcgetattr( fileno( stdin ), &oldSettings );
    newSettings = oldSettings;
    newSettings.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr( fileno( stdin ), TCSANOW, &newSettings );

    fd_set set;
    struct timeval tv;

    tv.tv_sec = 0.3;
    tv.tv_usec = 0;

    FD_ZERO( &set );
    FD_SET( fileno( stdin ), &set );

    int res = select( fileno( stdin )+1, &set, NULL, NULL, &tv );

    if( res > 0 )
    {
        char c;
        //printf( "Input available\n" );
        //read( fileno( stdin ), &c, 1 );
        c = getchar();
        clean_stdin();
        printf("\n");
        if( c==27 ) stop = true;
    }
    else if( res < 0 )
    {
        perror( "select error" );
    }
    else
    {
        fputc ( 'a', stdin );
    }


    tcsetattr( fileno( stdin ), TCSANOW, &oldSettings );
}



//===============================================================================


int get_my_files( char *path,vector <string> &files ){

    DIR *pf;
    struct dirent *pdir;
    if( (pf = opendir( path )) == NULL ){
        printf("ERROR: %d\n",errno);
        return -1;
    }
    while( (pdir = readdir(pf)) != NULL ){
        if( string(pdir->d_name)!="." &&  string(pdir->d_name)!=".." )
            files.push_back(string(pdir->d_name));
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


int main(int argc , char *argv[]){
    int sockfd;
    struct sockaddr_in addr;
    //*************getline()**************
    string instruction;
    char fileName[BUFF_SIZE] = {};
    char sys_instruct[BUFF_SIZE] = "mkdir ";
    int port = 0;
    char inet[BUFF_SIZE] = "";
    char my_path[2*BUFF_SIZE] = "";
    vector<string> upload_files = vector<string> ();
    vector<string> my_files = vector<string> ();
    vector<string> server_file = vector<string> ();

    //Get IP and Port Number
    char *token = strtok(argv[2],":");
    sprintf(inet,"%s",token);
    token = strtok(NULL,":");
    port = strtol(token,NULL,10);
    sprintf(fileName,"40771107H_%s_client_folder",argv[1]);
    // mkdir folder
    if( access(fileName,F_OK) != 0 ){
        //printf("%d\n",access("fileName",F_OK));
        strcat(sys_instruct,fileName);
        system(sys_instruct);
    }


    // Get socket file descriptor -> tcp
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        ERR_EXIT("socket failed\n");
    }

    // Set server address
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(inet);
    addr.sin_port = htons(port);

    // Connect to the server
    if(connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        ERR_EXIT("connect failed\n");
    }

    printf("$ ");
    while( getline(cin,instruction) ){

        char buffer[2*BUFF_SIZE] = {};
        //INITIAL
        my_files = vector<string> ();
        upload_files = vector<string> ();
        long long int file_size = 0;
        int amount_of_file = 0;
        int tmp_size = 0;
        int  read_byte = 0, write_byte = 0;


        //=====GET ALL Server file=======
        if( instruction == "ls" ){
            sprintf(buffer, "%s", GET_ALL_FILE_NAME);
            // Send instruction to server
            write_byte = send(sockfd, buffer, 2, 0);
            read_byte = 0;
            do{
                read_byte = read(sockfd, buffer,sizeof(buffer));
                cout<<"read_byte: "<<read_byte<<", buffer: "<<buffer<<endl;
            }while(read_byte>0);
            //cout<<"read_byte: "<<read_byte<<", file_size: "<<file_size<<endl;
            if( file_size==0 ){ cout <<"No files."<<endl;}
            else { read_byte = read(sockfd, buffer, file_size); cout << buffer;}

        }
        //=====PUT FILE FROM SERVER====================
        else if( instruction.find("put")!=std::string::npos ){

            istringstream in(instruction);
            string tmp;
            in >> tmp;
            while( in >> tmp ) upload_files.push_back(tmp);

            if( upload_files.size()==0 ){
                cout<<"There is no file to put."<<endl;
                goto exit;
            }

            sprintf(my_path,"./%s",fileName);
            get_my_files( my_path,my_files );
            //printf("%s\n",instruction);

            for( string file : upload_files ){
                vector<string>::iterator it = std::find(my_files.begin(),my_files.end(),file);
                if( it != my_files.end() )
                    amount_of_file += 1;
            }

            sprintf(buffer, "%s", PUT_FILE);
            // Send instruction to server
            write_byte = send(sockfd, buffer, 2, 0);

            //Sending amount_of_file to server
            write_byte = send(sockfd, &amount_of_file, sizeof(amount_of_file), 0);

            for( string file : upload_files ){

                vector<string>::iterator it = std::find(my_files.begin(),my_files.end(),file);

                //------Upload file--------------
                char file_path[3*BUFF_SIZE];
                sprintf(file_path,"%s/%s",my_path,file.c_str());
                if( it != my_files.end() ){


                    fstream new_file;
                    new_file.open(file_path, ios::in | ios::binary);
                    if( ! new_file.is_open() ){
                        printf("File could not be opened\n");
                        perror( "ERROR" );
                        return -1;
                    }
                    //send file name and file size to server
                    new_file.seekg(0,ios::end);
                    file_size = new_file.tellg();
                    //puts(buffer);

                    write_byte = send(sockfd, &file_size, sizeof(file_size), 0);
                    write_byte = send(sockfd, file.c_str(), file.size(), 0);
                    read_byte = read(sockfd, buffer, 3);
                    //cout<<"OK: "<<buffer<<endl;
                    new_file.seekg(0,ios::beg);
                    //Sending file to server -> sending 1024 bytes once a time
                    cout << "putting " << file <<"......"<<endl;
                    while( file_size > 0  ){

                        get_size( tmp_size,file_size );
                        memset(buffer,'\0',sizeof(buffer));
                        new_file.read(buffer,sizeof(char)*tmp_size);
                        write_byte = send(sockfd, buffer, tmp_size, 0);
                        //read_byte = read(sockfd, buffer, 3);
                    }
                    new_file.close();
                }
                //Not found
                else
                    cout<<"The "<<file<<" doesn't exist."<<endl;
            }

        }
        //=====GET FILE FROM SERVER=================
        else if( instruction.find("get")!=std::string::npos ){

            istringstream in(instruction);
            string tmp;
            in >> tmp;
            while( in >> tmp ){
                upload_files.push_back(tmp);
                amount_of_file += 1;
            }
            if( upload_files.size()==0 ){
                printf("There is no file to get.\n");
                goto exit;
            }

            sprintf(buffer, "%s", GET_FILE);
            // Send instruction to server
            if((write_byte = send(sockfd, buffer, 2, 0)) < 0){
                ERR_EXIT("write failed\n");
                return -1;
            }

            //Sending amount_of_file to server
            write_byte = send(sockfd, &amount_of_file, sizeof(amount_of_file), 0);

            sprintf(my_path,"./%s",fileName);
            //sending upload_files
            for( string file:upload_files ){
                char file_path[3*BUFF_SIZE];
                memset(buffer,'\0',sizeof(buffer));
                //sending each file name
                if((write_byte = send(sockfd, file.c_str(), file.size(), 0)) < 0){
                    ERR_EXIT("write failed\n");
                    return -1;
                }
                //Get hole file size
                if((read_byte = read(sockfd, &file_size, sizeof(file_size))) < 0){
                    ERR_EXIT("receive failed\n");
                    return -1;
                }
                //sscanf(buffer,"%lld",&file_size);
                //printf("file_size: %lld\n",file_size);
                if( file_size > 0 ){
                    //start getting file
                    memset(file_path,'\0',sizeof(file_path));
                    sprintf(file_path,"%s/%s",my_path,file.c_str());
                    fstream new_file;
                    new_file.open(file_path, ios::out | ios::trunc | ios::binary );
                    if( ! new_file.is_open() ){
                        printf("File could not be opened\n");
                        perror( "ERROR" );
                        return -1;
                    }
                    cout<<"getting "<<file<<"......"<<endl;

                    //Recieve hole file

                    //cout<<"file_size: "<<file_size<<endl;
                    while( file_size > 0  ){
                        memset(buffer,'\0',sizeof(buffer));
                        get_size( tmp_size,file_size );
                        read_byte = read(sockfd, buffer, tmp_size );
                        //write_byte = send(sockfd, "OK", 3, 0);
                        //cout << "Receiving "<< tmp_size <<" bytes."<<endl;
                        //===Write in file===
                        new_file.write( buffer,tmp_size );
                        //cnt += 1;

                    }
                    //read_byte = read(sockfd, buffer, tmp_size );
                    //cout <<"cnt: "<<cnt <<endl;
                    new_file.close();

                }
                else{
                    //File not exists
                    cout<<"The "<<file<<" doesn't exist."<<endl;
                }
            }


        }
        //=====PLAY VIDEO FROM SERVER=================
        else if( instruction.find("play")!=std::string::npos ){

            sleep(0.1);
            istringstream in(instruction);
            string tmp;
            in >> tmp;in >> tmp;
            //cout<< tmp.substr(tmp.size()-4) << endl;

            if( tmp.size() <= 4 || tmp.substr(tmp.size()-4) != ".mpg")
            {
                cout <<"The "<< tmp <<" is not a mpg ﬁle." << endl;
                goto exit;
            }
            sprintf(buffer, "%s", PLAY_VIDEO);
            // Send instruction to server
            if((write_byte = send(sockfd, buffer, 2, 0)) < 0){
                ERR_EXIT("write failed\n");
                return -1;
            }

            if((write_byte = send(sockfd, tmp.c_str(), tmp.size(), 0)) < 0){
                ERR_EXIT("write failed\n");
                return -1;
            }

            //Is video file exist?
            if((read_byte = read(sockfd, buffer, sizeof(buffer))) < 0){
                ERR_EXIT("receive failed\n");
                return -1;
            }
            if( strcmp(buffer,"OK")!=0 ){
                cout <<"The "<< tmp <<" doesn't exist." << endl;
                goto exit;
            }

            memset(buffer,0,sizeof(buffer));
            sleep(0.1);
            cout<<"playing the video..."<<endl;
            //Get hole file size
            if((read_byte = read(sockfd, buffer, sizeof(buffer))) < 0){
                ERR_EXIT("receive failed\n");
                return -1;
            }
            // Send instruction to server
            if((write_byte = send(sockfd, "OK", 3, 0)) < 0){
                ERR_EXIT("write failed\n");
                return -1;
            }

            namedWindow("video_test");
            sscanf(buffer,"%d",&amount_of_file);
            //cout<<"amount_of_file: "<<amount_of_file<<endl;
            Mat frame;
            bool stop = false;
            for( long long int i=0; i<amount_of_file-1 ; ++i ){
                keyPress( stop );
                memset(buffer,0,sizeof(buffer));
                //Getting frame size
                if((read_byte = read(sockfd, buffer, sizeof(buffer))) < 0){
                    ERR_EXIT("receive failed\n");
                    return -1;
                }
                //cout<<"buffer:"<<buffer<<endl;
                sscanf(buffer,"%lld",&file_size);
                //cout<<"file_size "<<file_size<<endl;
                // Client ready to get videos
                if( stop ) sprintf(buffer,"NO");
                else sprintf(buffer,"OK");

                if((write_byte = send(sockfd, buffer, 3, 0)) < 0){
                    ERR_EXIT("write failed\n");
                    return -1;
                }

                if(stop) break;

                string frame_data;
                //one frame
                while( file_size > 0  ){

                    get_size( tmp_size,file_size );
                    char *content = new char[BUFF_SIZE]();
                    string str_tmp;
                    if((read_byte = read(sockfd, content, tmp_size ))< 0){
                        ERR_EXIT("receive failed\n");
                        return -1;
                    }
                    for( int j=0; j<tmp_size; ++j ){
                        str_tmp.push_back(content[j]);
                    }
                    free(content);
                    frame_data += str_tmp;
                    //cout<<"frame_data:"<<frame_data.size()<<endl;
                    if((write_byte = send(sockfd, "OK", 3, 0)) < 0){
                        ERR_EXIT("write failed\n");
                        return -1;
                    }
                    //cout<<"str_tmp: "<<str_tmp<<endl<<endl;
                }
                //cout<<frame_data<<endl;
                //cout<<"frame_data:"<<frame_data.size()<<endl;
                vector<uchar> data_decode(frame_data.begin(),frame_data.end());
                frame = imdecode( data_decode,CV_LOAD_IMAGE_COLOR );
                if( !frame.empty() )
                    imshow("video_test",frame);
                waitKey(10);
                //if( i==amount_of_file-2 ) f = true;
            }
            destroyWindow("video_test");

        }
        else{
            cout<<"Command not found."<<endl;
        }
        exit:
        printf("$ ");
    };
    close(sockfd);
    return 0;
}
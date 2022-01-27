#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include<iostream>
#include<string>
#include<algorithm>
#include<vector>
#include<iterator>
#include<map>

int SERVER_PORT=1030;
#define QUEUE_SIZE 50

int gracz1;
int challenging_player=0;
int player_is_waiting=0;

pthread_cond_t player_join; 
pthread_mutex_t mutex;

class Board
{
    public:
    char tab[64];

    int index(int x, int y)
    {
        return 8*x+y;
    }
    Board()
    {
        for(int i=0;i<8;i++)
        {
            for(int j=0;j<8;j++)
            {
                tab[index(i,j)]='e';
            }
        }
        tab[index(3,3)]='w';
        tab[index(4,4)]='w';
        tab[index(3,4)]='b';
        tab[index(4,3)]='b';
    }
    void set_field(int i, char color)
    {
        if(i>=0 && i<64)
        {
            tab[i]=color;
        }
    }
    bool check_move(char color, int x, int y, bool test)
    {
        char t[8][8];
        for(int i=0;i<8;i++)
        {
            for(int j=0;j<8;j++)
            {
                t[i][j]=tab[index(i,j)];
            }
        }
        char op; 
        if(color=='b')
        {
            op='w';
        }
        else if(color=='w')
        {
            op='b';
        }
        if(t[x][y]!='e')
        {
            return false;
        }
        bool conditions[8];
        for(int i=0;i<8;i++)
        {
            conditions[i]=false;
        }
         t[x][y]=color;
        if(x>1 && t[x-1][y]==op)
        {
            int i=1;
            while(x-i>0 && t[x-i][y]==op)
            {
                i++;
            }
            if(x-i>=0 && t[x-i][y]==color)
            {
                conditions[0]=true;
                for(int j=0;j<=i;j++)
                {
                    t[x-j][y]=color;
                }
            }
        }
        if(x<6 && t[x+1][y]==op)
        {
            int i=1;
            while(x+i<7 && t[x+i][y]==op)
            {
                 i++;
            }
            if(x+i<=7 && t[x+i][y]==color)
            {
                conditions[1]=true;
                for(int j=0;j<=i;j++)
                {
                    t[x+j][y]=color;
                }
            }
        }
        if(y<6 && t[x][y+1]==op)
        {
            int i=1;
            while(y+i<7 && t[x][y+i]==op)
            {
                 i++;
            }
            if(y+i<=7 && t[x][y+i]==color)
            {
                conditions[2]=true;
                 for(int j=0;j<=i;j++)
                {
                    t[x][y+j]=color;
                }
            }
        }
        if(y>1 && t[x][y-1]==op)
        {
            int i=1;
            while(y-i>0 && t[x][y-i]==op)
            {
                 i++;
            }
            if(y-i>=0 && t[x][y-i]==color)
            {
                conditions[3]=true;
                  for(int j=0;j<=i;j++)
                {
                    t[x][y-j]=color;
                }
            }
        }
        if(y<=5 && x>1 && t[x-1][y+1]==op)
        {
            int i=1;
            while(y+i<7 && x-i>0&&t[x-i][y+i]==op)
            {
                i++;
            }
            if(y+i<=7 && x-i>=0&&t[x-i][y+i]==color)
            {
                conditions[4]=true;
                   for(int j=0;j<=i;j++)
                {
                    t[x-j][y+j]=color;
                }
                
            }
        }
        if(y<=5 && x<=5 && t[x+1][y+1]==op)
        {
            int i=1;
            while(y+i<7 && x+i<7 &&t[x+i][y+i]==op)
            {
                 i++;
            }
            if(y+i<=7 && x+i<=7 &&t[x+i][y+i]==color)
            {
                conditions[5]=true;
                for(int j=0;j<=i;j++)
                {
                     t[x+j][y+j]=color;
                }
            }
        }
          if(y>=2 && x<=5 && t[x+1][y-1]==op)
        {
            int i=1;
            while(y-i>0 && x+i<7 &&t[x+i][y-i]==op)
            {
                 i++;
            }
            if(y+i>=0 && x+i<=7 &&t[x+i][y-i]==color)
            {
                conditions[6]=true;
                 for(int j=0;j<=i;j++)
                {
                    t[x+j][y-j]=color;
                }
            }
        }
           if(y>=2 && x>=2 && t[x-1][y-1]==op)
        {
            int i=1;
            while(y-i>0 && x-i>0 &&t[x-i][y-i]==op)
            {
                i++;
            }
            if(y-i>=0 && x-i>=0 &&t[x-i][y-i]==color)
            {
                conditions[7]=true;
                for(int j=0;j<=i;j++)
                {
                    t[x-j][y-j]=color;
                }
            }
        }
        bool flag=false;
        for(int i=0;i<8;i++)
        {
            flag|=conditions[i];
        }
        if(test==true)
        {
            return flag;
        }
        if(flag==true)
        {
          for(int i=0;i<8;i++)
        {
            for(int j=0;j<8;j++)
            {
                tab[index(i,j)]=t[i][j];
            }
        }
            return true;
        }
        else if(flag==false)
        {
            return false;
        }
        return false;
    }
    bool exist_move(char color)
    {
        bool e_move=false;
        for(int i=0;i<8;i++)
        {
            for(int j=0;j<8;j++)
            {
                e_move|=check_move(color,i,j,true);
            }
        }
        return e_move;
    }
    int count_pawns(char color)
    {
        int tmp=0;
        for(int i=0;i<8;i++)
        {
            for(int j=0;j<8;j++)
            {
                if(tab[index(i,j)]==color)
                {
                    tmp++;
                }
            }
        }
        return tmp;
    }
};

void send(int player, char* msg, int size)
{
    int w=write(player,msg,size);
    if(w<=0 || w==EPIPE)
    {
        throw player;
    }
}
char * my_read(int player, int size)
{
    char* buffor= new char[size];
    int r=read(player,buffor,size);
    if(r<=0)
    {
        close(player);
        throw player;
    }

    return buffor;
}

void* game_room(void *client_scoket)
{                 
    int player_white=*(int*)client_scoket;
    int player_black;
    Board board;
    player_is_waiting=1;
    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&player_join, &mutex);
    player_black=challenging_player;
    player_is_waiting=0;
    pthread_mutex_unlock(&mutex);
    std::map<int,int>opposite_players_map;
    opposite_players_map[player_black]=player_white;
    opposite_players_map[player_white]=player_black;
    char text1[]="b";
    char text2[]="w";
    char connection_error[]="c";
    try
    {
        send(player_white,text2,1);
        send(player_black,text1,1);

        send(player_white,board.tab,64);
        send(player_black,board.tab,64);
        char turn[]="w"; 
        char good_move[]="g";
        char mistake_move[]="m";
        while(true)
        {
            char* move;
            send(player_white,turn,1); 
            send(player_black,turn,1); 
            if(turn[0]=='f')
            {
                int white_pawns=board.count_pawns('w');
                int black_pawns=board.count_pawns('b');
                if(white_pawns>black_pawns)
                {
                    char winner[]="w";
                    send(player_black,winner,1);
                    send(player_white,winner,1);
                }
                else if (white_pawns==black_pawns)
                {
                    char winner[]="d";
                    send(player_black,winner,1);
                    send(player_white,winner,1);
                }
                else
                {
                    char winner[]="b";
                    send(player_black,winner,1);
                    send(player_white,winner,1);
                }
                break;
            }
            if(turn[0]=='w')
            {
                while(true)
                {
                    sleep(0.01);
                    move=my_read(player_white,2);
                    int tmp=10*(move[0]-'0')+(move[1]-'0');
                    bool decision=board.check_move('w',tmp/8,tmp%8,false);
                    if(decision==true)
                    {
                        send(player_white,good_move,1);
                        break;
                    }
                    if(decision==false)
                    {
                        send(player_white,mistake_move,1);
                    }
                    
                }
            }
            else if (turn[0]=='b')
            {
                while(true)
                {
                    sleep(0.01);
                    move=my_read(player_black,2);
                    int tmp=10*(move[0]-'0')+(move[1]-'0');
                    bool decision=board.check_move('b',tmp/8,tmp%8,false);
                    if(decision==true)
                    {
                        send(player_black,good_move,1);
                        break;
                    }
                    if(decision==false)
                    {
                        send(player_black,mistake_move,1);
                    }
                }
            }
            send(player_white,board.tab,64);
            send(player_black,board.tab,64); 
            if(turn[0]=='w')
            {
                turn[0]='b';
            }
            else if (turn[0] =='b')
            {
                turn[0] ='w';
            }
            bool w_flag=board.exist_move('w');
            bool b_flag=board.exist_move('b');
            if(w_flag==false && b_flag==false)
            {
                turn[0]='f'; //finish game
            }
            else if(w_flag==false && turn[0]=='w')
            {
                turn[0]='b';
            }
            else if(b_flag==false && turn[0]=='b')
            {
                turn[0]='w';
            }
        }
    }
    catch (int player)
    {
        try
        {
            send(opposite_players_map[player],connection_error,1);
            close(opposite_players_map[player]);
        }
        catch(int player)
        {
           return NULL;
        }
    }
    return NULL;
}


int main(int argc, char* argv[])
{
    if(argc>1)
    {
        SERVER_PORT=strtol(argv[1],NULL,10);
    }
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler=SIG_IGN;
    if(sigaction(SIGPIPE,&act,NULL))
    {
        perror("sigaction");
        exit(1);
    }
    while(true)
    {
        sleep(0.1);
        pthread_t thread[1];
        pthread_cond_init(&player_join, NULL);
        pthread_mutex_init(&mutex, NULL);
        int nSocket, nClientSocket;
        socklen_t nTmp;
        struct sockaddr_in stAddr, stClientAddr;
        /* address structure */
        memset(&stAddr, 0, sizeof(struct sockaddr));
        stAddr.sin_family = AF_INET;
        stAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        stAddr.sin_port = htons(SERVER_PORT); 
        /* create a socket */
        nSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (nSocket < 0)
        {
            fprintf(stderr, "%s: Can't create a socket.\n", argv[0]);
            exit(1);
        }
        int nBind = bind(nSocket, (struct sockaddr*)&stAddr, sizeof(struct sockaddr));
        if ( nBind<0)
        {
        exit(1);
        }
        int nListen = listen(nSocket, QUEUE_SIZE);    
    
        if( nListen<0)
        {
            exit(1);
        }
        while(1)
        {
            sleep(0.1);
            nTmp = sizeof(struct sockaddr);
            nClientSocket = accept(nSocket, (struct sockaddr*)&stClientAddr, &nTmp);
            if (nClientSocket < 0)
            {
                exit(1);
            }
            pthread_mutex_lock(&mutex);
            if(player_is_waiting==0)
            {
                gracz1=nClientSocket;
                pthread_create(&thread[0],NULL,&game_room,&nClientSocket);
                pthread_mutex_unlock(&mutex);
            }
            else
            {
                challenging_player=nClientSocket;
                pthread_mutex_unlock(&mutex);
                pthread_cond_signal(&player_join);
            }
        }
        close(nSocket);
    }
    return 0;
}
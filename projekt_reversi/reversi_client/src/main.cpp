
#include<SFML/Graphics.hpp>
#include<SFML/System.hpp>
#include<SFML/Window.hpp>
#include<utility>
#include<iostream>
#include <stdexcept>
#include<thread>
#include<mutex>

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include<sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>

#define BUFSIZE 10000
char bufor[BUFSIZE];
char *server = (char *)"127.0.0.1";	
char *protocol = (char *) "tcp";
short service_port = 1030;	
bool thread_end=false;
std::mutex thread_end_mutex;
std::mutex socket_close_mutex;
enum field{
	white = 0,
	black = 1,
	empty = 2, 
};
class Field
{
	private:
	int type;
	std::pair<int,int>position;
	public:
	Field(){}
	Field(std::pair<int,int> pos)
	{
		position=pos;
	}
	int get_type()
	{
		return type;
	}
	std::pair<int,int> get_position()
	{
		return position;
	}
	void change_color()
	{
		if (type==white)
		{
			type=black;
		}
		else if(type==black)
		{
			type=white;
		}
	}
	void set_color(field c)
	{
		type=c;
	}
};

class Board
{
	public:
	Field game_board[8][8];
	Board() 
	{
		for(int i=0;i<8;i++)
		{
			for(int j=0;j<8;j++)
			{
				game_board[i][j]= Field(std::make_pair(i,j));
				game_board[i][j].set_color(empty);
			}
		}
		game_board[3][3].set_color(white);
		game_board[4][4].set_color(white);
		game_board[3][4].set_color(black);
		game_board[4][3].set_color(black);
	}
	void update_board(std::string board)
	{
		for(int i=0;i<8;i++)
		{
			for(int j=0;j<8;j++)
			{
				switch( board[i*8+j])
				{
					case 'e':
						game_board[i][j].set_color(empty);
						break;
					case 'w':
						game_board[i][j].set_color(white);
						break;
					case 'b':
						game_board[i][j].set_color(black);
				}
			}
		}
	}
};
class BSDsocket
{
	int sck;
	struct sockaddr_in sck_addr;
	public:
	void bsd_connect()
	{
		memset (&sck_addr, 0, sizeof (sck_addr));
		sck_addr.sin_family = AF_INET;
		inet_aton (server, &sck_addr.sin_addr);
		sck_addr.sin_port = htons (service_port);
		if ((sck = socket (AF_INET, SOCK_STREAM, 0)) < 0)
		{
			throw (std::string)"Socket creation error";
		}
		if (connect (sck, (struct sockaddr*) &sck_addr, sizeof sck_addr) < 0)
		{
			throw (std::string)"Connection with server error";
		}
	}
	char* bsd_read(int size)
	{
		char* buffor= new char[size];
		int r=read(sck,buffor,size);
		if(r<=0)
		{
			if(r==-1)
			{
				throw (std::string) "Connection error. \nTry again later";
			}
			if(r==0)
			{
				throw (std::string) "Server connection error. \nThe game ended in the draw.";
			}
		}
		for(int i=0;i<r;i++)
		{
			if(buffor[i]=='c')
			{
				throw (std::string) "Opponent's connection error. \nThe game ended in the draw.";
			}
		}
		return buffor;
	}
	void bsd_write(int size, char* msg)
	{
		int count=0;
		std::string tmpp=std::string(msg,size);
		int w=write(sck,msg,size);
		int tmp=0;
		tmp+=w;
		while(tmp<size)
		{
			count++;
			if(w==-1)
			{
				close(sck);
				throw (std::string)"Connection error";
			}
			w=write(sck,msg+tmp,size-tmp);
		}
	}
	void my_close()
	{
		shutdown(sck,SHUT_RDWR);
	}
};

class Game {
	public:
	Board board;
	sf::RenderWindow window;
	std::mutex window_mutex;
	BSDsocket connection;
	int odp;
	char color;
	bool turn;
	bool end_game;
	char winner;
	sf::Font myFont;
	public:
	Game()
	{
		board = Board();
		end_game=false;
		connection=BSDsocket();
		window.create(sf::VideoMode(8*41,8*41+100), "Reversi");
		window.setFramerateLimit(10);
	}
	void close_socket()
	{
		connection.my_close();
	}
	void send_move(int move)
	{
		char num[2];
		num[0]=(char)move/10+'0';
		num[1]=(char)move%10+'0';
		connection.bsd_write(2,num);
	}
	void get_color()
	{
		char* c=connection.bsd_read(1);
		if(c[0]=='b')
		{
			color='b';
		}
		else if (c[0]=='w')
		{
			color='w';
		}
		delete c;
	}
	bool  receive_move()
	{
		char* b=connection.bsd_read(1); 
		if(b[0]=='g') 
		{
			delete b;
			return true; 
		} 
		else if(b[0]=='m')
		{
			delete b;
			return false; 
		}
		delete b;
		return false;
	}
	void get_board()
	{
		char* b=connection.bsd_read(64);
		board.update_board(std::string(b,64));
		delete b;
	}
	void load_font()
	{
		bool flag=myFont.loadFromFile(std::string("src/fonts/arial.ttf"));
		if(flag==false)
		{
			throw (std::string)"Font error ";
		}
	}
	void get_turn()
	{
		char* b=connection.bsd_read(1);
		if(b[0]==color)
		{
			turn=true;
		}
		else if(b[0]!='f')
		{
			turn=false;
		}
		else
		{
			turn =false;
			end_game=true;
		}
		delete b;
	}
	void get_winner()
	{
		char*w =connection.bsd_read(1);
		winner=w[0];
		delete w;
	}
	void do_move()
	{
		sf::Event ev;
		while(true)
		{
			sleep(0.01);
			window_mutex.lock();
			while(window.pollEvent(ev))
			{
				if(ev.type==sf::Event::Closed)
				{
					window.close();
					window_mutex.unlock();
					close_socket();
					thread_end_mutex.lock();
					thread_end=true;
					thread_end_mutex.unlock();
					return;
				}
				else if(sf::Mouse::isButtonPressed(sf::Mouse::Left))
				{
					window_mutex.unlock();
					sf::Vector2i mouse_position=sf::Mouse::getPosition(window);
					int x=mouse_position.x;
					int y=mouse_position.y;
					if(x>=0 && x<=8*41 && y>=0 && y<=8*41)
					{
						int xx=x/41;
						int yy=y/41;
						int tmp=xx+8*yy;
						send_move(tmp); 
						bool flag=receive_move();
						if(flag==true) 
						{
							return;				
						}
					}		
					sleep(0.1);
					window_mutex.lock();
					continue;	
				}
				else
				{
					window_mutex.unlock();
					sleep(0.01);
					window_mutex.lock();
					continue;
				}
				window_mutex.lock();
			}
			window_mutex.unlock();
		}
	}
	void run()
	{
		try
		{
			load_font();
			draw_start_window();
			connection.bsd_connect();
			window_mutex.lock();
			draw_start_window();
			window_mutex.unlock();
			get_color();
			get_board();
			get_turn();
			window_mutex.lock();
			set_window_for_game_board();
			draw_board();
			window_mutex.unlock();
			while(true)
			{
				if(turn && end_game==false)
				{
					do_move(); 
				}
				else if(end_game)
				{
					
					get_winner();
					window_mutex.lock();
					draw_end_game();
					window_mutex.unlock();
					break;
				}
				get_board(); 
				get_turn(); 
				window_mutex.lock();
				draw_board();
				window_mutex.unlock();
			}
		}
		catch(std::string e)
		{
			draw_error_window(e);
		}
	}
	void start_window()
	{
		sf::Text message;
		message.setFont(myFont);
		message.setString((std::string)"Waiting for connection with server");
		message.setPosition(20,20);
		message.setCharacterSize(20);
		message.setFillColor(sf::Color::White);
		window.clear();
		window.draw(message);
		window.display();
		check_for_close();
	}
	void draw_turn()
	{
		sf::Text turn_text;
		turn_text.setFont(myFont);
		std::string turn_string;
		if( turn==true)
		{
			turn_string="Your turn ";
		}
		else
		{
			turn_string="Opponent's turn ";
		}
		turn_text.setString(turn_string);
		turn_text.setPosition(0,8*41+35);
		turn_text.setFillColor(sf::Color::Black);
		turn_text.setCharacterSize(30);
		turn_text.setFillColor(sf::Color::White);
		window.draw(turn_text);		
		window.display();
	}
	void draw_end_game()
	{
		window.create(sf::VideoMode(200,100), "Reversi");
		window.setFramerateLimit(10);
		sf::Text Congratulations;
		Congratulations.setFont(myFont);
		std::string winer_string;
		if(winner==color)
		{
			winer_string="You won";
		}
		else if (winner=='b')
		{
			winer_string="You lost";
		}
		else if (winner=='d')
		{
			winer_string="Draw";
		}
		Congratulations.setString(winer_string);
		Congratulations.setPosition(0,50);
		Congratulations.setCharacterSize(30);
		Congratulations.setFillColor(sf::Color::White);
		window.clear();
		window.draw(Congratulations);
		window.display();
	}
	void set_window_for_game_board()
	{
		window.create(sf::VideoMode(8*41,8*41+200), "Reversi");
		window.setFramerateLimit(10);
	}
	void check_for_close()
	{
		sf::Event ev;
		thread_end_mutex.lock();
		if(thread_end==true)
		{
			thread_end_mutex.unlock();
			return;
		}
		thread_end_mutex.unlock();
		while(true)
		{
			thread_end_mutex.lock();
			if(thread_end==true)
			{
				thread_end_mutex.unlock();
				return;
			}
			thread_end_mutex.unlock();
			sleep(1);
			window_mutex.lock();
			while(window.pollEvent(ev))			
			{	
				thread_end_mutex.lock();
				if(thread_end==true)
				{
					window_mutex.unlock();
					return;
				}
				thread_end_mutex.unlock();
				if(ev.type==sf::Event::Closed)
				{
					window.close();
					close_socket();
					thread_end_mutex.lock();
					thread_end=true;
					thread_end_mutex.unlock();
					window_mutex.unlock();
					close_socket();
					return;
				}
			}
			window_mutex.unlock();
		}
	}
	void draw_board()
	{
		window.clear(sf::Color::Blue);
		sf::RectangleShape shape;
		shape.setSize(sf::Vector2f(40.0,40.0));
		for(int i=0;i<8;i++)
		{
			for(int j=0;j<8;j++)
			{
				shape.setFillColor(sf::Color::Green);
				shape.setOutlineColor(sf::Color::Green);
				shape.setPosition(sf::Vector2f(j*41, i*41));
				if(board.game_board[i][j].get_type()==empty)
				{	
				shape.setFillColor(sf::Color::Green);
				shape.setOutlineColor(sf::Color::Green);
				}
				if(board.game_board[i][j].get_type()==black)
				{
				shape.setFillColor(sf::Color::Black);
				shape.setOutlineColor(sf::Color::Black);
				}
				if(board.game_board[i][j].get_type()==white)
				{
				shape.setFillColor(sf::Color::White);
				shape.setOutlineColor(sf::Color::White);
				}
				window.draw(shape);
			}
		}
		sf::Text color_text;
		color_text.setFont(myFont);
		std::string color_string="Your color is ";
		if( color=='w' )
		{
			color_string+="white";
		}
		else
		{
			color_string+="black";
		}
		color_text.setString(color_string);
		color_text.setPosition(0,450);
		color_text.setCharacterSize(30);
		color_text.setFillColor(sf::Color::White);
		draw_turn();	
		window.draw(color_text);	
		window.display();
	}
	void draw_error_window(std::string msg)
	{
		sf::Text message;
		message.setFont(myFont);
		message.setString(msg);
		message.setPosition(20,20);
		message.setCharacterSize(20);
		message.setFillColor(sf::Color::White);
		window.clear();
		window.draw(message);
		window.display();
		check_for_close();
	}
	void draw_start_window()
	{
		window.create(sf::VideoMode(400,100), "Reversi");
		window.setFramerateLimit(10);
		sf::Text message;
		window.clear();
		message.setFont(myFont);
		message.setString("Waiting for the opponent");
		message.setPosition(0,50);
		message.setCharacterSize(20);
		message.setFillColor(sf::Color::White);
		window.draw(message);
		window.display();
	}
};

int main (int argc, char * argv[])
{
	if(argc>1)
	{
		server=argv[1];
	}
	Game *game;
	game =new Game();
	std::thread close_window_thread(&Game::check_for_close,game);
	std::thread play(&Game::run,game);
	while(true)
	{
		sleep(0.01);
		thread_end_mutex.lock();
		if(thread_end==true)
		{
			thread_end_mutex.unlock();	
			break;	
		}
		thread_end_mutex.unlock();	
	}
	close_window_thread.join();
	play.join();
	return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include<arpa/inet.h>
#include<stdio.h>

#define PORT 8000

void error(const char *msg)
{
    #ifdef DEBUG
    perror(msg);
    #else
    printf("Either the server shut down or the other player disconnected.\nGame over.\n");
    #endif 

    exit(0);
}

/* Reads a message from the server socket. */
void recv_msg(int sockfd, char * msg)
{
    memset(msg, 0, 4);
    int n = read(sockfd, msg, 3);
    
    if (n < 0 || n != 3) /* Not what we were expecting. Server got killed or the other client disconnected. */ 
        error("ERROR reading message from server socket.");

}

/* Reads an int from the server socket. */
int recv_int(int sockfd)
{
    int msg = 0;
    int n = read(sockfd, &msg, sizeof(int));
    
    if (n < 0 || n != sizeof(int)) 
        error("ERROR reading int from server socket");
    
    return msg;
}

/*
 * Socket Write Functions
 */

/* Writes an int to the server socket. */
void write_server_int(int sockfd, int msg)
{
    int n = write(sockfd, &msg, sizeof(int));
    if (n < 0)
        error("ERROR writing int to server socket");
}

// Draws the game board 
void draw_board(char board[][3])
{
    printf(" %c | %c | %c \n", board[0][0], board[0][1], board[0][2]);
    printf("-----------\n");
    printf(" %c | %c | %c \n", board[1][0], board[1][1], board[1][2]);
    printf("-----------\n");
    printf(" %c | %c | %c \n", board[2][0], board[2][1], board[2][2]);
}

/* Get's the players turn and sends it to the server. */
void take_turn(int sockfd)
{
    char buffer[10];
    
    while (1) { /* Ask until we receive. */ 
        printf("Enter  to make a move: ");
	    fgets(buffer, 10, stdin);
	    int move = buffer[0] - '0';
        if (move <= 9 && move >= 0){
            printf("\n");
            /* Send players move to the server. */
            write_server_int(sockfd, move);   
            break;
        } 
        else
            printf("\nInvalid input. Try again.\n");
    }
}

// Gets a board update from the server. 
void get_update(int sockfd, char board[][3])
{
    // Get the update. 
    int player_id = recv_int(sockfd);
    int move = recv_int(sockfd);

    // Update the game board.
    board[move/3][move%3] = player_id ? 'X' : 'O';    
}

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in server;
    memset(&server,0,sizeof(server));
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        printf("Socket creation failed !\n");
        exit(0);
    }
    printf("Socket creation successful !\n");

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    if(connect(sockfd,(struct sockaddr *)&server,sizeof(server))!= 0){
        printf("Connection failed \n");
        exit(0);
    }
    printf("Connection successful !\n");

    /* The client ID is the first thing we receive after connecting. */
    int id = recv_int(sockfd);


    char msg[4];
    char board[3][3] = { {'0', '1', '2'}, /* Game board */
                         {'3', '4', '5'}, 
                         {'6', '7', '8'} };

    printf("Tic-Tac-Toe\n------------\n");

    // Waiting for the game to start.
    do {
        recv_msg(sockfd, msg);
        if (!strcmp(msg, "HLD"))
            printf("Waiting for a second player...\n");
    } while ( strcmp(msg, "SRT") );

    // The game commences :). 
    printf("Game on!\n");
    printf("Your are %c's\n", id ? 'X' : 'O');

    draw_board(board);

    while(1) {
        recv_msg(sockfd, msg);

        if (!strcmp(msg, "TRN")) { //play
	        printf("Your move...\n");
	        take_turn(sockfd);
        }
        else if (!strcmp(msg, "INV")) { //ivde oru pravashyam vannathaaadey
            printf("That position has already been played. Try again.\n"); 
        }
        else if (!strcmp(msg, "CNT")) { //No of active players
            int num_players = recv_int(sockfd);
            printf("There are currently %d active players.\n", num_players); 
        }
        else if (!strcmp(msg, "UPD")) { //updation for gameboard
            get_update(sockfd, board);
            draw_board(board);
        }
        else if (!strcmp(msg, "WAT")) { //waiting for the other player to play
            printf("Waiting for other players move...\n");
        }
        else if (!strcmp(msg, "WIN")) { //Winner
            printf("You win!\n");
            break;
        }
        else if (!strcmp(msg, "LSE")) { //Loser
            printf("You lost.\n");
            break;
        }
        else if (!strcmp(msg, "DRW")) { //Draw
            printf("Draw.\n");
            break;
        }
        else //-_-
            error("Unknown message.");
    }
    
    printf("Game over.\n");

    close(sockfd);
    return 0;
}
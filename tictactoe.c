#define clrscr() system("clear ||cls");

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <pthread.h>

void raise_error(){
    perror("Something Went Wrong");
    exit(1);
}
// Define virtual screen in characters  - Aspect Ratio 1:2
int const GAME_SIZE         = 3;
int const SCREEN_WIDTH      = 72;
int const SCREEN_HEIGHT     = 36;
int const BLOCK_X           = SCREEN_WIDTH/GAME_SIZE;
int const BLOCK_Y           = SCREEN_HEIGHT/GAME_SIZE;

char* screen;

// Defined Points on the Grid

const int offsets[] = {
    0,
    BLOCK_X,
    2*BLOCK_X,
    BLOCK_Y + BLOCK_Y*SCREEN_WIDTH,
    BLOCK_Y + BLOCK_Y*SCREEN_WIDTH + BLOCK_X,
    BLOCK_Y + BLOCK_Y*SCREEN_WIDTH + 2*BLOCK_X,
    2*BLOCK_Y + 2*BLOCK_Y*SCREEN_WIDTH ,
    2*BLOCK_Y + 2*BLOCK_Y*SCREEN_WIDTH + BLOCK_X,
    2*BLOCK_Y + 2*BLOCK_Y*SCREEN_WIDTH + 2*BLOCK_X
};

// Input Threading
int character = '\0';
pthread_t inputThread;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
bool readFromInput = false;


char* jumpToBlockCentre(char* screen , int blockNumber){
    // Used to get to the correct height level which game block is on
    int heightJumper = (blockNumber)/GAME_SIZE;

    if(heightJumper <0) 
        raise_error();
        
    // Move to Top Left of block
    char* start = screen + ((blockNumber%3))*BLOCK_X +  heightJumper*BLOCK_Y*SCREEN_WIDTH + heightJumper*BLOCK_Y ;

    // Move Start to Centre of Block
    start = start + BLOCK_X/2 + BLOCK_Y/2*SCREEN_WIDTH + BLOCK_Y/2;
    return start;
}
/**
 * 0=<blockNumber<=(GAME_SIZE**2)-1
 */

void drawCircle(char* screen , int blockNumber){

        char* blockCentre = jumpToBlockCentre(screen,blockNumber);

        for( double theta=0 ; theta < 2*M_PI ; theta+= M_PI/6){
            double x = cos(theta);
            double y = sin(theta);
            *(blockCentre + (int) ((BLOCK_X/4)*x) + (int)(y*BLOCK_Y/4)*(SCREEN_WIDTH+ 1) ) = '.';
        }
        
        // // Draw Top Dot
        // *(start + BLOCK_X/2 + (BLOCK_Y/4 )*SCREEN_WIDTH + BLOCK_Y/4) = '.';

        // // Draw Bottom Dot
        // *(start + BLOCK_X/2 + (BLOCK_Y/2 + BLOCK_Y/3 )*SCREEN_WIDTH + BLOCK_Y/2 + BLOCK_Y/3) = '.';

        // Draw Right dot
        // *(start + (BLOCK_X/4 + BLOCK_X/8 - ) + (BLOCK_Y/4 -2 )*SCREEN_WIDTH + BLOCK_Y/4 -1 ) = 'h';
        // // Draw Left Dot
        // *(start + (BLOCK_X/2 - BLOCK_X/4) + (BLOCK_Y/2)*SCREEN_WIDTH + BLOCK_Y/2 ) = '.';
}


void drawCross(char* screen , int blockNumber){
    char* blockCentre = jumpToBlockCentre(screen, blockNumber);

    /**
     * Write This in a better way
     */
    for(int offset=0 ; offset < sqrt(SCREEN_HEIGHT) ; offset++){
        *(blockCentre - BLOCK_X/4 + offset  - (BLOCK_Y/4 - offset)*SCREEN_WIDTH + (BLOCK_Y/4 - offset) +3) = 'x';
        *(blockCentre - BLOCK_X/4 + offset  - (BLOCK_Y/4 - offset)*SCREEN_WIDTH + (BLOCK_Y/4 + offset) -1 ) = 'x';
    }
}

void fill(char* screen, size_t width , size_t height){
    for(int i =1; i <= height ;i++){

        if( (i-1) % BLOCK_Y ==0){
            for( int j =0; j < width;j++){
                // NB: New Lines move the blocks of characters for a row i times 
                *(screen + width*(i-1) + j +i) = '-';
            }
        }
        // Insert new line characters
        *(screen + width*i + i-1) = '\n';
        for(int j =0; j < 3; j++){
            // NB : i-1 is added sinc we offset the rows by i-1(including 0) everytime when we insert newline
            *( screen + width*(i-1)  + (BLOCK_X)*j + (i-1) ) = '|';
        }
    }
}


void cleanup(char* screen){
    if( screen ){
        free(screen);
    }else{
        
        raise_error();
    }
    return;
}

// show Current State of game
void show(char* screen, size_t width , size_t height){
    // Refresh Screen
    clrscr();

    for(int i =0; i < width*height + height ; i++){
        printf("%c",*(screen+i));
    }
}

// Input and Game Logic

/**
 * Block Metadata
 */
typedef struct __blockData blockData;
struct __blockData{
    bool filled;
    char symbol;
};

// 
typedef struct __gameState gameState;
struct  __gameState
{
    bool playing;
    bool turn;
    blockData state[9];
    int currentBlock;
};


void* getInput(void* arg){
    pthread_mutex_lock(&lock);
    if( read(STDIN_FILENO,&character,1 ))
        readFromInput = true;
    pthread_cond_signal(&condition);
    pthread_mutex_unlock(&lock);
}

void getMove(int *block , bool turn , bool player1Move){
    readFromInput = false;
    pthread_create(&inputThread,NULL, getInput, NULL);

    pthread_mutex_lock(&lock);
    while( !readFromInput)
        pthread_cond_wait(&condition,&lock);
    pthread_mutex_unlock(&lock);
    switch (character)
    {
    case 'w':
        *block = abs(*block - GAME_SIZE );
        break;
    case 'a':
        *block = abs(*block -1);
        break;
    case 's':
        *block = (*block + GAME_SIZE)%9;
        break;
    case 'd':
        *block = (*block + 1)%9 ;
        break;
    default:
        break;
    }
}

// Draw in symbol for the current player's turn with the symbol determined by player1Move
void drawMove( gameState* state , bool player1Move){


    if( state->turn ){
        if ( player1Move){
            drawCircle(screen, state->currentBlock);
            state->state[state->currentBlock].symbol = 'O';
        }
        else{
            drawCross(screen , state->currentBlock);
            state->state[state->currentBlock].symbol = 'X';
        }

        
    }else{
        if ( player1Move){
            drawCross(screen, state->currentBlock);
            state->state[state->currentBlock].symbol = 'X';
        }
        else{
            drawCircle(screen , state->currentBlock);
            state->state[state->currentBlock].symbol = 'O';
        }
            
    }

    state->state[state->currentBlock].filled = true;

}

// Check Game state and Determine when there is a winner
bool decide(gameState *state){

}

void init(gameState* state){
    // initialize Screen Memory
    screen = (char*) malloc(sizeof(char) *( (SCREEN_HEIGHT*SCREEN_WIDTH + SCREEN_HEIGHT)) ); // Store new line characters also in the buffer 
    if(!screen){
        raise_error();
    }
    // INITIALIZE SCREEN BUFFER
    memset(screen,' ',SCREEN_HEIGHT*SCREEN_WIDTH + SCREEN_HEIGHT);

    // fill buffer with initial contents
    fill(screen, SCREEN_WIDTH,SCREEN_HEIGHT);

    // Set Game State
    memset(state,0,sizeof(gameState));
    // Start Game
    state->playing = true;
    return;
}

int main(int argc , char* argv[]){
    /**TODO:
     * Get Width as an optional argument -> Aspect ratio is 1:2 so height = 1/2 * (width)
     */

    // Game Loop
    {
        bool exit = false;
        int choice = '1';

        // Get Player 1 choice 
        printf("Please choose player 1 symbol : press 1 for O , Press 2 for X .\n\n\n");
        choice = getchar();
        while(!exit){
            readFromInput = false;
            bool player1 = false;// By Default Player 1 is O
            // get player 1 and 2 choices. Player 1 always starts
            // Parent Thread waits for input Sleep
            
            if(choice == '1' ){

                printf("You have chosen O. Press 3 to continue, 2 to change option or escape to exit. \n"); 
                player1 = false;
            }else if(choice == '2' ){
                printf("You have chosen X. Press 3 to continue ,1 to change option or escape to exit.\n"); 
                player1 = true;
            }else if(choice == '3'){
                    // Start game
                    gameState state;// Declare state variable
                    // Virtual Screen Initialization
                    printf("Starting New Game..\n\n");
                    init(&state);
                    show(screen,SCREEN_WIDTH,SCREEN_HEIGHT);
                    while(state.playing){
                            
                            getMove(&state.currentBlock, &state.turn , player1);
                            printf("\nCursor is at block : %d\n",state.currentBlock);
                            // Confirmation Block    
                            {
                                bool confirmed = false;
                                printf("You have selected to draw in block %d.\n",state.currentBlock + 1);
                                printf("Press 1 to confirm,2 to change block or esc to end game.\n");

                                int decision = getchar();// blocking and is on same thread
                                switch (decision)
                                {
                                case '1':
                                    confirmed = true;
                                    break;
                                case 27:
                                    state.playing = false;
                                    break;
                                default:
                                    confirmed = false;
                                    break;
                                }
                                
                                if( !confirmed || !state.playing )
                                    continue;
                            }

                            // Validation
                            if (state.state[state.currentBlock].filled == true) {
                                printf("Block %d is already filled, choose another block to play your turn \n", state.currentBlock);
                                continue;
                            }

                            drawMove( &state , player1);
                            state.turn = !state.turn;
                            show(screen,SCREEN_WIDTH,SCREEN_HEIGHT);
                            // playing = decide();
                    }
                    printf("Ending Current Game Session..\n");
                    sleep(1);
                    choice = '1';
                    clrscr();
                    continue;
            }else if(choice == 27){
                exit = true;
                break;
            }   

            pthread_create(&inputThread,NULL, getInput,NULL);

            pthread_mutex_lock(&lock);
            while(!readFromInput){
                pthread_cond_wait(&condition,&lock);
            }
            if(character != '\0')
                choice = character;
            else
                choice = '1';
            pthread_mutex_unlock(&lock);     
            pthread_join(inputThread,NULL);
        }
    }
    cleanup(screen);
    return 0;
}
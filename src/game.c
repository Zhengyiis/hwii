#include "game.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "snake_utils.h"

/* Helper function definitions */
static void set_board_at(game_t *game, unsigned int row, unsigned int col, char ch);
static bool is_tail(char c);
static bool is_head(char c);
static bool is_snake(char c);
static char body_to_tail(char c);
static char head_to_body(char c);
static unsigned int get_next_row(unsigned int cur_row, char c);
static unsigned int get_next_col(unsigned int cur_col, char c);
static void find_head(game_t *game, unsigned int snum);
static char next_square(game_t *game, unsigned int snum);
static void update_tail(game_t *game, unsigned int snum);
static void update_head(game_t *game, unsigned int snum);

/* Task 1 */
game_t *create_default_game() {
  // Allocate memory for the game struct
  game_t *game = malloc(sizeof(game_t));
  if (game == NULL) {
    return NULL;
  }
  
  // Initialize game properties
  game->num_rows = 18;
  game->num_snakes = 1;
  
  // Allocate memory for the board (array of char pointers)
  game->board = malloc(18 * sizeof(char *));
  if (game->board == NULL) {
    free(game);
    return NULL;
  }
  
  // Allocate memory for each row (20 chars + newline + null terminator = 22 chars)
  for (unsigned int row = 0; row < 18; row++) {
    game->board[row] = malloc(22 * sizeof(char));
    if (game->board[row] == NULL) {
      // Free previously allocated rows
      for (unsigned int i = 0; i < row; i++) {
        free(game->board[i]);
      }
      free(game->board);
      free(game);
      return NULL;
    }
  }
  
  // Initialize the board
  for (unsigned int row = 0; row < 18; row++) {
    for (unsigned int col = 0; col < 20; col++) {
      if (row == 0 || row == 17 || col == 0 || col == 19) {
        // Walls on the border
        game->board[row][col] = '#';
      } else if (row == 2 && col == 2) {
        // Snake tail
        game->board[row][col] = 'd';
      } else if (row == 2 && col == 3) {
        // Snake body
        game->board[row][col] = '>';
      } else if (row == 2 && col == 4) {
        // Snake head
        game->board[row][col] = 'D';
      } else if (row == 2 && col == 9) {
        // Fruit
        game->board[row][col] = '*';
      } else {
        // Empty space
        game->board[row][col] = ' ';
      }
    }
    // Add newline and null terminator
    game->board[row][20] = '\n';
    game->board[row][21] = '\0';
  }
  
  // Allocate and initialize snakes array
  game->snakes = malloc(1 * sizeof(snake_t));
  if (game->snakes == NULL) {
    // Free all previously allocated memory
    for (unsigned int i = 0; i < 18; i++) {
      free(game->board[i]);
    }
    free(game->board);
    free(game);
    return NULL;
  }
  
  // Initialize the snake
  game->snakes[0].tail_row = 2;
  game->snakes[0].tail_col = 2;
  game->snakes[0].head_row = 2;
  game->snakes[0].head_col = 4;
  game->snakes[0].live = true;
  
  return game;
}

/* Task 2 */
void free_game(game_t *game) {
  // TODO: Implement this function.
  return;
}

/* Task 3 */
void print_board(game_t *game, FILE *fp) {
  // TODO: Implement this function.
  return;
}

/*
  Saves the current game into filename. Does not modify the game object.
  (already implemented for you).
*/
void save_board(game_t *game, char *filename) {
  FILE *f = fopen(filename, "w");
  print_board(game, f);
  fclose(f);
}

/* Task 4.1 */

/*
  Helper function to get a character from the board
  (already implemented for you).
*/
char get_board_at(game_t *game, unsigned int row, unsigned int col) { return game->board[row][col]; }

/*
  Helper function to set a character on the board
  (already implemented for you).
*/
static void set_board_at(game_t *game, unsigned int row, unsigned int col, char ch) {
  game->board[row][col] = ch;
}

/*
  Returns true if c is part of the snake's tail.
  The snake consists of these characters: "wasd"
  Returns false otherwise.
*/
static bool is_tail(char c) {
  // TODO: Implement this function.
  return true;
}

/*
  Returns true if c is part of the snake's head.
  The snake consists of these characters: "WASDx"
  Returns false otherwise.
*/
static bool is_head(char c) {
  // TODO: Implement this function.
  return true;
}

/*
  Returns true if c is part of the snake.
  The snake consists of these characters: "wasd^<v>WASDx"
*/
static bool is_snake(char c) {
  // TODO: Implement this function.
  return true;
}

/*
  Converts a character in the snake's body ("^<v>")
  to the matching character representing the snake's
  tail ("wasd").
*/
static char body_to_tail(char c) {
  // TODO: Implement this function.
  return '?';
}

/*
  Converts a character in the snake's head ("WASD")
  to the matching character representing the snake's
  body ("^<v>").
*/
static char head_to_body(char c) {
  // TODO: Implement this function.
  return '?';
}

/*
  Returns cur_row + 1 if c is 'v' or 's' or 'S'.
  Returns cur_row - 1 if c is '^' or 'w' or 'W'.
  Returns cur_row otherwise.
*/
static unsigned int get_next_row(unsigned int cur_row, char c) {
  // TODO: Implement this function.
  return cur_row;
}

/*
  Returns cur_col + 1 if c is '>' or 'd' or 'D'.
  Returns cur_col - 1 if c is '<' or 'a' or 'A'.
  Returns cur_col otherwise.
*/
static unsigned int get_next_col(unsigned int cur_col, char c) {
  // TODO: Implement this function.
  return cur_col;
}

/*
  Task 4.2

  Helper function for update_game. Return the character in the cell the snake is moving into.

  This function should not modify anything.
*/
static char next_square(game_t *game, unsigned int snum) {
  // TODO: Implement this function.
  return '?';
}

/*
  Task 4.3

  Helper function for update_game. Update the head...

  ...on the board: add a character where the snake is moving

  ...in the snake struct: update the row and col of the head

  Note that this function ignores food, walls, and snake bodies when moving the head.
*/
static void update_head(game_t *game, unsigned int snum) {
  // TODO: Implement this function.
  return;
}

/*
  Task 4.4

  Helper function for update_game. Update the tail...

  ...on the board: blank out the current tail, and change the new
  tail from a body character (^<v>) into a tail character (wasd)

  ...in the snake struct: update the row and col of the tail
*/
static void update_tail(game_t *game, unsigned int snum) {
  // TODO: Implement this function.
  return;
}

/* Task 4.5 */
void update_game(game_t *game, int (*add_food)(game_t *game)) {
  // TODO: Implement this function.
  return;
}

/* Task 5.1 */
char *read_line(FILE *fp) {
  // TODO: Implement this function.
  return NULL;
}

/* Task 5.2 */
game_t *load_board(FILE *fp) {
  // TODO: Implement this function.
  return NULL;
}

/*
  Task 6.1

  Helper function for initialize_snakes.
  Given a snake struct with the tail row and col filled in,
  trace through the board to find the head row and col, and
  fill in the head row and col in the struct.
*/
static void find_head(game_t *game, unsigned int snum) {
  // TODO: Implement this function.
  return;
}

/* Task 6.2 */
game_t *initialize_snakes(game_t *game) {
  // TODO: Implement this function.
  return NULL;
}

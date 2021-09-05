//-----------------------------LICENSE NOTICE------------------------------------
//  This file is part of CPCtelera: An Amstrad CPC Game Engine
//  Copyright (C) 2015 ronaldo / Fremos / Cheesetea / ByteRealms (@FranGallegoBR)
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//------------------------------------------------------------------------------

// #include <cpctelera.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "asm/conio.h"
#include "asm/amsgraph.h"

struct Point
{
   uint8_t x;
   uint8_t y;
};

const struct Point glider[] = {{2,0}, {0,1}, {2,1}, {1,2}, {2,2}};

const struct Point gospel[] = {
  {2,6},{3,6},{2,7},{3,7},
  {12,6},{12,7},{12,8},{13,5},{13,9},{14,4},{14,10},{15,4},{15,10},{16,7},{17,5},{17,9},{18,6},{18,7},{18,8},{19,7},
  {22,4},{22,5},{22,6},{23,4},{23,5},{23,6},{24,3},{24,7},{26,2},{26,3},{26,7},{26,8},
  {36,4},{36,5},{37,4},{37,5}
};

// For mode 1 text
#define X_MIN 1
#define X_MAX 40
#define Y_MIN 1
#define Y_MAX 25

//New board stuff
#define BUF_WIDTH 0x0010 //16 addresses
#define BUF_WIDTH_MSK BUF_WIDTH-1
#define GET_CELL(board, p) board + (BUF_WIDTH*(p.y-1)) + ((p.x-1)/4)

#define BD_DATA_W X_MAX/4  //The buffer row width is 16, but only 10 bits are used
#define BD_H Y_MAX
#define BD_BUFSIZ BUF_WIDTH*BD_H*2+BUF_WIDTH_MSK //Enough for 2 boards to align to a 64bit boundary

uint8_t boardbuffer[BD_BUFSIZ];

#define IS_ALIVE(x) (x & (uint8_t)2)
#define IS_NOT_ALIVE(x) !(x & (uint8_t)2)

#define IS_ALIVE_0(x) x & (uint8_t)2
#define IS_NOT_ALIVE_0(x) !(x & (uint8_t)2)

#define IS_ALIVE_1(x) x & (uint8_t)8
#define IS_NOT_ALIVE_1(x) !(x & (uint8_t)8)

#define IS_ALIVE_2(x) x & (uint8_t)32
#define IS_NOT_ALIVE_2(x) !(x & (uint8_t)32)

#define IS_ALIVE_3(x) x & (uint8_t)128
#define IS_NOT_ALIVE_3(x) !(x & (uint8_t)128)

#define ASCII_0 48
#define ASCII_a 32

#define DRAW 1

void debug_str_uint8(char *s, uint8_t i) {
   cputs(s);
   cputc(i+ASCII_0);
   cputs("\r\n");
}

void debug_board(uint8_t * board) {
   uint8_t *cell;
   for (uint8_t y = 0; y < BD_H; y++) {
      printf("%02d:", y);
      for(uint8_t x = 0; x < BD_DATA_W; x++) {
         cell = board + (BUF_WIDTH*y) + x;
         if(*cell) inverse();
         printf(" %02x", *cell);
         if(*cell) inverse();
      }
      cputs("\r\n");
   }
}

void update_cellgp(uint8_t *cellgp, struct Point *p, uint8_t shift, bool set) {

   *cellgp = *cellgp | (1 << shift); //Set dirty flag
   if(set) {
      *cellgp = *cellgp | (2 << shift); //Set alive
      #if DRAW
         cputcxy(p->x, p->y, 0xCE);
      #endif
   }
   else {
      *cellgp = *cellgp & ~(2 << shift); //Set not alive &= ~(1UL << n);
      #if DRAW
         cputcxy(p->x, p->y, ' ');
      #endif
   }
}

void put_on_board(uint8_t * board, struct Point *shape, uint8_t len, uint8_t ox, uint8_t oy) {
   uint8_t *cell, shift;
   struct Point p;
   for(uint8_t i = 0; i < len; i++) {
      p.x = ox+shape[i].x;
      p.y = oy+shape[i].y;

      cell = GET_CELL(board, p);
      shift = ((p.x-1)%4)*2;
      update_cellgp(cell, &p, shift, true);
   }
}

#define UL(nbr, p) nbr.x = p->x-1; nbr.y = p->y-1
#define UC(nbr, p) nbr.x = p->x;   nbr.y = p->y-1
#define UR(nbr, p) nbr.x = p->x+1; nbr.y = p->y-1

#define CL(nbr, p) nbr.x = p->x-1; nbr.y = p->y
#define CR(nbr, p) nbr.x = p->x+1; nbr.y = p->y

#define LL(nbr, p) nbr.x = p->x-1; nbr.y = p->y+1
#define LC(nbr, p) nbr.x = p->x;   nbr.y = p->y+1
#define LR(nbr, p) nbr.x = p->x+1; nbr.y = p->y+1

#define SH_DN(s) (s==0 ? 6 : s-2)
#define SH_UP(s) (s==6 ? 0 : s+2)

uint8_t live_neigbs_cnt(uint8_t *board, struct Point *p, uint8_t shift) {

   uint8_t c = 0, *cell, start = 0, end = 8;
   struct Point nbr;

   uint8_t sh_up = SH_UP(shift);
   uint8_t sh_dn = SH_DN(shift);

   if(p->y == Y_MIN) {
      LC(nbr, p);
      cell = GET_CELL(board, nbr);
      if(IS_ALIVE(*cell >> shift)) c++;

      if(p->x == X_MIN) {
         CR(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_up)) c++;

         LR(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_up)) c++;
      }

      if(p->x > X_MIN && p->x < X_MAX) {
         CL(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_dn)) c++;

         LL(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_dn)) c++;

         CR(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_up)) c++;

         LR(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_up)) c++;
      }

      if(p->x == X_MAX) {
         CL(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_dn)) c++;

         LL(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_dn)) c++;
      }
   }
   
   if(p->y > Y_MIN && p->y < Y_MAX) {
      UC(nbr, p);
      cell = GET_CELL(board, nbr);
      if(IS_ALIVE(*cell >> shift)) c++;
      
      LC(nbr, p);
      cell = GET_CELL(board, nbr);
      if(IS_ALIVE(*cell >> shift)) c++;

      if(p->x == X_MIN) {
         UR(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_up)) c++;

         CR(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_up)) c++;

         LR(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_up)) c++;
      }
      
      if(p->x > X_MIN && p->x < X_MAX) {
         UR(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_up)) c++;

         CR(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_up)) c++;

         LR(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_up)) c++;

         UL(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_dn)) c++;

         CL(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_dn)) c++;

         LL(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_dn)) c++;
      }
      
      if(p->x == X_MAX) {
         UL(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_dn)) c++;

         CL(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_dn)) c++;

         LL(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_dn)) c++;
      }
   }

   if(p->y == Y_MAX) {
      UC(nbr, p);
      cell = GET_CELL(board, nbr);
      if(IS_ALIVE(*cell >> shift)) c++;
 
      if(p->x == X_MIN) {
         UR(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_up)) c++;

         CR(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_up)) c++;
      }

      if(p->x > X_MIN && p->x < X_MAX) {
         UR(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_up)) c++;

         CR(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_up)) c++;

         UL(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_dn)) c++;

         CL(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_dn)) c++;
      }

      if(p->x == X_MAX) {
         UL(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_dn)) c++;

         CL(nbr, p);
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> sh_dn)) c++;
      }
   }

   // if(c) {
   //    nbr.x = p->x; nbr.y = p->y;
   //    cell = GET_CELL(board, nbr);
   //    if(IS_ALIVE(*cell >> shift)) inverse();
   //    cputcxy(p->x, p->y, c+ASCII_0);
   //    if(IS_ALIVE(*cell >> shift)) inverse();
   // }

   return c;
}

void evolve(uint8_t *src_board, uint8_t *dst_board) {
   struct Point p = {0,0};
   uint8_t *dst_cell, *src_cell, c;

   memcpy(dst_board, src_board, BUF_WIDTH*BD_H);

   for (p.y = Y_MIN; p.y <= Y_MAX; p.y++) {
      for(p.x = X_MIN; p.x <= X_MAX; ) {

         src_cell = GET_CELL(src_board, p);
         dst_cell = GET_CELL(dst_board, p);

         //Evolve cell 0
         c = live_neigbs_cnt(src_board, &p, 0);
         if(c == 3 && IS_NOT_ALIVE_0(*src_cell))
            update_cellgp(dst_cell, &p, 0, true);
         else if (c != 2 && c != 3 && IS_ALIVE_0(*src_cell))
            update_cellgp(dst_cell, &p, 0, false);
         p.x++;

         //Evolve cell 1
         c = live_neigbs_cnt(src_board, &p, 2);
         if(c == 3 && IS_NOT_ALIVE_1(*src_cell))
            update_cellgp(dst_cell, &p, 2, true);
         else if (c != 2 && c != 3 && IS_ALIVE_1(*src_cell))
            update_cellgp(dst_cell, &p, 2, false);
         p.x++;

         //Evolve cell 2
         c = live_neigbs_cnt(src_board, &p, 4);
         if(c == 3 && IS_NOT_ALIVE_2(*src_cell))
            update_cellgp(dst_cell, &p, 4, true);
         else if (c != 2 && c != 3 && IS_ALIVE_2(*src_cell))
            update_cellgp(dst_cell, &p, 4, false);
         p.x++;

         //Evolve cell 3
         c = live_neigbs_cnt(src_board, &p, 6);
         if(c == 3 && IS_NOT_ALIVE_3(*src_cell))
            update_cellgp(dst_cell, &p, 6, true);
         else if (c != 2 && c != 3 && IS_ALIVE_3(*src_cell))
            update_cellgp(dst_cell, &p, 6, false);
         p.x++;
         
      }
   }
}

void main(void) {

   struct Point square[] = {{0,0}};
   struct Point p = {5,5};
   uint8_t *board1, *board2;

   bordercolor(0);

   //Clear buffer then align 2 boards to next 64bit boundary
   memset(&boardbuffer, 0, (BUF_WIDTH*BD_H*2)+BUF_WIDTH_MSK);
   board1 = (uint8_t*)(((uint16_t)&boardbuffer+BUF_WIDTH_MSK) & ~(BUF_WIDTH_MSK));
   board2 = board1 + BUF_WIDTH*BD_H;
   
   // put_on_board(board1, glider, 5, 3, 3);
   // put_on_board(board1, square, 1, 5, 5);
   put_on_board(board1, gospel, 36, 1, 1);

   while(true) {
      evolve(board1, board2);   
      evolve(board2, board1);   
   }
}


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
#include "asm/vidmem.h"
#include "asm/life.h"

struct Point
{
   uint8_t x;
   uint8_t y;
};

const struct Point square[] = {{0,0}};

const struct Point glider[] = {{2,0}, {0,1}, {2,1}, {1,2}, {2,2}};

const struct Point r_pentomino[] = {{1,0}, {2,0}, {0,1}, {1,1}, {1,2}};

const struct Point gosper[] = {
  {0,6},{1,6},{0,7},{1,7},
  {10,6},{10,7},{10,8},{11,5},{11,9},{12,4},{12,10},{13,4},{13,10},{14,7},{15,5},{15,9},{16,6},{16,7},{16,8},{17,7},
  {20,4},{20,5},{20,6},{21,4},{21,5},{21,6},{22,3},{22,7},{24,2},{24,3},{24,7},{24,8},
  {34,4},{34,5},{35,4},{35,5}
};

// For mode 1 text
#define X_MIN 1
#define X_MAX 80
#define Y_MIN 1
#define Y_MAX 50

//New board stuff
#define BUF_WIDTH 16 //16 addresses
#define BUF_WIDTH_MSK 0x000F
#define GET_CELL(board, p) board + (BUF_WIDTH*(p.y)) + (((p.x-1)/8)+1)

#define BD_DATA_W (X_MAX/8)  //The buffer row width is 16, but only 10 bits are used
#define BD_H (Y_MAX+2)  //The number of rows in the buffer has 2 extra for the edge's 0ed neighbours
#define BD_BUFSIZ (BUF_WIDTH*BD_H*2+BUF_WIDTH_MSK) //Enough for 2 boards to align to a 64bit boundary

uint8_t boardbuffer[BD_BUFSIZ];

#define IS_ALIVE(x, bit) (x & (uint8_t)bit)
#define IS_NOT_ALIVE(x, bit) !IS_ALIVE(x, bit)


#define IS_ALIVE_0(x) IS_ALIVE(x, 1)
#define IS_NOT_ALIVE_0(x) IS_NOT_ALIVE(x, 1)

#define IS_ALIVE_1(x) IS_ALIVE(x, 2)
#define IS_NOT_ALIVE_1(x) IS_NOT_ALIVE(x, 2)

#define IS_ALIVE_2(x) IS_ALIVE(x, 4)
#define IS_NOT_ALIVE_2(x) IS_NOT_ALIVE(x, 4)

#define IS_ALIVE_3(x) IS_ALIVE(x, 8)
#define IS_NOT_ALIVE_3(x) IS_NOT_ALIVE(x, 8)

#define IS_ALIVE_4(x) IS_ALIVE(x, 16)
#define IS_NOT_ALIVE_4(x) IS_NOT_ALIVE(x, 16)

#define IS_ALIVE_5(x) IS_ALIVE(x, 32)
#define IS_NOT_ALIVE_5(x) IS_NOT_ALIVE(x, 32)

#define IS_ALIVE_6(x) IS_ALIVE(x, 64)
#define IS_NOT_ALIVE_6(x) IS_NOT_ALIVE(x, 64)

#define IS_ALIVE_7(x) IS_ALIVE(x, 128)
#define IS_NOT_ALIVE_7(x) IS_NOT_ALIVE(x, 128)

#define ASCII_0 48
#define ASCII_a 32

#define DRAW 1
#define SHOW_NBRCNT 0

void debug_str_uint8(char *s, uint8_t i) {
   cputs(s);
   cputc(i+ASCII_0);
   cputs("\r\n");
}

void debug_board(uint8_t * board) {
   uint8_t *cell;
   gotoxy(1,1);
   for (uint8_t y = 0; y < BD_H; y++) {
      printf("%02d:", y);
      for(uint8_t x = 0; x < BD_DATA_W+2; x++) {
         cell = board + (BUF_WIDTH*y) + x;
         if(*cell) inverse();
         printf(" %02x", *cell);
         if(*cell) inverse();
      }
      cputs("\r\n");
   }
}

#define BLKSIZ 8

void update_cellgp(uint8_t *cellgp, struct Point *p, uint8_t shift, bool set) {

   uint8_t x = p->x-1;
   uint8_t y = p->y-1;

   if(set) {
      *cellgp = *cellgp | (1 << shift); //Set alive
      #if DRAW
         putblock(x, y);
      #endif
   }
   else {
      *cellgp = *cellgp & ~(1 << shift); //Set not alive &= ~(1UL << n);
      #if DRAW
         clrblock(x, y);
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
      shift = (p.x-1)%8;
      update_cellgp(cell, &p, shift, true);
   }
}
#define SH_DN(s) (s==0 ? 7 : s-1)
#define SH_UP(s) (s==7 ? 0 : s+1)

uint8_t live_neigbs_cnt(uint8_t *thiscell, uint8_t shift, bool v) {

   uint8_t c = 0, *cell;

   uint8_t sh_up = SH_UP(shift);
   uint8_t sh_dn = SH_DN(shift);

   //Center left
   cell = thiscell;
   if(shift==0) cell--;
   if(IS_ALIVE_0(*cell >> sh_dn)) c++;

   //Upper left
   cell = cell - BUF_WIDTH;
   if(IS_ALIVE_0(*cell >> sh_dn)) c++;

   //Lower left
   cell = cell + (BUF_WIDTH*2);
   if(IS_ALIVE_0(*cell >> sh_dn)) c++;


   //Center right
   cell = thiscell;
   if(shift==7) cell++;
   if(IS_ALIVE_0(*cell >> sh_up)) c++;

   //Upper right
   cell = cell - BUF_WIDTH;
   if(IS_ALIVE_0(*cell >> sh_up)) c++;

   //Lower right
   cell = cell + (BUF_WIDTH*2);
   if(IS_ALIVE_0(*cell >> sh_up)) c++;


   //Upper center
   cell = thiscell - BUF_WIDTH;
   if(IS_ALIVE_0(*cell >> shift)) c++;

   //Lower center
   cell = thiscell + BUF_WIDTH;
   if(IS_ALIVE_0(*cell >> shift)) c++;


   
   #if SHOW_NBRCNT
      if(v && c) {
         nbr.x = p->x; nbr.y = p->y;
         cell = GET_CELL(board, nbr);
         if(IS_ALIVE(*cell >> shift)) inverse();
         cputcxy(p->x, p->y, c+ASCII_0);
         if(IS_ALIVE(*cell >> shift)) inverse();
      }
   #endif

   return c;
}

void evolve(uint8_t *src_board, uint8_t *dst_board) {
   struct Point p = {0,0};
   uint8_t *dst_cell, *src_cell, c;

   memcpy(dst_board, src_board, BUF_WIDTH*BD_H);

   //Skip past 0 filled margin row 0
   src_cell = src_board + BUF_WIDTH;
   dst_cell = dst_board + BUF_WIDTH;

   for (p.y = Y_MIN; p.y <= Y_MAX; p.y++) {

      // gotoxy(1,1);
      // printf("evolving row %d ", p.y);

      // for(j = 0; j < 192; j++)
      //    for(k = 0; k < 255; k++);
      
      //Skip past 0 filled margin column 0
      src_cell++;
      dst_cell++;

      for(p.x = X_MIN; p.x <= X_MAX; ) {

         //Evolve cell 0
         c = live_neigbs_cnt(src_cell, 0, false);
         if(c == 3 && IS_NOT_ALIVE_0(*src_cell))
            update_cellgp(dst_cell, &p, 0, true);
         else if (c != 2 && c != 3 && IS_ALIVE_0(*src_cell))
            update_cellgp(dst_cell, &p, 0, false);
         p.x++;

         //Evolve cell 1
         c = live_neigbs_cnt(src_cell, 1, false);
         if(c == 3 && IS_NOT_ALIVE_1(*src_cell))
            update_cellgp(dst_cell, &p, 1, true);
         else if (c != 2 && c != 3 && IS_ALIVE_1(*src_cell))
            update_cellgp(dst_cell, &p, 1, false);
         p.x++;

         //Evolve cell 2
         c = live_neigbs_cnt(src_cell, 2, false);
         if(c == 3 && IS_NOT_ALIVE_2(*src_cell))
            update_cellgp(dst_cell, &p, 2, true);
         else if (c != 2 && c != 3 && IS_ALIVE_2(*src_cell))
            update_cellgp(dst_cell, &p, 2, false);
         p.x++;

         //Evolve cell 3
         c = live_neigbs_cnt(src_cell, 3, false);
         if(c == 3 && IS_NOT_ALIVE_3(*src_cell))
            update_cellgp(dst_cell, &p, 3, true);
         else if (c != 2 && c != 3 && IS_ALIVE_3(*src_cell))
            update_cellgp(dst_cell, &p, 3, false);
         p.x++;

         //Evolve cell 4
         c = live_neigbs_cnt(src_cell, 4, false);
         if(c == 3 && IS_NOT_ALIVE_4(*src_cell))
            update_cellgp(dst_cell, &p, 4, true);
         else if (c != 2 && c != 3 && IS_ALIVE_4(*src_cell))
            update_cellgp(dst_cell, &p, 4, false);
         p.x++;

         //Evolve cell 5
         c = live_neigbs_cnt(src_cell, 5, false);
         if(c == 3 && IS_NOT_ALIVE_5(*src_cell))
            update_cellgp(dst_cell, &p, 5, true);
         else if (c != 2 && c != 3 && IS_ALIVE_5(*src_cell))
            update_cellgp(dst_cell, &p, 5, false);
         p.x++;

         //Evolve cell 6
         c = live_neigbs_cnt(src_cell, 6, false);
         if(c == 3 && IS_NOT_ALIVE_6(*src_cell))
            update_cellgp(dst_cell, &p, 6, true);
         else if (c != 2 && c != 3 && IS_ALIVE_6(*src_cell))
            update_cellgp(dst_cell, &p, 6, false);
         p.x++;

         //Evolve cell 7
         c = live_neigbs_cnt(src_cell, 7, false);
         if(c == 3 && IS_NOT_ALIVE_7(*src_cell))
            update_cellgp(dst_cell, &p, 7, true);
         else if (c != 2 && c != 3 && IS_ALIVE_7(*src_cell))
            update_cellgp(dst_cell, &p, 7, false);
         p.x++;

         src_cell++;
         dst_cell++;
      }

      src_cell += (BUF_WIDTH-BD_DATA_W-1);
      dst_cell += (BUF_WIDTH-BD_DATA_W-1);

      // for(j = 0; j < 192; j++)
      //    for(k = 0; k < 255; k++);
   }
}

void main(void) {

   uint8_t *board1, *board2, *cell, r;
   struct Point p = {0,0};
   
   bordercolor(0);
   // bordercolor(9);
   setink(0, 0, 0);  //Paper
   setink(1, 21, 21);  //Pen

   move(0, 0);
   draw(0, (Y_MAX*BLKSIZ)-1);
   draw((X_MAX*BLKSIZ)-1, (Y_MAX*BLKSIZ)-1);  
   draw((X_MAX*BLKSIZ)-1, 0);  
   draw(0, 0);  

   //Clear buffer then align 2 boards to next 64bit boundary
   memset(&boardbuffer, 0, (BUF_WIDTH*BD_H*2)+BUF_WIDTH_MSK);
   board1 = (uint8_t*)(((uint16_t)&boardbuffer+BUF_WIDTH_MSK) & ~(BUF_WIDTH_MSK));
   board2 = board1 + (BUF_WIDTH*BD_H);

   //1,1 IS THE MINIMUM
   // put_on_board(board1, glider, 5, 3, 3);
   //put_on_board(board1, square, 1, 2, 1);
   //put_on_board(board1, gosper, 36, 1, 1);
   put_on_board(board1, r_pentomino, 5, 47, 20);


   
   // gotoxy(1, 10);
   // printf("Board at %04x\r\n", board1);

   // cell = board1 + BUF_WIDTH;
   // printf("Cell at %04x\r\n", cell);

   // r = asmevolve(cell+1);

   // printf("Cell was %x\r\n", r);

   // while(true);

   while(true) {
      evolve(board1, board2);
      evolve(board2, board1);
   }

   // debug_board(board1);

   // while(true) 
   //    if(rdkey()==' ') 
   //       break;


   // for (p.y = Y_MIN; p.y <= Y_MAX; p.y++) {
   //    for(p.x = X_MIN; p.x <= X_MAX; ) {
   //       live_neigbs_cnt(board1, &p, 0, true);
   //       p.x++;
   //       live_neigbs_cnt(board1, &p, 2, true);
   //       p.x++;
   //       live_neigbs_cnt(board1, &p, 4, true);
   //       p.x++;
   //       live_neigbs_cnt(board1, &p, 6, true);
   //       p.x++;
   //    }
   // }
}
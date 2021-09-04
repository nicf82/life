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


#define BD_W X_MAX/4
#define BD_H Y_MAX

uint8_t board1[BD_W][BD_H];
uint8_t board2[BD_W][BD_H];

#define SCR_TO_BD(x) x-1

#define IS_DIRTY(x) x & (uint8_t)1
#define IS_ALIVE(x) (x & (uint8_t)2)
#define IS_NOT_ALIVE(x) !(x & (uint8_t)2)

#define IS_DIRTY_0(x) x & (uint8_t)1
#define IS_ALIVE_0(x) x & (uint8_t)2
#define IS_NOT_ALIVE_0(x) !(x & (uint8_t)2)

#define IS_DIRTY_1(x) x & (uint8_t)4
#define IS_ALIVE_1(x) x & (uint8_t)8
#define IS_NOT_ALIVE_1(x) !(x & (uint8_t)8)

#define IS_DIRTY_2(x) x & (uint8_t)16
#define IS_ALIVE_2(x) x & (uint8_t)32
#define IS_NOT_ALIVE_2(x) !(x & (uint8_t)32)

#define IS_DIRTY_3(x) x & (uint8_t)64
#define IS_ALIVE_3(x) x & (uint8_t)128
#define IS_NOT_ALIVE_3(x) !(x & (uint8_t)128)

#define ASCII_0 48
#define ASCII_a 32

void debug_str_uint8(char *s, uint8_t i) {
   cputs(s);
   cputc(i+ASCII_0);
   cputs("\r\n");
}

void debug_board(uint8_t board[BD_W][BD_H]) {
   for (uint8_t y = 0; y < BD_H; y++) {
      printf("%02d:", y);
      for(uint8_t x = 0; x < BD_W; x++) {
         if(board[x][y]) inverse();
         printf(" %x", board[x][y]);
         if(board[x][y]) inverse();
      }
      cputs("\r\n");
   }
}

//Note - this broke when i made it inline
uint8_t * point_to_board_cellgp_ref(uint8_t board[BD_W][BD_H], struct Point *p) {
   return &board[(p->x-1) / 4][p->y-1];
}

void update_cellgp(uint8_t *cellgp, struct Point *p, uint8_t shift, bool set) {

   *cellgp = *cellgp | (1 << shift); //Set dirty flag
   if(set) {
      *cellgp = *cellgp | (2 << shift); //Set alive
      cputcxy(p->x, p->y, 0xCE);
   }
   else {
      *cellgp = *cellgp & ~(2 << shift); //Set not alive &= ~(1UL << n);
      cputcxy(p->x, p->y, ' ');
   }
}

void put_on_board(uint8_t board[BD_W][BD_H], struct Point *shape, uint8_t len, uint8_t ox, uint8_t oy) {
   uint8_t *cellgp, shift;
   struct Point p;
   for(uint8_t i = 0; i < len; i++) {
      p.x = ox+shape[i].x;
      p.y = oy+shape[i].y;

      cellgp = point_to_board_cellgp_ref(board, &p);
      shift = ((p.x-1)%4)*2;
      update_cellgp(cellgp, &p, shift, true);
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

uint8_t live_neigbs_cnt(uint8_t board[BD_W][BD_H], struct Point *p, uint8_t shift) {

   uint8_t c = 0, s, *cellgp, start = 0, end = 8;
   struct Point nbr;

   if(p->x > X_MIN && p->x < X_MAX && p->y > Y_MIN && p->y < Y_MAX) {

      // printf("--Checking nbrs of (%02d,%02d)[%d]\r\n", p->x, p->y, shift);

      UL(nbr, p);
      cellgp = point_to_board_cellgp_ref(board, &nbr);
      // printf("Value of cellgp at (%02d,%02d) is %02x\r\n", nbr.x, nbr.y, *cellgp);
      s = SH_DN(shift);
      if(IS_ALIVE(*cellgp >> s)) {
         // printf("(%02d,%02d)[%d] was alive\r\n", nbr.x, nbr.y, s);
         c++;
      }

      UC(nbr, p);
      // printf("Value of cellgp at (%02d,%02d) is %02x\r\n", nbr.x, nbr.y, *cellgp);
      cellgp = point_to_board_cellgp_ref(board, &nbr);
      if(IS_ALIVE(*cellgp >> shift)) {
         // printf("(%02d,%02d)[%d] was alive\r\n", nbr.x, nbr.y, s);
         c++;
      }

      UR(nbr, p);
      cellgp = point_to_board_cellgp_ref(board, &nbr);
      // printf("Value of cellgp at (%02d,%02d) is %02x\r\n", nbr.x, nbr.y, *cellgp);
      s = SH_UP(shift);
      if(IS_ALIVE(*cellgp >> s)) {
         // printf("(%02d,%02d)[%d] was alive\r\n", nbr.x, nbr.y, s);
         c++;
      }

      CL(nbr, p);
      cellgp = point_to_board_cellgp_ref(board, &nbr);
      // printf("Value of cellgp at (%02d,%02d) is %02x\r\n", nbr.x, nbr.y, *cellgp);
      s = SH_DN(shift);
      if(IS_ALIVE(*cellgp >> s)) {
         // printf("(%02d,%02d)[%d] was alive\r\n", nbr.x, nbr.y, s);
         c++;
      }

      CR(nbr, p);
      cellgp = point_to_board_cellgp_ref(board, &nbr);
      // printf("Value of cellgp at (%02d,%02d) is %02x\r\n", nbr.x, nbr.y, *cellgp);
      s = SH_UP(shift);
      if(IS_ALIVE(*cellgp >> s)) {
         // printf("(%02d,%02d)[%d] was alive\r\n", nbr.x, nbr.y, s);
         c++;
      }

      LL(nbr, p);
      cellgp = point_to_board_cellgp_ref(board, &nbr);
      // printf("Value of cellgp at (%02d,%02d) is %02x\r\n", nbr.x, nbr.y, *cellgp);
      s = SH_DN(shift);
      if(IS_ALIVE(*cellgp >> s)) {
         // printf("(%02d,%02d)[%d] was alive\r\n", nbr.x, nbr.y, s);
         c++;
      }

      LC(nbr, p);
      // printf("Value of cellgp at (%02d,%02d) is %02x\r\n", nbr.x, nbr.y, *cellgp);
      cellgp = point_to_board_cellgp_ref(board, &nbr);
      if(IS_ALIVE(*cellgp >> shift)) {
         // printf("(%02d,%02d)[%d] was alive\r\n", nbr.x, nbr.y, s);
         c++;
      }

      LR(nbr, p);
      cellgp = point_to_board_cellgp_ref(board, &nbr);
      // printf("Value of cellgp at (%02d,%02d) is %02x\r\n", nbr.x, nbr.y, *cellgp);
      s = SH_UP(shift);
      if(IS_ALIVE(*cellgp >> s)) {
         // printf("(%02d,%02d)[%d] was alive\r\n", nbr.x, nbr.y, s);
         c++;
      }
   }

   cputcxy(p->x, p->y, c+ASCII_0);

   return c;
}


uint8_t live_neigbs_cnt_old(uint8_t board[BD_W][BD_H], struct Point *p, uint8_t baseshift) {

   uint8_t c = 0, *cellgp, shift, start = 0, end = 8;
   struct Point all[] = { {p->x-1, p->y-1}, {p->x, p->y-1}, {p->x+1, p->y-1}, 
                          {p->x-1, p->y},                   {p->x+1, p->y},
                          {p->x-1, p->y+1}, {p->x, p->y+1}, {p->x+1, p->y+1} };

   //Taking this out breaks it
   // if(p->x == X_MIN) {
   //    all[0].x = all[3].x = all[5].x = 0;
   // }
   // else if (p->x == X_MAX) {
   //    all[2].x = all[4].x = all[7].x = 0;
   // }

   if(p->y == Y_MIN) start = 3;
   else if (p->y == Y_MAX) end = 4;

   // baseshift = ((p->x-1)%4)*2;
   
   for(uint8_t i = start; i <= end; i++) {
      if(all[i].x==0)
         continue;



      if(i==0 || i==3 || i==5) {
         if(p->x == X_MIN) continue;
         if(baseshift == 0) shift = 6;
         else shift = baseshift-2;
      }
      else if(i==2 || i==4 || i==7) {
         if(p->x == X_MAX) continue;
         if(baseshift == 6) shift = 0;
         else shift = baseshift+2;
      }
      else {
         shift = baseshift;
      }

      cellgp = point_to_board_cellgp_ref(board, &all[i]);

      if(IS_ALIVE(*cellgp >> shift))
         c++;
   }

   return c;
}

void evolve(uint8_t src_board[BD_W][BD_H], uint8_t dst_board[BD_W][BD_H]) {
   struct Point p = {0,0};
   uint8_t *dst_cellgp, *src_cellgp, c;

   cputcxy(1, 1, '.');
   memcpy(dst_board, src_board, BD_W*BD_H);
   cputcxy(1, 1, ' ');

   for (p.y = Y_MIN; p.y <= Y_MAX; p.y++) {
      for(p.x = X_MIN; p.x <= X_MAX; ) {

         src_cellgp = point_to_board_cellgp_ref(src_board, &p);
         dst_cellgp = point_to_board_cellgp_ref(dst_board, &p);

         //Evolve cell 0
         c = live_neigbs_cnt(src_board, &p, 0);
         if(c == 3 && IS_NOT_ALIVE_0(*src_cellgp))
            update_cellgp(dst_cellgp, &p, 0, true);
         else if (c != 2 && c != 3 && IS_ALIVE_0(*src_cellgp))
            update_cellgp(dst_cellgp, &p, 0, false);
         p.x++;

         //Evolve cell 1
         c = live_neigbs_cnt(src_board, &p, 2);
         if(c == 3 && IS_NOT_ALIVE_1(*src_cellgp))
            update_cellgp(dst_cellgp, &p, 2, true);
         else if (c != 2 && c != 3 && IS_ALIVE_1(*src_cellgp))
            update_cellgp(dst_cellgp, &p, 2, false);
         p.x++;

         //Evolve cell 2
         c = live_neigbs_cnt(src_board, &p, 4);
         if(c == 3 && IS_NOT_ALIVE_2(*src_cellgp))
            update_cellgp(dst_cellgp, &p, 4, true);
         else if (c != 2 && c != 3 && IS_ALIVE_2(*src_cellgp))
            update_cellgp(dst_cellgp, &p, 4, false);
         p.x++;

         //Evolve cell 3
         c = live_neigbs_cnt(src_board, &p, 6);
         if(c == 3 && IS_NOT_ALIVE_3(*src_cellgp))
            update_cellgp(dst_cellgp, &p, 6, true);
         else if (c != 2 && c != 3 && IS_ALIVE_3(*src_cellgp))
            update_cellgp(dst_cellgp, &p, 6, false);
         p.x++;
         
      }
   }
}



void main(void) {

   struct Point square[] = {{0,0}};
   struct Point p = {5,5};

   bordercolor(0);

   memset(&board1, 0, BD_W*BD_H);
   memset(&board2, 0, BD_W*BD_H);

   put_on_board(board1, square, 1, 2, 2);
   //put_on_board(board1, glider, 5, 6, 6);
   //put_on_board(board1, gospel, 36, 1, 1);

   // debug_board(board1);

   for (p.y = Y_MIN; p.y <= Y_MAX; p.y++) {
      for(p.x = X_MIN; p.x <= X_MAX; ) {

         //Evolve cell 0
         live_neigbs_cnt(board1, &p, 0);
         p.x++;

         //Evolve cell 1
         live_neigbs_cnt(board1, &p, 2);
         p.x++;

         //Evolve cell 2
         live_neigbs_cnt(board1, &p, 4);
         p.x++;

         //Evolve cell 3
         live_neigbs_cnt(board1, &p, 6);
         p.x++;
         
      }
   }

   //debug_board(board1);

   // evolve(board1, board2); 
   
   // debug_board(board2);

   // while(true) {
   //    evolve(board1, board2);   
   //    evolve(board2, board1);   
   // }

   while(true);
}


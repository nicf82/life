/* A Graphics library for the AMSTRAD CPC range of computers
   Uses firmware to realize graphics output..
   2003 H. Hansen
*/

#ifndef  __amsgraph_h__
#define __amsgraph_h__

extern void draw(int x, int y);
extern void plot(int x, int y);
extern unsigned char test(int x, int y);
extern void move(int x, int y);
extern void gpen(unsigned char pencolor);
extern unsigned char getgpen(void);

#endif /* __amsgraph_h__ */
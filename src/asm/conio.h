// extern void printf_small (char *,...) _REENTRANT;
// extern int printf (const char *,...);
// extern int vprintf (const char *, va_list);
// extern int sprintf (char *, const char *, ...);
// extern int vsprintf (char *, const char *, va_list);
// extern int _gotoxy(const char *);

extern void txtinit(void);
extern void clrscr(void);

extern void setink(unsigned char pen, unsigned char fst, unsigned char snd);
extern unsigned char bordercolor(unsigned char color);
extern unsigned char textcolor(unsigned char color);
extern unsigned char bgcolor(unsigned char color);

extern void inverse(void);

extern void gotox(unsigned char x);
extern void gotoy(unsigned char y);
extern void gotoxy(unsigned char x, unsigned char y);

extern void cputc(char c);
extern void cputcxy(unsigned char x, unsigned char y, char c);

extern void cputs(const char* s);
extern void cputsxy(unsigned char x, unsigned char y, const char* s);

extern unsigned char rdcurchr(void);
extern void wrcurchr(unsigned char);

extern unsigned char rdkey(void);
#include <stdio.h>

struct B{
	int c;
	int d;
};

struct A{
int a;
	struct B b[2];
};


struct C{
	int c[4];
	int d;
};

void main() { 
    struct C c;
    struct A **a;
	struct A e[2][2][2][2][2][2][2][2],*pe[2][2][2][2][2][2][2][2];
	c.c[3]=7;
	c.c[2]=6;
	pe[0][1][0][1][0][1][1][0]=&(e[0][1][0][1][0][1][1][0]);
	a=&(pe[0][1][0][1][0][1][1][0]);
	(**a).b[1].c = 999999;
	(**a).b[1].d = 1;
	printf("%d\n",e[0][1][0][1][0][1][1][0].b[c.c[3]-c.c[2]].c + e[0][1][0][1][0][1][1][0].b[1].d);

	return;
}

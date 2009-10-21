#include <stdio.h>

void inter(int x0, int y0, int x1, int y1)
{
	int dx, dy;
	int q, r;
	unsigned int error;
	
	dx = x1 - x0;
	dy = y1 - y0;
	q = dy / dx;
	r = dy % dx;
	
	error = 0;
	while(1) {
		//printf("%02d, %02d\n", x0, y0);
		
		if(x0 == x1)
			break;
		
		x0++;
		y0 += q;
		error += r;
		if((error > (dx/2)) && !(error & 0x80000000)) {
			y0++;
			error -= dx;
		}
	}
	if(y0 != y1)
		printf("ERROR - %d %d\n", y0, y1);
	if(error != 0)
		printf("error is %d\n", error);
}

int main()
{
	unsigned int x0, y0, x1, y1;
	unsigned int i, j;
	
	for(j=0;j<1000;j++) {
		for(i=0;i<1000;i++) {
			x0 = rand() % 65535;
			y0 = rand() % 65535;
			x1 = x0 + (rand() % 65535) + 1;
			y1 = y0 + (rand() % 65535);
			inter(x0, y0, x1, y1);
		}
		printf("%d.%d%%\r", j/10, j%10);
		fflush(stdout);
	}
	return 0;
}

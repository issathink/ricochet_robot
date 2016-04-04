#include "tools.h"

/* Retourne une enigme tiree aleatoirement dans la liste des enigmes predefinis. */
char* get_enigme() {
	int r;
	srand(time(NULL));
	r = rand() % NB_ENIGMES;

	switch(r) {
		case 0:
			return ENIGME1;
		case 1:
			return ENIGME2;
		case 2:
			return ENIGME3;
		case 3:
			return ENIGME4;
	}
	return ENIGME1;
}

/* Retourne une copie de l'enigme. */
Enigme* copy_of_enigme(Enigme *enigme) {
	Enigme* enig = malloc(sizeof(Enigme));
	enig->xr = enigme->xr;
	enig->yr = enigme->yr;
    	enig->xb = enigme->xb;
    	enig->yb = enigme->yb;
    	enig->xj = enigme->xj;
    	enig->yj = enigme->yj;
    	enig->xv = enigme->xv;
    	enig->yv = enigme->yv;
    	enig->xc = enigme->xc;
    	enig->yc = enigme->yc;
    	enig->c = enigme->c;
	
	return enig;
}

/* Verifie s'il n'y a pas de robots dans la case (x,y) */
int no_robot(Enigme* e, int x,int y){
	int present = 1;
	if(e->xr == x && e->yr == y)
		present = 0;
	if(e->xj == x && e->yj == y)
		present = 0;
	if(e->xv == x && e->yv == y)
		present = 0;
	if(e->xb == x && e->yb == y)
		present = 0;
	return present;
}

/* Bouge le client dans la case (x,y) vers la direction d. */
void move(Plateau* plateau,Enigme* enigme, int *x, int *y, char d) {
	if(d == 'H') 
		while(plateau->cases[*x][*y].h != 1 && no_robot(enigme,*x-1,*y)) 
			 *x-=1;
	else if(d == 'B') 
		while(plateau->cases[*x][*y].b != 1 && no_robot(enigme,*x+1,*y)) 
			*x+=1;
	else if(d == 'G')
		while(plateau->cases[*x][*y].g != 1 && no_robot(enigme,*x,*y-1))
			*y-=1;
	else if(d == 'D')
		while(plateau->cases[*x][*y].d != 1 && no_robot(enigme,*x,*y+1))
			*y+=1;
}

/* 
 * Verifie si la solution proposee resout l'enigme.
 * return 0 si la solution est bonne -1 sinon
 */ 
int solution_bonne(Plateau* plateau, Enigme* enigme, char* deplacements, int coups) {
	int i = 0;
	char c, d;
	
	if((int)strlen(deplacements) > 2*coups) {
	        fprintf(stderr, "Arrete de me troller merci coups %d deplacements %d\n", coups, strlen(deplacements));
	        return 0;
	}
	
	while(deplacements[i] != '\0') {
		c = deplacements[i++];
		d = deplacements[i];
		
		if(c == 'R') {
			// fprintf(stderr,"Position du rouge %d %d \n",enigme->xr,enigme->yr);
			move(plateau,enigme, &enigme->xr, &enigme->yr, d);
			// fprintf(stderr,"Position du rouge %d %d \n",enigme->xr,enigme->yr);
		} else if(c == 'B') {
			move(plateau,enigme, &enigme->xb, &enigme->yb, d);
		} else if(c == 'J') {
			move(plateau,enigme, &enigme->xj, &enigme->yj, d);
		} else if(c == 'V') {
			move(plateau,enigme, &enigme->xv, &enigme->yv, d);
		}
		i++;
	}
	
	if(enigme->c == 'R') {
		if(enigme->xr == enigme->xc && enigme->yr == enigme->yc)
			return 1;
		return 0;
	} else if(enigme->c == 'B') {
		if(enigme->xb == enigme->xc && enigme->yb == enigme->yc)
			return 1;
		return 0;
	} else if(enigme->c == 'J') {
		if(enigme->xj == enigme->xc && enigme->yj == enigme->yc)
			return 1;
		return 0;
	} else if(enigme->c == 'V') {
		if(enigme->xv == enigme->xc && enigme->yv == enigme->yc)
			return 1;
		return 0;
	} else {
		fprintf(stderr, "[solution_bonne] What ??? enigme->c: %c\n", enigme->c);
		return 0;
	}
}


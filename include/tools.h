#include <stdio.h>
#include <stdlib.h> 
#include <stdio.h> 
#include <string.h>
#include <signal.h>
#include <unistd.h> 
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h> 
#include <sys/time.h> 
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#define		NB_CASES			16
#define		SCORE_OBJ			100

#define		TEMPS_REFLEXION	60 //300		/* secondes */
#define		TEMPS_ENCHERE		60 // 30		/* secondes */
#define		TEMPS_RESOLUTION	60 // 60		/* secondes */

#define		SERVER_PORT		2016
#define		LINE_SIZE			200
#define		SMS_SIZE			140

extern int	USERS_CPT;
extern int	SESSIONS_CPT;

/* Donnees d'un utilisateur */
typedef struct _user {
	int 				id;
	int 				score;
	int 				coups;
	int				scom;
	char*			username;
	struct _user*		next;
} User;


typedef struct _enchere {
	int 				scom;
	int 				mise;
	struct _enchere*	next;
} Enchere;

/* Plateau du jeu (d'une session) */
typedef struct _plateau {
	int				cases[NB_CASES][NB_CASES];
} Plateau;

/* Donnees d'une session */
typedef struct _session {
	int 				id;
	int				size;
	int				countdown;
	Plateau			*plateau;
	User			*user;
	struct _session	*next;
} Session;

typedef enum { REFLEXION, ENCHERE, RESOLUTION, UNDEF } PHASE;
typedef enum { ROUGE, BLEU, JAUNE, VERT } COULEUR;
typedef enum { HAUT, BAS, GAUCHE, DROITE } COTE;

/* Manipulation des utilisateurs */
User*		create_user(char *username, int scom);
int 			add_user(User *user, Session *session);
User*		delete_user(User *user, Session *session);
void 		affiche_user(User *user);
void			free_user(User* user);

Enchere* 	create_enchere(int scom, int mise);
int 			add_enchere(Enchere *enchere, Enchere *init);
Enchere*	delete_enchere(Enchere* enchere, Enchere* init);
void			free_enchere(Enchere* enchere);
void			vider_enchere(Enchere* init);
Enchere* 	get_le_moins_offrant(Enchere *init);


/* Manipulation des sessions */
Session*		create_session();
int 			add_session(Session *new, Session *list);
Session* 	delete_session(Session *del, Session *list);
void 		affiche_session(Session *session);
void			affiche_sessions(Session *head);

char*		grow_char(char* old, char* to_add);
int 			decode_header(char *str);
User*		cherche_user(Session *session, int scom);
int 			get_username(char *buff, char *username);
int 			get_username_and_coups(char *buff, char *username, int *coups);
int 			get_username_and_deplacements(char* buff, char* username, char* deplacements, int coups);
void			vider_session(Session *joining);

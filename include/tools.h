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

#define		TEMPS_REFLEXION		300		/* secondes */
#define		TEMPS_ENCHERES		30		/* secondes */
#define		TEMPS_RESOLUTION	60		/* secondes */

#define		SERVER_PORT			2016
#define		LINE_SIZE			200

extern int	USERS_CPT;
extern int	SESSIONS_CPT;

/* Donnees d'un utilisateur */
typedef struct _user {
	int 			id;
	int 			score;
	int				scom;
	char 			*username;
	struct _user	*next;
} User;

/* Plateau du jeu (d'une session) */
typedef struct _plateau {
	int				cases[NB_CASES][NB_CASES];
} Plateau;

/* Donnees d'une session */
typedef struct _session {
	int 			id;
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
int 		add_user(User *user, Session *session);
User*		delete_user(User *user, Session *session);
void 		affiche_user(User *user);

/* Manipulation des sessions */
Session*	create_session();
int 		add_session(Session *new, Session *list);
Session* 	delete_session(Session *del, Session *list);
void 		affiche_session(Session *session);
void		affiche_sessions(Session *head);

int 		decode_header(char *str);
User*		cherche_user(Session *session, int scom);
int 		get_username(char *buff, char *username);
int 		get_username_and_coups(char *buff, char *username, int *coups);
void		vider_session(Session *joining);

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
#define		SCORE_OBJ			10

#define		TEMPS_REFLEXION	60 //300		/* secondes */
#define		TEMPS_ENCHERE		30 // 30		/* secondes */
#define		TEMPS_RESOLUTION	60 // 60		/* secondes */

#define		SERVER_PORT		2016
#define		LINE_SIZE			200
#define		SMS_SIZE			140
#define		MAX_INT				2147483647

extern int	USERS_CPT;
extern int	SESSIONS_CPT;
extern char*	ENIGME1;
extern char*	PLATEAU1;
extern int	R, B, J, V;
extern pthread_mutex_t mutex_phase;
extern int 		sock;


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

typedef struct _caze {
	int				h;
	int 				d;
	int 				b;
	int 				g;
} caze;

/* Plateau du jeu (d'une session) */
typedef struct _plateau {
	caze			cases[NB_CASES][NB_CASES];
} Plateau;

typedef struct _enigme {
	int				xr, yr;
	int 				xb, yb;
	int 				xv, yv;
	int 				xj, yj;
	int 				xc, yc;
	char				c;
} Enigme;

/* Donnees d'une session */
typedef struct _session {
	int 			id;
	int				size;
	int				countdown;
	Plateau			*plateau;
	Enchere 		*enchere;
	User			*user;
	struct _session	*next;
} Session;

typedef enum { REFLEXION, ENCHERE, RESOLUTION, UNDEF } PHASE;
typedef enum { ROUGE, BLEU, JAUNE, VERT } COULEUR;
typedef enum { HAUT, BAS, GAUCHE, DROITE } COTE;

extern PHASE 	phase;

/* Manipulation des utilisateurs */
User*		create_user(char *username, int scom);
int 			add_user(User *user, Session *session);
User*		delete_user(User *user, Session *session);
void 		affiche_user(User *user);
void			free_user(User* user);

Enchere* 	create_enchere(int scom, int mise);
int			add_enchere(Enchere *enchere, Session *init);
Enchere*	cherche_enchere(int scom, Session* init);
Enchere*	delete_enchere(Enchere* enchere, Session* init);
void			free_enchere(Enchere* enchere);
void			vider_enchere(Session* init);
Enchere* 	get_le_moins_offrant(Session *init);
void                 affiche_enchere(Session* init);

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
char* 		get_bilan(Session* session, int nb_tour);
char*		get_enigme();
Enigme*		copy_of_enigme(Enigme *enigme);
int 			solution_bonne(Plateau* plateau, Enigme* enigme, char* deplacements);
void                 affiche_plateau(Plateau* plateau);
void*               handle_client(void* arg);
void*               listen_to_clients(void* arg);

int connexion(int scom, char* buff);
int deconnexion(int scom, char* buff);
void client_enchere(int scom, char* buff);
void client_trouve(int scom, char *buff) ;
void client_resolution(int scom, char* buff);
void client_chat(int scom, char* buff);
void disconnect_if_connected(int scom);

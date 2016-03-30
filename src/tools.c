#include "tools.h"

int	USERS_CPT 		= 0;
int	SESSIONS_CPT 	= 0;
char*PLATEAU1		= "(0,3,D)(0,11,D)(0,13,B)(1,12,D)(2,5,D)(2,5,B)(2,9,D)(2,9,B)(4,0,B)(4,2,D)(4,2,H)(4,15,H)(5,7,G)(5,7,B)(5,14,G)(5,14,B)(6,1,G)(6,1,H)(6,11,H)(6,11,D)(7,7,G)(7,7,H)(7,8,H)(7,8,D)(8,7,G)(8,7,B)(8,8,B)(8,8,D)(8,5,H)(8,5,D)(9,1,D)(9,1,B)(9,12,D)(9,15,B)(10,4,G)(10,4,B)(11,0,B)(12,9,H)(12,9,G)(13,5,D)(13,5,H)(13,14,G)(13,14,B)(14,3,G)(14,3,H)(14,11,D)(14,11,B)(15,14,G)(15,6,D)";
// 13,5,D
char* ENIGME1 		= "(13,5,9,12,6,1,5,14,14,11,R)";
int 	R, B, J, V;

/*
 * Hello, to save you time, this part contains nothing interesting,
 * just some functions to handle lists and sessions.
 * If you want to debug then go ahead.
 */
User *create_user(char *username, int scom) {
	User *user = malloc(sizeof(User));
	if(user == NULL) return NULL;
	user->username = calloc((strlen(username)+1), sizeof(char));
	if(user == NULL) return NULL;
	strcpy(user->username, username);
	user->next = NULL;
	user->id = ++USERS_CPT;
	user->score = 0;
	user->coups = -1;
	user->scom = scom;

	return user;
}

int add_user(User *user, Session *session) {
        
	User *tmp = session->user;
	if(user == NULL || session == NULL)
		return -1;

	if(session->size == 0) {
		session->user = user;
		session->size++;
		return 0;
	}
	while(tmp->next != NULL)
		tmp = tmp->next;
	tmp->next = user;
	session->size++;
	affiche_session(session);

	return 0;
}

User* delete_user(User *user, Session *session) {
	Session *tmp = session;
	User* _user, *head = session->user;
	if(session->size == 0 || user == NULL || session == NULL) 
		return NULL;	
	if(tmp->user->id == user->id) {
		session->user = session->user->next;
		session->size--;
		return user;
	}
	_user = tmp->user->next;
	while(_user != NULL) {
		if(user->id == _user->id) {
			tmp->user->next = _user->next;
			session->user = head;
			session->size--;
			return user;
		}
		_user = _user->next;
		tmp->user = tmp->user->next;
	}

	return NULL;
}

void affiche_user(User *user) {
	if(user == NULL)
		printf("/* WARNING afficher_user */ Want to display NULL user.\n");
	else
		printf("  [User] ID: %d, name: %s, score: %d, scom: %d\n", user->id, user->username, user->score, user->scom);
}

void free_user(User* user) {
	if(user == NULL)
		return;

	free(user->username);
	free(user);
	user = NULL;
}

Enchere* create_enchere(int scom, int mise) {
	Enchere* enchere = malloc(sizeof(Enchere));
	if(enchere == NULL)
		return NULL;
	enchere->scom = scom;
	enchere->mise = mise;
	enchere->next = NULL;
	return enchere;
}


Enchere* add_enchere(Enchere *enchere, Enchere *init) {
	Enchere* tmp = init, *ench;
	if(enchere == NULL)
		return init;
		
	ench = cherche_enchere(enchere->scom, init);
	if(ench != NULL) {
		ench->mise = enchere->mise;
		return init;
	}
	if(init == NULL) {
		init = enchere;
		return init;
	}

	while(tmp->next != NULL)
		tmp = tmp->next;
	tmp = enchere;
	
	return init;
}

Enchere* cherche_enchere(int scom, Enchere* init) {
	Enchere *tmp;
	if(init == NULL) {
		return NULL;
        }
		
	tmp = init;
	while(tmp != NULL) {
		if(tmp->scom == scom)
			return tmp;
		tmp = tmp->next;
	}
	return NULL;
}

Enchere* delete_enchere(Enchere* enchere, Enchere* init) {
	Enchere* tmp, *_ench;

	if(init == NULL || enchere == NULL) return NULL; 

	if(init->next == NULL && init->scom == enchere->scom) {
	        tmp = init;
	        init = NULL;
		return tmp;
	}
	if(init->next == NULL)
		return NULL;

	if(init->scom == enchere->scom) {
		tmp = init;
		init = init->next;
		return tmp;
	}
	tmp = init->next;
	_ench = init;
	while(tmp != NULL) {
		if(tmp->scom == enchere->scom) {
			_ench->next = tmp->next;
			return tmp;
		}
		_ench = tmp;
		tmp = tmp->next;
	}
	return NULL;
}

void afficher_enchere(Enchere* init) {
        Enchere* tmp = init;
        if(init == NULL) {
                fprintf(stderr, ">>> Pas d'enchere ???\n");
                return;
        }
        while(tmp != NULL) {
                fprintf(stderr, ".............. scom: %d mise: %d ...............\n", tmp->scom, tmp->mise);
                tmp = tmp->next;
        }
}

void free_enchere(Enchere* enchere) {
	if(enchere != NULL)
		free(enchere);
}

/*
 * Retourne le moins offrant et le supprime de la liste des encheres.
 */
Enchere* get_le_moins_offrant(Enchere *init) {
	Enchere* tmp = init, *ench;
	if(init == NULL)
		return NULL;
	ench = tmp;
	while(tmp != NULL) {
		if(ench->mise > tmp->mise) {
			ench = tmp;
		}
		tmp = tmp->next;
	}
	return delete_enchere(ench, init);
}


Session* create_session(int countdown) {
	Session* session = malloc(sizeof(Session));
	if(session == NULL) return NULL;
	session->plateau = NULL;
	session->next = NULL;
	session->user = NULL;
	session->countdown = countdown;
	session->size = 0;
	session->id = ++SESSIONS_CPT;
	return session;
}

int add_session(Session *new, Session *list) {
	Session *tmp = list;
	if(tmp->next == NULL) {
		tmp->next = new;
		return 0;
	}
	while(tmp->next != NULL)
		tmp = tmp->next;
	tmp->next = new;
	return 0;
}

Session* delete_session(Session *del, Session *list) {
	Session *tmp = list;
	Session *session, *head = list;

	if(list->id == del->id) {
		printf("First one\n");		
		list = list->next;
		return del;
	}
	
	session = tmp->next;
	while(session != NULL) {
		if(del->id == session->id) {
			tmp->next = session->next;
			session = head;
			return del;
		}
		session = session->next;
		tmp = tmp->next;
	}

	return NULL;
}

void affiche_session(Session *session) {
	Session *tmp = session;
	User *user = tmp->user; 

	if(session == NULL)
		printf("/* WARNING afficher_session */ Want to display NULL session.\n");

	if(tmp->size == 0) {
		printf("[Session] Session(vide) ID: %d [countdown: %d]\n", tmp->id, tmp->countdown);
	} else {
		printf("[Session] Session ID: %d [size: %d, countdown: %d]\n", tmp->id, tmp->size, tmp->countdown);
		user = tmp->user;
		while(user != NULL) {
			affiche_user(user);						
			user = user->next;
		}
	}
}

void affiche_sessions(Session *head) {
	Session *tmp = head;
	fprintf(stderr, "/******************* Toutes les sessions: ***************/\n");
	while(tmp != NULL) {
		affiche_session(tmp);
		tmp = tmp->next;
	}
	fprintf(stderr, "/********************************************************/\n");
}


/*********************** Code de la partie serveur ***************************/
/*
 * Self explanatory function.
 */
int decode_header(char *str) {
	if(strncmp("CONNEXION/", str, strlen("CONNEXION/")) == 0)
		return 1;
	else if(strncmp("SORT/", str, strlen("SORT/")) == 0)
		return 2;
	else if(strncmp("TROUVE/", str, strlen("TROUVE/")) == 0)
		return 4;
	else if(strncmp("ENCHERE/", str, strlen("ENCHERE/")) == 0)
		return 5;
	else if(strncmp("SOLUTION/", str, strlen("SOLUTION/")) == 0)
		return 6;
	else if(strncmp("SEND/", str, strlen("SEND/")) == 0)
		return 7;
	return 0;
}

/*
 * Look for a user with a socket of communication equals scom inside session.
 * return user if there is one, NULL otherwise.
 */
User* cherche_user(Session* session, int scom) {
	User *tmp = session->user;
	if(session->size <= 0 || tmp == NULL)
		return NULL; 
	while(tmp != NULL) {
		if(tmp->scom == scom) {
			// fprintf(stderr, "Jai trouve scom: %d, username: %s\n", tmp->scom, tmp->username);		
			return tmp;
		}
		tmp = tmp->next;
	}

	return NULL;
}

/*
 * Decode buff information and writes the name of the user in username.
 * return 0 if everything is fine -1 otherwise.
 */
int get_username(char* buff, char* username) {
	unsigned int i = 0, j = 0;
	unsigned int size = strlen(buff);
	char name[50];
	
	while(i < size && buff[i] != '/') i++;
	i++;
	if(i == size || i == size - 1)
		return -1;
	while(buff[i] != '/')
		name[j++] = buff[i++];
	name[j] = '\0';
	if(j < 1)
		return -1;
	strcpy(username, name);

	return 0;
}

/*
 * Decode buff information and writes the name of the user in username
 * and number of steps on the location pointed by coups.
 * return 0 if everything is fine -1 otherwise.
 */
int get_username_and_coups(char* buff, char* username, int* coups) {
	unsigned int i = 0, j = 0;
	unsigned int size = strlen(buff);
	char tmp[11], *endptr;

	if(get_username(buff, username) == -1)
		return -1;
	while(i < size && buff[i] != '/') i++;
	i++;
	while(i < size && buff[i] != '/') i++;
	i++;
	while(buff[i] != '/')
		tmp[j++] = buff[i++];

	if(j < 1 || j > 10) return -1;
	
	*coups = strtol(tmp, &endptr, 10);
    if (errno == ERANGE || (errno != 0 && *coups == 0))
       	return -1;

	return 0;
}

/*
 * Decode buff information and writes the name of the user in username
 * and his solution on the location pointed by coups.
 * return 0 if everything is fine -1 otherwise.
 */
int get_username_and_deplacements(char* buff, char* username, char* deplacements, int coups) {
	int i = 0, j = 0;
	int size = strlen(buff);

	if(get_username(buff, username) == -1)
		return -1;
	while(i < size && buff[i] != '/') i++;
	i++;
	while(i < size && buff[i] != '/') i++;
	i++;
	while(buff[i] != '/')
		deplacements[j++] = buff[i++];
	deplacements[j] = '\0';
	
	// fprintf(stderr, "Cest avant deplacements qui ne crash pas j: %d.\n", j);
	if(j < 1) return -1;
	else if(j > (coups*2+1)) return -2;

	return 0;
}

/*
 * Free a session content.
 */
void vider_session(Session *joining) {
	User *user = joining->user;
	User *tmp;

	if(joining == NULL) {
	        fprintf(stderr, "Why joining is NULL ???\n");
		return;
	}
	
	while(user != NULL) {
		tmp = delete_user(user, joining);
		user = user->next;
		free_user(tmp);
	}
	joining->size = 0;
	joining->user = NULL;
}

/*
 * Free all bids of previous round.
 */
void vider_enchere(Enchere* init) {
	Enchere *ench = init;
	Enchere *tmp;
	if(init == NULL)
		return;

	while(ench != NULL) {
		tmp = ench;
		ench = ench->next;
		free_enchere(tmp);
	}
	ench = NULL;
	init = NULL;
}

/*
 * Try to grow the size of old and concatenate it with to_add.
 */
char* grow_char(char* old, char* to_add) {
	old = realloc(old, strlen(old) + strlen(to_add) + 1);
	strcat(old, to_add);
	return old;
}

char* get_bilan(Session* session, int nb_tour) {
	User *tmp;
   	char buff[70], *msg;

    	sprintf(buff, "%d", nb_tour);
    	msg = malloc(sizeof(char)*(strlen(buff)+1));
    	strcpy(msg, buff);
    	memset(buff, 0, 70);
    	tmp = session->user;
    	while(tmp != NULL) {
    		sprintf(buff, "(%s,%d)", tmp->username, tmp->score);
        	msg = grow_char(msg, buff);
		tmp = tmp->next;
	}
	return msg;
}

char* get_enigme() {
	// Nah we are returning this for the moment
	return ENIGME1;
}


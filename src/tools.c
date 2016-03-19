#include "tools.h"

int	USERS_CPT 		= 0;
int	SESSIONS_CPT 	= 0;

/*
 * Hello, to save you time, this part contains nothing interesting,
 * just some functions to handle lists and sessions.
 */
User *create_user(char *username, int scom) {
	User *user = malloc(sizeof(User));
	if(user == NULL) return NULL;

	user->username = malloc(sizeof(char)*(strlen(username)+1));
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
	Session *tmp = session;
	User *_user = session->user;	

	if(user == NULL || session == NULL)
		return -1;

	if(tmp->size == 0) {
		tmp->user = user;
		tmp->size++;
		return 0;
	}

	while(tmp->user->next != NULL)
		tmp->user = tmp->user->next;
	tmp->user->next = user;
	
	session->user = _user;
	session->size++;

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
		printf("  [User] ID: %d, name: %s, score: %d\n", user->id, user->username, user->score);
}

Enchere* create_enchere(int scom, int mise) {
	Enchere* enchere = malloc(sizeof(Enchere));
	if(enchere == NULL)
		return NULL;
	enchere->scom = scom;
	enchere->mise = mise;
	return enchere;
}

int add_enchere(Enchere *enchere, Enchere *init) {
	Enchere* tmp = init;

	if(enchere == NULL || init == NULL)
		return -1;

	while(tmp->next != NULL)
		tmp = tmp->next;
	tmp = enchere;
	return 0;
}

Enchere* delete_enchere(Enchere* enchere, Enchere* init) {
	Enchere* tmp, *_ench;

	if(init == NULL || enchere == NULL) return NULL; 

	if(init->next == NULL && init->scom == enchere->scom)
		return init;
	if(init->next == NULL)
		return NULL;

	if(init->scom == enchere->scom) {
		tmp = init;
		init = init->next;
		return init;
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

/*
 * Retourne le moins offrant et le supprime de la liste des encheres.
 */
Enchere* getLeMoinsOffrant(Enchere *init) {
	Enchere* tmp = init, *ench;


	if(init == NULL)
		return NULL;

	ench = tmp;

	while(tmp != NULL) {
		if(ench->mise > tmp->mise) {
			ench = tmp;
		}
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
	if(strncmp("CONNEX/", str, strlen("CONNEX/")) == 0)
		return 1;
	else if(strncmp("SORT/", str, strlen("SORT/")) == 0)
		return 2;
	else if(strncmp("TROUVE/", str, strlen("TROUVE/")) == 0)
		return 4;
	else if(strncmp("ENCHERE/", str, strlen("ENCHERE/")) == 0)
		return 5;
	else if(strncmp("SOLUTION/", str, strlen("SOLUTION/")) == 0)
		return 6;
	return 0;
}

/*
 * Look for a user with a socket of communication equals scom inside session.
 * return user if there is one, NULL otherwise.
 */
User* cherche_user(Session* session, int scom) {
	User *tmp = session->user;;

	while(tmp != NULL) {
		if(tmp->scom == scom)
			return tmp;
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

	if(j < 1 || j > coups) return -1;

	return 0;
}

/*
 * Free a session content.
 */
void vider_session(Session *joining) {
	User *user = joining->user;
	User *tmp;
	
	while(user != NULL) {
		tmp = delete_user(user, joining);
		user = user->next;
		free(tmp);
	}
}

/*
 * Try to grow the size of old and concatenate it with to_add.
 */
char* grow_char(char* old, char* to_add) {
	old = realloc(old, strlen(old) + strlen(to_add) + 1);
	strcat(old, to_add);
	return old;
}
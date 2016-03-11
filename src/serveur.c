#include "tools.h"

/* Variable pour stocker toutes les sessions en cours */
Session *init, *joining;
int session_started; 
pthread_mutex_t mutex_init = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_joining = PTHREAD_MUTEX_INITIALIZER;

int sock;

void handler_reflexion(int sig) {
	/* Reveiller le serveur (fin de la phase de reflexion) */
}

void handler_encheres(int sig) {
	/* Reveiller le serveur (fin de la phase de reflexion) */
}

void handler_resolution(int sig) {
	/* Reveiller le serveur (fin de la phase de reflexion) */
}

void end_session() {
	fprintf(stderr,"Ending session...no enough users\n");
}

void	start_session() {	
	fprintf(stderr,"Starting session...\n");
	User *tmp;
	char msg[100];

	pthread_mutex_lock (&mutex_init);
	
	pthread_mutex_unlock (&mutex_init);
}

void go() {
	/* Chaque tour de boucle constitue un Tour de jeu */
	while(1) {
		/* Rajouter les utilisateurs qui attendent */
		int size = 0;
		pthread_mutex_lock (&mutex_joining);
		pthread_mutex_lock (&mutex_init);
		User *user = joining->user;

		while(user != NULL) {
			add_user(user, init); 
			user = user->next;
		}
		size = init->size;
		// Liberer les donnees de la liste joining.
		vider_session(joining);
		pthread_mutex_unlock (&mutex_init);
		pthread_mutex_unlock (&mutex_joining);

		if(size >= 2)
			start_session();
		else if(size < 2)
			end_session();

		
		/* Phase de reflexion */
		

		/* Phase d'encheres */


		/* Phase de resolution */
		
		
	}
}

/*
 * Permet de notifer les autres client de l'arrivee d'un nouveau,
 * puis envoyer au nouveau la liste des joueurs presents avec leurs score.
 */
int send_to_all(int scom, char *name) {
	User *tmp;
	char msg[100];
	char usr[100];
	pthread_mutex_lock (&mutex_init); 
	tmp = init->user;
	while(tmp != NULL) {
		// Notifier les clients deja connectes d'une nouvelle connexion
		// Et envoyer au nouvel arrivant les joueurs connectes.
		if(tmp->scom != scom) {
			sprintf(msg, "CONNECTE/%s/\n", name);
			send(tmp->scom, msg, strlen(msg)+1, 0);
			sprintf(usr, "LIST/%s/%d/\n", tmp->username, tmp->score);
			send(scom, usr, strlen(usr)+1, 0);
		}
		tmp = tmp->next;
	}
	pthread_mutex_unlock (&mutex_init); 

	pthread_mutex_lock (&mutex_joining);
	tmp = joining->user;
	while(tmp != NULL) {
		if(tmp->scom != scom) {
			sprintf(msg, "CONNECTE/%s/\n", name);
			send(tmp->scom, msg, strlen(msg)+1, 0);
			sprintf(usr, "LIST/%s/%d/\n", tmp->username, tmp->score);
			send(scom, usr, strlen(usr)+1, 0);
		}
		tmp = tmp->next;
	}	
	pthread_mutex_unlock (&mutex_joining); 

	return 0;
}

/*
 * Envoie a chaque client le score des clients connectes apres un tour.
 */
void send_list() {
	User *tmp, *user;
	char buff[50];

	pthread_mutex_lock (&mutex_init); 
	tmp = init->user;
	while(tmp != NULL) {
		user = init->user;
		while(user != NULL) {
			sprintf(buff, "LIST/%s/%d/\n", user->username, user->score);
			send(user->scom, buff, strlen(buff)+1, 0);
			user = user->next;
		}	
		tmp = tmp->next;
	}
	pthread_mutex_unlock (&mutex_init); 
}

/*
 * Gere la commande de connexion d'un client.
 */
int connexion(int scom, char* buff) {
	int size = strlen(buff);
	char *name, msg[100];
	User *user = NULL;

	name = calloc(50, sizeof(char) );

	if(size < 8) {
		fprintf(stderr, "[Serveur] Yo client i don't this command: '%s' (is it in the protocol ?)\n", buff);
		send(scom, "Hey client i cannot connect you.\n", 34, 0);
		return -1;
	} else if(size > 60) {
		fprintf(stderr, "[Serveur] Client name too long, command: '%s' (is it in the protocol ?)\n", buff);
		send(scom, "Client name too long.\n", 23, 0);
		return -1;
	} else if(buff[strlen(buff)-2] != '/' || get_username(buff, name) == -1) {
		fprintf(stderr, "[Serveur] Command not well formed. : '%s' (is it in the protocol ?)\n", buff);
		send(scom, "Command not well formed.\n", 27, 0);
		return -1;
	}

	if((user = create_user(name, scom)) == NULL) {
		fprintf(stderr, "Cannot create user. Maybe there is no memory.\n");
		send(scom, "Cannot create user.\n", 21, 0);
		return -1;
	}
	
	pthread_mutex_lock (&mutex_joining);
	add_user(user, joining);
	pthread_mutex_unlock (&mutex_joining);
	printf("Successfully connected user: %s\n", name);
	sprintf(msg, "BIENVENUE/%s/\n", name);
	send(scom, msg, strlen(msg), 0);

	send_to_all(scom, name);

	return 0;
}

/*
 * Gere la commande de deconnexion d'un client.
 */
int deconnexion(int scom, char* buff) {
	char username[50];
	char tmp[60];
	User *user, *tmpU;

	if(get_username(buff, username) == -1) {
		fprintf(stderr, "[Serveur] Erreur lors de deconnexion. Command: %s\n", buff);
		return -1; 
	}
	fprintf(stderr, "Username : -> %s \n", username);

	// Envoyer a tout le monde que le client s'est deconnecte.
	pthread_mutex_lock (&mutex_init);
	tmpU = init->user;
	while(tmpU != NULL) {
		if(scom != tmpU->scom) {
			sprintf(tmp, "SORTI/%s/\n", username);
			send(tmpU->scom, tmp, strlen(tmp), 0);
			fprintf(stderr, "J'envoie a l'utilisateur: %s, tmp: %s\n", tmpU->username, tmp);		
		} else {			
			user = tmpU;
			fprintf(stderr, "J'ai trouve l'utilisateur: tmpU: %s, username: %s\n", tmpU->username, user->username);
		}
		tmpU = tmpU->next;
	}
	if(user != NULL) {
		delete_user(user, init);
		pthread_mutex_unlock (&mutex_init);
	} else {
		// Logique pour supprimer user de joining s'il y est bien sur.
		delete_user(user, joining);
	}	
	


	fprintf(stderr, "[Serveur] Fin deconnexion.\n");
	return 0;	
}

/*
 * Deconnecte le client au cas ou le client nous envoie une commande
 * vide (non specifiee dans le protocole => on a choisi de le deconnecter).
 */
void disconnect_if_connected(int scom) {
	User *user;
	char buff[60];
	
	pthread_mutex_lock (&mutex_joining);
	user = cherche_user(joining, scom);
	pthread_mutex_unlock (&mutex_joining);
	
	if(user == NULL) {
		pthread_mutex_lock (&mutex_init); 
		user = cherche_user(init, scom);
		pthread_mutex_unlock (&mutex_init);
	}

	if(user != NULL) {
		sprintf(buff, "SORT/%s/\n", user->username);
		deconnexion(scom, buff);
	}
}

/*
 * Phase de reflexion.
 */
int reflexion(int scom, char* buff) {

	return 0;	
}

/*
 * Phase d'encheres.
 */
int enchere(int scom, char* buff) {
	 
	return 0;
}

/*
 * Etape de la resolution d'un tour de l'enigme.
 */
int resolution(int scom, char* buff) {

	return 0;
}

/*
 * Attente et gestion des commandes d'un client (Thread).
 */
void* handle_client(void* arg) {
	char buff[LINE_SIZE];
	int scom;
	ssize_t count;
	scom = *((int *) arg);

	if (pthread_detach (pthread_self()) !=0 ) {
 		perror("pthread_detach"); pthread_exit ((void *)1);
 	}

	while(1) {
		if( (count=recv(scom, buff, LINE_SIZE, 0)) < 0) { perror("read"); exit(1); }

		fprintf(stderr, "Commande a traiter %s, count %d\n", buff, (int)count);

		if(strlen(buff) <= 1) {
			printf("[Serveur] Commande client inconnue.\n");
			break;
		}
		switch(decode_header(buff)) {
		case 1: /* CONNEXION  */			
			if(connexion(scom, buff) == -1)
				pthread_exit(0);
			break;
		case 2: /* DECONNEXION */
			deconnexion(scom, buff);
			break;
		case 3: /* START SESSION */
			printf("[Serveur] Commande pas cense etre la.\n");
			break;
		case 4: /* PHASE REFLEXION */
			reflexion(scom, buff);
			break;
		case 5: /* PHASE ENCHERE */
			enchere(scom, buff);
			break;
		case 6: /* PHASE RESOLUTION */
			resolution(scom, buff);
			break;
		default:
			printf("[Serveur] Something really weird happened.\n");
			break;
		}
		memset(buff, 0, LINE_SIZE);
	}
	close(scom);
	disconnect_if_connected(scom);
	pthread_exit(0);
}


int main() {
	
	int scom, fromlen, *pt_scom;
	struct sockaddr_in sin, exp;
	pthread_t conn_id;
	
	init = create_session(0);
	joining = create_session(0);
	session_started = 0;

	if( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { perror("socket"); exit(1); }
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(SERVER_PORT);
	sin.sin_family = AF_INET;
	
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
    		perror("setsockopt(SO_REUSEADDR) failed");
		exit(1);
	}

	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) { perror("bind"); exit(1); }	
	if (listen(sock, SOMAXCONN) < 0) { perror("listen"); exit(1); }

	while(1) {
		if ((scom = accept(sock, (struct sockaddr *)&exp, (socklen_t *)&fromlen)) < 0) { perror("accept"); exit(1); }
		
		pt_scom = (int *) malloc(sizeof(int));
		*pt_scom = scom;
		if (pthread_create(&conn_id, NULL, handle_client, (void *)pt_scom) != 0) {
			printf("[Serveur] Erreur thread.\n");
		}
	}

	close(sock);
	

	/*User *user = create_user("toto");
	User *user2 = create_user("titi");
	User *user3 = create_user("tutu");

	Session *session = create_session(30);
	Session *session2 = create_session(40);
	Session *session3 = create_session(50);
	
	add_user(user, session);	
	add_user(user2, session);	
	add_user(user3, session);

	// affiche_session(session);

	delete_user(user, session);

	add_session(session, init);
	add_session(session2, init);
	add_session(session3, init);

	affiche_sessions(init);
	printf("\n");

	delete_session(session3, init);

	affiche_sessions(init);*/

	return 0;
}

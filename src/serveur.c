#include "tools.h"

/* Variable pour stocker toutes les sessions en cours */
Session *init, *joining;
int 	session_started;

pthread_mutex_t mutex_init = 		PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_joining = 	PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_phase = 		PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_nb_tour = 	PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_is_timeout = 	PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_data_ref = 	PTHREAD_MUTEX_INITIALIZER;

int 	sock;
int 	nb_tour = 1;
PHASE 	phase;
pid_t 	main_pid;

int 	is_timeout_ref;
int 	is_timeout_enc;
int 	is_timeout_res;

int 	coups_ref;
char	username_ref[50];

/*
 * Handler de signal pour indiquer la fin de la phase de reflexion.
 */
void handler_reflexion(int sig) {
	struct itimerval itv; 
	sig++;
	fprintf(stderr, "Yay j'ai recu handler reflexion\n");
	if(sig == SIGINT) {
		// Desactiver le timer (qui est cense indiquer la fin de la phase).
		itv.it_value.tv_sec = 0;
		itv.it_value.tv_usec = 0;
	    itv.it_interval.tv_sec = 0;
	    itv.it_interval.tv_usec = 0;
		setitimer(ITIMER_REAL, &itv, (struct itimerval *)0);
		pthread_mutex_lock(&mutex_is_timeout); 
		is_timeout_ref = 1;
		pthread_mutex_unlock(&mutex_is_timeout);
	} else if(sig == SIGALRM) {
		/* Temps ecoule fin de la phase de reflexion */
		pthread_mutex_lock(&mutex_is_timeout); 
		is_timeout_ref = 0;
		pthread_mutex_unlock(&mutex_is_timeout);
	}
}

/*
 * Handler de signal pour indiquer la fin de la phase des encheres.
 */
void handler_encheres(int sig) {
	/* Reveiller le serveur (fin de la phase de reflexion) */
	sig++;
	fprintf(stderr, "Yay j'ai recu handler enchere\n");
}

/*
 * Handler de signal pour indiquer la fin de la phase de resolution.
 */
void handler_resolution(int sig) {
	/* Reveiller le serveur (fin de la phase de reflexion) */
	sig++;
	fprintf(stderr, "Yay j'ai recu handler resolution\n");
}


/*
 * Envoie a chaque client le score des clients connectes apres un tour.
 */
void send_enigme_and_bilan() {
	User *tmp, *user;
	char buff[50], *temp;

	pthread_mutex_lock (&mutex_init); 
	tmp = init->user;
	sprintf(buff, "TOUR/(2,2,3,4,7,5,6,6,9,3,R)/6(saucisse,3)(brouette,0)/\n");
	while(tmp != NULL) {
		user = init->user;
		while(user != NULL) {
			// sprintf(temp, "LIST/%s/%d/\n", user->username, user->score);
			// strcat(buff, temp);
			
			user = user->next;
			// Faire la chaine qui contiendra le score et le nom de tous les utilisateurs
		}	
		send(user->scom, buff, strlen(buff)+1, 0);
		tmp = tmp->next;
	}
	pthread_mutex_unlock (&mutex_init); 
}

/*
 * Phase de reflexion.
 */
int reflexion() {	
	int over;	
	sigset_t set;
	struct itimerval itv; 
	struct sigaction action;
	action.sa_handler = handler_reflexion; 
	sigemptyset(&action.sa_mask); 
	action.sa_flags = 0; 
	sigaction(SIGALRM, &action, (struct sigaction *) 0);
	itv.it_value.tv_sec = TEMPS_REFLEXION;
	itv.it_value.tv_usec = 0; 

	// Informer tout le monde du lancement de la phase de reflexion
	send_enigme_and_bilan();
	pthread_mutex_lock(&mutex_phase); 
	phase = REFLEXION;
	pthread_mutex_unlock(&mutex_phase);

	setitimer(ITIMER_REAL, &itv, (struct itimerval *)0);
	sigfillset(&set);
	sigdelset(&set, SIGINT);
	sigdelset(&set, SIGALRM);
	sigsuspend(&set);
	
	pthread_mutex_lock(&mutex_is_timeout);
	over = is_timeout_ref;
    pthread_mutex_unlock(&mutex_is_timeout);
	
	if(over == 0) {
		// Le temps ecoule sans avoir recu une proposition
		fprintf(stderr, "Yo tout le monde, la phase de reflexion is over.\n");

		
	} else {
		// Un client a propose une solution
		fprintf(stderr, "Un client annonce avoir trouve une solution.\n");
		// TUASTROUVE/ (S -> C)
		send(scom, "TUASTROUVE/\n", 13, 0);
		send_il_a_trouve(scom, username);
	}	

	
	return 0;
}

/*
 * Phase d'enche res.
 */
int enchere() {
	pthread_mutex_lock(&mutex_phase); 
	phase = ENCHERE;
	pthread_mutex_unlock(&mutex_phase);
	 
	return 0;
}

/*
 * Etape de la resolution d'un tour de l'enigme.
 */
int resolution() {
	pthread_mutex_lock(&mutex_phase); 
	phase = RESOLUTION;
	pthread_mutex_unlock(&mutex_phase);

	return 0;
}

void end_session() {
    User *user, *tmp;
    char buff[63];
	fprintf(stderr,"Ending session...no enough users\n");

    pthread_mutex_lock (&mutex_init);
    tmp = init->user;
    while(tmp != NULL) {
        sprintf(buff, "FINRESO/%s/\n", tmp->username);
        send(tmp->scom, buff, strlen(buff), 0);
        user = tmp;
        tmp = tmp->next;
        delete_user(user, init);
    }

    pthread_mutex_unlock (&mutex_init);
    fprintf(stderr, "Fin endsession\n");
}

void start_session() {
	fprintf(stderr,"Starting session...\n");
	User *tmp;
	char msg[100];

	pthread_mutex_lock(&mutex_init);
	tmp = init->user;
	sprintf(msg, "SESSION/(3,4,H)(3,4,G)(12,6,H)/\n");
	while(tmp != NULL) {
		send(tmp->scom, msg, strlen(msg)+1, 0);
		tmp = tmp->next;
	}
	pthread_mutex_unlock(&mutex_init);
}

/*
 * Informer tous les clients de la fin de la phase de reflexion.
 */
void fin_reflexion() {
	fprintf(stderr,"Fin reflexion...\n");
	User *tmp;

	pthread_mutex_lock(&mutex_init);
	tmp = init->user;
	while(tmp != NULL) {
		send(tmp->scom, "FINREFLEXION/\n", 15, 0);
		tmp = tmp->next;
	}
	pthread_mutex_unlock(&mutex_init);

}

void go() {
    /* Chaque tour de boucle constitue un Tour de jeu si une session a commence. */
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

		pthread_mutex_lock(&mutex_is_timeout); 
		is_timeout_ref = 0;
		pthread_mutex_unlock(&mutex_is_timeout);

        if(size < 2) {
        	end_session();
			pthread_mutex_lock(&mutex_nb_tour); 
	    	nb_tour = 1;	
			pthread_mutex_unlock(&mutex_nb_tour);
            	continue;
        } 

        printf("Starting...in 10sec\n");
        sleep(10);
        printf("Go...\n");

		pthread_mutex_lock(&mutex_nb_tour); 
	 	nb_tour++;
		pthread_mutex_unlock(&mutex_nb_tour);
       
        start_session();
		
		/* Phase de reflexion */
		reflexion();

		fin_reflexion();

		/* Phase d'encheres */
		enchere();

		/* Phase de resolution */
		resolution();

		
	}
}

/*
 * Permet de notifer les autres client de l'arrivee d'un nouveau,
 * puis envoyer au nouveau la liste des joueurs presents avec leurs score.
 */
int send_to_all(int scom, char *name) {
	User *tmp;
	char msg[100];
	pthread_mutex_lock (&mutex_init); 
	tmp = init->user;
	while(tmp != NULL) {
		// Notifier les clients deja connectes d'une nouvelle connexion
		// Et envoyer au nouvel arrivant les joueurs connectes.
		if(tmp->scom != scom) {
			sprintf(msg, "CONNECTE/%s/\n", name);
			send(tmp->scom, msg, strlen(msg)+1, 0);
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
		}
		tmp = tmp->next;
	}	
	pthread_mutex_unlock (&mutex_joining); 

	return 0;
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
	char tmp[63];
	User *user, *tmpU;

	if(get_username(buff, username) == -1) {
		fprintf(stderr, "[Serveur] Erreur lors de deconnexion. Command: %s\n", buff);
		return -1; 
	}
	fprintf(stderr, "Username : -> %s \n", username);

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
 * Ce cas arrive dans le cas ou un client quitte subitement une partie sans
 * informer le serveur.
 */
void disconnect_if_connected(int scom) {
	User *user;
	char buff[63];
	
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

void send_il_a_trouve(int scom, char* username) {
	User* tmp;
	char msg[63];
	sprintf(msg, "CONNECTE/%s/\n", username);

	pthread_mutex_lock (&mutex_init); 
	tmp = init->user;
	while(tmp != NULL) {
		// ILATROUVE/user/coups/ (S -> C)
		if(tmp->scom != scom) {
			
			send(tmp->scom, msg, strlen(msg)+1, 0);
		}
		tmp = tmp->next;
	}
	pthread_mutex_unlock (&mutex_init); 
}

/*
 * Traite un client lorsqu'un client annonce avoir trouve une solution.
 */
void client_trouve(int scom, char *buff) {
	char username[50];
	int coups;

	if(get_username_and_coups(buff, username, &coups) == -1) {
		fprintf(stderr, "[Seveur] Commande client inconnue: %s, count: %d\n", buff, (int)strlen(buff));
		send(scom, "Commande client inconnue.\n", 28, 0);
	} else {
		fprintf(stderr, "[Serveur] Cool command: %s\n", buff);
		pthread_mutex_lock (&mutex_data_ref); 
		strcpy(username_ref, username);
		coups_ref = coups;
		pthread_mutex_unlock(&mutex_data_ref); 
		kill(main_pid, SIGALRM);
	}
		
}

/*
 * Traite un client lorsqu'un client lorsqu'il enchere.
 */
void client_enchere(int scom, char *buff) {
	
}

/*
 * Traite la solution d'un client.
 */
void client_resolution(int scom, char *buff) {

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
			printf("[Serveur] Commande client inconnue.\n");
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
		case 4: /* TROUVE  */
			client_trouve(scom, buff);
			break;
		case 5: /* ENCHERE */
			client_enchere(scom, buff);
			break;
		case 6: /* PHASE RESOLUTION */
			client_resolution(scom, buff);
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
	
	main_pid = getpid();
	init = create_session(0);
	joining = create_session(0);
	session_started = 0;
	pthread_mutex_lock(&mutex_phase); 
	phase = UNDEF;
	pthread_mutex_unlock(&mutex_phase);
	pthread_mutex_lock(&mutex_is_timeout);
	is_timeout_ref = 0;
	is_timeout_enc = 0;
	is_timeout_res = 0;
    pthread_mutex_unlock(&mutex_is_timeout);

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

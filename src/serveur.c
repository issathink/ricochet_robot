#include "tools.h"

/* Variable pour stocker toutes les sessions en cours */
pthread_mutex_t mutex_init = 		PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_joining = 	PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_phase = 	PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_nb_tour = 	PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_is_timeout = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_data_ref = 	PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_data_enc = 	PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_data_sol = 	PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_is_playing = 	PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cond = 		PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_session = 		PTHREAD_COND_INITIALIZER; 

Session 	*init, *joining;
int 		session_started;
int		is_playing;

int 		sock;
int 		nb_tour = 1;
PHASE 	phase;
pid_t 	main_pid;

int 		is_timeout_ref;
int 		is_timeout_enc;
int 		is_timeout_res;

int 		scom_ref;
int 		coups_ref;
char		username_ref[50];

int 		scom_actif;
int 		coups_actif;
char*	depla_actif;
Enchere* init_enchere;
char		username_actif[50];
int 		game_over;

Plateau*	plateau;
Enigme*	enigme;

/*
 * Handler de signal pour indiquer la fin de la phase de reflexion.
 */
void handler_reflexion(int sig) {
	struct itimerval itv; 
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
	/* Reveil de serveur (fin de la phase des encheres) */
	sig++;
}

/*
 * Handler de signal pour indiquer la fin de la phase de resolution.
 */
void handler_resolution(int sig) {
	/* Reveil du serveur (fin de la phase de resolution) */
	struct itimerval itv;
	sig++;

	pthread_mutex_lock (&mutex_data_sol);
	if(is_timeout_res == 1) {
		itv.it_value.tv_sec = 0;
		itv.it_value.tv_usec = 0;
	        itv.it_interval.tv_sec = 0;
	        itv.it_interval.tv_usec = 0;
		setitimer(ITIMER_REAL, &itv, (struct itimerval *)0);
	}
	pthread_mutex_unlock (&mutex_data_sol);
}

/*
 * Envoie le message msg a tous les joueurs.
 */
void send_message(char *msg) {
	User* tmp;
	pthread_mutex_lock (&mutex_init); 
	tmp = init->user;
	while(tmp != NULL) {
		send(tmp->scom, msg, strlen(msg), 0);
		tmp = tmp->next;
	}
	pthread_mutex_unlock (&mutex_init);
}

/*
 * Envoie a chaque client le score des clients connectes apres un tour.
 */
void send_enigme_and_bilan() {
	User* user;
	char *msg, *bilan, *enig;
	
	pthread_mutex_lock (&mutex_init);
	bilan = get_bilan(init, nb_tour);
	enig = get_enigme();
	fprintf(stderr, "Enigme: %s\n", enig);
	sscanf(enig, "(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%c)", &enigme->xr, &enigme->yr, &enigme->xb, &enigme->yb,
			&enigme->xj, &enigme->yj, &enigme->xv, &enigme->yv, &enigme->xc, &enigme->yc, &enigme->c);
	msg = malloc(sizeof(char)*(strlen(enig) + strlen(bilan) + 9));
	user = init->user;
	sprintf(msg, "TOUR/%s/%s/\n", enig, bilan);
	while(user != NULL) {
		send(user->scom, msg, strlen(msg), 0);
		user = user->next;
	}
	pthread_mutex_unlock (&mutex_init);
	free(msg);
}

/*
 * Informe tous les clients de la propostion d'un joueur.
 */
void send_il_a_trouve(int scom, char* username, int coups) {
	User* tmp;
	char msg[70];
	sprintf(msg, "ILATROUVE/%s/%d/\n", username, coups);

	pthread_mutex_lock (&mutex_init); 
	tmp = init->user;
	while(tmp != NULL) {
		// ILATROUVE/user/coups/ (S -> C)
		if(tmp->scom != scom)
			send(tmp->scom, msg, strlen(msg), 0);
		tmp = tmp->next;
	}
	pthread_mutex_unlock (&mutex_init); 
}

/*
 * Notification de la fin de la phase des encheres.
 */
void send_fin_enchere() {
	User* tmp;
	// int scom;
	char msg[70];

	// fprintf(stderr, "[Serveur] Debut send fin enchere\n"); 
	pthread_mutex_lock(&mutex_data_sol);
	// scom = scom_actif;
	sprintf(msg, "FINENCHERE/%s/%d/\n", username_actif, coups_actif);
	pthread_mutex_unlock(&mutex_data_sol);

	pthread_mutex_lock (&mutex_init); 
	tmp = init->user;
	while(tmp != NULL) {
		// if(tmp->scom != scom) {
			send(tmp->scom, msg, strlen(msg), 0);
			// fprintf(stderr, "[Send_fin_enchere] J'envoie : %s\n", msg);
		// }
		tmp = tmp->next;
	}
	pthread_mutex_unlock (&mutex_init);
	// fprintf(stderr, "[Serveur] Fin send_fin_enchere\n");
}

/*
 * Phase de reflexion.
 */
int reflexion() {	
	int over, coups, scom;
	char username[50], msg[50];;	
	sigset_t set;
	struct itimerval itv; 
	struct sigaction action;
	action.sa_handler = handler_reflexion; 
	// sigemptyset(&action.sa_mask); 
	action.sa_flags = 0; 
	sigaction(SIGALRM, &action, (struct sigaction *) 0);
	sigaction(SIGINT, &action, (struct sigaction *) 0);
	itv.it_interval.tv_usec = 0;
	itv.it_interval.tv_sec = 0;
	itv.it_value.tv_sec = TEMPS_REFLEXION;
	itv.it_value.tv_usec = 0; 
	// fprintf(stderr,"Initialisation de la phase de reflexion \n");
	// Informer tout le monde du lancement de la phase de reflexion
	send_enigme_and_bilan();
	// fprintf(stderr,"Enigme et bilan envoye\n");
	pthread_mutex_lock(&mutex_phase); 
	phase = REFLEXION;
	pthread_mutex_unlock(&mutex_phase);

	if(setitimer(ITIMER_REAL, &itv, (struct itimerval *)0) < 0) { perror("setitimer"); }
	sigfillset(&set);
	sigdelset(&set, SIGINT);
	sigdelset(&set, SIGALRM);
	// fprintf(stderr, "[Serveur] 1 Reflexion sigsuspend\n");
	sigsuspend(&set);
	
	pthread_mutex_lock(&mutex_is_timeout);
	over = is_timeout_ref;
    	pthread_mutex_unlock(&mutex_is_timeout);
	
	if(over == 0) {
		// Le temps ecoule sans avoir recu une proposition
		// fprintf(stderr, "Yo tout le monde, la phase de reflexion is over.\n");
		/* Informer les utilisateurs de la fin de la phase d'encheres */
       	        sprintf(msg, "FINREFLEXION/\n");
                send_message(msg);
	} else {
		// Un client a propose une solution
		//fprintf(stderr, "Un client annonce avoir trouve une solution.\n");
		pthread_mutex_lock(&mutex_data_ref);
		scom = scom_ref;
		strcpy(username, username_ref);
		coups = coups_ref;
		pthread_mutex_unlock(&mutex_data_ref);
		// TUASTROUVE/	(S -> C)
		send(scom, "TUASTROUVE/\n", 12, 0);
		// ILATROUVE/user/coups/	(S -> C)
		send_il_a_trouve(scom, username, coups);
	}	
	//fprintf(stderr, "[Serveur] 1 Fin de la phase de reflexion\n");
	
	return 0;
}

/*
 * Phase d'encheres.
 */
int enchere() {
	sigset_t set;
	struct itimerval itv; 
	struct sigaction action;
	action.sa_handler = handler_encheres; 
	sigemptyset(&action.sa_mask); 
	action.sa_flags = 0; 
	sigaction(SIGALRM, &action, (struct sigaction *) 0);
	itv.it_interval.tv_usec = 0;
	itv.it_interval.tv_sec = 0;
	itv.it_value.tv_sec = TEMPS_ENCHERE;
	itv.it_value.tv_usec = 0; 

	// fprintf(stderr, "[Serveur] 2 Debut de la phase d'encheres\n");

	setitimer(ITIMER_REAL, &itv, (struct itimerval *)0);
	sigfillset(&set);
	sigdelset(&set, SIGINT);
	sigdelset(&set, SIGALRM);
	sigsuspend(&set);
	

	pthread_mutex_lock(&mutex_phase); 
	phase = RESOLUTION;
	pthread_mutex_unlock(&mutex_phase);

	// fprintf(stderr, "[Serveur] 2 Enchere apres sigsuspend et mutex_phase\n"); 
	
	// FINENCHERE/user/coups/	(S -> C)
	send_fin_enchere();
	// fprintf(stderr, "[Serveur] 2 Fin de la phase d'encheres\n");

	return 0;
}


void end_session() {
    	User *tmp;
    	char *msg, *bilan;
		// fprintf(stderr,"Ending session...no enough users\n");

    	pthread_mutex_lock (&mutex_init);
	bilan = get_bilan(init, nb_tour);
    	msg = malloc(sizeof(char)*strlen(bilan) + 13);    
    	sprintf(msg, "VAINQUEUR/%s/\n", bilan);
	tmp = init->user;
	while(tmp != NULL) {
		send(tmp->scom, msg, strlen(msg), 0);
		tmp = tmp->next;
	}
	vider_session(init);
	pthread_mutex_unlock (&mutex_init);
	pthread_mutex_lock (&mutex_joining);
	vider_session(joining);
	pthread_mutex_unlock (&mutex_joining);
	fprintf(stderr, "Fin end session\n");
	free(msg);
	exit(0);
}

void init_plateau() {
        char x[3], y[3], c;
        int i = 0, j= 0;
        x[2] = y[2] = 0;

	while(PLATEAU1[i] != '\0') {
		i++;
		while(PLATEAU1[i] != ',')
			x[j++] = PLATEAU1[i++];
		i++; j = 0;
		while(PLATEAU1[i] != ',')
			y[j++] = PLATEAU1[i++];
		i++; j = 0;
		c = PLATEAU1[i++];
		
		if(c == 'D') {
			plateau->cases[atoi(x)][atoi(y)].d = 1;
			plateau->cases[atoi(x)][atoi(y)+1].g = 1; // ajout
		} else if(c == 'H') {
			plateau->cases[atoi(x)][atoi(y)].h = 1;
			plateau->cases[atoi(x)-1][atoi(y)].b = 1; // ajout
		} else if(c == 'B') {
			plateau->cases[atoi(x)][atoi(y)].b = 1;
			plateau->cases[atoi(x)+1][atoi(y)].h = 1; // ajout
		} else if(c == 'G') {
			plateau->cases[atoi(x)][atoi(y)].g = 1;
			plateau->cases[atoi(x)][atoi(y)-1].d = 1; // ajout
		}
		i++; j = 0;
		fprintf(stderr, "x: %d, y: %d\n", atoi(x), atoi(y));
		memset(x, 0, 3);
		memset(y, 0, 3);
	}
}

void start_session() {
	// fprintf(stderr,"Starting session...\n");
	User *tmp;
	char* msg;
	int is_p;
	
        pthread_mutex_lock(&mutex_is_playing);
        is_p = is_playing;
        pthread_mutex_unlock(&mutex_is_playing);
	// Le jeu a deja commence pas besoin pas besoin d'envoyer le plateau		
	if(is_p == 1) return;
        init_plateau();
	pthread_mutex_lock(&mutex_init);
	tmp = init->user;
	// SESSION/plateau/	(S -> C)
	msg = malloc(sizeof(char)*(strlen(PLATEAU1)+11));
	sprintf(msg, "SESSION/%s/\n", PLATEAU1);
	while(tmp != NULL) {
		send(tmp->scom, msg, strlen(msg), 0);
		tmp = tmp->next;
	}
	pthread_mutex_unlock(&mutex_init);
	free(msg);
	// fprintf(stderr,"[Serveur] End start session function.\n");
}

/*
 * Etape de la resolution d'un tour de l'enigme.
 */
int resolution() {
	int scom, coups, time_out;
	char *buff, *dep, *message;
	sigset_t set;
	struct itimerval itv; 
	struct sigaction action;
	Enchere* enchere;
	User* tmp;
	action.sa_handler = handler_resolution; 
	sigemptyset(&action.sa_mask); 
	action.sa_flags = 0; 
	sigaction(SIGALRM, &action, (struct sigaction *) 0);
	sigaction(SIGUSR1, &action, (struct sigaction *) 0);
	itv.it_interval.tv_usec = 0;
	itv.it_interval.tv_sec = 0;
	itv.it_value.tv_sec = TEMPS_RESOLUTION;
	itv.it_value.tv_usec = 0; 
	dep = malloc(sizeof(char) * 10);
	message = malloc(sizeof(char) * 10);

	// Tant qu'il y a des joueurs qui ont fait des encheres
	pthread_mutex_lock(&mutex_data_sol);
	scom = scom_actif;
	coups = coups_actif;	
	pthread_mutex_unlock(&mutex_data_sol);
	pthread_mutex_lock(&mutex_init);
	delete_enchere(create_enchere(scom, coups), init);
	pthread_mutex_unlock(&mutex_init);
	
	while(scom != -1 && coups != -1) {
		setitimer(ITIMER_REAL, &itv, (struct itimerval *)0);
		sigfillset(&set);
		sigdelset(&set, SIGINT);
		sigdelset(&set, SIGUSR1);
		sigdelset(&set, SIGALRM);
		sigsuspend(&set);		
		pthread_mutex_lock(&mutex_data_sol);
		time_out = is_timeout_res;
		pthread_mutex_unlock(&mutex_data_sol);

		// Le client n'a pas propose une solution dans le temps imparti
		if(time_out == 0) {
			pthread_mutex_lock(&mutex_init);
			enchere = get_le_moins_offrant(init);
			if(enchere == NULL) {
				// fprintf(stderr, "Plus d'enchere time_out\n");
				pthread_mutex_unlock(&mutex_init);
				break;
			}
			tmp = cherche_user(init, enchere->scom);
			pthread_mutex_unlock(&mutex_init);

			if(tmp == NULL) break;
			message = realloc(message, strlen(tmp->username) + 12);
			sprintf(message, "TROPLONG/%s/\n", tmp->username);
			send_message(message);

			pthread_mutex_lock(&mutex_data_sol);
			scom = scom_actif = enchere->scom;
			coups = coups_actif = enchere->mise;
			pthread_mutex_unlock(&mutex_data_sol);
		} else {
			pthread_mutex_lock(&mutex_data_sol);
			scom = scom_actif;
			coups = coups_actif;
			dep = realloc(dep, 2*coups+1);
			strcpy(dep, depla_actif);
			is_timeout_res = 0;
			pthread_mutex_unlock(&mutex_data_sol);
			
			Enigme* enig = copy_of_enigme(enigme);
			if(solution_bonne(plateau, enig, dep)) {
				// BONNE/	(S -> C)	send_bonne_solution();
				pthread_mutex_lock(&mutex_init);
				tmp = cherche_user(init, scom);
				tmp->score++;
				if(tmp->score >= SCORE_OBJ) {
					end_session();
					game_over = 1;
				}
				pthread_mutex_unlock(&mutex_init);
			
				message = realloc(message, strlen(tmp->username) + 9);
				sprintf(message, "BONNE/%s/\n", tmp->username);
				send_message(message);
				free(enig);
				return 0;
			} else {
				// MAUVAISE/user/	(S -> C)
				// Rechercher le prochain utilisateur dans la liste des parieurs
				pthread_mutex_lock(&mutex_init);
				// fprintf(stderr, "++++++++++++++++++ Avant get le moins offrant\n");
				// affiche_enchere(init);
				enchere = get_le_moins_offrant(init);

				if(enchere == NULL) {
					// fprintf(stderr, "Plus d'enchere solution mauvaise scom: mise: \n");
					pthread_mutex_unlock(&mutex_init);
					break;
				}
				// fprintf(stderr, "------------------ Apres get le moins offrant\n");
				// affiche_enchere(init);
				tmp = cherche_user(init, enchere->scom);
				pthread_mutex_unlock(&mutex_init);
				if(tmp == NULL) break;

				message = realloc(message, strlen(tmp->username) + 11);
				sprintf(message, "MAUVAISE/%s/\n", tmp->username);
				send_message(message);
				// fprintf(stderr, "++++++++++++++++++ MAUVAISE\n");

				pthread_mutex_lock(&mutex_data_sol);
				scom = scom_actif = enchere->scom;
				coups = coups_actif = enchere->mise;
				pthread_mutex_unlock(&mutex_data_sol);
				free(enig);
			}
		}
	} 
    	buff = "FINRESO/\n";
        send_message(buff);    	
	free(dep);
	free(message);
	fprintf(stderr, "[Serveur] 3 Fin de la phase de resolution\n");
	return 0;
}

void go() {
	/* Chaque tour de boucle constitue un Tour de jeu si une session a commence. */
	while(1) {
		/* Rajouter les utilisateurs qui attendent */
		int size = 0;
		User *user, *tmp;
		// fprintf(stderr, "While go\n");
		pthread_mutex_lock(&mutex_data_sol);
		coups_actif = -1;
		scom_actif = -1;
		pthread_mutex_unlock(&mutex_data_sol);
		pthread_mutex_lock(&mutex_data_ref);
		coups_ref = -1;
		scom_ref = -1;
		pthread_mutex_unlock(&mutex_data_ref);

		pthread_mutex_lock (&mutex_init);
		pthread_mutex_lock (&mutex_joining);		
		user = joining->user;
		while(user != NULL) {
		        tmp = create_user(user->username, user->scom);
			add_user(tmp, init); 
			user = user->next;
		}
		size = init->size;

		// Liberer les donnees de la liste joining.
		vider_session(joining);
		pthread_mutex_unlock (&mutex_joining);
		vider_enchere(init);
		user = init->user;
		// Reinitialiser l'enchere pour le nouveau tour. 
		while(user != NULL) {
			user->coups = MAX_INT;
			user = user->next;
		}
		pthread_mutex_unlock (&mutex_init);

                pthread_mutex_lock(&mutex_is_playing);
		if(size < 2 && is_playing == 1) {
			is_playing = 0;
                        pthread_mutex_unlock(&mutex_is_playing);
			pthread_mutex_lock (&mutex_cond);
			// reset score
			user = init->user;
		        while(user != NULL) {
		                user->score = 0;
			        user = user->next;
		        }
			pthread_cond_wait(&cond_session, &mutex_cond);
			pthread_mutex_unlock (&mutex_cond);
			// end_session();
		        continue;
		} else if(size < 2) {
	                pthread_mutex_unlock(&mutex_is_playing);
			pthread_mutex_lock (&mutex_cond);			
			// fprintf(stderr, "Je vais etre bloque\n");
			pthread_cond_wait(&cond_session, &mutex_cond);
			pthread_mutex_unlock (&mutex_cond);
			continue;
		} else {
                        pthread_mutex_unlock(&mutex_is_playing);
			// fprintf(stderr, "Bah on peut commence a jouer\n");
		}

                pthread_mutex_lock(&mutex_is_playing);
		if(is_playing == 0) {
			pthread_mutex_lock(&mutex_nb_tour);
	 		nb_tour = 0;
			pthread_mutex_unlock(&mutex_nb_tour);
		}
                pthread_mutex_unlock(&mutex_is_playing);
		pthread_mutex_lock(&mutex_is_timeout); 
		is_timeout_ref = 0;
		pthread_mutex_unlock(&mutex_is_timeout);

		fprintf(stderr, "Starting... in 5sec\n");
		sleep(5);

		// Attente des retartadaires
		pthread_mutex_lock (&mutex_init);
		pthread_mutex_lock (&mutex_joining);		
		user = joining->user;
		while(user != NULL) {
			add_user(create_user(user->username, user->scom), init); 
			user = user->next;
		}
		size = init->size;
		vider_session(joining);
		pthread_mutex_unlock (&mutex_joining);
		pthread_mutex_unlock (&mutex_init);
		fprintf(stderr, "Go...\n");

		pthread_mutex_lock(&mutex_nb_tour);
		nb_tour++;
		pthread_mutex_unlock(&mutex_nb_tour);
                
		start_session();
		is_playing = 1;
		/* Phase de reflexion */
		reflexion();
		/* Phase d'encheres */
		enchere();
		/* Phase de resolution */
		resolution();
		fprintf(stderr, "[go] End tour.\n");
	}
}

/*
 * Permet de notifer les autres client de l'arrivee d'un nouveau,
 */
int send_to_all(int scom, char *name) {
	User *tmp;
	char msg[100], *message;
	int is_p;

	// CONNECTE/user/	(S -> C)
        sprintf(msg, "CONNECTE/%s/\n", name);
	pthread_mutex_lock (&mutex_init); 
	tmp = init->user;
	while(tmp != NULL) {
		// Notifier les clients deja connectes d'une nouvelle connexion
		if(tmp->scom != scom) 
			send(tmp->scom, msg, strlen(msg), 0);
		tmp = tmp->next;
	}
	pthread_mutex_unlock (&mutex_init); 
	pthread_mutex_lock (&mutex_joining);
	tmp = joining->user;
	while(tmp != NULL) {
		if(tmp->scom != scom)
			send(tmp->scom, msg, strlen(msg), 0);
		tmp = tmp->next;
	}	
	pthread_mutex_unlock (&mutex_joining);
	
	// Si le client arrive en plein milieu d'une session, lui enovoyer le plateau
	pthread_mutex_lock(&mutex_is_playing);
        is_p = is_playing;
        pthread_mutex_unlock(&mutex_is_playing);
	if(is_p == 1) {
	        message = malloc(sizeof(char)*(strlen(PLATEAU1)+11));
	        sprintf(message, "SESSION/%s/\n", PLATEAU1);
	        send(scom, message, strlen(message), 0);
	}
	return 0;
}

/*
 * Envoie a tous les clients l'enchere d'un nouveau client.
 */
void send_il_enchere(char* username, int scom, int coups) {
	User *tmp;
	char msg[100];

	// Je fais pas de lock parce que je l'ai locke
	tmp = init->user;
	sprintf(msg, "NOUVELLEENCHERE/%s/%d/\n", username, coups);
	while(tmp != NULL) {
		// Notifier les clients de l'enchere du client
		if(tmp->scom != scom)
			send(tmp->scom, msg, strlen(msg), 0);
		tmp = tmp->next;
	}
}

/*
 * Envoie la solution proposee par le joueur username.
 */
void send_sa_solution(int scom, char* username, char* deplacements) {
	User *tmp;
	char *msg;

	msg = malloc(sizeof(char)*(strlen(username)+strlen(deplacements)+14));
	pthread_mutex_lock (&mutex_init); 
	tmp = init->user;
	sprintf(msg, "SASOLUTION/%s/%s/\n", username, deplacements);
	while(tmp != NULL) {
		if(tmp->scom != scom)
			send(tmp->scom, msg, strlen(msg), 0);
		tmp = tmp->next;
	}
	pthread_mutex_unlock (&mutex_init);
	free(msg);
}

/*
 * Notifie les joueurs que la solution qu'ils ont recu est bonne.
 */
void send_bonne_solution() {
	User *tmp;
	char msg[10];

	sprintf(msg, "BONNE/\n");
	pthread_mutex_lock (&mutex_init); 
	tmp = init->user;
	while(tmp != NULL) {
		send(tmp->scom, msg, strlen(msg), 0);
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
	name = calloc(51, sizeof(char));

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

	pthread_mutex_lock (&mutex_init); 
	user = init->user;
	while(user != NULL) {
		if(strcmp(name, user->username) == 0) {
			pthread_mutex_unlock (&mutex_init); 
			send(scom, "ERREUR/Username already in use/\n", 32, 0);
			return -1;
		}
		user = user->next;
	}
	pthread_mutex_unlock (&mutex_init); 
	user = NULL;
	if((user = create_user(name, scom)) == NULL) {
		fprintf(stderr, "Cannot create user. Maybe there is no memory.\n");
		send(scom, "Cannot create user.\n", 21, 0);
		return -1;
	}
	pthread_mutex_lock (&mutex_cond); 
	pthread_cond_signal (&cond_session); 
	pthread_mutex_unlock (&mutex_cond); 
	pthread_mutex_lock (&mutex_joining);
	add_user(user, joining);
	pthread_mutex_unlock (&mutex_joining);		

	// BIENVENUE/user/	(S -> C)
	sprintf(msg, "BIENVENUE/%s/\n", name);
	send(scom, msg, strlen(msg), 0);
	send_to_all(scom, name);
	fprintf(stderr, "Successfully connected user: %s\n", name);
	free(name);
	return 0;
}

/*
 * Gere la commande de deconnexion d'un client.
 */
int deconnexion(int scom, char* buff) {
	char username[50];
	char tmp[65];
	User *user, *tmpU;

	if(get_username(buff, username) == -1) {
		fprintf(stderr, "[Serveur] Erreur lors de deconnexion. Command: %s\n", buff);
		return -1; 
	}
	// Envoyer a tout le monde que le client s'est deconnecte.
	pthread_mutex_lock (&mutex_init);
	tmpU = init->user;
	while(tmpU != NULL) {
		if(scom != tmpU->scom) {
			// SORTI/user/	(S -> C)
			sprintf(tmp, "DECONNEXION/%s/\n", username);
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
		pthread_mutex_unlock (&mutex_init);
		// Logique pour supprimer user de joining s'il y est bien sur.
		pthread_mutex_lock (&mutex_joining);
		tmpU = cherche_user(joining, user->scom);
		if(tmpU != NULL)
			delete_user(tmpU, joining);
		pthread_mutex_unlock (&mutex_joining);	
	}
	// Supprimer les encheres du joueur qui vient de se deconnecter.	
	pthread_mutex_lock (&mutex_init);
	delete_enchere(cherche_enchere(scom, init), init);
	pthread_mutex_unlock (&mutex_init);
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
		fprintf(stderr, "Is user null: %d %s& scom : %d;\n", (user == NULL), user->username, user->scom);
		sprintf(buff, "SORT/%s/\n", user->username);
		deconnexion(scom, buff);
	} else {
		fprintf(stderr, "Je sais pas user n'est ni dans joining ni dans init.\n");
	}
}

/*
 * Traite un client lorsqu'un client annonce avoir trouve une solution.
 */
void client_trouve(int scom, char *buff) {
	char username[50];
	int coups;
	User* user;
	Enchere* enchere;

	if(get_username_and_coups(buff, username, &coups) == -1) {
		fprintf(stderr, "[Seveur] Commande client inconnue: %s, count: %d\n", buff, (int)strlen(buff));
		send(scom, "Commande client inconnue.\n", 28, 0);
	} else {
		fprintf(stderr, "[Serveur] Cool command: %s\n", buff);
		pthread_mutex_lock(&mutex_init);
		user = cherche_user(init, scom);
		if(user != NULL) {
			user->coups = coups;
			pthread_mutex_unlock(&mutex_init);
			pthread_mutex_lock(&mutex_phase);
			if(phase == REFLEXION) {
				phase = ENCHERE;
				pthread_mutex_unlock(&mutex_phase); 
				pthread_mutex_lock (&mutex_data_ref); 
				strcpy(username_ref, username);
				coups_ref = coups;
				scom_ref = scom;
				pthread_mutex_unlock(&mutex_data_ref);
				pthread_mutex_lock(&mutex_data_sol);
				coups_actif = coups;
				scom_actif = scom;
				strcpy(username_actif, username);
				pthread_mutex_unlock(&mutex_data_sol);
				enchere = create_enchere(scom, coups);
				add_enchere(enchere, init);	
				// Notifier le thread main de la terminaison 
				kill(main_pid, SIGINT);
			} else {
				pthread_mutex_unlock(&mutex_phase); 
				send(scom, "Commande client inconnue.\n", 28, 0);
			}
		} else {
			pthread_mutex_unlock(&mutex_init);		
			send(scom, "Commande client inconnue.\n", 28, 0);
		}
	}
}

/*
 * Traite un client lorsqu'un client lorsqu'il enchere.
 */
void client_enchere(int scom, char* buff) {
	char username[50], tmp[65];
	int coups;
	User* user;
	Enchere* enchere;

	pthread_mutex_lock(&mutex_phase);
	if(phase != ENCHERE) {
		pthread_mutex_unlock(&mutex_phase);
		return;
	}
	pthread_mutex_unlock(&mutex_phase);
	
	if(get_username_and_coups(buff, username, &coups) == -1) {
		fprintf(stderr, "[Seveur] Commande client inconnue: %s, count: %d\n", buff, (int)strlen(buff));
		send(scom, "Commande client inconnue.\n", 28, 0);
	} else {
		pthread_mutex_lock(&mutex_init);
		pthread_mutex_lock(&mutex_data_sol);
		user = cherche_user(init, scom);
		fprintf(stderr, "Coups: %d coups_actif: %d\n", coups, coups_actif);
		if(coups < coups_actif) {
			user->coups = coups;
			coups_actif = coups;
			scom_actif = scom;
			strcpy(username_actif, username);
			enchere = create_enchere(scom, coups);
			// fprintf(stderr, "-------------ADD scom: %d mise: %d -----------\n", scom, coups);
			add_enchere(enchere, init);					
			// fprintf(stderr, "------------------------\n");

			// TUENCHERE/	(S -> C)
			send(scom, "VALIDATION/\n", 12, 0);
			// ILENCHERE/user/coups/	(S -> C)
			send_il_enchere(user->username, scom, coups);
		} else {
		    if(coups < user->coups) {
				user->coups = coups;
				enchere = create_enchere(scom, coups);
				add_enchere(enchere, init);
				affiche_enchere(init);
				// TUENCHERE/	(S -> C)
				send(scom, "VALIDATION/\n", 12, 0);
				// ILENCHERE/user/coups/	(S -> C)
				send_il_enchere(user->username, scom, coups);
			} else {
				// ECHECENCHERE/user/	(S -> C)
				sprintf(tmp, "ECHEC/%s/\n", user->username);
				send(scom, tmp, strlen(tmp), 0);
			}
		}
		pthread_mutex_unlock(&mutex_data_sol);
		pthread_mutex_unlock(&mutex_init);
	}
}

/*
 * Traite la solution d'un client.
 */
void client_resolution(int scom, char* buff) {
	char username[50], *deplacements;
	int coups_a, scom_a;

	pthread_mutex_lock(&mutex_data_sol);
	coups_a = coups_actif;
	scom_a = scom_actif;
	pthread_mutex_unlock(&mutex_data_sol);
	deplacements = malloc(sizeof(char)*(2*coups_a+1));
	if(scom_a != scom) {
		fprintf(stderr, "[Serveur] Mauvais utilisateur essaye d'envoyer une resolution");
		return;
	}
	// fprintf(stderr, "COups: %d\n", coups_a);
	if(get_username_and_deplacements(buff, username, deplacements, coups_a) == -1) {
		fprintf(stderr, "[Seveur] Commande client inconnue: %s, count: %d\n", buff, (int)strlen(buff));
		send(scom, "Commande client inconnue.\n", 26, 0);
	} else {	
		pthread_mutex_lock(&mutex_data_sol);
		is_timeout_res = 1;
		depla_actif = realloc(depla_actif, strlen(deplacements)+1);
		strcpy(depla_actif, deplacements);
		pthread_mutex_unlock(&mutex_data_sol);
		// SASOLUTION/user/deplacements/	(S -> C)
		send_sa_solution(scom, username, deplacements); 
		// Envoyer un SIGUSR1 au process pere pour l'informer de la solution
		kill(main_pid, SIGUSR1);
	}
	free(deplacements);
}

/*
 * Pour broadcaster le message a tout le monde.
 */ 
void client_chat(int scom, char* buff) {
	char* name, *message, chat[200];
	User* user;
	name = calloc(50, sizeof(char));
	message = calloc(SMS_SIZE+1, sizeof(char));
	
	if(get_username_and_deplacements(buff, name, message, SMS_SIZE/2) == -1) {
		fprintf(stderr, "[Serveur] Commande client inconnue cmd: %s.\n", buff);		
	}
	// fprintf(stderr, "[Serveur] Je vais broadcaster le sms venant de : %s | %s\n", name, message);
	sprintf(chat, "CHAT/%s/%s/\n", name, message);
	pthread_mutex_lock(&mutex_init); 	
	user = cherche_user(init, scom);
	if(user != NULL) {
		user = init->user;
		while(user != NULL) {
			send(user->scom, chat, strlen(chat), 0);
			user = user->next;
		}
	}
	pthread_mutex_unlock(&mutex_init);
	pthread_mutex_lock(&mutex_joining); 
	user = joining->user;
	while(user != NULL) {
		send(user->scom, chat, strlen(chat), 0);
		user = user->next;
	}
	pthread_mutex_unlock(&mutex_joining);
	free(message);
	free(name);
}

int main() {
	pthread_t t_id;
	int i,j;
	
	main_pid = getpid();
	init = create_session(0);
	joining = create_session(0);
	session_started = 0;
	coups_actif = -1;
	coups_ref = -1;
	is_playing = 0;
	game_over = 0;
	init_enchere = NULL;
	depla_actif = malloc(sizeof(char) * 10);
	pthread_mutex_lock(&mutex_phase); 
	phase = UNDEF;
	pthread_mutex_unlock(&mutex_phase);
	pthread_mutex_lock(&mutex_is_timeout);
	is_timeout_ref = 0;
	is_timeout_enc = 0;
	is_timeout_res = 0;
        pthread_mutex_unlock(&mutex_is_timeout);
        plateau = malloc(sizeof(Plateau));
        // plateau->cases = malloc(sizeof(caze)*NB_CASES*NB_CASES);
        enigme = malloc(sizeof(Enigme));
        R = 10;
        B = 11;
        J = 12;
        V = 13;
        for(i=0;i<NB_CASES; i++){
    	        for(j=0;j<NB_CASES;j++) {
    		        plateau->cases[i][j].h = 0;
            		plateau->cases[i][j].d = 0;
            		plateau->cases[i][j].b = 0;
            		plateau->cases[i][j].g = 0;
            		
            		if(i == 0)
            			plateau->cases[i][j].h = 1;
            		if(i == NB_CASES-1)
            			plateau->cases[i][j].b = 1;
            		if(j == 0)
            			plateau->cases[i][j].g = 1;
            		else if(j == NB_CASES - 1)
            			plateau->cases[i][j].d = 1;
            	}
        }
        enigme->xr = enigme->yr = -1;
   	enigme->xb = enigme->yb = -1;
        enigme->xj = enigme->yj = -1;
        enigme->xv = enigme->yv = -1;
        enigme->c = -1;
    	
	if (pthread_create(&t_id, NULL, listen_to_clients, NULL) != 0) {
		fprintf(stderr, "[Serveur] Erreur thread.\n");
	}
	go();
	close(sock);

	return 0;
}

#include "tools.h"

/* Traite un client lorsqu'un client annonce avoir trouve une solution. */
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

/* Traite un client lorsqu'un client lorsqu'il enchere. */
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
		
		enchere = cherche_enchere_valeur(coups, init);
		if(enchere != NULL) {
		        fprintf(stderr, "[client_enchere] Coups : %d a deja ete encheri par scom: %d\n", coups, enchere->scom);
		        sprintf(tmp, "ECHEC/%s/\n", user->username);
			send(scom, tmp, strlen(tmp), 0);
			pthread_mutex_unlock(&mutex_data_sol);
		        pthread_mutex_unlock(&mutex_init);
		        return;
		}
		
		if(coups < coups_actif) {
			user->coups = coups;
			coups_actif = coups;
			scom_actif = scom;
			strcpy(username_actif, username);
			enchere = create_enchere(scom, coups);
			add_enchere(enchere, init);					

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

/* Traite la solution d'un client. */
void client_resolution(int scom, char* buff) {
	char username[50], *deplacements, *msg;
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

	        msg = malloc(sizeof(char)*(strlen(username)+strlen(deplacements)+14));
        	sprintf(msg, "SASOLUTION/%s/%s/\n", username, deplacements);
		send_message_except(msg, scom);
		// Envoyer un SIGUSR1 au process pere pour l'informer de la solution
		kill(main_pid, SIGUSR1);
	}
	free(deplacements);
}

/* Pour broadcaster le message a tout le monde. */ 
void client_chat(int scom, char* buff) {
	char* name, *message, chat[200];
	User* user;
	name = calloc(50, sizeof(char));
	message = calloc(SMS_SIZE+1, sizeof(char));
	
	if(get_username_and_deplacements(buff, name, message, SMS_SIZE/2) == -1) {
		fprintf(stderr, "[Serveur] Commande client inconnue cmd: %s.\n", buff);		
	}
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

/*
 * Handler de signal pour indiquer la fin de la phase de reflexion.
 */
void handler_reflexion(int sig) {
	struct itimerval itv; 
	
	if(sig == SIGUSR2) {
	        // faire un exec
	        execl("/bin/serveur", "0", NULL);
	}
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
	if(sig == SIGUSR2) {
	      	// faire un exec
	        execl("/bin/serveur", "0", NULL);
	}
}

/*
 * Handler de signal pour indiquer la fin de la phase de resolution.
 */
void handler_resolution(int sig) {
	/* Reveil du serveur (fin de la phase de resolution) */
	struct itimerval itv;
	if(sig == SIGUSR2) {
	        // faire un exec
	        execl("/bin/serveur", "0", NULL);
	}

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

/* Self explanatory function. */
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

/* Attente et gestion des commandes d'un client (Thread). */
void* handle_client(void* arg) {
	char buff[LINE_SIZE];
	int scom;
	ssize_t count;
	scom = *((int *) arg);
	
	if (pthread_detach (pthread_self()) !=0 ) {
 		perror("pthread_detach"); pthread_exit ((void *)1);
 	}

	while(1) {
		if( (count=recv(scom, buff, LINE_SIZE, 0)) < 0) { perror("read"); break; }

		if(strlen(buff) <= 1) {
			fprintf(stderr, "[Serveur] Commande client inconnue.\n");
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
			fprintf(stderr, "[Serveur] Commande pas cense etre la.\n");
			break;
		case 4: /* TROUVE  */
			client_trouve(scom, buff);
			fprintf(stderr, "[Handle_client] Why are you still calling this old protocol ?\n");
			break;
		case 5: /* ENCHERE */
			client_enchere(scom, buff);
			break;
		case 6: /* SOLUTION */
		{	
			pthread_mutex_lock(&mutex_phase); 
			if(phase == REFLEXION) {
				pthread_mutex_unlock(&mutex_phase);
				client_trouve(scom, buff);
			} else if(phase == RESOLUTION) {
				pthread_mutex_unlock(&mutex_phase);
				client_resolution(scom, buff);
			} else {
				fprintf(stderr, "[Handle_client] Cmd: %s recu au phase: %d\n", buff, phase);
				pthread_mutex_unlock(&mutex_phase);
			}
			break;
		}
		case 7:
			client_chat(scom, buff);
			break;
		default:
			fprintf(stderr, "[Serveur] Something really weird happened.\n");
			break;
		}
		memset(buff, 0, LINE_SIZE);
	}
	disconnect_if_connected(scom);
	close(scom);
	pthread_exit(0); 
}

/* Attend la connexion d'un nouveau client et cree un thread pour le gerer ses commandes. */ 
void* listen_to_clients(void* arg) {
	int scom, fromlen, *pt_scom;
	struct sockaddr_in sin, exp;
	pthread_t conn_id;

	arg++;
	if( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { perror("socket"); exit(1); }
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(SERVER_PORT);
	sin.sin_family = AF_INET;
	fromlen = sizeof(struct sockaddr_in);
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
    	perror("setsockopt(SO_REUSEADDR) failed");
		exit(1);
	}
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) { perror("bind"); exit(1); }	
	if (listen(sock, SOMAXCONN) < 0) { perror("listen"); exit(1); }
	fprintf(stderr, "Server ready...\n");

	while(1) {
		if ((scom = accept(sock, (struct sockaddr *)&exp, (socklen_t *)&fromlen)) < 0) { perror("accept"); exit(1); }
		pt_scom = (int *) malloc(sizeof(int));
		*pt_scom = scom;
		if (pthread_create(&conn_id, NULL, handle_client, (void *)pt_scom) != 0) {
			fprintf(stderr, "[Serveur] Erreur thread.\n");
		}
	}
}


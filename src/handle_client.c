#include "tools.h"


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




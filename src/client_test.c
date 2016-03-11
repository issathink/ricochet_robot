#include "tools.h"

int sock;

char *get_line(void) {
	char * line = malloc(COMMAND_SIZE), * linep = line;
	size_t lenmax = COMMAND_SIZE, len = lenmax;
	int c;
	char * linen;

	if(line == NULL)
	return NULL;

	for(;;) {
		c = fgetc(stdin);
		if(c == EOF)
		break;

        	if(--len == 0) {
			len = lenmax;
		    	linen = realloc(linep, lenmax *= 2);

		   	if(linen == NULL) {
		        	free(linep);
		        	return NULL;
		    	}
		    	line = linen + (line - linep);
		    	linep = linen;
		}

		if((*line++ = c) == '\n')
			break;
	}
	*line = '\0';
	return linep;
}


int decode_command(char *str) {
	if(str[0] == 'L' || str[0] == 'l')
		return 1;
	else if(str[0] == 'U' || str[0] == 'u')
		return 2;
	else if(str[0] == 'D' || str[0] == 'd')
		return 3;

	return 0;
}


void list(char *buff) {
	char response[LINE_SIZE];

	if (send(sock, buff, strlen(buff), 0) < 0) { perror("[ftp_client] send list request"); exit(1); }
	if (recv(sock, response, LINE_SIZE, 0) < 0) { perror("[ftp_client] recv list request"); exit(1); }

	printf("[Client] List of files : \n%s\n", response);
}

void download(char *buff) {
	FILE *file;
	char file_name[COMMAND_SIZE], t_file[COMMAND_SIZE];
	char content[LINE_SIZE];
	int i = 0, j = 0, taille_fic, count, actual = 0;

	if (send(sock, buff, strlen(buff), 0) < 0) { perror("[ftp_client] download request"); exit(1); }

	while(buff[i] != ' ') i++;
	i++;
	while(buff[i] != '\0') { file_name[j] = buff[i]; i++; j++; }
	file_name[j] = '\0';

	file = fopen(file_name, "w+");
	if(file == NULL) { fprintf(stderr, "[Client] Le fichier '%s' n'a pas pu être ouvert.\n", file_name); exit(1); }

	i = 0;
	if(recv(sock, content, sizeof(int), 0) > 0) {
		/* while(content[i] != ';') { t_file[i] = content[i]; i++; } */
		taille_fic = atoi(content);
	}
	printf("CLient lecture : %s\n", content);
	printf("Client taille fichier: %d\n", taille_fic);
	memset(content, 0, LINE_SIZE);
	
	while ( (count = recv(sock, content, LINE_SIZE, 0)) > 0) {
		printf("Client down: %s\n", content);
		fwrite(content, sizeof(char), strlen(content), file);
		actual += strlen(content);
		if(actual >= taille_fic) 
			break;
		memset(content, 0, LINE_SIZE);
	}

	fclose(file);
	printf("[Client] Fin download.\n");
}

void upload(char *buff) {
	FILE *file;
	struct stat st;
	char file_name[COMMAND_SIZE], taille_fic[COMMAND_SIZE];
	char content[LINE_SIZE];
	int i = 0, j = 0;
	
	while(buff[i] != ' ') i++;
	i++;
	while(buff[i] != 0) { file_name[j] = buff[i]; i++; j++; }
	file_name[j] = 0;

	stat(file_name, &st);
	sprintf(taille_fic, ";%d;", (int)st.st_size);
	strcat(buff, taille_fic);

	if (send(sock, buff, strlen(buff), 0) < 0) { perror("[ftp_client] upload request"); exit(1); }

	printf("Filename to upload: %s\n", file_name); 
	file = fopen(file_name, "r");
	if(file == NULL) { fprintf(stderr, "[Client] Le fichier '%s' n'a pas pu être ouvert.\n", file_name); exit(1); }

	while(! feof(file)) {
		if(fgets (content, LINE_SIZE, file) != NULL) {
			content[strlen(content)] = '\0';
			send(sock, content, strlen(content), 0);
		}
		memset(content, 0, LINE_SIZE);
	}
	printf("[Client] Fin upload.\n");
}


int main(int argc, char **argv) {
	struct sockaddr_in dest;
 	int fromlen, PORTSERV;
	char buff[COMMAND_SIZE];
	
	if(argc < 2) { perror("arguments"); exit(1); }
	
	PORTSERV = atoi(argv[2]);
	fromlen = sizeof(dest);

	dest.sin_addr.s_addr = inet_addr(argv[1]);
	dest.sin_port = htons(PORTSERV);
	dest.sin_family = AF_INET;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { perror("socket"); exit(1); }
	if (connect(sock, (struct sockaddr *)&dest, fromlen) < 0) { perror("connect client"); exit(1); }

	while(1) {
		strcpy(buff, get_line());
		
		if(strlen(buff) <= 1)
			break;
		
		buff[strlen(buff) - 1] = 0; /* Remplacer le caractere de fin de ligne. */

		switch(decode_command(buff)) {
			case 1:
				list(buff);
				break;
			case 2:
				upload(buff);
				break;
			case 3:
				download(buff);
				break;
			default:
				printf("[Client] Commande non reconnue. Reessayer..\n");
				break;
		}
		memset(buff, 0, COMMAND_SIZE);
	}

	if (send(sock, &buff[0], sizeof(char), 0) < 0) { perror("[ftp_client] end requests"); exit(1); }
	close(sock); 

	printf("ftp_client fin.\n");
	return 0;
}

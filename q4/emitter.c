#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define BUFFER_LENGTH 1024

int main(int argc, char* argv[]){

	//Controle des arguments du programme
	if (argc != 6){
		fprintf(stderr , "usage : %s <fichier_a_envoyer> <fichier_recu> <adr_IP_dist> <port_dist> [<port_local>]\n", argv[0]);
		exit(1);
	}

	//Clear console
	system("clear");
	printf("----- CLIENT UDP -----\n");

	//Déclaration des variables
	int fd;							//Descripteur du socket
	struct sockaddr_in adr; 		//Adresse distante
	struct sockaddr_in adrLocale;	//Adresse locale
	char buffer[BUFFER_LENGTH];		//buffer
	char bufAck[1];					//buffer pour l'acquittement
	char tmpBuffer[BUFFER_LENGTH-1];//buffer - sequence et ack
	char seqAck						//sequence et ack
	int output_fd;					//Descripteur du fichier receveur
	int input_fd;					//Descripteur du fichier à lire
	int nbRecv;						//Check variable
	int nb_lu;						//Check variable
	int reception;					//Check variable
	int emission;					//Check variable
	fd_set rfds;					//Listes des descripteurs
	struct timeval timer;

	//Création du socket
	printf("\nCréation du socket ...\n");
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1){
		perror("-> Echec de la création, fin du programme.\n\n");
		exit(1); 
	}else{
		printf("-> Succes de la création !\n");
	}

	//Préparation de l'adresse locale
	printf("\nPréparation de l'adresse locale ...\n");
	adrLocale.sin_family = AF_INET;					//Protocol
	adrLocale.sin_addr.s_addr = htonl(INADDR_ANY);	//Adresse IP distante
	adrLocale.sin_port = htons(atoi(argv[5]));		//numéro de port choisi
	socklen_t addrlen = sizeof(adr);  		//longueur de l'adresse
	printf("-> Préparation terminée !\n");

	//Attache de la socket
	printf("\nAttache de la socket ...\n");
	if (bind(fd, (struct sockaddr*)&adrLocale, sizeof(adrLocale)) == -1){
		perror("-> Echec de l'attachement, fin du programme.\n\n");
		exit(1);
	}else{
		printf("-> Succes de l'attachement !\n");
	}

	//Préparation de l'adresse distante
	printf("\nPréparation de l'adresse distante ...\n");
	adr.sin_family = AF_INET;						//Type de protocol
	inet_pton(AF_INET, argv[3], & (adr.sin_addr)); 	//Adresse ip du serveur
	adr.sin_port = htons(atoi(argv[4]));			//numéro de port choisi
	printf("-> Préparation terminée !\n");

	//Envoi d'un datagramme de connection vide
	printf("\nPhase de connexion ...\n");
	if(sendto(fd, &buffer, 0, 0, (struct sockaddr*) &adr, addrlen) == -1){
		perror("-> Echec de la phase de connexion, fin du programme.\n\n");
		exit(1);
	}else{
		//Attente du retour serveur
		printf("-> Attente du retour serveur ...\n");
		if (recvfrom(fd, &buffer, BUFFER_LENGTH, 0, (struct sockaddr*) &adrLocale, &addrlen) == -1){
			perror("-> Echec de la phase de connexion, fin du programme.\n\n");
			exit(1);
		}
	}
	emission = 1;
	reception = 1;
	printf("-> Fin de la phase de connexion.\n");

	//Ouverture du fichier à lire
	printf("\nOuverture du fichier à lire ...\n");
	input_fd = open(argv[1], O_RDONLY);
	if(input_fd == -1){	
		perror("-> Echec de l'ouverture, fin du programme.\n"); 
		exit(1); 		
	}else{
		printf("-> Succes de l'ouverture !\n");
	}

	//Ouverture du fichier recepteur
	printf("\nCréation du fichier receveur ...\n");
	output_fd = open(argv[2], O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR);
	if(output_fd == -1){	
		perror("-> Echec de la création, fin du programme.\n\n");
		exit(1); 		
	}else{
		printf("-> Succes de la création !\n");
	}

	//Envoi et reception des fichiers
	//TODO Faire les modulo du ack
	seqAck = 0;
	printf("\nEnvoi et reception des fichiers ...\n");
	while (1 == 1){
		//Ajout dans la liste du descripteur de la socket
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		//Initialisation du timer pour les acquittement
		timer.tv_sec = 1;
		timer.tv_usec = 0;

		int nbFd = fd+1;

		if (emission == 1){
			nb_lu = read(input_fd, tmpBuffer, BUFFER_LENGTH-1);
			//Création du buffer pour le ack et le message
			buffer[0] = (char)seqAck;
			strncat(buffer, tmpBuffer, BUFFER_LENGTH-1);
			if (nb_lu == -1){
				printf("-> Echec de la lecture d'un bloc, fin du programme.\n\n");
			}else{
				printf("-> Envoi d'un bloc (taille : %d) ...\n",nb_lu);
				sendto (fd, &buffer, nb_lu, 0, (struct sockaddr*) &adr, addrlen);
				if (nb_lu == 0)
					emission = 0;
				else{
					while (1 == 1){	//Tant que l'on a pas le bon ack
						//Select pour l'attente d'un message avec res
						int res = select(nbFd, &rfds, NULL, NULL, &timer);

						//Cas de l'erreur
						if (res == -1){
							perror("-> Echec du select, fin du programme.\n\n");
							exit(1);
						}else if(res){	//Si on recoi un message
							recvfrom(fd, &bufAck, 1, 0, (struct sockaddr*) &adrLocale, &addrlen);
							if (bufAck[0] == (char)((seqAck+1)%2){	//Si l'ack est le bon
								seqAck ++;					//On peux passer à la suite
								break;
							}else{	//Sinon on renvoi le buffer
								sendto (fd, &buffer, nb_lu, 0, (struct sockaddr*) &adr, addrlen);
							}
						}else{	//Apres le timeout renvoi
							sendto (fd, &buffer, nb_lu, 0, (struct sockaddr*) &adr, addrlen);
						}
					}
				}
			}
		}
		
		if (reception == 1){
			nbRecv = recvfrom(fd, &buffer, BUFFER_LENGTH, 0, (struct sockaddr*) &adrLocale, &addrlen);
			if (nbRecv == 0){
				reception = 0;
			}
			if ( nbRecv == -1){
				perror("-> Echec de la reception, fin du programme.\n\n");
			}else{
				printf("-> Reception d'un bloc (taille : %d)...\n",nbRecv);
				write(output_fd, buffer, nbRecv);
			}
		}

		if(emission == 0 && reception == 0){
			break;
			printf("-> Fin de reception.\n");
		}
	}

	//Fermeture de la socket
	printf("\n\nFermeture de la socket ...\n");
	if (close(fd) == -1){
		perror("-> Echec de la fermeture, fin du programme.\n\n");
		exit(1);
	}else{
		printf("-> Succes de la fermeture !\n");
	}

	//Fermeture du fichier
	printf("\nFermeture des fichiers ...\n");
	if (close(input_fd) == -1){
		perror("-> Echec de la fermeture, fin du programme.\n\n");
		exit(1);
	}else{
		printf("-> Succes de la fermeture !\n");
	}
	if (close(output_fd) == -1){
		perror("-> Echec de la fermeture, fin du programme.\n\n");
		exit(1);
	}else{
		printf("-> Succes de la fermeture !\n");
	}

	printf("\n\nFin du programme.\n\n");

	return 1;
}
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#define BUFFER_LENGTH 1024

int main(int argc, char* argv[]){

	//controle des arguments
	if (argc != 4){
		fprintf(stderr , "usage : %s <fichier_a_envoyer> <fichier_recu> <port_local>\n", argv[0]);
		exit(0);
	}

	//Clear console
	system("clear");
	printf("----- SERVEUR UDP -----\n");

	//Déclaration des variables
	int fd;							//Descripteur de la socket
	struct sockaddr_in adr; 		//Adresse distante
	struct sockaddr_in adrLocale; 	//Adresse locale
	int output_fd;					//Descripteur du fichier receveur
	int input_fd;					//Descripteur du fichier à lire
	char buffer[BUFFER_LENGTH];		//Le buffer recevant le message
	char bufToWrite[BUFFER_LENGTH-1];//Le buffer à écrire
	char newBTW[BUFFER_LENGTH-1];		//Le buffer recevant le message
	char bufAck[1];					//buffer pour l'acquittement
	char ipRecu[INET_ADDRSTRLEN];	//La chaine contenant l'adresse src
	int nbRecv;						//Check variable
	int nb_lu;						//Check variable
	int reception;					//Check variable
	int emission;					//Check variable
	int connected;					//Si le serveur est considéré comme connected
	int estPareilBuffer;			//Pour savoir si le buffer arrivé est le même que l'ancien ou non

	//Création du socket
	printf("\nCréation du socket ...\n");
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1){
		perror("-> Echec de la création, fin du programme.\n\n");
		exit(0); 
	}else{
		printf("-> Succes de la création !\n");
	}

	//Préparation de l'adresse locale
	printf("\nPréparation de l'adresse locale ...\n");
	adrLocale.sin_family = AF_INET;					//Protocol
	adrLocale.sin_addr.s_addr = htonl(INADDR_ANY);	//Adresse IP distante
	adrLocale.sin_port = htons(atoi(argv[3]));		//numéro de port choisi
	socklen_t addrlen = sizeof(adrLocale);  	//Longeur de l'adresse
	printf("-> Préparation terminée !\n");

	//Attache de la socket
	printf("\nAttache de la socket ...\n");
	if (bind(fd, (struct sockaddr*)&adrLocale, sizeof(adrLocale)) == -1){
		perror("-> Echec de l'attachement, fin du programme.\n\n");
		exit(0);
	}else{
		printf("-> Succes de l'attachement !\n");
	}

	//Ouverture du fichier à lire
	printf("\nOuverture du fichier à lire ...\n");
	input_fd = open(argv[1], O_RDONLY);
	if(input_fd == -1){	
		perror("-> Echec de l'ouverture, fin du programme.\n"); 
		exit(0); 		
	}else{
		printf("-> Succes de l'ouverture !\n");
	}

	//Ouverture du fichier recepteur
	printf("\nCréation du fichier receveur ...\n");
	output_fd = open(argv[2], O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR);
	if(output_fd == -1){	
		perror("-> Echec de la création, fin du programme.\n\n");
		exit(0); 		
	}else{
		printf("-> Succes de la création !\n");
	}

	//Envoi et reception des fichiers
	printf("\nDébut des échanges ...\n");

	//Préparation de l'adresse distante reception d'un datagramme vide
	printf("\nPhase de connexion ...\n");
	if (recvfrom(fd, &buffer, BUFFER_LENGTH, 0, (struct sockaddr*) &adrLocale, &addrlen) == -1){
		perror("-> Echec de la phase de connexion, fin du programme.\n\n");
		exit(0);
	}else{
		inet_ntop(AF_INET ,&adrLocale.sin_addr, ipRecu, INET_ADDRSTRLEN);		//Convertion de l'adresse ip recu
		printf("-> Préparation de l'adresse distante ...\n");
		adr.sin_family = AF_INET;						//Type de protocol
		inet_pton(AF_INET, ipRecu, & (adr.sin_addr)); 	//Adresse ip du client
		adr.sin_port = adrLocale.sin_port;				//Numéro du client
		printf("-> Préparation terminée !\n");
		//Envoi du retour au client
		printf("-> Envoi du retour ...\n");
		if (sendto(fd, &buffer, 0, 0, (struct sockaddr*) &adr, addrlen) == -1){
			perror("---> Echec de l'envoi, fin du programme.\n");
			exit(0);
		}
	}
	connected = 0;
	emission = 0;
	reception = 1;
	
	while (1 == 1){
		/*if (emission == 1 && connected == 1){
			nb_lu = read(input_fd, buffer, BUFFER_LENGTH);
			if (nb_lu == -1){
				printf("-> Echec de la lecture d'un bloc, fin du programme.\n\n");
			}else{
				printf("-> Envoi d'un bloc (taille : %d) ...\n",nb_lu);
				sendto (fd, &buffer, nb_lu, 0, (struct sockaddr*) &adr, addrlen);
				if (nb_lu == 0){
					printf("-> Fermeture de l'emission\n");
					emission = 0;
				}
			}
		}*/
		
		if (reception == 1){
			nbRecv = recvfrom(fd, &buffer, BUFFER_LENGTH, 0, (struct sockaddr*) &adrLocale, &addrlen);
			if (nbRecv == 0 && connected == 0){	//Renvoi du retour serveur pour la connexion
				printf("-> Renvoi du retour serveur ...\n");
				sendto (fd, &buffer, 0, 0, (struct sockaddr*) &adr, addrlen);
			}
			if (nbRecv == 0 && connected == 1){	//Fin de la reception car bloc vide
				printf("-> Fermeture de la reception\n");
				reception = 0;
			}

			if ( nbRecv == -1){	//Cas de l'erreur
				perror("-> Echec de la reception, fin du programme.\n\n");
			}else if (nbRecv > 1){	//Si c'est un bloc normal et non un acquittement
				printf("-> Reception d'un bloc (taille : %d) : %s\n",nbRecv, buffer);
				//Ouverture de l'envoi si on reçois une tram > 0
				if (connected == 0){
					printf("-> Succes de la connexion !\n");
					printf("-> Fin de la phase de connexion.\n\n");
					connected = 1;
				}

				//On copie ce que l'on a recu sans le numéro de sequence dans le buffer
				int i;
				for (i = 0; i < BUFFER_LENGTH-1; i++){
					bufToWrite[i] = buffer[i+1];
				}

				//Si l'ancien buffer est différent du nouveau, on sauvegarde le nouveau
				//Permet d'éviter de l'écrire plusieur fois lorsque l'acquittement
				// à été perdu dans l'envoi
				if (strcmp(bufToWrite, newBTW) != 0){
					for (i = 0; i < BUFFER_LENGTH-1 ; i++){
						newBTW[i] = bufToWrite[i];
					}
					estPareilBuffer = 0;
				}else if (strcmp(bufToWrite, newBTW) == 0){
					estPareilBuffer = 1;
				}

				//Si il sont différents on écrit la chaine
				if (estPareilBuffer == 0){
					printf("-> Ecriture dans le fichier de : %s\n", bufToWrite);

					//Si le buffer n'est pas le même que celui d'avant on écrit pas le buffer

					write(output_fd, bufToWrite, nbRecv-1);
					printf("-> Fin d'écriture dans le fichier\n");
				}
				

				//Envoi de l'acquittement correspondant
				bufAck[0] = buffer[0];
				printf("-> Envoi de l'acquittement %c\n", bufAck[0]);
				sendto (fd, &bufAck, 1, 0, (struct sockaddr*) &adr, addrlen);

			}
			printf("\n");
		}

		if(emission == 0 && reception == 0){
			printf("-> Fin de reception.\n");
			break;			
		}
		//sleep(2);
	}
	
	//Fermeture de la socket
	printf("\n\nFermeture de la socket ...\n");
	if (close(fd) == -1){
		perror("-> Echec de la fermeture, fin du programme.\n\n");
		exit(0);
	}else{
		printf("-> Succes de la fermeture !\n");
	}

	//Fermeture du fichier
	printf("\nFermeture des fichiers ...\n");
	if (close(input_fd) == -1){
		perror("-> Echec de la fermeture, fin du programme.\n\n");
		exit(0);
	}else{
		printf("-> Succes de la fermeture !\n");
	}
	if (close(output_fd) == -1){
		perror("-> Echec de la fermeture, fin du programme.\n\n");
		exit(0);
	}else{
		printf("-> Succes de la fermeture !\n");
	}

	printf("\n\nFin du programme.\n\n");

	return 1;
}
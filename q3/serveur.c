#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

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
	char ipRecu[INET_ADDRSTRLEN];	//La chaine contenant l'adresse src
	int nbRecv;						//Check variable
	int nb_lu;						//Check variable
	int reception;					//Check variable
	int emission;					//Check variable

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
		printf("--> Préparation terminée !\n");
		//Envoi du retour au client
		printf("--> Envoi du retour ...\n");
		if (sendto(fd, &buffer, 0, 0, (struct sockaddr*) &adr, addrlen) == -1){
			perror("---> Echec de l'envoi, fin du programme.\n");
			exit(0);
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
	printf("\nEnvoi et reception des fichiers ...\n");
	while (1 == 1){
		if (emission == 1){
			nb_lu = read(input_fd, buffer, BUFFER_LENGTH);
			if (nb_lu == -1){
				printf("-> Echec de la lecture d'un bloc, fin du programme.\n\n");
			}else{
				printf("-> Envoi d'un bloc (taille : %d) ...\n",nb_lu);
				sendto (fd, &buffer, nb_lu, 0, (struct sockaddr*) &adr, addrlen);
				if (nb_lu == 0)
					emission = 0;
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
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_LENGTH 1024

int main(int argc, char* argv[]){

	//Controle des arguments du programme
	if (argc != 4){
		fprintf(stderr , "usage : %s <fichier_a_envoyer> <adr_IP_dist> <port_dist>\n", argv[0]);
		exit(0);
	}

	//Clear console
	system("clear");
	printf("----- CLIENT UDP -----\n");

	//Déclaration des variables
	int fd;						//Descripteur du socket
	struct sockaddr_in adr; 	//Adresse distante
	char buffer[BUFFER_LENGTH];	//buffer
	int input_fd;				//Descripteur du fichier à lire
	int nb_lu;					//Check variable

	//Création du socket
	printf("\nCréation du socket ...\n");
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1){
		perror("-> Echec de la création, fin du programme.\n\n");
		exit(0); 
	}else{
		printf("-> Succes de la création !\n");
	}

	//Préparation de l'adresse distante
	printf("\nPréparation de l'adresse distante ...\n");
	adr.sin_family = AF_INET;						//Type de protocol
	inet_pton(AF_INET, argv[2], & (adr.sin_addr)); 	//Adresse ip du serveur
	adr.sin_port = htons(atoi(argv[3]));			//numéro de port choisi
	printf("-> Préparation terminée !\n");

	//Ouverture du fichier à lire
	printf("\nOuverture du fichier à lire ...\n");
	input_fd = open(argv[1], O_RDONLY);
	if(input_fd == -1){	
		perror("-> Echec de l'ouverture, fin du programme.\n"); 
		exit(0); 		
	}else{
		printf("-> Succes de l'ouverture !\n");
	}

	//Envoi du fichier
	socklen_t addrlen = sizeof(adr);  		//longueur de l'adresse
	printf("\nEnvoi du fichier ...\n");
	while(1 == 1)
	{
		nb_lu = read(input_fd, buffer, BUFFER_LENGTH);
		if (nb_lu == -1){
			printf("-> Echec de la lecture d'un bloc, fin du programme.\n\n");
		}else{
			printf("-> Envoi d'un bloc au serveur (taille : %d) ...\n",nb_lu);
			sendto (fd, &buffer, nb_lu, 0, (struct sockaddr*) &adr, addrlen);
			if(nb_lu < BUFFER_LENGTH){
				break;
			}
		}
	}

	//Fermeture de la socket
	printf("\nLe fichier a été entierement transféré, fermeture de la socket ...\n");
	if (close(fd) == -1){
		perror("-> Echec de la fermeture, fin du programme.\n\n");
		exit(0);
	}else{
		printf("-> Succes de la fermeture !\n");
	}


	//Fermeture du fichier
	printf("\nFermeture du fichier ...\n");
	if (close(input_fd) == -1){
		perror("-> Echec de la fermeture, fin du programme.\n\n");
		exit(0);
	}else{
		printf("-> Succes de la fermeture !\n");
	}




	printf("\n\nFin du programme.\n\n");

	return 1;
}
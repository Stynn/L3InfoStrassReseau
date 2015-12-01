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
	char tmpBuffer[BUFFER_LENGTH-1];//buffer - sequence et ack
	char bufToWrite[BUFFER_LENGTH-1];//Le buffer à écrire
	char newBTW[BUFFER_LENGTH-1];		//Le buffer recevant le message
	char bufAck[BUFFER_LENGTH];					//buffer pour l'acquittement
	char ipRecu[INET_ADDRSTRLEN];	//La chaine contenant l'adresse src
	int nbRecv;						//Check variable
	int nb_lu;						//Check variable
	int reception;					//Check variable
	int emission;					//Check variable
	int connected;					//Si le serveur est considéré comme connected
	fd_set rfds;					//Ensemble des descripteur
  	struct timeval timer;			//Timer pour le select
	int seqAck;						//Numéro de sequence et d'acquittement
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
	emission = 1;
	reception = 1;
	seqAck = 0;		//Initialisation du compteur à zero
	
	while (1 == 1){
		if (emission == 1 && connected == 1){
			//Préparation de l'envoi
			nb_lu = read(input_fd, tmpBuffer, BUFFER_LENGTH-1);
			sprintf(buffer, "%d", seqAck);	//Ajout du numéro de sequence

			int i;
      		for (i = 0 ; i < BUFFER_LENGTH-1; i++){
      			buffer[i+1] = tmpBuffer[i];
      		}

			if (nb_lu == -1){
				printf("E-> Echec de la lecture d'un bloc, fin du programme.\n\n");
			}else{
				printf("E-> Envoi d'un bloc (taille : %d) (seq = %c): %s\n",nb_lu, buffer[0],buffer);
				sendto (fd, &buffer, nb_lu+1, 0, (struct sockaddr*) &adr, addrlen);
				if (nb_lu == 0){
					printf("E-> Fermeture de l'emission\n");
					emission = 0;
				}else{
					while (1 == 1){ //Tant que l'on a pas le bon ack
						//Ajout dans la liste du descripteur de la socket
					    FD_ZERO(&rfds);
					    FD_SET(fd, &rfds);

					    //Initialisation du timer pour les acquittement
					    timer.tv_sec = 0;
					    timer.tv_usec = 100000;

					    int nbFd = fd+1;

			            //Select pour l'attente d'un message avec res
			            int res = select(nbFd, &rfds, NULL, NULL, &timer);

			            //Cas de l'erreur du select
						if (res == -1){
							perror("E-> Erreur dans la reception de l'acquittement, fin de programme.\n\n");
							exit(1);
						}//Cas du timeout
						else if (res == 0){
							//Renvoie du message
							printf("E-> timeout, renvoi du message ...\n");
							sendto (fd, &buffer, nb_lu+1, 0, (struct sockaddr*) &adr, addrlen);
						}//Cas de la reception d'un ack
						else{
							//Vérification de l'ack
							nbRecv = recvfrom(fd, &bufAck, BUFFER_LENGTH, 0, (struct sockaddr*) &adrLocale, &addrlen);
							if (nbRecv > 1){
								printf("E-> Il faut écrire ce que l'on à recu");
							}else if (nbRecv == 1){
								printf("E-> Reception acquittement %c\n",bufAck[0]);
								if (bufAck[0] == buffer[0]){	//Si l'ack est le bon
									printf("E--> Acquittement correcte\n");
					                seqAck = (seqAck+1)%2;		//On peux passer à la suite
					                break;
					            }else{	//Sinon on renvoi le buffer
					            	printf("E--> Mauvais acquittement renvoi du bloc ...\n");
					                sendto (fd, &buffer, nb_lu+1, 0, (struct sockaddr*) &adr, addrlen);
					            }
							}
						}
						sleep(2);
					}
				}
			}
			printf("\n");
		}
		
		if (reception == 1){
			nbRecv = recvfrom(fd, &buffer, BUFFER_LENGTH, 0, (struct sockaddr*) &adrLocale, &addrlen);
			if (nbRecv == 0 && connected == 0){	//Renvoi du retour serveur pour la connexion
				printf("R-> Renvoi du retour serveur ...\n");
				sendto (fd, &buffer, 0, 0, (struct sockaddr*) &adr, addrlen);
			}
			if (nbRecv == 0 && connected == 1){	//Fin de la reception car bloc vide
				printf("R-> Fermeture de la reception\n");
				reception = 0;
			}

			if ( nbRecv == -1){	//Cas de l'erreur
				perror("R-> Echec de la reception, fin du programme.\n\n");
			}else if (nbRecv > 1){	//Si c'est un bloc normal et non un acquittement
				printf("R-> Reception d'un bloc (taille : %d) : %s\n",nbRecv, buffer);
				//Ouverture de l'envoi si on reçois une tram > 0
				if (connected == 0){
					printf("R-> Succes de la connexion !\n");
					printf("R-> Fin de la phase de connexion.\n\n");
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
					printf("R-> Ecriture dans le fichier de : %s\n", bufToWrite);

					//Si le buffer n'est pas le même que celui d'avant on écrit pas le buffer

					write(output_fd, bufToWrite, nbRecv-1);
					printf("R-> Fin d'écriture dans le fichier\n");
				}
				

				//Envoi de l'acquittement correspondant
				bufAck[0] = buffer[0];
				printf("R-> Envoi de l'acquittement %c\n", bufAck[0]);
				sendto (fd, &bufAck, 1, 0, (struct sockaddr*) &adr, addrlen);

			}
			printf("\n");
		}

		if(emission == 0 && reception == 0){
			printf("-> Fin de reception.\n");
			break;	
		}
		sleep(2);
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
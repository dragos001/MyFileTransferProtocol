#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <dirent.h>
#include <libgen.h>

#define MAXIM 500

/* portul folosit */
#define PORT 2908

/* codul de eroare returnat de anumite apeluri */
extern int errno;

typedef struct thData
{
  int idThread; //id-ul thread-ului tinut in evidenta de acest program
  int cl;       //descriptorul intors de accept
} thData;

struct utilizatori
{
  char user[30];
  short connectat;

} utilizatori[500];

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
char *comanda(void *arg);

int main()
{
  struct sockaddr_in server; // structura folosita de server
  struct sockaddr_in from;
  int nr; //mesajul primit de trimis la client
  int sd; //descriptorul de socket
  int pid;
  pthread_t th[100]; //Identificatorii thread-urilor care se vor crea
  int i = 0;

  for (int i = 0; i < 500; i++)
  {
    utilizatori[i].connectat = 0;
  }

  /* crearea unui socket */
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("[server]Eroare la socket().\n");
    return errno;
  }
  /* utilizarea optiunii SO_REUSEADDR */
  int on = 1;
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  /* pregatirea structurilor de date */
  bzero(&server, sizeof(server));
  bzero(&from, sizeof(from));

  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
  server.sin_family = AF_INET;
  /* acceptam orice adresa */
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  /* utilizam un port utilizator */
  server.sin_port = htons(PORT);

  /* atasam socketul */
  if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[server]Eroare la bind().\n");
    return errno;
  }

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen(sd, 2) == -1)
  {
    perror("[server]Eroare la listen().\n");
    return errno;
  }
  /* servim in mod concurent clientii...folosind thread-uri */
  while (1)
  {
    int client;
    thData *td; //parametru functia executata de thread
    int length = sizeof(from);

    printf("[server]Asteptam la portul %d...\n", PORT);
    fflush(stdout);

    // client= malloc(sizeof(int));
    /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
    if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
    {
      perror("[server]Eroare la accept().\n");
      continue;
    }

    /* s-a realizat conexiunea, se astepta mesajul */

    // int idThread; //id-ul threadului
    // int cl; //descriptorul intors de accept

    td = (struct thData *)malloc(sizeof(struct thData));
    td->idThread = i++;
    td->cl = client;

    pthread_create(&th[i], NULL, &treat, td);

  } //while
};
static void *treat(void *arg)
{
  struct thData tdL;
  tdL = *((struct thData *)arg);
  printf("[thread]- %d -Astept comanda de la client\n", tdL.idThread);
  fflush(stdout);
  pthread_detach(pthread_self());
  do
  {
    char *verificare = comanda((struct thData *)arg);
    printf("verificare: %s\n", verificare);
    if (strcmp(verificare, "quit") == 0)
    {
      printf("Clientul cu thread %d s-a deconectat!\n", tdL.idThread);
      fflush(stdout); //am terminat cu acest client, inchidem conexiunea
      close((intptr_t)arg);
      return (NULL);
    }
  } while (1);
};

char *password(char *pass)
{
  for (int i = 0; i < strlen(pass); i++)
    pass[i] -= 10;

  return pass;
}

int block_user(void *arg, char *user)
{
  struct thData tdL;
  tdL = *((struct thData *)arg);

  char pack[100];
  if (getcwd(pack, sizeof(pack)) == 0)
    perror("getcwd error");
  else
  {
    strcat(pack, "/user/blacklist.txt");
    FILE *file = fopen(pack, "r");
    if (file == 0)
    {
      printf("Fisierul blacklist nu exista\n");
    }
    else
    {
      char ver[100];
      while (fscanf(file, "%s", ver) == 1)
      {
        if (strcmp(user, ver) == 0)
        {
          fclose(file);
          return 0;
        }
      }
    }
    fclose(file);
    return 1;
  }
  return 0;
}

int verificare_login(void *arg, char *user, char *pass)
{
  struct thData tdL;
  tdL = *((struct thData *)arg);

  char pack[100];
  if (getcwd(pack, sizeof(pack)) == 0)
    perror("getcwd error");
  else
  {
    strcat(pack, "/user/whitelist.txt");
    FILE *file = fopen(pack, "r");
    if (file == 0)
    {
      printf("Fisierul whitelist nu exista\n");
    }
    else
    {
      char ver[100];
      char *userf;
      char *pasf;
      while (fscanf(file, "%s", ver) == 1)
      {
        userf = strtok(ver, ":");
        pasf = strtok(NULL, ":");

        if (strcmp(userf, user) == 0)
        {
          printf("%s,%s", user, userf);
          if (strcmp(pasf, pass) == 0)
          {
            fclose(file);
            return 1;
          }
        }
      }
    }
  }

  return 0;
}

void *login(void *arg)
{
  struct thData tdL;
  tdL = *((struct thData *)arg);

  char comanda[MAXIM];
  char raspuns[MAXIM];

  if (utilizatori[tdL.idThread].connectat == 1)
  {

    strcpy(raspuns, "Esti logat deja!");
    if (write(tdL.cl, raspuns, strlen(raspuns) + 1) < 0)
    {
      printf("[Thread %d]\n", tdL.idThread);
      perror("Eroare la read() de la client.\n");
    }
    else
      printf("[Thread %d]Mesajul a fost trasmis cu succes. %s\n", tdL.idThread, raspuns);
  }
  else
  {
    strcpy(raspuns, "user:");
    if (write(tdL.cl, raspuns, strlen(raspuns) + 1) < 0)
    {
      printf("[Thread %d]\n", tdL.idThread);
      perror("Eroare la read() de la client.\n");
    }
    else
      printf("[Thread %d]Mesajul a fost trasmis cu succes. %s\n", tdL.idThread, raspuns);

    bzero(comanda, MAXIM);
    if (read(tdL.cl, comanda, MAXIM) < 0)
    {
      printf("[Thread %d]\n", tdL.idThread);
      perror("Eroare la read() de la client.\n");
    }
    printf("login:%s\n", comanda);

    char *user = strtok(comanda, ":");
    char *pw = strtok(NULL, ":");

    if (verificare_login(((struct thData *)arg), user, password(pw)) == 1 &&
        block_user(((struct thData *)arg), user) == 1)
    {

      printf("%d", block_user(((struct thData *)arg), user));
      strcpy(raspuns, "Te-ai conectat cu succes!");
      if (write(tdL.cl, raspuns, strlen(raspuns) + 1) < 0)
      {
        printf("[Thread %d]\n", tdL.idThread);
        perror("Eroare la read() de la client.\n");
      }
      else
        printf("[Thread %d]Mesajul a fost trasmis cu succes. %s\n", tdL.idThread, raspuns);

      utilizatori[tdL.idThread].connectat = 1;
      strcpy(utilizatori[tdL.idThread].user, user);
    }
    else
    {
      strcpy(raspuns, "Eroare la conectare!");
      if (write(tdL.cl, raspuns, strlen(raspuns) + 1) < 0)
      {
        printf("[Thread %d]\n", tdL.idThread);
        perror("Eroare la read() de la client.\n");
      }
      else
        printf("[Thread %d]Mesajul a fost trasmis cu succes. %s\n", tdL.idThread, raspuns);
    }
  }
}

char *show(void *arg)
{

  struct thData tdL;
  tdL = *((struct thData *)arg);

  char pack[100];
  if (getcwd(pack, sizeof(pack)) == NULL)
    perror("getcwd() error. \n");
  else
  {
    strcat(pack, "/Fisiere/");
    strcat(pack, utilizatori[tdL.idThread].user);
    strcat(pack, "/");
  }

  char *txt;
  txt = (char *)malloc(MAXIM);
  strcpy(txt, "Fisiere disponibile:\n");
  DIR *dir;
  struct dirent *d;
  if ((dir = opendir(pack)) != NULL)
  {
    while ((d = readdir(dir)) != NULL)
    {
      strcat(txt, d->d_name);
      strcat(txt, " ");
    }
    closedir(dir);
  }

  return txt;
}

void *DeleteFile(void *arg, char *name)
{
  struct thData tdL;
  tdL = *((struct thData *)arg);
  char raspuns[100];
  char pack[100];
  if (getcwd(pack, sizeof(pack)) == NULL)
    perror("getcwd() error. \n");
  else
  {
    strcat(pack, "/Fisiere/");
    strcat(pack, utilizatori[tdL.idThread].user);
    strcat(pack, "/");
  }
  strcat(pack, name);
  FILE *file = fopen(pack, "r");
  if (file == 0)
  {
    strcpy(raspuns, "Fisierul nu exista\n");
  }
  else
  {
    if (remove(pack) == 0)
      strcpy(raspuns, "Fisieru a fost sters cu succes!");

    else

      strcpy(raspuns, "Error: Nu s-a putut sterge fisierului!");
  }

  if (write(tdL.cl, raspuns, strlen(raspuns) + 1) < 0)
  {
    printf("[Thread %d] ", tdL.idThread);
    perror("[Thread]Eroare la write() catre client.\n");
    return "quit";
  }
  else
    printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
}

void *descarca(void *arg, char *name)
{
  struct thData tdL;
  tdL = *((struct thData *)arg);
  char pack[100], buff[MAXIM], raspuns[100];
  int nr, bytes;
  if (getcwd(pack, sizeof(pack)) == NULL)
    perror("getcwd() error. \n");
  else
  {
    strcat(pack, "/Fisiere/");
    strcat(pack, utilizatori[tdL.idThread].user);
    strcat(pack, "/");
  }
  strcat(pack, name);
  printf("%s", pack);
  FILE *fp = fopen(pack, "r");
  if (fp == 0)
  {
    strcpy(raspuns, "Fisierul nu exista!");
    if (write(tdL.cl, raspuns, strlen(raspuns) + 1) < 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return "quit";
    }
    else
      printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
  }
  else
  {
    strcpy(raspuns, "OK");
    if (write(tdL.cl, raspuns, strlen(raspuns) + 1) < 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return "quit";
    }
    else
      printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);

    while (fgets(buff, MAXIM, fp) != NULL)
    {
      if (write(tdL.cl, buff, MAXIM) < 0)
      {
        printf("[Thread %d] ", tdL.idThread);
        perror("[Thread]Eroare la write() catre client.\n");
        return "quit";
      }
      else
        printf("[Thread %d]Mesajul a fost trasmis cu succes.[BUff]\n", tdL.idThread);
    }

    printf("Fisier transmis!");
  }
}

void *ChangeName(void *arg, char *name)
{
  struct thData tdL;
  tdL = *((struct thData *)arg);
  char pack[100];
  char *old_name;
  char *new_name;
  char raspuns[100];
  if (getcwd(pack, sizeof(pack)) == NULL)
    perror("getcwd() error. \n");
  else
  {
    strcat(pack, "/Fisiere/");
    strcat(pack, utilizatori[tdL.idThread].user);
    strcat(pack, "/");
  }
  old_name = strtok(name, ":");
  new_name = strtok(NULL, ":");
  char pack2[100];
  strcpy(pack2, pack);
  strcat(pack2, new_name);
  strcat(pack, old_name);
  FILE *file = fopen(pack, "r");
  if (file == 0)
  {
    strcpy(raspuns, "Fisierul nu exista\n");
  }
  else
  {
    if (rename(pack, pack2) == 0)
    {
      strcpy(raspuns, "Fisieru a fost modificat cu succes!");
    }
    else
    {
      strcpy(raspuns, "Error: Nu s-a putut modifica numele fisierului!");
    }
  }

  if (write(tdL.cl, raspuns, strlen(raspuns) + 1) < 0)
  {
    printf("[Thread %d] ", tdL.idThread);
    perror("[Thread]Eroare la write() catre client.\n");
    return "quit";
  }
  else
    printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
}

void *incarca(void *arg, char *nume)
{
  struct thData tdL;
  tdL = *((struct thData *)arg);

  char pack[100];
  char buff[100];
  char *files;
  char *comanda;
  if (getcwd(pack, sizeof(pack)) == NULL)
    perror("getcwd() error. \n");
  else
  {
    strcat(pack, "/Fisiere/");
    strcat(pack, utilizatori[tdL.idThread].user);
    strcat(pack, "/");
  }
  if (read(tdL.cl, comanda, MAXIM) < 0)
  {
    printf("[Thread %d]\n", tdL.idThread);
    perror("Eroare la read() de la client.\n");
    return "quit";
  }
  strcat(pack, nume);
  FILE *fp;
  fp = fopen(pack, "w");
  if (fp == NULL)
  {
    printf("Error opening file!");
  }
  else
  {
    read(tdL.cl, buff, MAXIM);
    fprintf(tdL.cl, "%s", buff);
  }
}

char *comanda(void *arg)
{
  char comanda[MAXIM];
  char raspuns[MAXIM];
  struct thData tdL;
  tdL = *((struct thData *)arg);
  bzero(comanda, MAXIM);
  if (read(tdL.cl, comanda, MAXIM) < 0)
  {
    printf("[Thread %d]\n", tdL.idThread);
    perror("Eroare la read() de la client.\n");
    return "quit";
  }

  else if (strcmp(comanda, "help") == 0)
  {
    strcpy(raspuns, "\033[1;31mFoloseste comanda: \033[0m\n");
    strcat(raspuns, "\033[0;32m -login\033[0m pentru a te loga\n");
    strcat(raspuns, "\033[0;32m -show\033[0m pentru a viziona fisierele incarcate\n");
    strcat(raspuns, "\033[0;32m -schimba\033[0m pentru a schimba numele fisierului\n");
    strcat(raspuns, "\033[0;32m -descarca\033[0m pentru a descarca un fisier\n");
    strcat(raspuns, "\033[0;32m -quit \033[0m pentru a iesi\n");
    if (write(tdL.cl, raspuns, strlen(raspuns) + 1) < 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return "quit";
    }
    else
      printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
  }
  else if (strcmp(comanda, "quit") == 0)
  {
    utilizatori[tdL.idThread].connectat = 0;
    strcpy(utilizatori[tdL.idThread].user, "");
    return "quit";
  }
  else if (strcmp(comanda, "login") == 0)
  {
    login((struct thData *)arg);
  }
  else if (strcmp(comanda, "show") == 0)
  {
    if (utilizatori[tdL.idThread].connectat == 0)
    {
      strcpy(raspuns, "Trebuie sa fii logat pentru a avea acces la comanda!");
      if (write(tdL.cl, raspuns, strlen(raspuns) + 1) < 0)
      {
        printf("[Thread %d] ", tdL.idThread);
        perror("[Thread]Eroare la write() catre client.\n");
        return "quit";
      }
      else
        printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
    }
    else
    {
      strcpy(raspuns, show((struct thData *)arg));
      if (write(tdL.cl, raspuns, strlen(raspuns) + 1) < 0)
      {
        printf("[Thread %d] ", tdL.idThread);
        perror("[Thread]Eroare la write() catre client.\n");
        return "quit";
      }
      else
        printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
    }
  }
  else if (strcmp(comanda, "schimba") == 0)
  {
    if (utilizatori[tdL.idThread].connectat == 0)
    {
      strcpy(raspuns, "Error1");
      if (write(tdL.cl, raspuns, strlen(raspuns) + 1) < 0)
      {
        printf("[Thread %d] ", tdL.idThread);
        perror("[Thread]Eroare la write() catre client.\n");
        return "quit";
      }
      else
        printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
    }
    else
    {
      strcpy(raspuns, "ok");
      if (write(tdL.cl, raspuns, strlen(raspuns) + 1) < 0)
      {
        printf("[Thread %d] ", tdL.idThread);
        perror("[Thread]Eroare la write() catre client.\n");
        return "quit";
      }
      else
        printf("[Thread %d]Mesajul a fost trasmis cu succes.[Schimba!]\n", tdL.idThread);

      if (read(tdL.cl, comanda, MAXIM) < 0)
      {
        printf("[Thread %d]\n", tdL.idThread);
        perror("Eroare la read() de la client.\n");
        return "quit";
      }
      ChangeName(((struct thData *)arg), comanda);
    }
  }
  else if (strcmp("sterge", comanda) == 0)
  {
    if (utilizatori[tdL.idThread].connectat == 0)
    {
      strcpy(raspuns, "Error1");
      if (write(tdL.cl, raspuns, strlen(raspuns) + 1) < 0)
      {
        printf("[Thread %d] ", tdL.idThread);
        perror("[Thread]Eroare la write() catre client.\n");
        return "quit";
      }
      else
        printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
    }
    else
    {
      strcpy(raspuns, "ok");
      if (write(tdL.cl, raspuns, strlen(raspuns) + 1) < 0)
      {
        printf("[Thread %d] ", tdL.idThread);
        perror("[Thread]Eroare la write() catre client.\n");
        return "quit";
      }
      else
        printf("[Thread %d]Mesajul a fost trasmis cu succes.[Delete!]\n", tdL.idThread);

      if (read(tdL.cl, comanda, MAXIM) < 0)
      {
        printf("[Thread %d]\n", tdL.idThread);
        perror("Eroare la read() de la client.\n");
        return "quit";
      }

      DeleteFile(((struct thData *)arg), comanda);
    }
  }
  else if (strcmp(comanda, "descarca") == 0)
  {
    if (utilizatori[tdL.idThread].connectat == 0)
    {
      strcpy(raspuns, "Error1");
      if (write(tdL.cl, raspuns, strlen(raspuns) + 1) < 0)
      {
        printf("[Thread %d] ", tdL.idThread);
        perror("[Thread]Eroare la write() catre client.\n");
        return "quit";
      }
      else
        printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
    }
    else
    {
      strcpy(raspuns, "ok");
      if (write(tdL.cl, raspuns, strlen(raspuns) + 1) < 0)
      {
        printf("[Thread %d] ", tdL.idThread);
        perror("[Thread]Eroare la write() catre client.\n");
        return "quit";
      }
      else
        printf("[Thread %d]Mesajul a fost trasmis cu succes.[descarca!]\n", tdL.idThread);

      if (read(tdL.cl, comanda, MAXIM) < 0)
      {
        printf("[Thread %d]\n", tdL.idThread);
        perror("Eroare la read() de la client.\n");
        return "quit";
      }

      descarca(((struct thData *)arg), comanda);
    }
  }
 

  printf("[Thread %d]Comanda data este: %s\n", tdL.idThread, comanda);
  return "cont";
}

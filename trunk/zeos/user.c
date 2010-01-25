#include <libc.h>
#include <stats.h>
#include <interrupt.h>
#include <stdio.h>
#include <filesystem.h>

void runjp();

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
    //__asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) );
    //__asm__ __volatile__ ("int $0x0");
    //__asm__ ("ud2");
    /* Uncomment next line to call the initial routine for the test cases */
    runjp();
    
	/*write(1,"Write SysCall\nWORKS!!!!!!\n\n!",22);
	perror();
	write(1,"Write SysCall\nWORKS!!!!!!\n\n!",-1);
	perror();
	write(6,"ciao",4);
	perror();*/
	
	/* This tests for fork() and getpid()
	-------------------------------------
	int i,j,e;
	
	write(1,"Prova getpid():\n",16);
	e=getpid();
	if(e<0)
		{
		write(1,"getpid(): errore",16);}
	else
		for(i=0;i<e;i++) write(1,"a",1);
	
	write(1,"\n",1);
	
	write(1,"Prova 5 fork():\n",16);
	for(j=0;j<10;j++){
		e=fork();
		if(e<0)
			{
			write(1,"\nfork(): errore\n",16);
			perror();}
		else
			for(i=0;i<e;i++) write(1,"a",1);
	
		write(1," ",1);	
	}
	write(1,"\nfine\n",6);*/
	
	/*_______________________________________________________*/
/*
	write(1,"ciao\n",5);

	sem_init(0,0);

	fork();
	fork();
	fork();

	if(getpid()==0)
		write(1,"Processo 0\n",11);
	else if(getpid()==1){
		write(1,"Processo 1\n",11);
		sem_init(2,0);
		sem_wait(0);
		write(1,"Una scritta qualsiasi (pid 1)\n",30);
		sem_destroy(2);}
	else if(getpid()==2){
		write(1,"Processo 2\n",11);
		sem_wait(2);
		write(1,"Una scritta qualsiasi (pid 2)\n",30);
	}
	else if(getpid()==3){
		write(1,"Processo 3\n",11);
		sem_wait(2);
		write(1,"Una scritta qualsiasi (pid 3)\n",30);
	}
	else if(getpid()==4){
		write(1,"Processo 4\n",11);
		sem_signal(0);
		sem_wait(2);
		write(1,"Una scritta qualsiasi (pid 4)\n",30);
	}
	if(getpid()>4){
		exit();
	}
	write(1,"ciao2\n",6);
	struct stats st[5];
	get_stats(0,&st[0]);
	get_stats(1,&st[1]);
	get_stats(2,&st[2]);
	get_stats(3,&st[3]);
	get_stats(4,&st[4]);*/



	/* _______________________
	 * test per read e write
	 *

	char buffer[30];
	char prova[30];
	char lettura[5];
	char primo[]={'b','a','a','a','a','a','a','a','b','\0'};
	int j=0,i=0;

	for(i=0;i<29;i++) buffer[i]='a';
	buffer[29]='\0';

	write(1,"Inizio:\n",8);

	fork();

	/*if(getpid()==0){
		i=open("Prova1",O_CREAT | O_EXCL  | O_RDWR);
		if(i>=0) write(1,"File Prova1 creato...\n",22);
		else write(1,"Errore creazione Prova1...\n",27);

		// Scrive un file da 30bytes (6 blocchi)
		write(i,buffer,strlen(buffer));
		write(1,"Scritto in Prova1...\n",21);

		// Cancella quello stesso file
		close(i);
		unlink("Prova1");

		// Crea e scrive un file da 10 bytes (2 blocchi)
		i=open("Prova2",O_CREAT | O_EXCL  | O_RDWR);
		if(i>=0) write(1,"File Prova2 creato...\n",22);
		else write(1,"Errore creazione Prova2...\n",27);

		write(i,"123456789\0",10);
		write(1,"Scritto in Prova2...\n",21);

		j=open("Prova2",O_RDONLY);

		if(j>=0) write(1,"File Prova2 aperto...\n",22);
		else write(1,"Errore apertura Prova2...\n",26);

		read(j,prova,10);
		write(1,"Letti ",6);
		itoa(strlen(prova),lettura);
		write(1,lettura,strlen(lettura));
		write(1," bytes: ",8);
		write(1,prova,strlen(prova));
		write(1,"\nFINE\n",6);

	}*
	/else  if(getpid()==1){
		write(1,"Processo 1\n",11);
		read(0,primo,1);
		write(1,"Prima risposta: ",16);
		write(1,primo,strlen(primo));
		write(1,"\n",1);
		read(0,primo,8);
		write(1,"Seconda risposta: ",18);
		write(1,primo,strlen(primo));
		write(1,"\n",1);
	}*/

	while(1);
	return(0);
}

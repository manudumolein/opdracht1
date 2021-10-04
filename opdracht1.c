#include <mysql.h>
#include "PJ_RPI.h"
#include <stdio.h>
#include <stdlib.h>

void controlStateChange(int);
void makeDatabase(void);
void updateDatabase(int);

//globale var
char state[27] = {0};

int main()
{
	if (map_peripheral(&gpio) == -1)
	{
		printf("Failed to map the physical GPIO registers into the virtual memory space.\n");
		return -1;
	}

	makeDatabase();

//BCM 17,27,22 instellen als input
	INP_GPIO(17);
	INP_GPIO(27);
	INP_GPIO(22);

	while (1)
	{
		controlStateChange(17);
		controlStateChange(27);
		controlStateChange(22);
	}

	return 0;
}

void makeDatabase(void)
{
	MYSQL *con = mysql_init(NULL);

	if (con == NULL)
	{
		fprintf(stderr, "%s\n", mysql_error(con));
		exit(1);
	}

	if (mysql_real_connect(con, "localhost", "root", "root_pswd",
						   NULL, 0, NULL, 0) == NULL)
	{
		fprintf(stderr, "%s\n", mysql_error(con));
		mysql_close(con);
		exit(1);
	}

	if (mysql_query(con, "CREATE DATABASE logPorts"))
	{
		fprintf(stderr, "%s\n", mysql_error(con));
	}	
	if (mysql_query(con, "USE logPorts"))
	{
		fprintf(stderr, "%s\n", mysql_error(con));
	}

	if (mysql_query(con, "CREATE TABLE portInfo(id INT NOT NULL AUTO_INCREMENT, PRIMARY KEY (id), GPIO_port INT,state INT, date_time datetime NOT NULL DEFAULT CURRENT_TIMESTAMP)"))
	{
		fprintf(stderr, "%s\n", mysql_error(con));
		mysql_close(con);
		return;
	}

	mysql_close(con);
	printf("databank created \n");
	return;
}

void controlStateChange(int port)
{
	long input = GPIO_READ(port);

	if (input >> port ^ state[port])
	{		
		state[port] = !state[port];
		printf("port: %i state: %i\n",port,state[port]);

		updateDatabase(port);
		sleep(1); //tegen jitter
	}
}

void updateDatabase(int port)
{
	MYSQL *con = mysql_init(NULL);

	if (con == NULL)
	{
		printf("MySQL initialization failed");
		return;
	}
	if (mysql_real_connect(con, "localhost", "root", "root_pswd",
						   "logPorts", 0, NULL, 0) == NULL)
	{
		fprintf(stderr, "%s\n", mysql_error(con));
		mysql_close(con);
		return;
	}

	char buffer[500];
	snprintf(buffer, sizeof(buffer), "insert into portInfo(GPIO_port,state) values( %i,%i)", port, state[port]);


	if (mysql_query(con, buffer))
	{
		fprintf(stderr, "%s\n", mysql_error(con));
		mysql_close(con);
		return;
	}
	mysql_close(con);
	return;
}
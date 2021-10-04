#include <stdio.h>
#include <mysql.h>
#include <stdlib.h>

#include <unistd.h>
#include <gpiod.h>

void controlStateChange(struct gpiod_line *, int);
void makeDatabase(void);
void updateDatabase(int);

char state[27] = {0};
const char *chipname = "gpiochip0";
struct gpiod_chip *chip;

struct gpiod_line *GPIO_17;
struct gpiod_line *GPIO_22;
struct gpiod_line *GPIO_27;

int main()
{
    makeDatabase();

    chip = gpiod_chip_open_by_name(chipname);

    GPIO_17 = gpiod_chip_get_line(chip, 17);
    GPIO_22 = gpiod_chip_get_line(chip, 22);
    GPIO_27 = gpiod_chip_get_line(chip, 27);

    gpiod_line_request_input(GPIO_17, "ManuD");
    gpiod_line_request_input(GPIO_22, "ManuD");
    gpiod_line_request_input(GPIO_27, "ManuD");

    while (1)
    {
        controlStateChange(GPIO_17, 17);
    }
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

void controlStateChange(struct gpiod_line *gpio, int port)
{
    int val = gpiod_line_get_value(gpio);

    if (val ^ state[port])
    {
        state[port] = !state[port];
        printf("port: %i state: %i\n", port, state[port]);

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
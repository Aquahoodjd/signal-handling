#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "program.h"
#include "program-utils.h"

/* FILE DESCRIPTORS */
/* file descriptor for file that is path name of given by -i option */
int fd_input;
/* file descriptor for file that is path name of given by -o option */
int fd_output;
/* file descriptor for temp file for process 1*/
int fd_temp; 
/* file descriptor for temp file for process 2*/
int fd_temp_2;

/* FILE PATHS */
/* file path for file that is path name of given by -i option */
char* input_file_path = NULL;
/* file path for file that is path name of given by -i option */
char* output_file_path = NULL;
/* file descriptor for temp file */
char temp_file_path[] = "./tempXXXXXX";

/* process id*/
pid_t pid = -1;

int flag2 = 1, criticalFlag = 0;

/* ERROR VALUE ARRAYS*/
double* mean_absolute_error = NULL;
double* mean_squared_error = NULL;
double* root_mean_squared_error = NULL;
char* rewrite = NULL;

int main(int argc, char* argv[])
{
	int option;

	if(argc != 5)
	{
		fprintf(stderr, "Wrong input option usage! Use: ./programA -i inputPath -o outputPath\n");
		exit(1);
	}

	while ((option = getopt (argc, argv, "io")) != -1)
  	{
	    switch (option)
	      {
	      case 'i':
	        input_file_path = argv[optind];
	        break;
	      case 'o':
	        output_file_path = argv[optind];
	        break;
	      default:
		  	fprintf(stderr, "Wrong input option usage! Use: ./programA -i inputPath -o outputPath\n");
			exit(1);
	      }
	}
	create_fd();

	signal(SIGUSR1, handler);
	signal(SIGUSR2, handler);
    signal(SIGTERM, handler);

	pid = fork();
	int status;
	sigset_t mask;
	switch (pid)
	{
	case 0:
		process_2();
		break;
	case -1:
		fprintf (stderr, "\nERROR %d, %s (Fork failed)\n", errno, strerror (errno));
		return 1;
	default:
			sigfillset(&mask);
			sigdelset(&mask, SIGUSR2);
			sigsuspend(&mask);

			process_1();
			waitpid(pid, &status, 0);
			if(WIFEXITED(status)) 
			{
				return 0;
			}
				
		break;
	}
	return 0;
}

void process_1()
{
	char coordinate[BLOCK] = "";
	char coordinates[LENGTH] = "";
	char *line_ptr;
	char line[BLOCK] = "";
	/* counters */ 
	int read_bytes,
		written_bytes = 0,
		total_bytes = 0,
		total_read = 0;
	int counter = 0;

	struct flock file_lock;
	
	memset(&file_lock, 0, sizeof(file_lock));

	sigset_t last_mask, actual_mask;
    struct sigaction sig_action;

	while(TRUE)
	{
		strcpy(coordinates, "");
		do
		{
			if(!((read_bytes = read(fd_input, coordinates, LENGTH)) == -1) && 
				 (errno == EINTR))
				break;
		}
		while(TRUE);

		if(read_bytes != 20) 
		{
			break;
		}
		total_read += read_bytes;
		strcpy(line, "");
		strcpy(coordinate, "");
		for(int i = 0; i < LENGTH; i += 2)
		{
			sprintf(coordinate, "(%d, %d), ", (int)((unsigned char) coordinates[i]), (int)((unsigned char) coordinates[i+1]));
			strcat(line, coordinate);
		}

        sigemptyset(&actual_mask);
        sigaddset(&actual_mask, SIGINT);
        sigemptyset(&sig_action.sa_mask);
        sig_action.sa_flags = 0;
        sig_action.sa_handler = handler;

        if (sigaction(SIGINT, &sig_action, NULL) == -1)
        {
        	fprintf (stderr, "\nERROR %d, %s signal handler!\n", errno, strerror (errno));
            exit(1);
        }
        if (sigprocmask(SIG_BLOCK, &actual_mask, &last_mask) == -1)
        {
        	fprintf (stderr, "\nERROR %d, %s blocking signal\n", errno, strerror (errno));
            exit(1);
        }

		/* CRITICAL SECTION START */
		least_squares_method(coordinates, line);
		read_bytes = strlen(line);
		/* CRITICAL SECTION END */

		if (sigprocmask(SIG_UNBLOCK, &actual_mask, NULL) == -1)
        {
        	fprintf (stderr, "\ERROR %d, %s unblocking signal\n", errno, strerror (errno));
            exit(1);
        }

        sig_action.sa_handler = SIG_DFL;
        if (sigaction(SIGINT, &sig_action, NULL) == -1)
        {
        	fprintf (stderr, "\nERROR %d, %s setting default handler\n", errno, strerror (errno));
            exit(1);
        }

		file_lock.l_type = F_WRLCK;
		fcntl(fd_temp, F_SETLKW, &file_lock);

		line_ptr = NULL;
		line_ptr = line;
		lseek(fd_temp, 0, SEEK_END);
		while(read_bytes > 0)
		{
			do
			{
				if (!((written_bytes = write(fd_temp, line_ptr, read_bytes)) == -1) && (errno == EINTR))
				{
					break;
				}
				
			} while (TRUE);
			
			if(written_bytes < 0)
			{
				break;
			}
			
			total_bytes += written_bytes;
			read_bytes -= written_bytes;
			line_ptr += written_bytes;
		}
		if(written_bytes == -1)
		{
			break;
		}

		file_lock.l_type = F_UNLCK;
		fcntl(fd_temp, F_SETLKW, &file_lock);
		counter++;

		kill(pid, SIGUSR2);
	}

	printf("\nRead bytes: %d\nEstimated line equations: %d\n", total_read, counter);
	print_signal_info();
	close_files_process_1();
	kill(pid, SIGUSR1);
}

void process_2()
{
	kill(getppid(), SIGUSR2);

	char *line_ptr;
	char line[BLOCK];
	char temp[BLOCK];

	int read_bytes,
		written_bytes = 0,
		total_bytes = 0,
		temp_total_bytes = 0,
		i = 0,
		j = 1, 
		flag = 0,
		count = 0,
		size = 20;

	mean_absolute_error = (double*) malloc(size * sizeof(double));
	mean_squared_error = (double*) malloc(size * sizeof(double));
	root_mean_squared_error = (double*) malloc(size * sizeof(double));
	double mean_mae = 0, 
		   mean_mse = 0, 
		   mean_rmse = 0, 
		   mean_dev_mae = 0, 
		   mean_dev_mse = 0, 
		   mean_dev_rmse = 0, 
		   standard_dev_mae = 0, 
		   standard_dev_mse = 0, 
		   standard_dev_rmse = 0; 

	struct stat fileStat;

	struct flock lock;
	memset(&lock, 0, sizeof(lock));

	sigset_t prevMask, intMask;
	sigset_t new_mask;
	struct sigaction sa;

	while (TRUE)
	{
		kill(getppid(), SIGUSR2);
		strcpy(line, "");
		strcpy(temp, " ");
		i = 0;
		j = 1;
		flag = 0;
		line_ptr = NULL;

		lseek(fd_temp_2, 0, SEEK_SET);
		while(j == 1)
		{
			i += 1;
			j = read(fd_temp_2, temp, 1);
			if(temp[0] == '\n')
			{
				flag = 1;
				break;
			}
		}
		lseek(fd_temp_2, 0, SEEK_SET);
		while (((read_bytes = read(fd_temp_2, line, i - 1)) == -1) && (errno == EINTR));
		
		if(flag == 1)
		{ 
			line[i-1] = '\0';
		}
		if((read_bytes <= 0 || flag == 0) && flag2 == 0)
		{
			break;
		}
		
		if((read_bytes <= 0 || flag == 0) && flag2 == 1)
		{
			sigfillset(&new_mask);
			sigdelset(&new_mask,SIGUSR1);
			sigdelset(&new_mask,SIGUSR2);
			sigsuspend(&new_mask);
			continue;
		}
		
		if(count == size)
		{
			size *= 2;
			mean_absolute_error = (double *) realloc(mean_absolute_error, size * sizeof(double));
			mean_squared_error = (double *) realloc(mean_squared_error, size * sizeof(double));
			root_mean_squared_error = (double *) realloc(root_mean_squared_error, size * sizeof(double));
		}

		sigemptyset(&intMask);
        sigaddset(&intMask, SIGINT);
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sa.sa_handler = handler;

        if (sigaction(SIGINT, &sa, NULL) == -1)
        {
        	fprintf (stderr, "\nerrno = %d: %s (Failed to install SIGINT signal handler)\n\n", errno, strerror (errno));
            exit(1);
        }
        if (sigprocmask(SIG_BLOCK, &intMask, &prevMask) == -1)
        {
        	fprintf (stderr, "\nerrno = %d: %s (Failed to SIG_BLOCK)\n\n", errno, strerror (errno));
            exit(1);
        }

		set_metrics(line, mean_absolute_error, mean_squared_error, root_mean_squared_error, count);
		read_bytes = strlen(line);
		count += 1;
		
		if (sigprocmask(SIG_UNBLOCK, &intMask, NULL) == -1)
        {
        	fprintf (stderr, "\nerrno = %d: %s (Failed to SIG_UNBLOCK)\n\n", errno, strerror (errno));
            exit(1);
        }

        sa.sa_handler = SIG_DFL;

        if (sigaction(SIGINT, &sa, NULL) == -1)
        {
        	fprintf (stderr, "\nerrno = %d: %s (Failed to get old handler for SIGINT)\n\n", errno, strerror (errno));
            exit(1);
        }
		
		lock.l_type = F_WRLCK;
		fcntl(fd_output, F_SETLKW, &lock);

		line_ptr = line;
		lseek(fd_output, 0, SEEK_END);
		while(read_bytes > 0)
		{
			while(((written_bytes = write(fd_output, line_ptr, read_bytes)) == -1) && (errno == EINTR));
			if(written_bytes < 0)
			{
				break;
			}
			total_bytes += written_bytes;
			read_bytes -= written_bytes;
			line_ptr += written_bytes;
		}
		if(written_bytes == -1)
		{
			break;
		}
		lock.l_type = F_UNLCK;
		fcntl(fd_output, F_SETLKW, &lock);

		lock.l_type = F_WRLCK;
		fcntl(fd_temp_2, F_SETLKW, &lock);

		if(stat(temp_file_path, &fileStat) < 0)
		{
			fprintf (stderr, "\nerrno = %d: %s (stat error. Check stat function)\n\n", errno, strerror (errno));
			exit(1);
		}
		rewrite = (char*) malloc(fileStat.st_size - i);

		lseek(fd_temp_2, i, SEEK_SET);
		while(((read_bytes = read(fd_temp_2, rewrite, fileStat.st_size - i)) == -1) && (errno == EINTR));
		truncate(temp_file_path, fileStat.st_size - i);
		if (read_bytes <= 0)
			break;
		line_ptr = rewrite;
		lseek(fd_temp_2, 0, SEEK_SET);
		while(read_bytes > 0)
		{
			while(((written_bytes = write(fd_temp_2, rewrite, read_bytes)) == -1) && (errno == EINTR));
			if(written_bytes < 0)
			{
				break;
			}
			temp_total_bytes += written_bytes;
			read_bytes -= written_bytes;
			line_ptr += written_bytes;
		}
		strcpy(rewrite," ");
		free(rewrite);
		if(written_bytes == -1)
		{
			break;
		}

		lock.l_type = F_UNLCK;
		fcntl(fd_temp_2, F_SETLKW, &lock);
	}
	
	if(count > 0)
	{
		mean_mae = mean(mean_absolute_error, count);
		mean_mse = mean(mean_squared_error, count);
		mean_rmse = mean(root_mean_squared_error, count);

		mean_dev_mae = median_deviation(mean_absolute_error, mean_mae, count);
		mean_dev_mse = median_deviation(mean_squared_error, mean_mse, count);
		mean_dev_rmse = median_deviation(root_mean_squared_error, mean_rmse, count);

		standard_dev_mae = standard_deviation(mean_absolute_error, mean_mae, count);
		standard_dev_mse = standard_deviation(mean_squared_error, mean_mse, count);
		standard_dev_rmse = standard_deviation(root_mean_squared_error, mean_rmse, count);
	}

	printf("\nmean absolute error, mean deviation: %.3lf, standard deviation: %.3lf\n", mean_dev_mae, standard_dev_mae);
	printf("\nmean squared error, mean deviation: %.3lf, standard deviation: %.3lf\n", mean_dev_mse, standard_dev_mse);
	printf("\nroot mean squared error, mean deviation: %.3lf, standard deviation: %.3lf\n", mean_dev_rmse, standard_dev_rmse);

	free(mean_absolute_error);
	free(mean_squared_error);
	free(root_mean_squared_error);

	if(close(fd_output) == -1)
	{
		fprintf (stderr, "\nerrno = %d: %s (close outputPath)\n\n", errno, strerror (errno));
		exit(1);
	}

	if(close(fd_temp_2) == -1)
	{
		fprintf (stderr, "\nerrno = %d: %s (close temporary file)\n\n", errno, strerror (errno));
		exit(1);
	}

	unlink(temp_file_path);
	unlink(input_file_path);
}

void handler(int sig)
{
	switch (sig)
	{
	case SIGUSR1:
		flag2 = 0;
        unlink(input_file_path);
		break;
	case SIGTERM:
		close(fd_output);
		close(fd_input);
		close(fd_temp);
		close(fd_temp_2);
		unlink(temp_file_path);
		if(input_file_path != NULL)
    	{
			unlink(input_file_path);
		}
    	if(pid == 0)
    	{
    		if(mean_absolute_error != NULL)
    			free(mean_absolute_error);
    		if(mean_squared_error != NULL)
    			free(mean_squared_error);
    		if(root_mean_squared_error != NULL)
    			free(root_mean_squared_error);
    		if(rewrite != NULL)
    			free(rewrite);
    		kill(getppid(), SIGKILL);
		}
    	else
    		kill(pid, SIGKILL);

        exit(0);
	case SIGINT:
		criticalFlag = 1;
		break;
	default:
		break;
	}
}

void print_signal_info()
{
	if(criticalFlag == 0)
		printf("No signal in critical section!\n");
	else
		printf("SIGINT was handled in critical section!\n");
}

void create_fd()
{
	fd_input = open(input_file_path, O_RDONLY);
	if(fd_input == -1)
	{
		fprintf (stderr, "\nerrno = %d: %s (open inputPath)\n\n", errno, strerror (errno));
		exit(1);
	}

	fd_output = open(output_file_path, O_WRONLY);
	if(fd_output == -1)
	{
		fprintf (stderr, "\nerrno = %d: %s (open outputPath)\n\n", errno, strerror (errno));
		exit(1);
	}

	fd_temp = mkstemp(temp_file_path);
	if(fd_temp == -1)
	{
		fprintf (stderr, "\nerrno = %d: %s (mkstemp could not create file)\n\n", errno, strerror (errno));
		exit(1);
	}

	fd_temp_2 = open(temp_file_path, O_RDWR);
	if(fd_temp_2 == -1)
	{
		fprintf (stderr, "\nerrno = %d: %s (open temp file)\n\n", errno, strerror (errno));
		exit(1);
	}
}

void close_files_process_1()
{
	if(close(fd_input) == -1)
	{
		fprintf (stderr, "\nerrno = %d: %s (close inputPath)\n\n", errno, strerror (errno));
		exit(1);
	}

	if(close(fd_temp) == -1)
	{
		fprintf (stderr, "\nerrno = %d: %s (close temporary file)\n\n", errno, strerror (errno));
		exit(1);
	}
}
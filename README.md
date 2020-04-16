# signal-handling

The project consists of two processes: the parent process **P1** and its child process **P2** that will run *concurrently.*
#
**P1:**

 - admits two command line parameters as paths to regular files,
 - read the contents of the file denoted by inputPath, 
 - and every couple of unsigned bytes it reads, is interpreted as a 2D coordinate (x,y), 
 - for every 10 coordinates (i.e. every 20 bytes) it reads, it’ll apply the **least squares method** to the corresponding 2D coordinates and calculate the
line equation (ax+b) that fits them. This calculation will be considered a critical section, and is not to be interrupted by **SIGINT/SIGSTOP.** 
 - then, **P1** writes in a comma separated line the 10 coordinates followed by the line equation as a new line of a temporary file created via *mkstemp*.

    (x_1, y_1),....,(x_10, y_10), ax+b

and proceed to the next 20 bytes and repeat.

#
**P2:** 

 - is forked early by **P1**, and will read the contents of the temporary file created by **P1**, line by line, 
 - for every line it reads, it calculates the mean absolute error (MAE), mean squared error (MSE) and root mean squared error (RMSE) between the coordinates and the estimated line. This calculation will be considered a critical section, and is not to be interrupted by **SIGINT/SIGSTOP** 
 - It then removes the line it read from the file and writes its own output to the file denoted by outputPath as 10 coordinates, the line equation (ax+b) and the three error estimates in a comma separated form:

    (x_1, y_1),....,(x_10, y_10), ax+b, MAE, MSE, RMSE

After processing all the contents of the temporary file and making sure that no more input will arrive, **P2** terminates and prints on screen for each error metric, its mean and standard deviation.

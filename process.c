#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include<string.h>

/*This structure is created to allacate dynamically pipes*/
struct pipes{
	int fdp[2];
};

void write_pipe(int fd, int id, long long int **array, int dim);
void read_pipe(int fd, long long int ***array,int fdW, int dim);
void err_sys(const char *msg);
void process_create(int n);
int getCharNum(char *argv[]);
void multipleArray(long long int ***array,int dimension);
void getNumbers(long long int numbers[]);
int getDimension();
void createArray(long long int ***array, int dimension,long long int numbers[]);
void createResultArray(long long int ***multiply, int dimension);
void process_create(int n);
void writeOutput(long long int **array,int dim, int order);
void createPipes(struct pipes *pipes, int n);
void writePipe(int fdp[],int i, long long int **array, int dim);
void writeNewLine(FILE *fp);
void writeProcID(FILE *fp, int queue, int id);
void checkError(int id);
void closeMainPipe(struct pipes pipe);
void checkParent(int id,int i, long long int **array,int fdp[], int dim);

/*this is main function of code.*/
int main(int argc, char *argv[]){
	
	int num=atoi(argv[1]);
	process_create(num);
	return 0;
}

/*This funciton is used to get array elements from given file.*/
void getNumbers(long long int numbers[]){
	
	FILE* f = fopen("matrix.txt", "r");
    long long int number = 0,i=0;
    
    while( fscanf(f, "%lld,", &number) > 0 ) /* parse %lld followed by ','*/
    	numbers[i++]=number;

    fclose(f);
}
/*This function is used to write order and process id as an output to screen and file*/
void writeProcID(FILE *fp, int queue, int id){
	printf("Process-%d %d\n",queue,id);
	fprintf(fp,"Process-%d %d\n",queue,id);
}
/*This function is used to write new line as an output to screen and file*/
void writeNewLine(FILE *fp){
	printf("\n");
    fprintf(fp,"\n");
}
/*This funciton is used to create dynamic array according for matrix multiplication result*/
void createResultArray(long long int ***multiply, int dimension){
	
	int i;
    *multiply = (long long int **)calloc(dimension, sizeof(long long int *));
    for (i=0; i<dimension; i++)
         (*multiply)[i] = (long long int *)calloc(dimension, sizeof(long long int));
}
/*This function is used to create pipes for communication between children*/
void createPipes(struct pipes *pipes, int n){
	int i;
    for(i=0;i<(n+1);i++){
    	if (pipe(pipes[i].fdp) < 0)
    		perror("Pipe Failed!");
	}
}
/*This function is used to write matric multiplication result to pipe*/
void writePipe(int fdp[],int i, long long int **array, int dim){
	close(fdp[0]);
    write_pipe(fdp[1],i,array,dim);
    close(fdp[1]);
}
/*This function is used to write all array elements to pipe*/
void write_pipe(int fd, int id, long long int **array, int dim){
    int j,i;
    
    for (i = 0; i <  dim; i++)
      for (j = 0; j < dim; j++){
      	if (write(fd, &array[i][j], sizeof(long long int)) < 0) 
    		printf("writing is not succesfull\n");
	  }
}
/*This funciton is used to get dimension of given array*/
int getDimension(){
	
	FILE* f = fopen("matrix.txt", "r");
    int dim,number = 0;
    
    if( fscanf(f, "%d,", &number) > 0 ) /* parse %d followed by ','*/
    	dim=number;/*Dimension is first element.*/

    fclose(f);
    return dim;
}
/*This funciton is used to create dynamic array according to given dimension,*/
/*then, array elements in given file are written to created dynamic array.*/
void createArray(long long int ***array, int dimension,long long int numbers[]){
	
	int i,j,x=1;
    *array = (long long int **)calloc(dimension, sizeof(long long int *));
    for (i=0; i<dimension; i++)
         (*array)[i] = (long long int *)calloc(dimension, sizeof(long long int));
         
    /*x is started from 1 as first index has dimension*/
    for (i = 0; i <  dimension; i++)
      for (j = 0; j < dimension; j++)
      	(*array)[i][j]=numbers[x++];
	  
}
/*This funciton is used to multiply by itself, then results of multiplicataion are overrited on origin array */
void multipleArray(long long int ***array,int dimension){
	int c,d,k,i,j;
	long long int sum=0;
	long long int **multiply;
	createResultArray(&multiply,dimension);
	
	/*array is multipied*/
	for (c = 0; c < dimension; c++) {
      for (d = 0; d < dimension; d++) {
        for (k = 0; k < dimension; k++) 
          sum = sum + (*array)[c][k]*(*array)[k][d];
        
        multiply[c][d] = sum;
        sum = 0;
      }
    }
    /*result of multipy is coppied to array.*/
    for (i = 0; i <  dimension; i++)
      for (j = 0; j < dimension; j++)
      	(*array)[i][j]=multiply[i][j];
}
/*This function is used to create processes according to wanted number*/
/*And, necessary functions are called to comminiate with children and parent with pipes*/
void process_create(int n) {
 	int i,dim=getDimension();
	long long int **array, *numbers=(long long int *)malloc(sizeof(long long int) *(dim*dim+1));
	getNumbers(numbers);
	
    createArray(&array,dim,numbers);
 	/*variables which are pipes and fork return values are dynamically allocated.*/
    struct pipes *pipes=(struct pipes *)malloc((n+1)*sizeof( struct pipes));
    int *pid = (int *)malloc(sizeof(int) * n);
    
    createPipes(pipes, n);
    /*children is created*/
    for (i = 0; i <n ; i++) {
        pid[i] = fork();
        
        checkError(pid[i]);
        checkParent(pid[i], i, array,pipes[i].fdp, dim);
        
        if(pid[i]== 0){/*(pid == 0) CHILD*/
        	read_pipe(pipes[i].fdp[0],&array,pipes[i].fdp[1],dim);/*reading from pipe by child*/
	  		multipleArray(&array,dim);
	  		writeOutput(array,dim,i);
        	writePipe(pipes[i+1].fdp,i,array,dim);/*wrting to pipe by child*/
        	exit(0);
		}/*original pipe endpoints are closed so, other child do not duplicate into of it*/
		closeMainPipe(pipes[i]);
    }/*reading by parent*/
    read_pipe(pipes[i].fdp[0],&array,pipes[i].fdp[1],dim);
}
/*This function is used to check whether there is error*/
void checkError(int id){
	if (id < 0) {
    	perror("Fork Failed!");
        exit(1);
    }
}
/*This function is used to check whether it is a child*/
void checkParent(int id,int i, long long int **array,int fdp[], int dim){
	if (id > 0){/*PARENT*/
		if(i==0) /*writing by parent*/
		    writePipe(fdp,i,array,dim);
    }
}
/*This function is used to close original pipe endpoints*/
void closeMainPipe(struct pipes pipe){
	close(pipe.fdp[1]);
   	close(pipe.fdp[0]);
}
/*This function is used to write matrix multiplication as an output to screen and files*/
void writeOutput(long long int **array,int dim, int order){
	
	FILE *fp;
	/*file name is converted to string*/
    char fileName[30];
    sprintf(fileName, "%d", (order+1));
    strcat(fileName,".txt");
    
    /*File is openned to write outputs.*/
    fp = fopen(fileName, "w");
    if (fp == NULL) {
    	printf("fp is NULL\n");
        exit(0);
    }
	writeProcID(fp,(order+1),getpid());
	int i,j;
	/*all array elements are written one by one*/
	for (i = 0; i <  dim; i++){
    	for (j = 0; j < dim; j++){
      		printf("%lli\t", array[i][j]);
      		fprintf(fp,"%lli\t",array[i][j]);
	  	}
	  	writeNewLine(fp);
	}
    writeNewLine(fp);
}
/*This function is used to read the result of matrix multiplication from pipe*/
void read_pipe(int fd, long long int ***array,int fdW, int dim){
	close(fdW);
	
 	long long int val,j,i,result;
 	/*all array elements are read one by one*/
	for (i = 0; i <  dim; i++)
      for (j = 0; j < dim; j++){
      	if ((result = read(fd, &val, sizeof(long long int))) > 0) 
      		(*array)[i][j]=val;
    	if (result < 0)
        	printf("reading is not succesfull\n");
	  }
	  
	close(fd);
}



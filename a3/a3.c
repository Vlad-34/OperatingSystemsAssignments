#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int fdREQ = -1, fdRESP = -1; // pipe file descriptors
int shmFd, fdMAP; // shared memory / mapped file file descriptor
char freeSharedMemFlag, freeMAPFlag;
unsigned int sharedMemSize; // size of shared memory
off_t filesize; // size of mapped file
volatile char* sharedMem; // shared memory pointer
char* map; // mapped file pointer

void connect()
{
	if(mkfifo("RESP_PIPE_75415", 0644) != 0) // make response pipe
	{
		perror("ERROR\n");
		exit(1);
	}
	fdREQ = open("REQ_PIPE_75415", O_RDONLY); // open request pipe (made by tester) as read-only
	fdRESP = open("RESP_PIPE_75415", O_WRONLY); // open response pipe as write-only
	if(fdREQ == -1 || fdREQ == -1)
	{
		perror("ERROR\n");
		exit(1);
	}
	char nrChars = 7; // length of CONNECT
	write(fdRESP, &nrChars, 1);
	write(fdRESP, "CONNECT", nrChars); // send CONNECT
}

void ping()
{
	char nrChars = 4; // length of PING; also length of PONG
	write(fdRESP, &nrChars, 1);
	write(fdRESP, "PING", nrChars); // send PING
	write(fdRESP, &nrChars, 1);
	write(fdRESP, "PONG", nrChars); // send PONG
	unsigned int variant = 75415; // variant
	write(fdRESP, &variant, sizeof(unsigned int)); // send variant
}

void create_shm()
{
	freeSharedMemFlag = 1; // flag for freeing the shared memory
	char nrChars = 10; // length of CREATE_SHM
	char success = 1;
	write(fdRESP, &nrChars, 1);
	write(fdRESP, "CREATE_SHM", nrChars); // send CREATE_SHM

	shmFd = shm_open("/Yy1nGX", O_CREAT | O_RDWR, 0664);
	if(shmFd < 0)
		success = 0;

	read(fdREQ, &sharedMemSize, sizeof(unsigned int)); // receive size of shared memory
	ftruncate(shmFd, sharedMemSize);

	sharedMem = (volatile char*) mmap(NULL, sharedMemSize, PROT_WRITE | PROT_READ, MAP_SHARED, shmFd, 0); // shared memory map
	if(sharedMem == (void*)-1)
		success = 0;

	if(!success)
	{
		nrChars = 5; // length of ERROR
		write(fdRESP, &nrChars, 1);
		write(fdRESP, "ERROR", nrChars); // send ERROR
	}
	else
	{
		nrChars = 7; // length of SUCCESS
		write(fdRESP, &nrChars, 1);
		write(fdRESP, "SUCCESS", nrChars); // send SUCCESS
	}
}

void write_to_shm()
{
	unsigned int offset = 0, value = 0;
	char success = 1;
	read(fdREQ, &offset, sizeof(unsigned int)); // receive offset
	read(fdREQ, &value, sizeof(unsigned int)); // receive value

	char nrChars = 12; // length of WRITE_TO_SHM
	write(fdRESP, &nrChars, 1);
	write(fdRESP, "WRITE_TO_SHM", nrChars); // send WRITE_TO_SHM

	if(offset + 3 > sharedMemSize)
		success = 0;

	if(!success)
	{
		nrChars = 5; // length of ERROR
		write(fdRESP, &nrChars, 1);
		write(fdRESP, "ERROR", nrChars); // send ERROR
	}
	else
	{
		sharedMem[offset + 3] = (char)(value >> 24);
		sharedMem[offset + 2] = (char)((value & 0x00FF0000) >> 16);
		sharedMem[offset + 1] = (char)((value & 0x0000FF00) >> 8);
		sharedMem[offset + 0] = (char)(value & 0x000000FF);

		nrChars = 7; // length of SUCCESS
		write(fdRESP, &nrChars, 1);
		write(fdRESP, "SUCCESS", nrChars); // send SUCCESS
	}
}

void map_file()
{
	freeMAPFlag = 1;
	char success = 1;
	char nrChars = 8; // length of MAP_FILE
	write(fdRESP, &nrChars, 1);
	write(fdRESP, "MAP_FILE", nrChars); // send MAP_FILE

	read(fdREQ, &nrChars, 1); // receive filename length
	char* filename = (char*)malloc((nrChars + 1) * sizeof(char));
	read(fdREQ, filename, nrChars); // receive filename
	filename[(int) nrChars] = '\0';

	fdMAP = open(filename, O_RDONLY); // open file
	if(fdMAP == -1)
		success = 0;

	filesize = lseek(fdMAP, 0, SEEK_END); // get filesize
	lseek(fdMAP, 0, SEEK_SET);

	map = (char*)mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fdMAP, 0); // map the file
	if(map == (void*)-1)
		success = 0;

	if(!success)
	{
		nrChars = 5; // length of ERROR
		write(fdRESP, &nrChars, 1);
		write(fdRESP, "ERROR", nrChars); // send ERROR
	}
	else
	{
		nrChars = 7; // length of SUCCESS
		write(fdRESP, &nrChars, 1);
		write(fdRESP, "SUCCESS", nrChars); // send SUCCESS
	}
	free(filename);
}

void read_from_file_offset()
{
	unsigned int offset = 0, no_of_bytes = 0;
	read(fdREQ, &offset, sizeof(unsigned int)); // receive offset
	read(fdREQ, &no_of_bytes, sizeof(unsigned int)); // receive offset

	char success = 1;
	if(offset + no_of_bytes > filesize)
		success = 0;

	char nrChars = 21; // length of READ_FROM_FILE_OFFSET
	write(fdRESP, &nrChars, 1);
	write(fdRESP, "READ_FROM_FILE_OFFSET", nrChars); // send READ_FROM_FILE_OFFSET

	if(!success)
	{
		nrChars = 5; // length of ERROR
		write(fdRESP, &nrChars, 1);
		write(fdRESP, "ERROR", nrChars); // send ERROR
	}
	else
	{
        for(int i = 0; i < no_of_bytes; i++) // read data
            sharedMem[i] = map[i + offset];


		nrChars = 7; // length of SUCCESS
		write(fdRESP, &nrChars, 1);
		write(fdRESP, "SUCCESS", nrChars); // send SUCCESS
	}
}

unsigned int chars_to_unsigned_int(char a, char b, char c, char d)
{
	return ((a & 0xFF) << 0) | ((b & 0xFF) << 8) | ((c & 0xFF) << 16) | ((d & 0xFF) << 24);
}

void read_from_file_section()
{
	unsigned int section_no = 0, offset = 0, no_of_bytes = 0;
	read(fdREQ, &section_no, sizeof(unsigned int)); // receive section_no
	read(fdREQ, &offset, sizeof(unsigned int)); // receive offset
	read(fdREQ, &no_of_bytes, sizeof(unsigned int)); // receive no_of_bytes

	char success = 1;

	if(map[0] != 'G' || map[1] != 'r') // check MAGIC
		success = 0;

	if(chars_to_unsigned_int(map[4], map[5], 0, 0) < 83 || chars_to_unsigned_int(map[4], map[5], 0, 0) > 185) // check VERSION
		success = 0;

	if(map[6] < 8 || map[6] > 14 || map[6] < section_no) // check NO_OF_SECTIONS
		success = 0;

	unsigned int sect_offset = 7 + 17 * (section_no - 1) + 9, sect_size = sect_offset + 4; // get offset of section offset and section size
	unsigned int sect_offset_number = 0, sect_size_number = 0; // convert them to numbers
	sect_offset_number = chars_to_unsigned_int(map[sect_offset], map[sect_offset + 1], map[sect_offset + 2], map[sect_offset + 3]);
	sect_size_number = chars_to_unsigned_int(map[sect_size], map[sect_size + 1], map[sect_size + 2], map[sect_size + 3]);

	if(sect_size_number < no_of_bytes) // check SECT_SIZE
		success = 0;

	for(int i = 0; i < map[6]; i++)
		if(map[7 + 17 * i + 8] != 16 && map[7 + 17 * i + 8] != 80) // check SECT_TYPE
			success = 0;

	char nrChars = 22; // length of READ_FROM_FILE_SECTION
	write(fdRESP, &nrChars, 1);
	write(fdRESP, "READ_FROM_FILE_SECTION", nrChars); // send READ_FROM_FILE_SECTION

	if(!success)
	{
		nrChars = 5; // length of ERROR
		write(fdRESP, &nrChars, 1);
		write(fdRESP, "ERROR", nrChars); // send ERROR
	}
	else
	{
		for(int i = 0; i < no_of_bytes; i++) // read data
            sharedMem[i] = map[i + sect_offset_number + offset];
		nrChars = 7; // length of SUCCESS
		write(fdRESP, &nrChars, 1);
		write(fdRESP, "SUCCESS", nrChars); // send SUCCESS
	}
}

void read_from_logical_space_offset()
{
	unsigned int logical_offset = 0, no_of_bytes = 0;
	read(fdREQ, &logical_offset, sizeof(unsigned int)); // receive logical_offset
	read(fdREQ, &no_of_bytes, sizeof(unsigned int)); // receive no_of_bytes

	char success = 1;

	if(map[0] != 'G' || map[1] != 'r') // check MAGIC
		success = 0;

	if(chars_to_unsigned_int(map[4], map[5], 0, 0) < 83 || chars_to_unsigned_int(map[4], map[5], 0, 0) > 185) // check VERSION
		success = 0;

	if(map[6] < 8 || map[6] > 14) // check NO_OF_SECTIONS
		success = 0;

	for(int i = 0; i < map[6]; i++)
		if(map[7 + 17 * i + 8] != 16 && map[7 + 17 * i + 8] != 80) // check SECT_TYPE
			success = 0;

	char nrChars = 30; // length of READ_FROM_FILE_SECTION
	write(fdRESP, &nrChars, 1);
	write(fdRESP, "READ_FROM_LOGICAL_SPACE_OFFSET", nrChars); // send READ_FROM_FILE_SECTION

	if(!success)
	{
		nrChars = 5; // length of ERROR
		write(fdRESP, &nrChars, 1);
		write(fdRESP, "ERROR", nrChars); // send ERROR
	}
	else
	{
		unsigned int sect_offset_number[14], sect_size_number[14], start[14] = { 0 }, finish[14] = { 0 };
		for(int i = 0; i < map[6]; i++) // for each section, find the physical offset and size
		{
			unsigned int sect_offset = 7 + 17 * i + 9, sect_size = sect_offset + 4;
			sect_offset_number[i] = chars_to_unsigned_int(map[sect_offset], map[sect_offset + 1], map[sect_offset + 2], map[sect_offset + 3]);
			sect_size_number[i] = chars_to_unsigned_int(map[sect_size], map[sect_size + 1], map[sect_size + 2], map[sect_size + 3]);

			if(i == 0)
				start[i] = 0, finish[i] = sect_size_number[i]; // compute the start and end of section in logical memory space
			else
			{
				start[i] = (finish[i - 1] + 2048) / 2048 * 2048;
				finish[i] = start[i] + sect_size_number[i];
			}
		}

		unsigned int physicalSection, physicalRelativeOffset; // find the physical section and physical offset relative to the start of the section
		for(int i = 0; i < map[6]; i++)
		{
			if(logical_offset < start[i])
			{
				physicalSection = i;
				break;
			}
		}
		physicalRelativeOffset = logical_offset - start[physicalSection - 1];
		
		for(int i = 0; i < no_of_bytes; i++) // read data
            sharedMem[i] = map[i + sect_offset_number[physicalSection - 1] + physicalRelativeOffset];

		nrChars = 7; // length of SUCCESS
		write(fdRESP, &nrChars, 1);
		write(fdRESP, "SUCCESS", nrChars); // send SUCCESS
	}
}

void quit()
{
	if(freeSharedMemFlag == 1) // free shared memory
	{
		munmap((void*)sharedMem, sharedMemSize);
		sharedMem = NULL;
		close(shmFd);
		shm_unlink("/Yy1nGX");
	}
	if(freeMAPFlag == 1)
	{
		munmap(map, filesize);
		close(fdMAP);
	}
	close(fdREQ); // close connection / request pipe
	close(fdRESP); // close response pipe
	unlink("RESP_PIPE_75415"); // remove response pipe
	exit(0);
}

int main()
{
	connect();
	for(;;)
	{
		unsigned int commandLength = 0;
		read(fdREQ, &commandLength, 1); // receive command length
		char *command = (char*) malloc((commandLength + 1) * sizeof(char));
		read(fdREQ, command, commandLength); // receive command
		command[commandLength] = '\0';

		if(strcmp(command, "PING") == 0) // execute commands
			ping();
		else if(strcmp(command, "CREATE_SHM") == 0)
			create_shm();
		else if(strcmp(command, "WRITE_TO_SHM") == 0)
			write_to_shm();
		else if(strcmp(command, "MAP_FILE") == 0)
			map_file();
		else if(strcmp(command, "READ_FROM_FILE_OFFSET") == 0)
			read_from_file_offset();
		else if(strcmp(command, "READ_FROM_FILE_SECTION") == 0)
			read_from_file_section();
		else if(strcmp(command, "READ_FROM_LOGICAL_SPACE_OFFSET") == 0)
			read_from_logical_space_offset();
		else if(strcmp(command, "EXIT") == 0)
			quit();
		free(command);
	}
	return 0;
}

#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Runner.h"

#define clusSize() 			((bootEntry.SecPerClus)*(bootEntry.BytsPerSec))
#define rsvdSize()			((bootEntry.RsvdSecCnt)*(bootEntry.BytsPerSec))
#define FATSize()			((bootEntry.FATSz32)*(bootEntry.BytsPerSec))
#define clusPos(n)			(((bootEntry.RsvdSecCnt) + ((bootEntry.NumFATs)*(bootEntry.FATSz32)) + ((n-2)*(bootEntry.SecPerClus)))*(bootEntry.BytsPerSec))
#define FATEntPos(fcn, f)	(rsvdSize() + f*FATSize() + 4*fcn)

Runner::Runner()
{

}

Runner::~Runner()
{
	free(FAT);
}

bool Runner::syntaxCheck(int argc, char* argv[])
{
	//testing
	//std::cout << "argc: "<<this->argc<< std::endl;
	//for(int i=0; i<this->argc; i++)
	//	std::cout << "argv["<<i<<"]: "<<argv[i] << std::endl;
	//end testing
	int 		c;
	bool 		dd, ii, ll, rr, RR, oo;
	std::string dvalue, rvalue, Rvalue, ovalue;

	dd=ii=ll=rr=RR=oo = false;

	while ((c = getopt (argc, argv, "d:ilr:R:o:")) != -1)
	    switch (c)
		{
			case 'd':
				dd = true;
				dvalue = optarg;
				break;
			case 'i':
				ii = true;
				break;
			case 'l':
				ll = true;
				break;
			case 'r':
				rr = true;
				rvalue = optarg;
				break;
			case 'R':
				RR = true;
				Rvalue = optarg;
				break;
			case 'o':
				oo = true;
				ovalue = optarg;
				break;
			case '?':
			/*	if (optopt == 'c')
				  fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint (optopt))
				  fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
				  fprintf (stderr,
				           "Unknown option character `\\x%x'.\n",
				           optopt);
				return 1;*/
				return false;
				break;
			default:
				return false;
		}

	//finish extracting options. Now process them
	if(!dd)		return false;
	device.filename = dvalue;
	if 		(ii && !ll && !rr && !RR && !oo)
	{
		action = PRINTBOOT;
		//testing
		//std::cout << "action: PRINTBOOT" << std::endl;
		return true;
	}
	else if (ll && !ii && !rr && !RR && !oo)
	{
		action = LISTDIR;
		//testing
		//std::cout << "action: LISTDIR" << std::endl;
		return true;
	}
	else if (rr && oo && !ii && !ll && !RR)
	{
		action = EIGHTPTHREE;
		target = rvalue;
		dest   = ovalue;
		//testing
		//std::cout << "action: EIGHTPTHREE" << std::endl;
		return true;
	}
	else if (RR && oo && !ii && !ll && !rr)
	{
		action = LONGFN;
		target = Rvalue;
		dest   = ovalue;
		//testing
		//std::cout << "action: LONGFN" << std::endl;
		return true;
	}

	return false;
}

void Runner::printHelp()
{
	std::cout << "Usage: ./recover -d [device filename] [other arguments]"	<< std::endl;
	std::cout << "-i\t\t\tPrint boot sector information"					<< std::endl;	
	std::cout << "-l\t\t\tList all the directory entries"					<< std::endl;	
	std::cout << "-r target -o dest\tFile recovery with 8.3 filename"		<< std::endl;			
	std::cout << "-R target -o dest\tFile recovery with long filename"		<< std::endl;				
}

void Runner::run()
{
	if((device.fd=open(device.filename.c_str(), O_RDONLY))==-1)
    {
    	std::cout <<"Cannot open device"<< std::endl;
    	return;
    }

	if(readBootEntry()==-1)		return;
	if(readFAT()==-1)			return;
	if(readDirEntry()==-1)		return;

	switch(action)
	{
		case PRINTBOOT:
			printFSinfo();
			break;
		case LISTDIR:
			listDir();
			break;
		case EIGHTPTHREE:
			eightPthree();
			break;
		case LONGFN:
			longFN();
			break;
		default:
			std::cout <<"what should I do?"<< std::endl;
	}

	close(device.fd);
}

int Runner::readBootEntry()
{
    unsigned char buf[512];                     //buffer

    lseek(device.fd, 0, SEEK_SET);
    read(device.fd, buf, 512);

    memcpy(&bootEntry.BytsPerSec, buf+11, 2);
    memcpy(&bootEntry.SecPerClus, buf+13, 1);
    memcpy(&bootEntry.RsvdSecCnt, buf+14, 2);
    memcpy(&bootEntry.NumFATs, 	  buf+16, 1);
    memcpy(&bootEntry.TotSec16,   buf+19, 2);
    memcpy(&bootEntry.TotSec32,   buf+32, 4);
    memcpy(&bootEntry.FATSz32,    buf+36, 4);
    memcpy(&bootEntry.RootClus,   buf+44, 4);

    //testing
	//printf("bytesPerSector    : %d\n",bootEntry.BytsPerSec);
    //printf("sectorsPerCluster : %d\n",bootEntry.SecPerClus);
    //printf("reservedSectors   : %d\n",bootEntry.RsvdSecCnt);
    //printf("Number of FATs	  : %d\n",bootEntry.NumFATs	);
    //printf("Num of Sectors-16 : %d\n",bootEntry.TotSec16	);
    //printf("Num of Sectors-32 : %ld\n",bootEntry.TotSec32	);
    //printf("Sectors per FAT	  : %ld\n",bootEntry.FATSz32	);
    //printf("RootDir Location  : %ld\n",bootEntry.RootClus	);
    return 0;
}

int Runner::readFAT()
{
    lseek(device.fd, rsvdSize(), SEEK_SET);				//set offset to read FAT
    FAT = (unsigned int*)malloc(FATSize());
    //std::cout << (bootEntry.FATSz32)*(bootEntry.BytsPerSec) << std::endl;
    read(device.fd, (void*)FAT, FATSize());

    alloClusCnt = -2;
	freeClusCnt = 0;
	unknClusCnt = 0;
    //std::cout << (bootEntry.FATSz32)*(bootEntry.BytsPerSec)/sizeof(int) << std::endl;
    for(int i=0; i<(FATSize()/sizeof(int)); i++){
    	if(FAT[i]==0)
    		freeClusCnt++;
    		//printf("FAT entry [%d] : %x\n", i, FAT[i]);
    	else //if(FAT[i]>=0x0ffffff8)
    		alloClusCnt++;
    	//else
    	//	unknClusCnt++;
    }

    return 0;
}

int Runner::readDirEntry()
{
	LFNEntry* lfnEntry;
	DirEntry* dirEntry;
	std::vector<LFNEntry*> LFN;
	unsigned char* buf;

	buf = (unsigned char*)malloc(clusSize());
	//printf("rootDir Clus count : %d\n",clusCnt(bootEntry.RootClus));
	//for(int i=bootEntry.RootClus; FAT[i]<0x0ffffff8; i=FAT[i])
	//for(int i=0; i<3; i++)
	int i = bootEntry.RootClus;
	do
	{
		//printf("next FAT value : %x\n", FAT[i]);
		//printf("now in cluster : %d\n", i);
		//printf("DirectoryEntry Position : %x\n",clusPos(bootEntry.RootClus+i));
		lseek(device.fd, clusPos(i), SEEK_SET);
		read(device.fd, buf, clusSize());
		//printf("bytes read : %d\n",r);
		//pread(device.fd, buf, clusSize(), clusPos(bootEntry.RootClus+i));	//alternative implementation

		unsigned char* tmp = buf;
		for(int i=0; i<clusSize()/32; i++)
		{
			//testing
			//for(int i=0; i<32; i++)
			//	printf("%x ",tmp[i]);
			//printf("\n");
			//end testing

			//if 		(memcmp(tmp+0, "\xE5", 1)==0)
			//	goto next;													//E5 represent delete entries
			//else 
			if 		(memcmp(tmp+0, "\x00", 1)==0)
				goto next;													//00 represent unallocated slots

		    else if (memcmp(tmp+11, "\x0F", 1)==0)							//check if LFN entry
		    {
		    	lfnEntry = (LFNEntry*)malloc(sizeof(LFNEntry));				//push_front to LFN Entry (temp)
		    	memcpy(&(lfnEntry->SeqNum),	tmp+0, 1);
		    	memcpy((lfnEntry->Name)+0,	tmp+1, 10);						//why???? I don't know why no &
		    	memcpy((lfnEntry->Name)+10,tmp+14, 12);						//why???? I don't know why no &
		    	memcpy((lfnEntry->Name)+22,tmp+28, 4);						//why???? I don't know why no &
		    	LFN.push_back(lfnEntry);
		    	//testing
		    	//printf("LFN SeqNum  :  %x\n",(int)LFN[LFN.size()-1]->SeqNum);
		    }
		    else	//main entry
		    {
		    	dirEntry = (DirEntry*)malloc(sizeof(DirEntry));
		    	dirEntry->LFN = LFN;
	    		memcpy(&(dirEntry->Name), 						tmp+0, 11);
	    		memcpy(&(dirEntry->Attr), 						tmp+11, 1);
	    		memcpy((unsigned char*)&(dirEntry->FstClus)+2, 	tmp+20, 2);
	    		memcpy((unsigned char*)&(dirEntry->FstClus), 	tmp+26, 2);
	    		memcpy(&(dirEntry->FileSize), 					tmp+28, 4);
	    		LFN.clear();

	    		if (memcmp(tmp+0, "\xE5", 1)==0)
	    			e5.push_back(dirEntry);									//assign to deleted entry
	    		else
	    			dir.push_back(dirEntry);								//assign to normal entry

	    		//testing
	    		//for(int i=0; i<dirEntry->LFN.size(); i++)
	    		//	printf("LFN SeqNum  :  %x\n",(int)dirEntry->LFN[i]->SeqNum);
	    		//std::cout<< dirEntry->Name <<std::endl;
		    }

		    next:
			tmp += 32;
		}
		i = FAT[i];
	}while(i<0x0ffffff8);
	free(buf);

	return 0;
}

void Runner::printFSinfo()
{
	std::cout <<"Number of FATs = "					<<	(int)bootEntry.NumFATs	<<std::endl;
	std::cout <<"Number of bytes per sector = "		<<	bootEntry.BytsPerSec 	<<std::endl;
	std::cout <<"Number of sectors per cluster = "	<< 	(int)bootEntry.SecPerClus<<std::endl;
	std::cout <<"Number of reserved sectors = "		<<	bootEntry.RsvdSecCnt	<<std::endl;
	std::cout <<"Number of allocated clusters = "	<<	alloClusCnt				<<std::endl;
	std::cout <<"Number of free clusters = "		<<	freeClusCnt				<<std::endl;
	//std::cout <<"Number of unknown clusters = "		<<	unknClusCnt				<<std::endl;//no use, delete it before final version
}

void Runner::listDir()
{
	for(int i=0; i<dir.size(); i++)
	{
		//index
		printf("%d, ",i+1);
		//8.3 filename
		for(int j=0; j<8; j++)
			if(dir[i]->Name[j]!=' ')
				printf("%c", dir[i]->Name[j]);
		if(dir[i]->Name[8]!=' ')
			printf(".");
		for(int j=8; j<=10; j++)
			if(dir[i]->Name[j]!=' ')
				printf("%c", dir[i]->Name[j]);
		if(dir[i]->Attr=='\x10')						//for DirectoryEntry 	0x10 = 00001000
			printf("/");
		//Long Filename
		if(dir[i]->LFN.size()>0)
			printf(", ");
		for(int j=dir[i]->LFN.size()-1; j>=0; j--)
		{
			unsigned char* ptr=dir[i]->LFN[j]->Name;
			int off=0;
			//printf("%x",*(ptr+off));
			//printf("%x",*(ptr+off+2));
			while(off<26 && *(ptr+off)!='\x00' && *(ptr+off+1)=='\x00')
			{
				printf("%c", *(ptr+off));
				off += 2;
			}
		}
		if(dir[i]->Attr=='\x10')						//for directory
			printf("/");
		//file size
		printf(", ");
		printf("%ld", dir[i]->FileSize);
		//starting cluster number
		printf(", ");
		printf("%d", dir[i]->FstClus);
		//line break
		printf("\n");
	}
}

int Runner::eptFilenameCheck()
{
	//filter illegal characters
	for(int i=0; i<target.size(); i++)
		if(!isupper(target[i]) && !isdigit(target[i]) && target[i]!='.')			//de-morgan's theorem
			return -1;
	//check only one dot exist
	int count = 0;
	for(int i=0; i<target.size(); i++)
		if(target[i]=='.')
			count++;
	if(count>1)
		return -1;
	//check number of character before and after dot
	std::string eight = target.substr(0,target.find('.'));
	std::string three = target.substr(target.find('.')+1);
	if(eight.size()==0 || eight.size()>8)
		return -1;
	if(count==1)
		if(three.size()==0 || three.size()>3)
			return -1;

	//return value is count of dots
	return count;
}

void Runner::eightPthree()
{
	typedef struct FN{
		unsigned char 	e;
		unsigned char 	eight[8];
		int				dotCnt;
		unsigned char 	three[3];
	}FN;
	FN fn;

	if((fn.dotCnt=eptFilenameCheck())==-1)
	{
		//std::cout << target<<": error - file not found (actually is invalid filename. Delete this bracket later)" << std::endl;
		std::cout << target<<": error - file not found" << std::endl;
		return;
	}
	std::string eight = target.substr(0,target.find('.'));
	std::string three = target.substr(target.find('.')+1);
	//fill-up fn.eight
	memcpy(fn.eight, (const unsigned char*)eight.c_str(), eight.size());
	memset(fn.eight+eight.size(), '\x20', 8-eight.size());
	//fill-up fn.three
	if(fn.dotCnt==0)
		memset(fn.three, '\x20', 3);
	else
	{
		memcpy(fn.three, (const unsigned char*)three.c_str(), three.size());
		memset(fn.three+three.size(), '\x20', 3-three.size());
	}
	//testing
	//for(int i=0; i<8; i++)
	//	printf("%x ",fn.eight[i]);
	//for(int i=0; i<3; i++)
	//	printf("%x ",fn.three[i]);

	//start searching in e5 vector
	memcpy(&(fn.e), fn.eight, 1);
	memset(fn.eight, '\xE5', 1);
	int ambCnt = 0;
	for(int i=0; i<e5.size(); i++)
		if(memcmp(fn.eight, e5[i]->Name+0, 8)==0 && memcmp(fn.three, e5[i]->Name+8, 3)==0)
			ambCnt++;
	if(ambCnt>1)
	{
		std::cout << target<<": error - ambiguous" << std::endl;
		return;
	}

	//search in disk for that entry
	unsigned char* buf;
	buf = (unsigned char*)malloc(clusSize());

	int clus = bootEntry.RootClus;
	do
	{
		lseek(device.fd, clusPos(clus), SEEK_SET);
		read(device.fd, buf, clusSize());

		unsigned char* tmp = buf;
		//for reading each entry 		entry = tmp[0]->tmp[31]
		for(int i=0; i<clusSize()/32; i++)
		{
			//check for 1. deleted entry 2. matched filename
			if(memcmp(fn.eight, tmp+0, 8)==0 && memcmp(fn.three, tmp+8, 3)==0)
			{
				//check FAT entry. Return for cluster occupied. Assume recovery size to be 1 only
				int FstClus;
				int FileSize;
	    		memcpy((unsigned char*)&(FstClus)+2, 	tmp+20, 2);
	    		memcpy((unsigned char*)&(FstClus), 	tmp+26, 2);
	    		memcpy((unsigned char*)&(FileSize), tmp+28, 4);
	    		//printf("%x %x ", *(tmp+20), *(tmp+21));
	    		//printf("%x %x\n", *(tmp+26), *(tmp+27));
	    		//printf("FstClus : %x\n", FstClus);
	    		//printf("FAT[FstClus] : %lx\n", FAT[FstClus]);
	    		if(FAT[FstClus]==0x00000000)
	    		{
	    			//test dest file open-able
	    			FILE* destFP;
	    			destFP = fopen(dest.c_str(), "w");
	    			if(destFP != NULL)
	    			{
	    				//can open output file, do recovery

						//write the deleted entry (from e5 to char)
//						lseek(device.fd, clusPos(clus)+ i*32 +0, SEEK_SET);
//						write(device.fd, &fn.e, 1);

						//write FATs, assume more than 1 FAT table
//						for(int f=0; f<bootEntry.NumFATs; f++)
//						{
//							lseek(device.fd, FATEntPos(FstClus, f), SEEK_SET);
//							write(device.fd, &FstClus, 4);
//						}
						

						//write file to buf, then to target file
						//printf("filesize : %d\n", FileSize);
						lseek(device.fd, clusPos(FstClus), SEEK_SET);
						read(device.fd, buf, FileSize);			//byte 28 indicated filesize
						for(int i=0; i<FileSize; i++)
							fprintf(destFP, "%c", buf[i]);

						//print successful message
						std::cout <<target<<": recovered" << std::endl;

						//testing
						//printf("FAT0EntryPos : %lx\n", FATEntPos(FstClus, 0));
						//printf("FAT1EntryPos : %lx\n", FATEntPos(FstClus, 1));
						//printf("Entry e5 Pos : %lx\n", clusPos(clus)+ i*32 +0);
						//for(int i=0; i<32; i++)
						//	printf("%p ",tmp);
						//printf("\n");
						//end testing
	    			}
	    			else
	    			{
						//cannot open output file
	    				std::cout <<target<<": failed to open" << std::endl;
	    			}
	    			fclose(destFP);
	    			goto exitPoint;
	    		}
	    		else
	    		{
	    			//cluster occupied. Return
	    			std::cout << target<<": error - fail to recover" << std::endl;
	    			goto exitPoint;
	    		}
			}
			tmp += 32;
		}
		clus = FAT[clus];
	}while(clus<0x0ffffff8);

	//only file not found can reach here
	std::cout << target<<": error - file not found" << std::endl;

	exitPoint:
	free(buf);
	return;
}

int Runner::lfnCheck()
{
	if(target.size()>255)
		return -1;
	for(int i=0; i<target.size(); i++)
		if(!isascii(target[i]))
			return -1;
	return 0;
}

int Runner::lfnBlockCmp(std::vector<LFNEntry*> entLFN, std::vector<LFNEntry*> fnLFN)
{
	std::reverse(entLFN.begin(), entLFN.end());

	for(int i=0; i<target.size(); i++)
	{
		//printf("%c %c\n", entLFN[i*2/26]->Name[(i*2)%26] ,fnLFN[i*2/26]->Name[(i*2)%26]);
		if(entLFN[i*2/26]->Name[(i*2)%26] != fnLFN[i*2/26]->Name[(i*2)%26])
			return -1;
	}
	return 0;
}

void Runner::longFN()
{
	typedef struct fn{
		std::vector<LFNEntry*> LFN;
	}FN;
	FN fn;

	if(lfnCheck()==-1)
	{
		//std::cout << target<<": error - file not found (actually is invalid filename. Delete this bracket later)" << std::endl;
		std::cout << target<<": error - file not found" << std::endl;
		return;
	}
	
	int j = -1;
	for(int i=0; i<target.size(); i++)
	{
		if(i%13 == 0)
		{
			fn.LFN.push_back((LFNEntry*)malloc(sizeof(LFNEntry)));
			j++;
		}
		fn.LFN[j]->Name[(i*2)%26] = (unsigned char)target[i];
	}
	//testing
	//printf("testing LFN : ");
	//for(int i=0; i<fn.LFN.size(); i++)
	//	for(int j=0; j<13; j++)
	//		printf("%c",fn.LFN[i]->Name[j*2]);
	//printf("\n");

	//std::reverse(fn.LFN.begin(), fn.LFN.end());
	//testing
	//printf("testing LFN : ");
	//for(int i=0; i<fn.LFN.size(); i++)
	//	for(int j=0; j<13; j++)
	//		printf("%c",fn.LFN[i]->Name[j*2]);
	//printf("\n");

	//to compare, they must have same vector length
	for(int i=0; i<e5.size(); i++)
	{
		if(e5[i]->LFN.size() == fn.LFN.size())						//they must have same vector length
		{
			if(lfnBlockCmp(e5[i]->LFN, fn.LFN)==0)
			{
				//recover process
	    		if(FAT[e5[i]->FstClus]==0x00000000)
	    		{
	    			//test dest file open-able
	    			FILE* destFP;
	    			destFP = fopen(dest.c_str(), "w");
	    			if(destFP != NULL)
	    			{
	    				unsigned char* buf;
						buf = (unsigned char*)malloc(clusSize());

						//write file to buf, then to target file
						lseek(device.fd, clusPos(e5[i]->FstClus), SEEK_SET);
						read(device.fd, buf, e5[i]->FileSize);			//byte 28 indicated filesize
						//printf("filesize : %d\n", e5[i]->FileSize);
						for(int j=0; j<e5[i]->FileSize; j++)
							fprintf(destFP, "%c", buf[j]);

						//print successful message
						std::cout <<target<<": recovered" << std::endl;

						//testing
						//printf("FAT0EntryPos : %lx\n", FATEntPos(FstClus, 0));
						//printf("FAT1EntryPos : %lx\n", FATEntPos(FstClus, 1));
						//printf("Entry e5 Pos : %lx\n", clusPos(clus)+ i*32 +0);
						//for(int i=0; i<32; i++)
						//	printf("%p ",tmp);
						//printf("\n");
						//end testing
	    			}
	    			else
	    			{
						//cannot open output file
	    				std::cout <<target<<": failed to open" << std::endl;
	    			}
	    			fclose(destFP);
	    			return;
	    		}
	    		else
	    		{
	    			//cluster occupied. Return
	    			std::cout << target<<": error - fail to recover" << std::endl;
	    			return;
	    		}
				std::cout <<target<<": recovered" << std::endl;

				return;												//leave the sub routine!!!
			}
			else
			{
				
			}
		}
	}
	//error message
	std::cout << target<<": error - file not found" << std::endl;
	return;
}
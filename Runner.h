class Runner
{
	public:

	private:
		typedef enum 	{PRINTBOOT,LISTDIR,EIGHTPTHREE,LONGFN} Action;
		Action 			action;
		std::string 	device_filename;
		std::string 	target;
		std::string		dest;

		typedef struct Device
		{
			int 		fd;
			std::string filename;
		}Device;
		Device device;

		typedef struct BootEntry
		{
			unsigned short BytsPerSec;	/* [11][12] Bytes per sector. Allowed values include 512, 1024, 2048, and 4096 */
			unsigned char SecPerClus;	/* [13]		Sectors per cluster (data unit). Allowed values are powers of 2, but the cluster size must be 32KB or smaller */
			unsigned short RsvdSecCnt;	/* [14][15] Number of sectors in the reserved area */
			unsigned char NumFATs;		/* [16]		Number of FATs */
			unsigned short TotSec16;	/* [19][20] 16-bit value of number of sectors in file system */
			unsigned long TotSec32;		/* [32-25]	32-bit value of number of sectors in file system. Either this value or the 16-bit value above must be 0 */
			unsigned long FATSz32;		/* [36-39]	Sectors per FAT. 32-bit size in sectors of one FAT */
			unsigned long RootClus;		/* [44-47]	Cluster where the root directory can be found */
		}BootEntry;
		BootEntry bootEntry;

		int alloClusCnt;
		int freeClusCnt;
		int unknClusCnt;

		unsigned int* FAT;				/*	to be malloc */

		typedef struct LFNEntry
		{
			unsigned char SeqNum;		/* [0]		LFN Sequence Number*/
			unsigned char Name[26];
			//unsigned char Name1[10];	/* [1-10]	First part of Filename (5 characters in Unicode) */
			//unsigned char Name2[12];	/* [14-25]	Second part of Filename (6 characters in Unicode) */
			//unsigned char Name3[4];		/* [28-31]	Third part of Filename (2 characters in Unicode) */
		}LFNEntry;

		typedef struct DirEntry
		{
			std::vector<LFNEntry*> LFN;
			unsigned char Name[11];		/* [0-10]	File name */
			unsigned char Attr;			/* [11]		File attributes */
			unsigned int FstClus;
			//unsigned short FstClusHI;	/* [20-21]	High 2 bytes of the first cluster address */
			//unsigned short FstClusLO;	/* [26-27]	Low 2 bytes of the first cluster address */
			unsigned long FileSize;		/* [28-31]	File size in bytes. (0 for directories) */
		}DirEntry;

		std::vector<DirEntry*> dir;		/* Every DirEntry and LFNEntry are exactly 32-bytes */
		std::vector<DirEntry*> e5;

	public://public function
		Runner();
		~Runner();
		bool syntaxCheck(int, char*[]);
		void printHelp();
		void run();

	private://private function
		//helper function on reading device info
		int readBootEntry();
		int readFAT();
		int readDirEntry();

		void printFSinfo();
		void listDir();

		//helper function on recovery
		int eptFilenameCheck();
		int lfnCheck();
		int lfnBlockCmp(std::vector<LFNEntry*>, std::vector<LFNEntry*>);

		void eightPthree();
		void longFN();
};